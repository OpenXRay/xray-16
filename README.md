X-Ray Engine 1.6 expansion
==========================

This repository contains X-Ray Engine sources based on version 1.6.02.
The original engine is used in S.T.A.L.K.E.R. Call of Pripyat game released by GSC Game World.

It is a place to share ideas on what to implement, gather people that want to work on the engine,
and work on the source code.

Changelist is available in [wiki](https://github.com/OpenXRay/xray-16/wiki/Changes).

Build instructions (Windows): [doc/howto/build.txt](doc/howto/build.txt)

Build instructions (Linux):
Dependencies (Ubuntu 18.04): sudo apt install git cmake lua5.1-dev libssl-dev libtheora-dev libogg-dev liblzo2-dev libjpeg-dev
Init: git submodule update --init --recursive
Building: mkdir bin && cd bin && cmake ../src

Current status (Linux):
Build Failed:
[ 70%] Building CXX object Externals/NVTT/src/nvimage/CMakeFiles/nvimage.dir/ImageIO.cpp.o
In function ‘nv::FloatImage* nv::ImageIO::loadFloat(const char*)’:
xray-16/src/Externals/NVTT/src/nvimage/ImageIO.cpp:138:10: error: cannot convert ‘bool’ to ‘nv::FloatImage*’ in return
   return false;
          ^~~~~



If you find a bug or have an enhancement request, file an [Issue](https://github.com/openxray/xray-16/issues).

Pull requests appreciated! However, the following things should be taken into consideration:
* We want to keep the game as close as possible to the vanilla game, so instead of introducing new gameplay features,
  consider adding non-gameplay features, fixing bugs, improving performance and code quality
* Major changes should be discussed before implementation
* Follow the [procedures](doc/procedure)

Be advised that this project is not sanctioned by GSC Game World in any way – and they remain the copyright holders
of all the original source code.
