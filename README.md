# The Binding of Isaac: 120 FPS Interpolation

This patch makes supported animations, sprite movement, and transitions between main menu screens smoother at up to **120 FPS**. Gameplay keeps its original speed. The patch builds on REPENTOGON's animation interpolation.

Supported versions are **Repentance 1.7.9b (J835)** and **Repentance+ 1.9.7.15 (J374) and 1.9.7.19 (J510)**. The patch keeps achievements enabled and works during **Daily Runs** (use at your own discretion regarding any leaderboards or anti-cheat policies).

---

## Installation

Download the ZIP that matches your exact game version from the Releases page.

Copy `dinput8.dll` and `interpol.ini` into your game directory (next to `isaac-ng.exe`):
```
C:\Program Files (x86)\Steam\steamapps\common\The Binding of Isaac Rebirth\
```

---

## Configuration (`interpol.ini`)

```ini
[Interpolation]
Enabled=1
```

* Set `Enabled=1` to turn the patch on.
* Set `Enabled=0` to use the game's original animations and frame rate.

---

## Building

```powershell
cmake -B build/1.7.9b -A Win32 "-DGAME_VERSION=1.7.9b"
cmake --build build/1.7.9b --config Release

cmake -B build/1.9.7.15 -A Win32 "-DGAME_VERSION=1.9.7.15"
cmake --build build/1.9.7.15 --config Release

cmake -B build/1.9.7.19 -A Win32 "-DGAME_VERSION=1.9.7.19"
cmake --build build/1.9.7.19 --config Release
```
Each build produces `build/<game-version>/Release/dinput8.dll`. Keep the DLLs in separate versioned packages with `interpol.ini` and retain the name `dinput8.dll` when installing.

`GAME_VERSION` defaults to `1.9.7.19`. Unsupported values are rejected by CMake. Shared interpolation code lives in `src/InterpolationHook.cpp`; version-specific addresses and patterns live in `src/GameVersion.h`.

[memory-addresses.json](memory-addresses.json) lists each version's RVAs and structure offsets. Keep it synchronized with the source profiles.

---

## Releases (GitHub Actions)

Start the workflow manually from GitHub Actions on `main`. Each run builds **all versions in `game-versions.txt`** and publishes a complete GitHub release. Releases use an automatically generated `build-<run-id>` tag pointing to the source commit.

Release titles use the last version listed in `game-versions.txt`, for example **120 FPS Interpolation — Repentance+ 1.9.7.19**, while each release includes all supported versions.

Each release includes one ZIP per supported game version and `SHA256SUMS.txt`. The ZIP filenames identify the DLC and version, such as `isaac-120fps-interpolation-repentance-1.7.9b.zip` and `isaac-120fps-interpolation-repentance-plus-1.9.7.19.zip`. Each ZIP contains only `dinput8.dll` and `interpol.ini`. After every version builds and its packages are ready, the workflow deletes all previous releases and tags before publishing the new release.

To support a new game version:

1. Add its addresses and patterns to `src/GameVersion.h`, keeping existing profiles.
2. Append its version to `game-versions.txt` (oldest to newest).
3. Commit and push to `main`, then start the release workflow manually from GitHub Actions.

The workflow recompiles and packages the entire list, including all older versions. Existing profiles continue to use their original values; rebuilding does not guarantee byte-identical DLLs if shared code or the compiler changes. CMake defaults to the last listed version for a fresh build directory.

---
## Credits

* **[REPENTOGON](https://github.com/TeamREPENTOGON/REPENTOGON)** - Original `InterpolV2` logic & concept.
* **[MinHook](https://github.com/TsudaKageyu/minhook)** - API Hooking Library.
* **[isaac-highfps](https://github.com/hunchcldev/isaac-highfps)** - Reference for keeping the engine update cadence independent from the renderer.
