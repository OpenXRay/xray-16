# macOS Setup Guide (from a brand-new Mac)

Steps to get the engine cloned, configured, and built on a fresh macOS machine
(Apple Silicon). Each step lists the exact command(s) to run in Terminal.

Verified on: macOS (Darwin 25.5.0, Apple Silicon), Apple clang 21, Homebrew 5.1.

## 1. Install Xcode Command Line Tools

Provides `git`, `clang`, and the macOS SDK. A brand-new Mac has none of these.

```sh
xcode-select --install
```

Click "Install" in the dialog that appears and wait for it to finish.
(If you install Homebrew first, its installer will offer to do this for you.)

## 2. Install Homebrew

Package manager used for all remaining dependencies. From <https://brew.sh>:

```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the "Next steps" the installer prints — on Apple Silicon it asks you to
add `eval "$(/opt/homebrew/bin/brew shellenv)"` to your `~/.zprofile`.

## 3. Install build tools and libraries

```sh
brew install cmake ninja sdl2 lzo libogg libvorbis theora openssl@3
```

- `cmake` + `ninja` — the build system (the repo scripts use the Ninja generator).
- `sdl2 lzo libogg libvorbis theora` — system libraries the engine links against
  (same list the macOS CI job installs; see `.github/workflows/cibuild.yml`).
- `openssl@3` — required by `Externals/GameNetworkingSockets`. CI doesn't list
  it because GitHub's macOS runners ship with it preinstalled; without it the
  configure step fails with `Could NOT find OpenSSL`.

## 4. Clone the repository

```sh
git clone git@github.com:OpenXRay/xray-16.git --recurse-submodules
cd xray-16
```

The `--recurse-submodules` flag matters: the engine bundles many third-party
dependencies as git submodules under `Externals/`.

## 5. Switch to the development branch

```sh
git checkout yohji/feat/framegraph
git submodule update --init --recursive
```

The second command syncs submodules to the commits this branch expects —
required after any branch switch, since branches may pin different submodule
versions.

## 6. Configure

```sh
./cmake-initial-configure
```

Creates and configures the build directory `cmake_builds/releasemastergold`.
The default config is `RMG` (ReleaseMasterGold); other options: `release`,
`mixed`, `debug` (see `./cmake-initial-configure --help`).

## 7. Build

```sh
./cmake-build
```

Builds all targets with Ninja using all CPU cores. Pass a target name to build
just one (e.g. `./cmake-build xr_3da`), `-j N` to limit parallelism.

Binaries land in `bin/<arch>/<config>/` inside the repo — on Apple Silicon with
the default config that is `bin/arm64/ReleaseMasterGold/`, containing the game
executable `xr_3da` plus the engine dylibs.

## 8. Set up the game directory

The engine is only half the story — it loads the original S.T.A.L.K.E.R.:
Call of Pripyat assets at runtime from a separate game directory. The layout
used here is a `scop` directory next to the repo checkout:

```
modding/
├── xray-16/      # this repo
└── scop/           # game directory
    ├── fsgame.ltx      # copied from the repo's res/fsgame.ltx
    ├── levels/         # ┐
    ├── localization/   # │ copied from a Call of Pripyat install
    ├── patches/        # │ (Steam/GOG)
    ├── resources/      # ┘
    ├── bin       -> symlink into the repo's build output
    └── gamedata  -> symlink into the repo's res/gamedata
```

Steps:

```sh
mkdir -p ../scop
cp res/fsgame.ltx ../scop/
# copy levels/, localization/, patches/, resources/ from your CoP install
# into ../scop/, then:
ln -sfh "$(pwd)/bin/arm64/ReleaseMasterGold" ../scop/bin
ln -sfh "$(pwd)/res/gamedata" ../scop/gamedata
```

(Run from the repo root. `-h` makes `ln` replace an existing symlink instead
of creating the new link inside it.)

The symlinks mean a rebuild or a `res/gamedata` edit is picked up by the game
immediately — no copying. `fsgame.ltx` resolves all game paths relative to the
directory it lives in: `$game_data$` → `gamedata/` (the symlink) and the
archive dirs → `levels/` etc. The engine writes logs, saves and screenshots to
`_appdata_/`, which it creates on first launch.

## 9. Run

```sh
cd ../scop
./bin/xr_3da
```

The engine log is written to `_appdata_/logs/openxray_<username>.log` — check
it first when something goes wrong.

After a `git pull`, re-run `git submodule update --init --recursive` (pins may
move) and `./cmake-build` to refresh `bin/`.
