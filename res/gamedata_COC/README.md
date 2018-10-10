CALL OF CHERNOBYL
------------------------------
Credits:

------------------------------
Gameplay Design
------------------------------
Alundaio
Bangalore
Balathruin

------------------------------
Level Design
------------------------------
2C.LiryC
Bangalore
Borovos

------------------------------
Programming
------------------------------
Alundaio
AVoitishin
AxelDominator
Darryl
Nuor
SadBlackFox

------------------------------
Story/Dialog/Text
------------------------------
2C.LiryC
Alundaio
Ardias
Bangalore
Darryl
Koma
Taffy

------------------------------
Textures/SFX
------------------------------
2C.LiryC
Borovos
Darkenneko
Darryl
Genion23
Koma
R_populik
VodkaChicken

------------------------------
Meshes/Modelling
------------------------------
Borovos
Darkenneko
R_populik
VodkaChicken

------------------------------
Quality Assurance
------------------------------
2C.LiryC
Ardias
AndroIDDQD
Atarumoroboshi18
AxelDominator
Balathruin
Bodach
Borovos
CakeLizard
Darkenneko
Darryl
David.m.e.
ExImIer
GoTDTruth
iBook
Inzann
Junx
LT Albrecht
Mannix
muizza
Nexx
Oddzball
Olivius74
PentagonBlack
Swartz
Taffy
Trebic
VodkaChicken
Wolfhound
Wothan
Xenotekno

------------------------------
Other/Special Thanks
------------------------------
This is a list of people who may have original works within the mod through our thirdparty usage of other mods or have been helpful to the project.
------------------------------
-HUN-Riot_Gin - textures
3ncryptabl3/manIK - TGZ hud
Anonymous - Thank you for the great main menu .ogm
Ap-pro.ru
Balathruin - Ambient sound overhaul, Outfit/damages rebalance
Cromm Cruac
Darius
Den Stash - 3D Max tools
DoctorX
ExImIer
FatalFunnel
GSC Game World - for all the games
Genion23 - TC PDA fix, better heli sounds, better PDA map texture
Gridgt - Bump and Thm fixes
L00 - Better blood wallmarks
Misery Development Team
MoozE - main menu soundtrack
Mr. Fusion and TheSweetLittle16-bit - close caption
Nitrocaster – Open X-ray
Noxx
OGSE Team
Paranoia
Ponney68 – For his wonderful meshes and textures for mutant foods
Roman Goroff - Textures
Rulix - AI Additions and inspiration
SZA team - SZA Call of Pripyat Freeplay mod
Shoker – Engine fixes
Sinaps - Fixed meshes
Smurth
Surs
Swartz – Always shares work and inspires
Tronex
Zealot_K
av661194 – Russian localization
ikdoemaarwat - csky and freedom rookie suit fixes
lostalpharus1 - TC terrain texture fix

------------------------------
Call of Chernobyl - FAQ
------------------------------
Q: How do I install Call of Chernobyl?
A: Call of Chernobyl comes prepackaged and no longer uses an installer. Simply extract
the contents.
You need Visual C++ Redist 2013 x86, if game fails to launch due to missing

msvcr120.dll
You need DirectX End-User Runtimes 2010.

Q: Does Call of Chernobyl have Windows XP support?
A: Sorry, there is no Windows XP support at this time. The engine binaries were
compiled on Visual Studio 2013 which only has support for Windows XP SP3 and only if
it is compiled with certain settings.

Q: What are the system requirements?
A: Windows XP is not currently supported. Although the engine supports DirectX 8, I
strongly recommend not using it since there are crash-related shader errors on some
maps when using this renderer that we do not intend to fix.
Windows Vista
4GB RAM
Intel Core 2 Duo E7400 / AMD Athlon 64 X2 5600+
512 MB DirectX® 9.0c compatible card / NVIDIA® GeForce™ 9800 GTX / ATI
Radeon™ HD 4850

