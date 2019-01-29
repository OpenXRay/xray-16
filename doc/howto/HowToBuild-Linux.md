# X-Ray 1.6 build and setup on Linux

### Required packages:

- GLEW
- FreeImage
- LockFile
- OpenAL
- TBB
- Crypto++
- pugixml
- Theora
- OGG
- SDL2
- LZO
- JPEG

**Ubuntu 18.04 build dependencies:**

```shell
sudo apt install git cmake libglew-dev libfreeimage-dev liblockfile-dev libopenal-dev libtbb-dev libcrypto++-dev libpugixml-dev libogg-dev libtheora-dev libvorbis-dev libsdl2-dev liblzo2-dev libjpeg-dev libncurses5-dev libreadline-dev
```
**Clone command:**
```shell
git clone https://github.com/OpenXRay/xray-16.git --recurse-submodules
```

**Build commands:**

```shell

cd xray-16

mkdir bin

cd bin

cmake ..

make -jX # where X is the number of cores in your system
```

## To enable debugging:

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo # Building with Debug flag isn't supported now

## To use clang:
```
CC=clang CXX=clang++ cmake ..
```
## To enable all instruction for the local machine:
```
CFLAGS="-march=native" CXXFLAGS="-march=native" cmake ..
```
## To output log and error to different files:
```
make >out.log 2>error.log
```
## Libraries (OLD LIST !!! ):

To build X-Ray Engine you'll need following libraries:

* BugTrap: https://github.com/Xottab-DUTY/BugTrap

* CryptoPP: https://github.com/weidai11/cryptopp

* FreeImage: http://freeimage.sourceforge.net

* FreeMagic: https://github.com/OpenXRay/FreeMagic

* GameSpy: https://github.com/nitrocaster/GameSpy

* libjpeg: https://github.com/OpenXRay/libjpeg

- Use "jpeg" as project root directory name for compatibility with CxImage

* libogg-1.1.4: http://xiph.org/downloads

* libtheora-1.1.1: http://xiph.org/downloads

* libvorbis-1.2.3: http://xiph.org/downloads

* Lightwave: https://github.com/OpenXRay/LightWave

* Luabind: https://github.com/Xottab-DUTY/luabind-deboostified

* LuaJIT: https://github.com/Xottab-DUTY/LuaJIT

* lzo: https://github.com/alexgdi/lzo

* Autodesk Maya 2008/2009 SDK: https://github.com/OpenXRay/maya

* Autodesk 3DS Max 6.0 SDK: https://github.com/OpenXRay/3dsmax

* NVAPI: https://developer.nvidia.com/nvapi

* OpenAutomate: https://developer.nvidia.com/openautomate

* pugixml: https://github.com/zeux/pugixml/

* zlib: http://zlib.net

All mentioned libraries are already in repository as submodules!

## Setup:

- Install S.T.A.L.K.E.R Call of Pripyat using wine (or obtain game resources another way, e.g. via Steam or GOG, all path below for example with wine).

For example if you have a cd-rom you can type `wine setup.exe` to start installation.

It's recommended do not use path with spaces, for instance you can use "c:\cop" as

an installation path (it will be `~/.wine/drive_c/cop` path on your host linux machine).

After building in the engine folder do (assuming you installed CoP in "c:\cop"):

```shell
mkdir ~/.wine/drive_c/cop/bin-linux-dbg # here will be installed binaries with dbg symbols

make DESTDIR=~/.wine/drive_c/cop/bin-linux-dbg install

cd ~/.wine/drive_c/cop/bin-linux-dbg
```

if you want to debug the game you need to prevent input grabbing:

```
setxkbmap -option grab:break_actions
```

now you can run the game using

```
./xr_3da.sh -fsltx ../fsgame.ltx
```

or if you want to debug using gdb:

```
DEBUGGER="gdb --ex=r --args" ./xr_3da.sh -fsltx ../fsgame.ltx
```