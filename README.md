<div align="center">
  <p>
    <a href="https://github.com/OpenXRay">
      <img src="misc/media/OpenXRayCover.png" alt="Open for everyone" />
    </a>
  </p>
</div>

<h1 align="center">
  OpenXRay
</h1>

**OpenXRay** is an improved version of the X-Ray Engine, the game engine used in the world-famous S.T.A.L.K.E.R. game series by GSC Game World.

## Goals
1. Make it a drop-in replacement for original engine.
    1. Aim at 99% compatibility and same behaviour, where possible.
    2. Compile engine into a single executable file that you can just drop into `bin` folder. (see [#210](https://github.com/OpenXRay/xray-16/issues/210))
2. Support all three games in the series: SOC/CS/COP. (see [Supported games](#supported-games) below)
3. Fix original S.T.A.L.K.E.R. series bugs.
4. Introduce a solid platform for modmakers:
    1. Add frame/render graph for those who want to add new graphics features.
    2. Improve performance via refactoring the code, parallelizing the engine, making it multithreaded.
    3. Add new scripting, development and debugging features.
    4. New game SDK with new features.
5. Clean up engine code, make it easily portable to new platforms, minimize platform-specific code.
6. Enhance player's experience with new graphics, gameplay and other features that can be enabled optionally. (by default, we stay close to vanilla)

## Main differences from original X-Ray are:
- Support for 64-bit.
- Support for ARM, ARM64, E2K (Elbrus 2000), PPC64LE.
- Works on Linux, macOS, OSL (Elbrus OS).
- New OpenGL renderer. (currently, requires OpenGL 4.1 minimum, lowering to at least OpenGL 3.3 is planned)
- Improved performance, better FPS.
- Original bugs fixes.
- New features for modmakers.
- Gamepad support. (not yet finished, but you can try already, see [#943](https://github.com/OpenXRay/xray-16/issues/943))
- New game SDK being currently developed. (see [Game Editor](https://github.com/OpenXRay/xray-16/wiki/[EN]-Game-Editor))

You can see the detailed differences table [here](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-Differences-from-original-X‐Ray)

## Supported games
OpenXRay is based on X-Ray 1.6.02, used in S.T.A.L.K.E.R.: Call of Pripyat, so initially it supported only this game. <br>
Currently, we are working on support for all three games in the series.
|Call of Pripyat|Clear Sky|Shadow of Chernobyl|
|---|---|---|
|Yes|Release candidate (see [#382](https://github.com/OpenXRay/xray-16/issues/382)).<br>Minor bugs possible, but game is stable finishable.| **Not supported** yet (see [#392](https://github.com/OpenXRay/xray-16/issues/392))|

## Documentation:
Make sure to visit our [wiki](https://github.com/OpenXRay/xray-16/wiki).
|How to|||
|---|---|---|
|Build and setup|[On Windows](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-build-and-setup-on-Windows)|[On Linux](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-build-and-setup-on-Linux)|
|Install and play|[On Windows](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-install-and-play)|-|

## Contributing
All contributions are more than welcomed. There are several ways how you can contribute:

### Community
[![Discord](https://img.shields.io/discord/410170555619082240?label=Discord)](https://discord.gg/sjRMQwv)

Play and enjoy the game, [file an Issue](https://github.com/OpenXRay/xray-16/issues/new/choose) when you encounter any bugs, or you have an enhancement request.

Join us on our [Discord](https://discord.gg/sjRMQwv), subscribe to our [YouTube channel](https://www.youtube.com/OpenXRay), join our [VK group](https://vk.com/openxray), leave a comment, put a like and communicate there! <br>
Also you can put a star on this repository :)

### Development
[![GitHub Actions Build Status](https://github.com/OpenXRay/xray-16/actions/workflows/cibuild.yml/badge.svg)](https://github.com/OpenXRay/xray-16/actions/workflows/cibuild.yml)
[![Contributors](https://img.shields.io/github/contributors/OpenXRay/xray-16.svg?label=Contributors)](https://github.com/OpenXRay/xray-16/graphs/contributors)

Join our efforts in making our beloved game better, send pull requests, participate in discussions and code reviews!

It is a place to share ideas on what to implement, gather people that want to work on the engine, and work on the source code. However, the following things should be taken into consideration:
* We want to keep the game close to the vanilla, so if you want to introduce new gameplay features, make sure it is optional, and doesn't break compatibility with original game resources (i.e. everything in `gamedata` folder and `.db*`/`.xdb` archives). You also may want to add non-gameplay features, fix bugs, or improve engine performance and code quality.
* Major changes should be discussed before implementation.

Take a look at our [Issues](https://github.com/openxray/xray-16/issues) page:
* See issues labeled as [good first issue](https://github.com/OpenXRay/xray-16/issues?q=is%3Aissue+is%3Aopen+sort%3Aupdated-desc+label%3A%22Good+first+issue%22) to get familiar with the engine code in practice.
* You may also want to look at issues labeled as [help wanted](https://github.com/OpenXRay/xray-16/issues?q=is%3Aissue+is%3Aopen+sort%3Aupdated-desc+label%3A%22Help+wanted%22). Some of them are difficult ones, though.

The `dev` branch is the default and base branch for the project. It is used for development, and all pull requests should go there. But be aware that this branch sometimes may be broken, and we can only rarely do force pushes to this branch.

Be advised that this is a community project not sanctioned by GSC Game World in any way – and they remain the copyright holders
of all the original source code and S.T.A.L.K.E.R. franchise. However, they know about many community projects, including this, and support the S.T.A.L.K.E.R. community efforts to make the game better.

### Funding
[![Financial Contributors](https://opencollective.com/openxray/tiers/badge.svg?label=Financial%20contributors)](https://opencollective.com/openxray) [![Sponsors](https://img.shields.io/github/sponsors/openxray?color=brightgreen&label=Sponsors)](https://github.com/sponsors/OpenXRay) [![Patreon](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fwww.patreon.com%2Fapi%2Fcampaigns%2F5950725&query=data.attributes.patron_count&suffix=%20Patrons&color=success&label=Patreon&style=flat)](https://patreon.com/openxray)

You may provide financial support for this project by donating via different ways:
* [Boosty](https://boosty.to/openxray) – a large part of the team is located in Russia, if you have an ability to donate through Boosty, please use it, since we don't have an ability to withdraw funds from services like Patreon, etc. to our local Russian banking cards/accounts.
* [GitHub Sponsors](https://github.com/sponsors/OpenXRay), [Patreon](https://patreon.com/openxray), [Open Collective](https://opencollective.com/openxray) – funds raised from these services will be used to support our developers outside of Russia, and also we may use them to pay for paid services on GitHub, AppVeyor, etc.
* BTC: 363ZUoWcQe9fDvRPK9Kee2YuPdyhSFQpr2
* ETH: 0x45a4fe8566e76946591e1eeabf190aa09b1cdb66
* TRX: TGx7QAhTPsRcwnb4mwCtNDU7NF6kuoACpt
* Please, contact @xottab_duty in [our Discord](discord.gg/sjRMQwv) if you would like to use another cryptocurrency.

Thank you for your support!

## Thanks
* [GSC Game World](https://gsc-game.com/) – for creating S.T.A.L.K.E.R. and supporting the community;
* Loxotron – for making the engine sources available;
* [All the OpenXRay contributors](https://github.com/OpenXRay/xray-16/graphs/contributors) – for making the project better.
  * The first OpenXRay team (2014-2017) – for being at the origins of the project.
    * [nitrocaster](https://github.com/nitrocaster) – original project founder.
    * [Kaffeine](https://github.com/Kaffeine) – initial work on the Linux port, refactoring, polishing.
    * [CrossVR](https://github.com/CrossVR) (Armada651) – creation of the OpenGL renderer, work on the build system, other project maintenance work.
    * [andrew-boyarshin](https://github.com/andrew-boyarshin) – work on the build system.
    * [CasualDev242](https://github.com/CasualDev242) (Swartz27) – work on renderer features.
    * [awdavies](https://github.com/awdavies) – project maintenance work.
  * The second OpenXRay team (2017-now) – for continuing work on the project.
    * [Xottab_DUTY](https://github.com/Xottab-DUTY) – current project leader.
    * [intorr](https://github.com/intorr) – work on the project quality. (memory leaks, refactoring, optimizations)
    * [eagleivg](https://github.com/eagleivg) – main part of the work on Linux port.
    * [q4a](https://github.com/q4a) – main part of the work on Linux port.
    * [SkyLoader](https://github.com/SkyLoaderr) – OpenGL renderer improvements and polishing, other project work.
    * [qweasdd136963](https://github.com/qweasdd136963) – supporting the [OXR_COC](https://github.com/qweasdd136963/OXR_CoC) project (Call of Chernobyl port to latest OpenXRay), other project work on new features, refactoring and bug fixing.
    * JohnDoe_71Rus – our regular tester.
    * [Chip_exe](https://github.com/007exe) – work on Linux port, maintaining AUR package, our regular tester.
    * [a1batross](https://github.com/a1batross) – work on Linux port.
    * [The Sin!](https://github.com/FreeZoneMods) – new features, refactoring, bug fixing polishing.
    * [Zegeri](https://github.com/Zegeri) – work on Linux port, code quality, fixes, polishing.
    * [drug007](https://github.com/drug007) – work on Linux port.
    * [vTurbine](https://github.com/vTurbine) – work on renderer unification, refactoring, polishing.
    * [Zigatun](https://github.com/Zigatun) – work on ARM port.
    * [Masterkatze](https://github.com/Masterkatze) – work on the build system, bug fixing.
    * [Chugunov Roman](https://github.com/ChugunovRoman) – work on [porting Call of Chernobyl to latest OpenXRay](https://github.com/ChugunovRoman/xray-16), extending functionality for modmakers.
    * [yohjimane](https://github.com/yohjimane) – work on original game bugs fixes and new features.
  * Other contributors:
    * [alexgdi](https://github.com/alexgdi) – work on organizing project infrastructure, external dependencies.
    * [NeoAnomaly](https://github.com/NeoAnomaly) – help with debug functionality on Windows.
    * [RainbowZerg](https://github.com/RainbowZerg) – work on the renderer features, bug fixing.
    * [FozeSt](https://github.com/FozeSt) – help with some fixes and features.
    * [justtails](https://github.com/justtails) (mrnotbadguy) – work on gamepads support and bug fixing.
    * [devnexen](https://github.com/devnexen) – work on FreeBSD support and portability.
    * [vamit611](https://github.com/vamit611) – work on code quality and bug fixes.
    * [ZeeWanderer](https://github.com/ZeeWanderer) – work on the build system.
    * [GeorgeIvlev](https://github.com/GeorgeIvlev) – work on the build system, bug fixing.
    * [r-a-sattarov](https://github.com/r-a-sattarov) – work on portability and E2K support.
    * [TmLev](https://github.com/TmLev) – work on code quality and Docker support.
    * [Plotja](https://github.com/Plotja) – work on new gameplay features, bug fixes, portability, polishing.
    * [jjdredd](https://github.com/jjdredd) – work on various useful features.
    * [dimhotepus](https://github.com/dimhotepus) – work on code quality.
    * [HeapRaid](https://github.com/HeapRaid) – work on renderer cleanup, code quality, portability.
    * [OPNA2608](https://github.com/OPNA2608) – maintaining NixOS package, work on portability.
    * [kosumosu](https://github.com/kosumosu) – work on portability, including E2K support, and renderer features.
    * [Graff46](https://github.com/Graff46) – work on various scripting features.
    * [vertver](https://github.com/vertver) – work on macOS support.
    * [Lnd-stoL](https://github.com/Lnd-stoL) – work on macOS support.
    * [GermanAizek](https://github.com/GermanAizek) – work on code quality, finding and fixing vanilla bugs.
    * [dasehak](https://github.com/dasehak) – work on FreeBSD support, finding and fixing vanilla bugs.
    * [Hrust](https://github.com/Hrusteckiy) – work various features, including UI, CS/SOC support and bug fixes.
    * [johncurley](https://github.com/johncurley) – work on EFX, bugs and portability.
    * [v2v3v4](https://github.com/v2v3v4) – work on physics and useful help with the engine.
    * [Neloreck](https://github.com/Neloreck) – work on extending Lua scripting features.
    * [sobkas](https://github.com/sobkas) – work on code quality and bug fixing.
    * [AMS21](https://github.com/AMS21) – work on CMake, code quality, and project standards and infrastructure.
    * [olefirenque](https://github.com/olefirenque) – work on multithreading and code optimization.
    * [tsmp](https://github.com/tsmp) – work on performance and code optimization.
* Particular projects:
  * [Oxygen](https://github.com/xrOxygen) – for being our friends and giving tips and help with new features, optimizations, bug fixes, etc.
  * [Shoker Weapon Mod](https://github.com/ShokerStlk/xray-16-SWM) and [Shoker](https://github.com/ShokerStlk) – for contributing new features, bug fixing.
  * [Im-Dex](https://github.com/Im-dex/xray-162) – for the work on the engine.
  * [OGSR](https://github.com/OGSR/OGSR-Engine) – for amazing work on Shadow of Chernobyl.
  * [Call of Chernobyl](https://github.com/revolucas/CoC-Xray) and its contributors – for useful new features, bug fixes and optimizations.
  * Lost Alpha – for their effort on restoring the old game concept.
  * Lost Alpha DC – for continuing work on Lost Alpha.
* Individuals:
  * [tamlin-mike](https://github.com/tamlin-mike) – for work on the build system.
  * [Vincent](https://github.com/0xBADEAFFE) – for work on the Linux port.
  * [abramcumner](https://github.com/abramcumner) – for useful fixes and additions.
  * [Morrey](https://github.com/morrey) – for work on Clear Sky support and his Return to Clear Sky mod.
* Companies:
  * [CoderGears](https://www.cppdepend.com) – thanks for providing a [free Pro Licence for CppDepend](https://www.cppdepend.com/cppdependfoross), an amazing and powerful tool for C and C++. <br>
    [![CppDepend logo](https://www.cppdepend.com/images/cppdependlogo.png)](https://www.cppdepend.com)
  * [PVS-Studio LLC](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) – thanks for proving us a [free licence](https://pvs-studio.ru/ru/order/open-source-license/?utm_source=website&utm_medium=github&utm_campaign=open_source) for PVS-Studio, a static analyzer for C, C++, C#, and Java code.

If your work is being used in our project and you are not mentioned here or in the [contributors page](https://github.com/OpenXRay/xray-16/graphs/contributors), please, write to us and we will add you. Or send us a pull request with you added to this list ;)
