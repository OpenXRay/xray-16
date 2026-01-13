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

This is a fan-made project not affiliated with GSC Game World in any way.
However, they know about many community projects, including this, and support S.T.A.L.K.E.R. community efforts to make the game better.

Installation instructions are on the [How to install and play](https://github.com/OpenXRay/xray-16/wiki/[EN]-How-to-install-and-play) page.

## Supported game platforms
- Call of Chernobyl 1.4.22.
- Call of Pripyat 1.6.02.
- Clear Sky 1.5.10. (minor bugs are possible, but the game is stable finishable. See [#382](https://github.com/OpenXRay/xray-16/issues/382))

Shadow of Chernobyl is **not supported** yet. (see [#392](https://github.com/OpenXRay/xray-16/issues/392)) <br>
Legends of the Zone/Enhanced Edition is not supported and won't ever be likely. (see [#1865](https://github.com/OpenXRay/xray-16/issues/1865))

## Main differences from the original X-Ray
- Support for 64-bit.
- Improved performance, better FPS.
- Original bugs fixes.
- New features for modmakers.
- Works on Linux, macOS, *BSDs and supports ARM, ARM64, E2K (Elbrus 2000), PPC64LE architectures.

You can see the detailed differences table [here](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-Differences-from-original-X‐Ray).

## Goals
1. Clean up engine code, boost performance, and fix original X-Ray Engine bugs that were polluting S.T.A.L.K.E.R. series.
2. Make it a drop-in replacement for original engine.
    1. Aim at 99% compatibility and same behaviour, where possible.
3. Support all three games in the series: SOC/CS/COP.
4. Introduce a solid platform for modmakers:
    1. Add frame/render graph for those who want to add new graphics features.
    2. Add new scripting, development and debugging features.
    3. New game SDK with new features.
5. Enhance player's experience with new graphics, gameplay and other features that can be enabled optionally. (by default, we stay close to vanilla)

## Contributing
All contributions are more than welcomed. There are several ways how you can contribute:

### Community
[![Discord](https://img.shields.io/discord/410170555619082240?label=Discord)](https://discord.gg/sjRMQwv)

Play and enjoy the game, [file an Issue](https://github.com/OpenXRay/xray-16/issues/new/choose) when you encounter any bugs, or you have an enhancement request.

Join us on our [Discord](https://discord.gg/sjRMQwv), subscribe to our [YouTube channel](https://www.youtube.com/OpenXRay), join our [VK group](https://vk.com/openxray), leave a comment, put a like and communicate there! <br>
Also you can put a star on this repository or boost our Discord server :)

### Modding

Use OpenXRay as a platform for your work!

Make sure to follow the official EULA and Fan Content Creation Guidelines when making modifications for S.T.A.L.K.E.R. games: <br>
https://www.gsc-game.com/eula/ <br>
https://www.gsc-game.com/guidelines/

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

Make sure to visit our [wiki](https://github.com/OpenXRay/xray-16/wiki):
* [Build instructions for Windows](https://github.com/OpenXRay/xray-16/wiki/[EN]-How-to-build-and-setup-on-Windows).
* [Build instructions for Linux and other platforms](https://github.com/OpenXRay/xray-16/wiki/%5BEN%5D-How-to-build-and-setup-on-Linux-and-MacOS).

The `dev` branch is the default and base branch for the project. It is used for development, and all pull requests should go there. But be aware that this branch sometimes may be broken, and we can only rarely do force pushes to this branch.

The code base is based on X-Ray 1.6.02 that is used in S.T.A.L.K.E.R.: Call of Pripyat and it was greatly refactored.

### Funding
[![Financial Contributors](https://opencollective.com/openxray/tiers/badge.svg?label=Financial%20contributors)](https://opencollective.com/openxray) [![Sponsors](https://img.shields.io/github/sponsors/openxray?color=brightgreen&label=Sponsors)](https://github.com/sponsors/OpenXRay) [![Patreon](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fwww.patreon.com%2Fapi%2Fcampaigns%2F5950725&query=data.attributes.patron_count&suffix=%20Patrons&color=success&label=Patreon&style=flat)](https://patreon.com/openxray)

You may provide financial support for this project by donating via different ways:
* [Boosty](https://boosty.to/openxray) – a large part of the team is located in Russia, if you have an ability to donate through Boosty, please use it, since we don't have an ability to withdraw funds from services like Patreon, etc. to our local Russian banking cards/accounts.
* [GitHub Sponsors](https://github.com/sponsors/OpenXRay), [Patreon](https://patreon.com/openxray), [Open Collective](https://opencollective.com/openxray) – funds raised from these services will be used to support our developers outside of Russia, and also we may use them to pay for paid services on GitHub, AppVeyor, etc.
* BTC: 363ZUoWcQe9fDvRPK9Kee2YuPdyhSFQpr2
* ETH: 0x45a4fe8566e76946591e1eeabf190aa09b1cdb66
* TRX: TGx7QAhTPsRcwnb4mwCtNDU7NF6kuoACpt
* Please, contact @1yohji in [our Discord](discord.gg/sjRMQwv) if you would like to use another cryptocurrency.

Thank you for your support!

## Thanks
* [GSC Game World](https://gsc-game.com/) – for creating S.T.A.L.K.E.R. and supporting the community.
* Loxotron – for making the engine sources available.
* [All the OpenXRay contributors](https://github.com/OpenXRay/xray-16/graphs/contributors) – for making the project what it is:
  * The OpenXRay team:
    * [nitrocaster](https://github.com/nitrocaster) – original project founder.
    * [Kaffeine](https://github.com/Kaffeine) – initial work on the Linux port, refactoring, polishing.
    * [CrossVR](https://github.com/CrossVR) (Armada651) – creation of the OpenGL renderer, work on the build system, other project maintenance work.
    * [andrew-boyarshin](https://github.com/andrew-boyarshin) – work on the build system.
    * [CasualDev242](https://github.com/CasualDev242) (Swartz27) – work on renderer features.
    * [awdavies](https://github.com/awdavies) – project maintenance work.
    * [Xottab_DUTY](https://github.com/Xottab-DUTY) – former project leader (2018-2026), gathering a new team, creation of the community (GitHub, Discord, VK), defining project guiding principles and goals, working on many areas of tasks (core, renderering, AI, gameplay, UI), SOC/CS/COC support.
    * [intorr](https://github.com/intorr) – work on the project quality, memory leaks, refactoring and optimizations.
    * [eagleivg](https://github.com/eagleivg) – main part of the work on Linux port.
    * [q4a](https://github.com/q4a) – main part of the work on Linux port.
    * [SkyLoader](https://github.com/SkyLoaderr) – OpenGL renderer improvements, stabilization and polishing, other project work.
    * [qweasdd136963](https://github.com/qweasdd136963) – supporting the [OXR_COC](https://github.com/qweasdd136963/OXR_CoC) project (Call of Chernobyl port to latest OpenXRay), other project work on new features, refactoring and bug fixing.
    * [JohnDoe_71Rus](https://github.com/johndoe71rus) – our regular tester.
    * [Chip_exe](https://github.com/007exe) – work on Linux port, maintaining AUR package, our regular tester.
    * [a1batross](https://github.com/a1batross) – work on Linux port.
    * [The Sin!](https://github.com/FreeZoneMods) – new features, refactoring, bug fixing polishing.
    * [Zegeri](https://github.com/Zegeri) – work on Linux port, code quality, fixes, polishing.
    * [drug007](https://github.com/drug007) – work on Linux port.
    * [vTurbine](https://github.com/vTurbine) – work on renderer multithreading, improvements and refactoring.
    * [Zigatun](https://github.com/Zigatun) – work on ARM port.
    * [Masterkatze](https://github.com/Masterkatze) – work on the build system, bug fixing.
    * [Chugunov Roman](https://github.com/ChugunovRoman) – work on [porting Call of Chernobyl to latest OpenXRay](https://github.com/ChugunovRoman/xray-16), extending functionality for modmakers.
    * [yohjimane](https://github.com/yohjimane) – lead developer (2026-current), work on introducing many new features, fixing original engine bugs
  * Other contributors:
    * [alexgdi](https://github.com/alexgdi) – work on organizing project infrastructure, external dependencies.
    * [Shoker](https://github.com/ShokerStlk) – contributing new features, bug fixing.
    * [Alundaio](https://github.com/revolucas) – useful new features, bug fixes and optimizations.
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
    * [v2v3v4](https://github.com/v2v3v4) – work on physics, useful help with the engine and showing sexy screenshots and videos about his X-Ray fork, but refusing to send pull requests :D
    * [Neloreck](https://github.com/Neloreck) – work on extending Lua scripting features.
    * [sobkas](https://github.com/sobkas) – work on code quality and bug fixing.
    * [AMS21](https://github.com/AMS21) – work on CMake, code quality, and project standards and infrastructure.
    * [olefirenque](https://github.com/olefirenque) – work on multithreading and code optimization.
    * [tsmp](https://github.com/tsmp) – work on performance and code optimization.
  * Individuals, whose work was used, merged or imported:
    * [Im-Dex](https://github.com/Im-dex/xray-162) – x64 support, work on the engine.
    * [tamlin-mike](https://github.com/tamlin-mike) – work on the build system.
    * [Vincent](https://github.com/0xBADEAFFE) – work on the Linux port.
    * [abramcumner](https://github.com/abramcumner) – useful fixes and additions.
    * [Morrey](https://github.com/morrey) (nouverbe, [viventaje](https://github.com/viventaje)) – work on DX12 renderer, Clear Sky support and his Return to Clear Sky mod.
    * [avoitishin](https://github.com/avoitishin) – work on scripting features expansion, other improvements and fixes.
  * Financial supporters:
    * [nitrocaster](https://github.com/nitrocaster), Lukas Friedrich, Luke Jones, NekoIt, Igor Polyakov,
    * Incognito, PJ, RazDva, astral jellybean, Kirill Reprintsev,
    * [John Curley](https://github.com/johncurley), The ParaziT, [clayne](https://github.com/clayne), [sobkas](https://github.com/sobkas), MANfromMOON,
    * Valevicor, Nac, Midiy, Vadim Balashov, Jacob Arms,
    * CatWMuttonChops, Reed777, Interpreter_, nexusasx10, [Egor Olefirenko](https://github.com/olefirenque),
    * Igor Zharenko, SLF, Dmitriy Terletskiy, Alex Brodskiy, Neizvestniy Chelovek,
    * LinuxNerd, [tyabus](https://github.com/tyabus), [Sevenfortyseven](https://github.com/Sevenfortyseven), 777yur0k, ItzVladik,
    * @psistore, @forealdo25, Tech Racoon
* Particular projects and their contributors:
  * [Oxygen](https://github.com/xrOxygen) – for being our friends and giving tips and help with new features, optimizations, bug fixes, etc.
  * [Shoker Weapon Mod](https://github.com/ShokerStlk/xray-16-SWM) – for first introducing 3D (PiP) scopes and implementing new features to overall improve the weaponry of the game.
  * [OGSR](https://github.com/OGSR/OGSR-Engine) – for amazing work on Shadow of Chernobyl.
  * [Call of Chernobyl](https://github.com/revolucas/CoC-Xray) – for useful new features, bug fixes and optimizations.
    * [Anomaly](https://www.moddb.com/mods/stalker-anomaly) – for pushing the boundaries, adding new features and enhancing player experience.
  * [Lost Alpha](https://www.moddb.com/mods/lost-alpha) – for their effort on restoring the old game concept.
    * Lost Alpha DC – for continuing work on Lost Alpha and mastering it.
  * [Living Zone](https://vk.com/projektx) – for pushing the limits of our engine past the edge.
  * [OpenXRay Gunslinger](https://www.moddb.com/mods/openxray-gunslinger) – for introducing new fascinating features and enhancing the game experience with players-approved weapon pack on top of OpenXRay.
  * [IX-Ray](https://github.com/ixray-team) – for being at the edge of technology.
* Companies:
  * [CoderGears](https://www.cppdepend.com) – thanks for providing a [free Pro Licence for CppDepend](https://www.cppdepend.com/cppdependfoross), an amazing and powerful tool for C and C++. <br>
    [![CppDepend logo](https://www.cppdepend.com/images/cppdependlogo.png)](https://www.cppdepend.com)
  * [PVS-Studio LLC](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) – thanks for proving us a [free licence](https://pvs-studio.ru/ru/order/open-source-license/?utm_source=website&utm_medium=github&utm_campaign=open_source) for PVS-Studio, a static analyzer for C, C++, C#, and Java code.

If your work is being used in our project and you are not mentioned here or in the [contributors page](https://github.com/OpenXRay/xray-16/graphs/contributors), please, write to us and we will add you. Or send us a pull request with you added to this list ;)
