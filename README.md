# The Binding of Isaac: 120 FPS Interpolation

This patch makes supported animations, sprite movement, and transitions between main menu screens smoother at up to **120 FPS**. Gameplay keeps its original speed. The patch builds on REPENTOGON's animation interpolation.

Fixed builds are available for **Repentance 1.7.9b (J835)** and **Repentance+ 1.9.7.15 (J374)**. The updater can build the patch for newer Steam versions of Repentance+. The patch keeps achievements enabled and works during **Daily Runs** (use at your own discretion regarding any leaderboards or anti-cheat policies).

---

## Installation

Download the ZIP that matches your exact game version from the Releases page. You can also build for your installed version of Repentance+ using the [Steam update build instructions](#build-for-a-steam-update).

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
```
Each build produces `build/<game-version>/Release/dinput8.dll`. Keep the DLLs in separate versioned packages with `interpol.ini` and retain the name `dinput8.dll` when installing.

`GAME_VERSION` defaults to `1.9.7.15` for a fresh CMake build. Shared interpolation code lives in `src/InterpolationHook.cpp`. Fixed addresses and patterns live in `src/GameVersion.h`.

[memory-addresses.json](memory-addresses.json) lists the RVAs and structure offsets for the fixed profiles.

## Build for a Steam update

After Steam updates Repentance+, run this from the repository in PowerShell.

```powershell
.\update_interpolator.ps1
```

The script uses Python 3.9 or later, CMake and Visual Studio 2022 with the C++ workload. It reads the exact Steam game version and verifies its addresses. It compiles from a generated profile and saves that profile to `tools/latest/GeneratedGameVersion.h` after a successful build. The profile records the game version and executable SHA-256.

Find `dinput8.dll` under `build/<release-version>/Release/`, for example `build/1.9.7.20/Release/`. The default `interpol.ini` is in the repository root. The scanner uses 1.9.7.16.J450 as its reference for Steam updates.

Use `-Executable` to select a specific copy of `isaac-ng.exe`. Use `-VerifyOnly` to inspect the generated profile in `build/validation/`. The validation directory also holds reports for failed builds.

---

## Releases (GitHub Actions)

Start the workflow manually from GitHub Actions on `main`. It compiles the fixed versions in `game-versions.txt` and the Repentance+ version in `tools/latest/GeneratedGameVersion.h`. The workflow uses the profile's numeric release version for the build directory, ZIP and release title. Releases use an automatically generated `build-<run-id>` tag pointing to the source commit.

After a Steam update, run `update_interpolator.ps1` and include the refreshed profile in the commit used for the release.

Each release includes one ZIP per game version and `SHA256SUMS.txt`. Each ZIP contains `dinput8.dll` and `interpol.ini`. After all packages are ready, the workflow deletes all previous releases and tags before publishing the new release.

---
## Credits

* **[REPENTOGON](https://github.com/TeamREPENTOGON/REPENTOGON)** - Original `InterpolV2` logic & concept.
* **[MinHook](https://github.com/TsudaKageyu/minhook)** - API Hooking Library.
* **[isaac-highfps](https://github.com/hunchcldev/isaac-highfps)** - Reference for keeping the engine update cadence independent from the renderer.