Q: How do I report issues or bugs?
A: Please sign up for the bugtracker site.
please try to report all issues to the bugtracker as coherently as possible. It's fairly easy
to use - simply create an account, login, then click the Report Issue link. Fill out the
form with the best description as possible. Copy & Paste crash log if applicable. If you
can visually see the issue please attach a screenshot to report or post a link to an
image. For any serious issues, like repeatable crashes, corrupt saves or game breaking
bugs report them in the Bugs thread as well. If you manage to get a save right before a
reproducible bug, attach it to your bug report.
Your crash log should be located in your appdata folder. If you can't find it look in
fsgame.ltx in your main game directory and the key '$app_data_root$' should tell you the
exact directory. You should find a logs folder in that directory with the xray_*.log. You
can also find your screenshot and savegame directory this way, too.

Q: How do I enable extra Debugging or Spawner menu?
A: If you need to enable extra debugging features, put -dbg in the command line for
your shortcut to Stalker-CoC.exe. Using this command line will enable the use of g_god
and g_unlimited_ammo in the developer console. It will also print out more much better
stack traces in the crash log.
Press 'S' in main menu to open the Debug UI. Here you can find a new console with
many commands, simply type and enter 'help'. The panel on the right has a spawner and
even a jump to level selection list.
Additional debugging to log for the lua scripts can be done using Notepad++. Simply
'Find and Replace' all --alun_utils.debug_write with alun_utils.debug_write.
By default you can now reload system_ini() by pressing numpad0 in main menu. This
means you can see most changes immediately after loading your game without ever
having to restart XrEngine. This is fantastic for weapon ltx editing, icon positioning and
much more!
DO NOT use the developer console command jump_to_level, use xrs_debug_tools level
jumper instead. You can find this in the Advanced tab. There are issues with the dev
console jump_to_level that breaks things.
The Icon Editor can be accessed without going in-game by pressing 'S' in main menu.
This allows you to edit the offsets of icons. You must place new item sections in
ui_debug_main.script for you to see them in the list.

In-game you can press, by default, Insert and it will display the Attachable Items Offset
editor. Simply stand next to any stalker and press 'Attach' after selecting an item section
from the drop-down list. These sections are read directly from the m_stalker.ltx
attachable_items field. Beware that editing the offset fields will auto-save. Either backup
your files or use some sort of subversion control (ie. GitHub) when using these tools!

Q: Does Call of Chernobyl feature or support multiplayer?
A: No. Call of Chernobyl was designed as a singleplayer modification to Call of Pripyat
with no plans otherwise.

Q: Is mod 'X' compatible with Call of Chernobyl?
A: Call of Chernobyl is a standalone game. Many addons for Call of Chernobyl exist on
the ModDB profile.

Q: Can I change factions or my character name?
A: Call of Chernobyl does not have such features in-game. But you are welcome to
change your faction or name through gamedata\axr_options.ini. Here is an example:
[temp]
new_game_character_name = Alundaio
new_game_faction = dolg
Simply load your save and the changes will take effect. Here are a list of valid faction
names:
stalker
bandit
csky
dolg
freedom
army
monolith
ecolog
killer

Q: Can I change FOV?
A: Yes. Bring up developer console. Type 'fov <number>' or 'hud_fov <number>'

Q: Can I change grass radius?
A: Yes. Bring up developer console. Type 'r__detail_radius <number>'

Q: I have poor performance, what should I do?
A: First make sure Grass Density Detail is low as possible. If so, disable Sun Shadows.
Considering playing on DX9 enhanced, aka Enhanced Dynamic Lighting (renderer 2.5) if you aren't already.

Q: DX10 and DX11 no longer show up in game settings renderer list, how do I fix this?
A: Make sure you are not using SweetFX or Reshade. If you force them to use DX9,
obviously you won't see DX10 or DX11 in the list. If you aren't using these then t he
cause is unknown and seems to only happen for certain GPU chipsets or maybe it's a
Windows Update. Pass -perfhud_hack in the command line; it is a workaround
