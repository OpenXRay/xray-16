# OpenXRay in the browser with WebAssembly

WIP Emscripten build with wasm64, pthreads, WebGL2, load game files in the Origin
Private File System, sound through Emscripten's OpenAL. Chrome only for now.

Call of Pripyat and Call of Chernobyl are playable, have not tested all the way through, expect crashes. 

## Install Dependencies

```sh
brew install emscripten ninja glslang
git submodule update --init --recursive
emcmake cmake --preset web-release
```

## Build

```sh
cmake --build --preset web-release
```

Built files are output in `misc/web/dist`

```sh
node misc/web/tools/build_dist.mjs
```

## Run

```sh
node misc/web/serve.mjs misc/web/dist 8081
```

Open http://localhost:8081 in Chrome only. Select your game folder and click launch.

You can skip the intro in CoP by using this commandline arg:

```
-start server(all/single/alife/new) client(localhost)
```

## Validating shaders offline

```sh
./misc/web/tools/essl_check.sh          # rewrites every GL shader and runs glslangValidator
python3 misc/web/tools/essl_report.py   # unique error sites, mapped back to the original files
```

