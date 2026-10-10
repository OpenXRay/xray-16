# Building OpenXRay

You build the engine the same way on every platform: with CMake presets and the Ninja generator.
`CMakePresets.json` defines each supported combination of platform, architecture and configuration.

## Presets

| Host | Presets | Compiler | Architecture |
|---|---|---|---|
| Linux, macOS, BSD | `unix-debug`, `unix-mixed`, `unix-release`, `unix-rmg` | system default (GCC/Clang/AppleClang) | host |
| Windows | `win64-debug`, `win64-mixed`, `win64-release`, `win64-rmg` | clang-cl + lld-link | x64 |
| Windows | `win32-debug`, `win32-mixed`, `win32-release`, `win32-rmg` | clang-cl + lld-link | x86 |

Presets that don't match your host OS are disabled. `rmg` is ReleaseMasterGold, the shipping configuration.
`mixed` gives you debug checks with optimized code.

## Prerequisites

On every platform you need CMake 3.25 or newer, Ninja, a C++20 compiler and the libraries below. Install them with your platform's package manager:

| Platform | Install |
|---|---|
| Windows | Visual Studio 2022+ or Build Tools with the *Desktop development with C++* workload, which includes the MSVC x64/x86 libraries, the Windows SDK, CMake and Ninja. Also `winget install LLVM.LLVM`; let the installer add LLVM to `PATH` so `clang-cl` is found. Libraries are bundled in `sdk/`. |
| macOS | `xcode-select --install`, then with [Homebrew](https://brew.sh): `brew install cmake ninja sdl2 lzo libogg libvorbis theora openssl@3`. Also install Apple's [Metal Shader Converter](https://developer.apple.com/metal/shader-converter/) package (it installs into `/usr/local`); the native Metal renderer needs it. |
| Debian/Ubuntu | `sudo apt install cmake ninja-build g++ libssl-dev liblzo2-dev libjpeg-dev libopenal-dev libogg-dev libtheora-dev libvorbis-dev` |
| Other Linux/BSD | The same libraries. For exact package names, see the CI job for your platform in `.github/workflows/cibuild.yml`. |

On Windows, run every command in a shell that has the MSVC environment set up for the target architecture:
*Developer PowerShell for VS* for x64, or *x64_x86 Cross Tools Command Prompt for VS* for x86.
You can also call `Launch-VsDevShell.ps1 -Arch amd64|x86 -HostArch amd64` yourself.
Visual Studio's Open Folder mode sets this environment up automatically.

Clone with submodules. After every branch switch or pull, run the submodule update again, because the third-party code in `Externals/` is pinned per commit:

```sh
git clone --recurse-submodules git@github.com:OpenXRay/xray-16.git
git submodule update --init --recursive
```

## Build

```sh
cmake --workflow --preset unix-rmg
```

The workflow configures the build tree if needed, then builds everything. On Windows, use `win64-rmg` or `win32-rmg`.

To configure and build as separate steps:

| Task | Command |
|---|---|
| Configure | `cmake --preset unix-rmg` |
| Clean reconfigure | `cmake --preset unix-rmg --fresh` |
| Change an option | `cmake --preset unix-rmg -DXRAY_USE_AI_PBR=ON` |
| Build everything | `cmake --build --preset unix-rmg` |
| Build one target | `cmake --build --preset unix-rmg --target xr_3da` |
| Package | `cpack --preset unix-rmg` |

## Where builds go

| Path | Contents | Lifetime |
|---|---|---|
| `cmake_builds/<preset>/` | Build tree: objects, `CMakeCache.txt`, `compile_commands.json`. One per preset. | Disposable. Use `--fresh` or delete it. |
| `bin/<arch>/<Config>/` | Everything you run: `xr_3da`, the engine shared libraries and, on Windows, the SDK DLLs. | Overwritten in place on every build. |
| `cmake_builds/<preset>/artifacts/` | Packages from `cpack --preset`. | Overwritten by each `cpack` run. |

`<arch>` is `x64` or `x86` on Windows and the processor name elsewhere (`arm64`, `x86_64`, `i686`, ...).
`<Config>` is `Debug`, `Mixed`, `Release` or `ReleaseMasterGold`.

The output folder depends only on architecture and configuration, not on the preset or build tree.
A rebuild therefore updates the same files, so launch commands and IDE settings that point at `bin/<arch>/<Config>/` stay valid.
Any other build tree with the same architecture and configuration also writes to that folder.
This includes old `cmake_builds/releasemastergold`-style trees from the removed wrapper scripts, so delete those.

## Game directory setup (recommended)

Keep the game data in its own directory and run the binaries straight from the repo.
The only link is `gamedata`, which points at `res/gamedata`, so engine-side shader, config and script edits are picked up without copying.

```
modding/
├── oxr-gunsl/                  # this repo
│   ├── bin/<arch>/<Config>/    # build output, run from here
│   └── res/gamedata/           # engine shaders, configs, scripts
└── scop/                       # game directory
    ├── fsgame.ltx              # copy of res/fsgame.ltx, edit locally
    ├── levels/                 # ┐
    ├── localization/           # │ copied from a Call of Pripyat
    ├── patches/                # │ install (Steam/GOG)
    ├── resources/              # ┘
    ├── gamedata -> oxr-gunsl/res/gamedata
    └── _appdata_/              # created by the engine: logs, saves, screenshots, user.ltx
```

The engine sets `$fs_root$` to the directory that contains the `fsgame.ltx` passed with `-fsltx`, and resolves every game path in `fsgame.ltx` relative to it.
The executable's own location doesn't matter.

### One-time setup

1. Copy `levels/`, `localization/`, `patches/` and `resources/` from a Call of Pripyat install (Steam/GOG) into the game directory.
2. Run these commands from the repo root.

macOS / Linux:

```sh
cp res/fsgame.ltx ../scop/
ln -sfn "$PWD/res/gamedata" ../scop/gamedata
```

Windows (`cmd`). Directory junctions need neither admin rights nor Developer Mode:

```bat
copy res\fsgame.ltx C:\Games\scop\
mklink /J C:\Games\scop\gamedata %CD%\res\gamedata
```

### Running

```sh
bin/arm64/Debug/xr_3da -fsltx /abs/path/scop/fsgame.ltx
```

To run a different configuration or architecture, launch a different `bin/<arch>/<Config>/xr_3da`; the game directory stays the same.
Debuggers work the same way, for example `lldb -- bin/arm64/Debug/xr_3da -fsltx /abs/path/scop/fsgame.ltx`.
The `-fsltx` path must be absolute and must not contain spaces.

The engine writes logs, saves and screenshots to `_appdata_/` in the game directory.
When something goes wrong, check `_appdata_/logs/openxray_<username>.log` first.

| Change | Action |
|---|---|
| Rebuild, `git pull`, edit `res/gamedata` | None. |
| Switch configuration or architecture | Run the other `bin/<arch>/<Config>/xr_3da`. |
| Move the repo | Recreate the `gamedata` link. On Windows, `rmdir` the old junction first; that removes only the junction, not its target. |

## 32-bit builds

| Platform | 32-bit support |
|---|---|
| Windows | `win32-*` presets. clang-cl targets x86 with `-m32` and links against `sdk/libraries/x86`. |
| Linux | Build natively on a 32-bit userland, for example i386 Alpine or a Debian i386 chroot, with the `unix-*` presets. |
| macOS | Not possible: macOS no longer runs 32-bit code. |

## IDEs

| IDE | Setup |
|---|---|
| Visual Studio | *File → Open → Folder*, then pick a preset from the toolbar. VS sets up the MSVC environment for the preset's architecture. No solution files are generated. |
| VS Code | Install the CMake Tools extension and pick a configure preset. |
| CLion | Enable the presets under *Settings → Build, Execution, Deployment → CMake*. |

To run or debug from an IDE, launch `bin/<arch>/<Config>/xr_3da` with `-fsltx <absolute path to fsgame.ltx>`, as described in [Game directory setup](#game-directory-setup-recommended).
In Visual Studio, put this in *Debug → Debug and Launch Settings for xr_3da* (`launch.vs.json`):

```json
{
  "version": "0.2.1",
  "configurations": [
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "xr_3da.exe",
      "name": "xr_3da",
      "args": [ "-fsltx", "C:/Games/stalker/fsgame.ltx" ]
    }
  ]
}
```

## Without presets

Plain CMake still works. Any generator that CMake supports for your platform will do:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=ReleaseMasterGold
cmake --build build
```

On Windows, also pass `-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl`. MSVC `cl.exe` is not supported.
