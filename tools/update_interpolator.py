import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from tempfile import NamedTemporaryFile, TemporaryDirectory

from game_profile import GameImage, ProfileError, hex_value, render_header, require, resolve_profile


REPO = Path(__file__).resolve().parent.parent
BUILD_ROOT = REPO / "build"
J450_SIGNATURES = Path(__file__).with_name("repentance-plus-j450-signatures.json")
LATEST_PROFILE = Path(__file__).with_name("latest") / "GeneratedGameVersion.h"


def steam_roots():
    if os.name == "nt":
        import winreg

        for hive, key, name in (
            (winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam", "SteamPath"),
            (winreg.HKEY_LOCAL_MACHINE, r"Software\WOW6432Node\Valve\Steam", "InstallPath"),
            (winreg.HKEY_LOCAL_MACHINE, r"Software\Valve\Steam", "InstallPath"),
        ):
            try:
                with winreg.OpenKey(hive, key) as registry:
                    value, _ = winreg.QueryValueEx(registry, name)
                    if value:
                        yield Path(value)
            except OSError:
                continue
    for variable in ("ProgramFiles(x86)", "ProgramFiles"):
        if os.environ.get(variable):
            yield Path(os.environ[variable]) / "Steam"


def steam_executable():
    installations = set()
    for root in steam_roots():
        libraries = [root]
        folders = root / "steamapps" / "libraryfolders.vdf"
        if folders.is_file():
            content = folders.read_text(encoding="utf-8-sig")
            libraries.extend(Path(value.replace("\\\\", "\\"))
                             for value in re.findall(r'"path"\s+"((?:\\.|[^"\\])*)"', content))
        for library in libraries:
            apps = library / "steamapps"
            manifest = apps / "appmanifest_250900.acf"
            if not manifest.is_file():
                continue
            folder = re.search(r'"installdir"\s+"([^"]+)"', manifest.read_text(encoding="utf-8-sig"))
            if folder:
                exe = apps / "common" / folder[1] / "isaac-ng.exe"
                if exe.is_file():
                    installations.add(exe.resolve())
    require(installations, "Steam no tiene un isaac-ng.exe localizado. Usa -Executable si esta en otra carpeta.")
    require(len(installations) == 1, "Steam tiene varias copias de Isaac. Indica la ruta con -Executable.")
    return installations.pop()


def write_file(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with NamedTemporaryFile(dir=path.parent, prefix=".writing-", delete=False) as temporary:
        staging = Path(temporary.name)
    try:
        staging.write_bytes(data)
        staging.replace(path)
    finally:
        staging.unlink(missing_ok=True)


def write_json(path, value):
    write_file(path, (json.dumps(value, indent=2, ensure_ascii=False) + "\n").encode("utf-8"))


def compile_dll(build, profile_dir):
    subprocess.run(["cmake", "-S", str(REPO), "-B", str(build), "-G", "Visual Studio 17 2022",
                    "-A", "Win32", f"-DGAME_PROFILE_DIR={profile_dir}"], check=True)
    subprocess.run(["cmake", "--build", str(build), "--config", "Release"], check=True)
    return (build / "Release" / "dinput8.dll").read_bytes()


def build_latest(output, executable, header, addresses, report):
    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=BUILD_ROOT, prefix="compile-") as directory:
        work = Path(directory).resolve()
        require(work.parent == BUILD_ROOT.resolve(), "Carpeta temporal fuera de build")
        profile_dir = work / "profile"
        write_file(profile_dir / LATEST_PROFILE.name, header)
        dll = compile_dll(work / "cmake", profile_dir)
        require(sha256(executable.read_bytes()).hexdigest() == report["sha256"],
                "Steam cambio el ejecutable durante la compilacion. Ejecuta el script de nuevo.")
        package = work / "package"
        release = package / "Release"
        release.mkdir(parents=True)
        (release / "dinput8.dll").write_bytes(dll)
        report.update(result="built", dll="Release/dinput8.dll", dll_sha256=sha256(dll).hexdigest())
        write_json(package / "memory-addresses.json", addresses)
        write_json(package / "verification.json", report)

        previous = work / "previous"
        if output.exists():
            output.rename(previous)
        try:
            package.rename(output)
        except OSError:
            if previous.exists() and not output.exists():
                previous.rename(output)
            raise
        write_file(LATEST_PROFILE, header)
    return output / report["dll"]


def update(executable, verify_only):
    data = executable.read_bytes()
    digest = sha256(data).hexdigest()
    validation = BUILD_ROOT / "validation"
    report_path = validation / "verification.json"
    report = {"file": str(executable), "sha256": digest}
    stage = "validation"
    try:
        image = GameImage(data)
        report["version"] = image.version
        print(f"Repentance+ {image.version} | SHA-256 {digest}", flush=True)
        signatures = json.loads(J450_SIGNATURES.read_text(encoding="utf-8"))
        report["reference"] = signatures["reference"]["version"]
        profile = resolve_profile(image, signatures)
        header = render_header(image, profile, signatures["runtime_patterns"], digest).encode("ascii")
        addresses = {
            "executable": {
                "version": image.version,
                "sha256": digest,
                "timestamp": hex_value(image.timestamp),
            },
            "reference": signatures["reference"],
            "rvas": {name: hex_value(value) for name, value in profile.rvas.items()},
            "offsets": {
                **{name: hex_value(value) for name, value in profile.offsets.items()},
                "MenuPositionOffsets": list(map(hex_value, profile.menu_positions)),
            },
        }
        report.update(result="verified", addresses=len(profile.rvas), menu_positions=len(profile.menu_positions))
        print(f"Validadas {report['addresses']} direcciones y {report['menu_positions']} posiciones de menus "
              f"con la referencia {report['reference']}.", flush=True)
        if verify_only:
            write_file(validation / LATEST_PROFILE.name, header)
            write_json(validation / "memory-addresses.json", addresses)
        else:
            stage = "compilation"
            version = image.release_version
            fixed_versions = (REPO / "game-versions.txt").read_text(encoding="utf-8-sig").split()
            require(version not in fixed_versions, f"{version} tiene un perfil fijo de CMake")
            output = BUILD_ROOT / version
            require(output.resolve() == BUILD_ROOT.resolve() / version, "Ruta de compilacion inesperada")
            artifact = build_latest(output, executable, header, addresses, report)
            report_path = output / "verification.json"
            print(f"Perfil: {LATEST_PROFILE}")
            print(f"DLL: {artifact}")
    except (ProfileError, OSError, subprocess.CalledProcessError) as error:
        report.update(result="failed", stage=stage, error=str(error))
        print(f"Error durante {stage}: {error}", file=sys.stderr)
    if report["result"] != "built":
        write_json(report_path, report)
    print(f"Informe: {report_path}")
    return int(report["result"] == "failed")


def main():
    parser = argparse.ArgumentParser(description="Valida y compila el interpolador para una build de Repentance+.")
    parser.add_argument("--exe", type=Path, help="Ruta alternativa a isaac-ng.exe")
    parser.add_argument("--verify-only", action="store_true", help="Genera el perfil y el informe sin compilar")
    args = parser.parse_args()
    try:
        executable = args.exe.resolve() if args.exe else steam_executable()
        return update(executable, args.verify_only)
    except (ProfileError, OSError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
