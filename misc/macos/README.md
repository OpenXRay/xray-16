# Run Shadow of Chernobyl on an Apple silicon Mac

This procedure is experimental. The main OpenXRay project does not yet list
Shadow of Chernobyl as a supported game. Use a licensed Steam copy of the
original game.

## 1. Install the build tools

Install the Xcode command-line tools:

```sh
xcode-select --install
```

Install the required Homebrew packages:

```sh
brew install git cmake ninja dylibbundler sdl2 lzo libogg libvorbis theora openal-soft jpeg-turbo
```

Install SteamCMD for macOS as described in the
[Valve SteamCMD guide](https://developer.valvesoftware.com/wiki/SteamCMD).
Make sure that the `steamcmd` command is in `PATH`.

## 2. Get the source code

Clone the repository with its submodules:

```sh
git clone https://github.com/OpenXRay/xray-16.git --recurse-submodules
cd xray-16
```

If you already have the repository, update its submodules:

```sh
git submodule update --init --recursive
```

## 3. Get the licensed game data from Steam

Your Steam account must own S.T.A.L.K.E.R.: Shadow of Chernobyl. The Steam app
ID is `4500`.

Create a data directory in the repository:

```sh
mkdir -p "$PWD/build/game-data/soc"
```

Download the Windows game data. Replace `YOUR_STEAM_ACCOUNT_NAME` with your
Steam account name. Do not put your password in the command. SteamCMD asks for
the password and the Steam Guard code when they are necessary.

```sh
steamcmd \
  +@sSteamCmdForcePlatformType windows \
  +force_install_dir "$PWD/build/game-data/soc" \
  +login YOUR_STEAM_ACCOUNT_NAME \
  +app_update 4500 validate \
  +quit
```

The selected directory must contain the original archives from
`gamedata.db0` through `gamedata.dbd`. Keep these files unchanged. Do not
extract or decrypt them. OpenXRay detects the encrypted SoC archives and
decrypts their contents when it reads them.

Do not add these licensed data files to Git. The application package contains
only OpenXRay files.

## 4. Build OpenXRay

Configure and build the Release version:

```sh
cmake -S . -B build-macos -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos --config Release -j "$(sysctl -n hw.logicalcpu)"
```

The Apple silicon build files are in `bin/arm64/Release`.

## 5. Build the macOS application

Create the application, ZIP file, and DMG file:

```sh
bash misc/macos/make_app_bundle.sh arm64 Release soc
```

For a faster local package without a DMG file, use:

```sh
OPENXRAY_SKIP_DMG=1 bash misc/macos/make_app_bundle.sh arm64 Release soc
```

The files are in `build/artifacts`.

## 6. Install and start the game

If `/Applications/OpenXRay SoC.app` already exists, move it to the Trash or to
a backup directory first. Do not copy a new application over an old
application. macOS can merge the two directory trees and keep old scripts.

Copy `build/artifacts/OpenXRay SoC.app` to `/Applications`. Then start it from
Finder.

To skip the publisher title videos for one start, run:

```sh
open -F -n --env OPENXRAY_SKIP_INTRO=1 "/Applications/OpenXRay SoC.app"
```

This starts the application through macOS LaunchServices. The normal Finder
launch keeps the title videos. The `-F` option also tells macOS not to restore
old application window state.

At the first start, select this directory when OpenXRay asks for game data:

```text
<repository>/build/game-data/soc
```

OpenXRay creates its runtime directory here:

```text
~/Library/Application Support/GSC Game World/S.T.A.L.K.E.R. - Shadow of Chernobyl
```

The runtime directory has links to the licensed archives and to the OpenXRay
resources in the application. Saves, settings, and logs also use this runtime
directory.

To select a different game-data directory, run:

```sh
"/Applications/OpenXRay SoC.app/Contents/MacOS/xr_3da" -reset_gamedata_path
```

## 7. Stop a process that does not close

If the game does not close, stop only the OpenXRay SoC process:

```sh
pkill -f '/Applications/OpenXRay SoC.app/Contents/MacOS/xr_3da'
```

Confirm that no game process remains:

```sh
pgrep -fl '/Applications/OpenXRay SoC.app/Contents/MacOS/xr_3da'
```

The second command has no output when the process is stopped.

## 8. Use a resizable window with borders

Open the in-game console and run:

```text
vid_window_mode st_opt_windowed
vid_restart
```

The setting is saved in `user.ltx`. Other available values include
`st_opt_windowed_borderless`, `st_opt_fullscreen`, and
`st_opt_fullscreen_borderless`.

## 9. Get a crash log

Logs are in:

```text
~/Library/Application Support/GSC Game World/S.T.A.L.K.E.R. - Shadow of Chernobyl/_appdata_/logs
```

Send the newest `.log` file with a crash report. It usually has more useful
information than the short stack trace in the error window.

## Known visual detail

The black area in the upper-right part of the main menu is in the original SoC
`ui_mainmenu.dds` image. It is not a missing texture.

The general OpenXRay build procedure is in the
[Linux and macOS build guide](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-build-and-setup-on-Linux-and-MacOS).
