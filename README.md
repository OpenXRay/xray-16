![Open for everyone](OpenXRayCover.png)

OpenXRay [![Discord](https://img.shields.io/discord/410170555619082240?label=Discord)](https://discord.gg/sjRMQwv) [![Financial Contributors](https://opencollective.com/openxray/tiers/badge.svg)](https://opencollective.com/openxray)
==========================
OpenXRay is an improved version of the X-Ray Engine, the game engine used in the world-famous S.T.A.L.K.E.R. game series by GSC Game World.

### Supported games
|Call of Pripyat|Clear Sky|Shadow of Chernobyl|
|---|---|---|
|Yes|Release Candidate (see [#382](https://github.com/OpenXRay/xray-16/issues/382))| **Not supported** yet (see [#392](https://github.com/OpenXRay/xray-16/issues/392))|


### Build status
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/40d84a83b38147c49b4e66705c47a16a)](https://www.codacy.com/app/OpenXRay/xray-16?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=OpenXRay/xray-16&amp;utm_campaign=Badge_Grade)

|CI|Platform|Compiler|Configurations|Platforms|Status|
|---|---|---|---|---|---|
|AppVeyor|Windows, Linux|MSVC, GCC|Debug, Mixed, Release, Release Master Gold|x64, x86|[![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/16mp39v0d7fts6yf?svg=true)](https://ci.appveyor.com/project/OpenXRay/xray-16)|
|Travis|Linux|GCC|Debug, Release|ARM64, x64, x86|[![Travis Build Status](https://api.travis-ci.org/OpenXRay/xray-16.svg?branch=xd_dev)](https://travis-ci.org/OpenXRay/xray-16)|
|GitHub Actions|Windows, Linux|MSVC, GCC|Debug, Mixed, Release, Release Master Gold|x64, x86|[![GitHub Actions Build Status](https://github.com/OpenXRay/xray-16/actions/workflows/cibuild.yml/badge.svg)](https://github.com/OpenXRay/xray-16/actions/workflows/cibuild.yml)
### Documentation
|How to|||
|---|---|---|
|Build and setup|[On Windows](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-build-and-setup-on-Windows)|[On Linux](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-build-and-setup-on-Linux)|
|Install and play|[On Windows](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-install-and-play)|-|

[Changelist](https://github.com/OpenXRay/xray-16/wiki/Changes) and more is available in [wiki](https://github.com/OpenXRay/xray-16/wiki).


### More details
This repository contains OpenXRay Engine sources based on the X-Ray Engine version 1.6.02.
The original engine is used in S.T.A.L.K.E.R.: Call of Pripyat game released by GSC Game World.

It is a place to share ideas on what to implement, gather people that want to work on the engine,
and work on the source code.

If you find a bug or have an enhancement request, file an [Issue](https://github.com/openxray/xray-16/issues).

Pull requests appreciated! However, the following things should be taken into consideration:
* We want to keep the game as close as possible to the vanilla game, so instead of introducing new gameplay features,
  consider adding non-gameplay features, fixing bugs, improving performance and code quality
* Major changes should be discussed before implementation
* Follow the [procedures](doc/procedure)

Be advised that this is a community project not sanctioned by GSC Game World in any way – and they remain the copyright holders
of all the original source code and S.T.A.L.K.E.R. franchise.
