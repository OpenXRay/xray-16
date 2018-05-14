X-Ray Engine 1.6 expansion [![Build status](https://ci.appveyor.com/api/projects/status/vxccu3mfu33sp04i?svg=true)](https://ci.appveyor.com/project/Xottab-DUTY/xray-16)
==========================

This repository contains X-Ray Engine sources based on version 1.6.02.
The original engine is used in S.T.A.L.K.E.R. Call of Pripyat game released by GSC Game World.

It is a place to share ideas on what to implement, gather people that want to work on the engine,
and work on the source code.

Changelist is available in [wiki](https://github.com/OpenXRay/xray-16/wiki/Changes).

Build instructions (Windows): [doc/howto/build.txt](doc/howto/build.txt)

Build instructions (Linux): [doc/howto/build-linux.txt](doc/howto/build-linux.txt)

Current status (Linux):
[ 66%] Building CXX object Externals/cximage/CMakeFiles/cximage.dir/ximaenc.cpp.o
In file included from /home/1/xray-16/Externals/cximage/ximage.h:57:0,
                 from /home/1/xray-16/Externals/cximage/ximaenc.cpp:6:
/home/1/xray-16/Externals/cximage/xiofile.h:21:12: error: ‘LPCTSTR’ has not been declared
  bool Open(LPCTSTR filename, LPCTSTR mode)

Old status (Linux):
Build Failed:
[ 82%] Building CXX object xrCore/CMakeFiles/xrCore.dir/clsid.cpp.o
xray-16/src/xrCore/_types.h:14:24: error: expected initializer before ?s64?
 typedef signed __int64 s64;
                        ^~~


If you find a bug or have an enhancement request, file an [Issue](https://github.com/openxray/xray-16/issues).

Pull requests appreciated! However, the following things should be taken into consideration:
* We want to keep the game as close as possible to the vanilla game, so instead of introducing new gameplay features,
  consider adding non-gameplay features, fixing bugs, improving performance and code quality
* Major changes should be discussed before implementation
* Follow the [procedures](doc/procedure)

Be advised that this project is not sanctioned by GSC Game World in any way ? and they remain the copyright holders
of all the original source code.
