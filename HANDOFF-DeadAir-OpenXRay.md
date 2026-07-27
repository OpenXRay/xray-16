# Dead Air / DeadZone on OpenXRay — Project Handoff

Last updated: 2026-07-24. This document captures everything needed to continue the effort of running the
modded STALKER install at `D:\DAR3` on a freshly-compiled OpenXRay (xray-16) engine.

**2026-07-25: grenade equipping + in-hand toggle — DONE & USER-CONFIRMED (§27, §27a-d).** Dead Air's `[inventory]`
defines 15 data-driven slots; bases 1-12 align with OpenXRay's enum but the UI `GetSlotList()` had no `case` for the
grenade/sidearm/binocular slots, so they fell through to the ruck and any drag snapped back. Fix: added visible
`eInventoryGrenadeList`/`eInventorySidearmList` bound to `dragdrop_binocular`/`dragdrop_sidearm`, realigned
grenade/binoc configs `slot=13→3` (real `GRENADE_SLOT`); plus a double-click-refresh fix (§27a) and an empty-slot
swap-desync fix (§27b). The **G-key ready/holster** is a loose-Lua handler in `itms_manager.on_key_press` keyed to
`SDL_SCANCODE_G` calling `activate_slot(4)`/`activate_slot(0)` (§27d) — NOT an engine change.
**⚠️ READ §27c BEFORE ANY KEYBIND WORK:** an attempt to add a `kWPN_7` engine action was reverted; ignoring its
`initialize_bindings` assert (clicking Continue) corrupted `user.ltx` (inventory+console both on I, LMB=cycle). Also:
the LIVE appdata (config/saves/logs) is `bin\x64\Release\appdata\`, NOT `D:\DAR3\appdata\`.
**This was the 4th data-vs-enum divergence — suspect that class FIRST.**

**2026-07-24: console flooded with `! Unknown command: r2_dof_time / r2_dof_pickable` (then `r2_lenswater_val /
r2_lensdirt_val`) — FIXED (§26).** `level_weathers.script` sets ~18 Dead Air-custom render cvars every weather
update that stock OpenXRay never registered (found in two passes; §26 has the full-audit command so no pass 3 is
needed). All registered as storage-only cvars in `xrRender_console.cpp` (not yet wired to the render path —
silences the spam faithfully). This closes §8 point 5. Also fixed the separate (different-class) warning
`! Setting rank to 0. Cannot find rank for: [wpn_sks_short]`: added `wpn_sks_short` to `[rank_1] available_items`
in the loose `configs\mp\mp_ranks.ltx` (that file IS in the SP `pSettings` chain — `system.ltx` →
`mp\deathmatch_game.ltx` → `mp_ranks.ltx` — and is the single source of `rank_0..4` `get_rank` reads; NPC
weapon-selection in `ai_stalker_alife.cpp` is the SP caller). NOTE: a loose config CANNOT redefine an
already-defined section — `xr_ini.cpp:507` fatals on "Duplicate section" — so appending to the existing line in its
own source file is the only correct route (do NOT add a second `[rank_1]` to `oxr_compat_fixes.ltx`).

**2026-07-24: game crashed to desktop on load — FIXED (§25).** Two A-Life registry `THROW2` asserts that retail
tolerated as NDEBUG no-ops but this `XRAY_EXCEPTIONS=1` build fatals on: a teleport `remove` "hasn't been found"
(fires on the first A-Life churn right after load — the user's CTD) and two unload `add` "already found" asserts
(graph + schedule registries, fire on quit-to-menu / load-a-save). Same class as §14 Crash B. Both load AND
quit/unload paths engine-verified with 0 fatals; NOT yet user-confirmed.
**When a native fatal fingers a `THROW`/`R_ASSERT` on an A-Life registry op that retail survived, suspect this
class first (§25).**

**2026-07-24: night vision did nothing at all — FIXED (§24).** Root cause was NOT rendering: Dead Air handles
`kNIGHT_VISION` in Lua and the port also kept the stock engine binding, so each key press toggled twice and
cancelled itself out. This is now the SECOND instance of that pattern (§22 was `kTORCH`). **Whenever a Dead Air
key "does nothing", check for a duplicate engine binding in `ActorInput.cpp` before looking anywhere else.**
§24 also leaves a new open lead on the unresolved §11-§13 Escape bugs: `itms_manager.on_key_press` compares
against the hard-coded action id `52`, which in THIS engine is `kQUIT` (Escape) and opens the PDA.

**BOTH variants of the in-session save-load render-corruption bug are now RESOLVED and user-confirmed, as of
2026-07-23. The color-scrambled/"confetti" variant was fixed 2026-07-22 (§19, stale distortion-buffer read). The
uniformly-DIM variant was fixed 2026-07-23 (§20, NaN-valued unset `aberration`/`lumasharpen` postprocess
constants in `combine_2`'s shader). Do NOT re-investigate either variant from scratch — read §19/§20 first if
new corruption is reported, since it may be a third, different mechanism.**

The Escape-key/dialog-stacking work from §11-§13 below was last left "awaiting user re-test #3" (§13) and was
NOT touched this session — status unknown, re-read §11-§13 before resuming it, do not assume it's resolved.

**2026-07-23: the "dynamic lights only light a few steps" bug (player headlamp + NPC torches) is ROOT-CAUSED and
FIXED, user-confirmed — see §21.** Root cause was the spot cookie texture (alpha-shaped, RGB black) which the
`accum_spot` shader multiplies the beam by, zeroing it entirely. §21 also documents a fast way to disassemble this
build's compiled shaders without RenderDoc, and leaves ONE open follow-up: **the glowstick/lighter only emit light
while the headlamp is enabled** (all handheld lights share one `light_render`) — read §21's final subsection
before starting that.

---

## 1. Current state (TL;DR)

**It works far enough to boot into gameplay.** OpenXRay is compiled from source and launches the `D:\DAR3`
install: it mounts the archives, initializes DX11, loads the full AXR/Dead Air Lua script suite, builds the
A‑Life world (~23k spawn points, ~27k objects), reaches the main menu, starts a new game, and **loads into the
Cordon level (`l01_escape`) and renders/plays**. The pause menu (Esc) works.

It is an **early port**, not finished. Expect intermittent crashes during load/spawn and in-game from remaining
content/export gaps (see §8). This has been a long incremental effort; the method is now turnkey (see §7).

Milestone screenshots (proof it runs): `D:\openxray-build\ingame_capture.png` (in Cordon),
`D:\openxray-build\esc_after2.png` (DeadZone pause menu).

---

## 2. What the install is (`D:\DAR3`)

- Base: **S.T.A.L.K.E.R. Call of Chernobyl 1.4.22** (the original engine log prints "Call of Chernobyl version
  1.4.22"; xrCore build 7090, Jul 2018). On top: **Dead Air (AXR)** + a **DeadZone** layer (archives dated
  Dec 2025). This matters: OpenXRay explicitly supports CoC 1.4.22, so compatibility is much better than a
  bespoke engine — the script API is close, most gaps are small.
- Data: ~25 GB of `.xdb0/.xdb1` archives in `D:\DAR3\database` (base 2018 + `xtra_dar2` 2024 + `zzzDeadZone`
  Dec 2025). `fsgame.ltx` at `D:\DAR3\fsgame.ltx`. Russian locale.
- The original modded engine binaries (xrEngine.exe, xrGame.dll, …) still sit in `D:\DAR3` — untouched; we run
  our own engine from elsewhere and point it at this folder.
- Note: DeadZone (Dec 2025) is **newer than the user's saves** (Mar 2024), so some DeadZone content bugs were
  likely never validated even on the retail engine (e.g. the dangling ammo section, §7).

---

## 3. Environment & toolchain (already verified present)

- **Visual Studio 2022 Community**, MSVC toolset **v143** (14.44.35207), with bundled CMake + Ninja.
  MSBuild: `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe`
- **Git for Windows** at `C:\Program Files\Git\cmd` — **NOT on the system PATH**; must be prepended for builds
  (the xrCore prebuild step shells out to `git`).
- No vcpkg needed — OpenXRay vendors deps. NuGet packages (SDL2/DirectXMath/DirectXTex) restore via MSBuild.
- GPU: NVIDIA RTX 3080 Ti Laptop (DX11 / renderer_r4 works). Windows 11.

---

## 4. Repo location & how to build

Repo cloned (dev branch, shallow, --recursive) at **`D:\openxray-build\xray-16`**. It is a working copy with
**27 modified source files** (all our patches — see §7 and the saved patch file
`D:\openxray-build\deadair-openxray-source-patches.diff`).

### Build command (PowerShell)
```powershell
$env:PATH = "C:\Program Files\Git\cmd;" + $env:PATH          # git MUST be on PATH
$msb = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
$sln = "D:\openxray-build\xray-16\src\engine.sln"
# One-time (already done): restore NuGet packages.config packages
& $msb $sln /t:Restore /p:RestorePackagesConfig=true /p:Configuration=Release /p:Platform=x64
# Build (incremental after the first full build is ~30–60s for a single-file xrGame change)
& $msb $sln /p:Configuration=Release /p:Platform=x64 /m /nologo /v:minimal
```

### Build gotchas (learned the hard way)
- **Build `Release`, not `Mixed`.** The wiki says "Mixed for playable", but Mixed defines `DEBUG` so `VERIFY()`
  assertions are active and trip on Dead Air's slightly-off assets (an over-4KB localization string; a malformed
  `.thm`). Release defines `NDEBUG` → `VERIFY` compiles out → tolerates them like the retail engine.
- **Do NOT disable WholeProgramOptimization** to speed iteration — it breaks the xrPhysics link (LNK2019 on a
  LTCG-dependent `CPHItemList<CPHUpdateObject>::iterator` template instantiation). Keep standard WPO Release.
- The `xrCore.vcxproj` prebuild (git-info) was patched to `cd /d "$(ProjectDir)"` + `exit /b 0` because
  `call .GitInfo.cmd` fails when building the .sln from the CLI (wrong cwd). Already applied.
- Output binaries: **`D:\openxray-build\xray-16\bin\x64\Release\`** (`xrEngine.exe` = project XR_3DA).
- `dbghelp.dll` was copied into the bin dir so native crash stack traces work (see §7 fix "dbghelp").

---

## 5. How to run

Desktop shortcut **"Dead Air (OpenXRay)"** already created. Manual/canonical launch:
```
Target : D:\openxray-build\xray-16\bin\x64\Release\xrEngine.exe
Args   : -fsltx D:\DAR3\fsgame.ltx -r4 -noprefetch
Start in (working dir, IMPORTANT) : D:\openxray-build\xray-16\bin\x64\Release
```
- `-fsltx D:\DAR3\fsgame.ltx` → sets `$fs_root$ = D:\DAR3` (path must have **no spaces**).
- `-r4` → force DX11 (user.ltx asks for `renderer_r3` which this build doesn't include; `-r4` overrides).
- `-noprefetch` → skip stale prefetch lists (the ORIGINAL Dead Air engine also ran with this; without it, the
  object-pool prefetch reads undefined sections like `water_drink_2` and fatals).
- For headless/automated testing add `-silent_error_mode -no_call_stack_assert` (suppresses the crash dialog;
  errors still go to the log).

### Important path quirk
`$app_data_root$` in fsgame.ltx resolves to the **working directory**, NOT `D:\DAR3`. So logs, saves, shader
cache, and cdb_cache all land under **`D:\openxray-build\xray-16\bin\x64\Release\appdata\`**, e.g. the log is
`...\bin\x64\Release\appdata\logs\openxray_user0.log` (NOT `D:\DAR3\appdata\...`). The game also creates and
auto-loads its own autosave there. **The runtime log to read is the one in the bin dir.**

---

## 6. The fix-override layer (how content fixes are applied)

X-Ray loads loose `gamedata\` files that OVERRIDE the same paths inside the `.xdb` archives. We use that as an
editable fix layer at `D:\DAR3\gamedata\` (originally only had a few loose configs):
- **Unpacked** `$game_config$` (4299 files) and `$game_scripts$` (432 files) from the archives to loose files at
  `D:\DAR3\gamedata\configs\` and `D:\DAR3\gamedata\scripts\`. These are grep-able and editable.
- The unpacker is a custom engine flag we added: **`-unpack_configs`**. Run
  `xrEngine.exe -fsltx D:\DAR3\fsgame.ltx -unpack_configs` and it dumps configs+scripts to loose files then
  exits (impl in `x_ray.cpp`, `UnpackAliasToLoose`). Needed because xrCompress only PACKS; the archives use
  OpenXRay-specific compression so a Python unpacker isn't trivial.
- **`D:\DAR3\gamedata\configs\oxr_compat_fixes.ltx`** — our config fix file, `#include`d at the END of the loose
  `system.ltx` (Dead Air's system.ltx does NOT wildcard-include `mod_system_*.ltx`, so a fix file must be
  included explicitly). Put missing-section definitions here.

---

## 7. Complete list of source patches applied (27 files)

Full diff saved at **`D:\openxray-build\deadair-openxray-source-patches.diff`** (614 lines). Grouped by purpose:

### Build / infrastructure
- `src/xrCore/xrCore.vcxproj` — fix git-info prebuild (cwd + `exit /b 0`).
- `src/xrCore/Debug/StackTrace.cpp` — `GetModuleHandleA("dbghelp.dll")` → add `LoadLibraryA` fallback so native
  crashes produce stack traces (CRITICAL diagnostic; also copied System32 dbghelp.dll into the bin dir).
- `src/xrEngine/x_ray.cpp` — added `-unpack_configs` mode + `<filesystem>` include (the config/script unpacker).
- `src/xrCore/xr_ini.cpp` — added a diagnostic Msg loop before the `r_section` fatal (harmless; helped diagnose
  the ammo issue; only fires on the fatal path).

### Engine-strictness relaxations (Release-tolerated behaviors)
- `src/xrEngine/Environment_misc.cpp` — `CEnvAmbient::load` R_ASSERT (must have sound channels or effects) →
  downgraded to a warning; Dead Air ships empty ambients.
- `src/xrCDB/ISpatial.cpp` — `ISpatial_DB::_remove` added `if (octant >= 8) return;` guard. In Release the
  `VERIFY(octant<8)` is compiled out, so a spatial-tree inconsistency indexed `children[u32(-1)]` and crashed
  during level load. **This unblocked level loading.**

### Missing script exports Dead Air's engine had (stock OpenXRay lacks)
- `xrServer_Objects_ALife_Items.h` + `..._script.cpp` — `CSE_ALifeItemWeapon::get_addon_flags()` (returns
  `Flags8&`).
- `game_object_space.h` + `script_game_object_script.cpp` — added `callback.take_item_from_ground` enum const.
- `level_script.cpp` — `level.get_rain_volume()` (= rain_density, alias of rain_factor).
- `UIGameCustom_script.cpp` — single-arg `AddCustomStatic(id)` overload (Dead Air calls with 1 arg).
- `Actor.h` / `Actor.cpp` + `script_game_object.h` / `script_game_object2.cpp` / `script_game_object_script2.cpp`
  — actor methods: `set_actor_zoom_inertion(f)` (functional, applied in cam_Update), `set_radiation_detector(b)`
  (stored), `set_actor_recoil_coeff(f)` (stored).
- `script_game_object_inventory_owner.cpp` (+ `character_community.h` include) — `set_actor_community(str)`
  (functional: `CHARACTER_COMMUNITY c; c.set(str); owner->SetCommunity(c.index())`).
- `Torch.h` + `torch_script.cpp` + `script_game_object_script2.cpp` — the FULL Dead Air torch API (~22 methods:
  `torch_set_color_r/g/b/a`, `torch2_set_color_r/g/b`, `torch_set_offset_y/z`, `torch2_set_offset_x/y`,
  `torch_set_radius/range`, `torch2_set_radius/range`, `torch_set_inertion`, `torch_set_animation(str)`,
  `torch_set_texture(str)`, `torch_switch_spot(b)`, `enable_torch(b)`→Switch, `enable_torch2(b)` stub,
  `torch_enabled()`→torch_active). IMPORTANT: these are called on the game_object wrapper
  (`db.actor:object("device_torch")` returns `CScriptGameObject`), so they are registered on CScriptGameObject
  via forwarding-lambda macros (`DA_TORCH_F/B/S`) that `smart_cast<CTorch*>(&o->object())`. Most are store-only
  stubs (flashlight uses engine default look); enable_torch/torch_enabled are functional.
- `HangingLamp.h` + `HangingLamp.cpp` — `hanging_lamp:is_on()` (returns `m_bState`).
- `Artefact.h` + `hit_immunity.h` + `script_game_object_script3.cpp` — 11 dynamic-artefact setters:
  `set_artefact_weight`(→SetWeight), `set_artefact_additional_weight`(→new SetAdditionalWeight), and 9 immunities
  (burn/shock/chemical_burn/radiation/telepatic/wound/fire_wound/strike/explosion) via new
  `CArtefact::SetArtefactImmunity(EHitType,v)` using new `CHitImmunity::SetHitImmunity`.

### UI / input fixes (Esc key + crash)
- `src/xrGame/Level_input.cpp` — `kQUIT` (Escape) handler: let the top dialog process Escape, but if it
  "consumes" the key yet stays the shown top receiver (Dead Air's PDA swallows Escape without closing),
  force-close it via HideDialog. Makes Escape reliably dismiss PDA/inventory/dialogs and open the pause menu.
- `src/xrGame/ui/UIMainIngameWnd.cpp` — guard `UpdateRankingWnd()` with `&& GetPdaMenu().IsShown()`. The PDA
  ranking window (a deep CUICharacterInfo child tree) was updated EVERY frame even when the PDA was closed;
  with Dead Air's partly-broken PDA UI (missing texture `ui_ingame2_pda_buttons_background_e`) that walked a
  corrupted child tree and crashed in `CUIWindow::Update`. **This fixed the "second Esc crash" / general HUD
  crash** (verified 0 crashes over a 19k-line run vs crashing at ~10k before).

### Content fixes (loose gamedata, NOT source)
- `D:\DAR3\gamedata\configs\oxr_compat_fixes.ltx` (included at end of loose system.ltx):
  `[ammo_7.62x54_ap_2]:ammo_7.62x54_ap` — a dangling ammo section referenced by prefetch/treasure lists but
  never defined (a DeadZone content bug).
- `D:\DAR3\gamedata\configs\creatures\m_crow.ltx` — added single-value `speed`, `angular_speed`,
  `goal_change_delta`, `idle_sound_delta` (Dead Air's crow config uses `_min/_max` variants but OpenXRay's
  ai_crow reads single values). Expect OTHER monster configs to have the same mismatch.

---

## 8. Known issues / remaining work (the long tail)

The game is not stable end-to-end. Categories of remaining work, in rough priority:
1. **Intermittent load/spawn crashes** — different runs crash at different points during async monster/artefact
   spawning. These are more missing script exports (`attempt to call/index (nil)`), missing enum consts
   (`no static X in class Y`), and config gaps (`Can't open/find section/variable`). Fix each via the loop in §9.
2. **More monster configs** — like `m_crow`, other monsters likely have `_min/_max` vs single-value field
   mismatches that OpenXRay's AI reads. (Check each `ai_*` monster's `r_float(section, "...")` calls.)
3. **Dead Air PDA UI is genuinely broken on OpenXRay** — missing textures (e.g.
   `ui_ingame2_pda_buttons_background_e`), it renders invisibly and its window tree gets corrupted. Root cause of
   several UI issues. Fully porting the PDA (map/tasks/contacts/ranking) is a separate, larger piece of work.
4. **Other object/feature clusters** will surface per level and per game system (weapons, anomalies, sleep,
   backpack, crafting, etc.). Each is bounded and findable via grep + the crash loop.
5. ~~`! Unknown command: r2_dof_time / r2_dof_pickable` spam~~ — FIXED §26 (registered the missing Dead Air
   render cvars in `xrRender_console.cpp`).

---

## 9. Debugging methodology (turnkey — how to continue)

Tight crash-driven loop, each cycle ~1–4 min (incremental xrGame builds are fast):
1. **Launch** with `-r4 -noprefetch -silent_error_mode -no_call_stack_assert`. Wait ~45–60s (it auto-loads into
   Cordon).
2. **Read the log** at `D:\openxray-build\xray-16\bin\x64\Release\appdata\logs\openxray_user0.log`. Look for the
   LAST of these near the end:
   - `! [LUA] ... attempt to call/index method 'X' (a nil value)` → missing engine export `X`. Find the object
     type from the script (`gamedata\scripts\...` — now loose & greppable), find the class in the engine, add the
     method + luabind `.def`. Remember: methods called on `db.actor:object(...)` / most game objects go on
     `CScriptGameObject` (cast to the concrete C++ type), NOT the concrete class directly.
   - `no static 'X' in class 'callback'` (etc.) → missing enum constant; add to the enum + the luabind
     `value("X", ...)` registration.
   - `Can't open section 'X'` / `Can't find variable 'Y' in [Z]` → missing config; add to
     `oxr_compat_fixes.ltx` (sections) or edit the relevant loose config directly (fields). Sections defined via
     inheritance `[X]:existing_section` are easiest.
   - Native crash with NO Lua line → read the **stack trace** in the log (dbghelp fix makes this work). It names
     the exact function/file/line.
3. **Fix, rebuild** (§4), relaunch. Repeat.
- Tip: grep the loose scripts to enumerate a whole CLUSTER at once (e.g. all `torch_*`, all `set_artefact_*`,
  all `callback.*`) and batch the fixes into one rebuild.
- Test-env note: an automated/headless PowerShell harness here injects phantom keyboard scancode-19 ('P' =
  open PDA) events, which repeatedly opens the PDA and makes UI repro chaotic (it's NOT a gamepad; `-no_gamepad`
  doesn't stop it; it's NOT the real user's input). A human on the real machine won't see this.

---

## 10. Key file/path reference
- Repo / source: `D:\openxray-build\xray-16` (dev branch, 27+ files modified — see §11 for additions since §7 was
  written)
- Built engine: `D:\openxray-build\xray-16\bin\x64\Release\xrEngine.exe`
- Source patch: `D:\openxray-build\deadair-openxray-source-patches.diff` — regenerated at the end of §26's session
  (2026-07-24), current as of that point (60 files, 2413 lines; includes the §26 `xrRender_console.cpp` cvars).
  Regenerate again with `git diff` in `D:\openxray-build\xray-16` before relying on it if more changes land after this.
- Runtime log: `D:\openxray-build\xray-16\bin\x64\Release\appdata\logs\openxray_user0.log`
- Game data: `D:\DAR3` (fsgame.ltx, database\*.xdb, gamedata\ loose fix layer)
- Content fix file: `D:\DAR3\gamedata\configs\oxr_compat_fixes.ltx`
- Original engine log (retail, for reference): `D:\DAR3\appdata\logs\xray_user0_ORIGINAL_engine.log`
- Milestone screenshots: `D:\openxray-build\ingame_capture.png`, `esc_after2.png`, `esc_final.png`

---

## 11. ACTIVE — Escape key / dialog-stacking bugs (started 2026-07-20, unresolved, DO NOT re-derive from scratch)

### Symptom history (chronological, as reported by the user across several retests of the SAME running game)
1. **Original report**: inventory doesn't close on Esc; closing PDA with Esc opens the pause menu; a second Esc
   then closes that menu back to gameplay normally.
2. **After attempt 1** (see below): user reported UNCHANGED — "In-game Esc does nothing, Inventory is not closed
   when Esc is pressed, Closing the PDA opens pause menu."
3. **After attempt 2** (see below): user reports PARTIAL fix — **Esc now correctly closes Inventory**. Still
   broken: **closing the PDA still shows the pause menu**, AND a **new symptom surfaced: Esc does not close the
   NPC dialogue/talk window (`CUITalkWnd`)**. User asked to stop fixing and just document for next session — no
   further code changes have been made past this point.

### What's already been tried (in order — do not repeat blindly, but the reasoning/evidence below should save
   re-investigation time)

**Attempt 1** — `src/xrGame/UIGameSP.cpp`: `StartTrade()` and `StartUpgrade()` call `ActorMenu->ShowDialog(true)`
directly (bypassing the `ShowActorMenu()`/`ShowPdaMenu()` mutual-exclusion wrapper, unlike the sibling
`StartCarBody()` which guards with `if (TopInputReceiver()) return;`). Added `HidePdaMenu();` at the top of both.
Rationale: if PDA was open when a trade/upgrade sequence started, both would end up stacked. **Confirmed this
particular gap is real and the fix is still correct/harmless, but it was NOT the user's actual repro path** (they
weren't trading/upgrading) — hence "unchanged" in symptom report #2.

**Attempt 2** — Root-caused via `[ESCDBG]` diagnostic `Msg()` logging (see below) added to: `CDialogHolder::
IR_UIOnKeyboardPress`, `StartMenu`/`StopMenu` (`UIDialogHolder.cpp`); the `kQUIT` case in `Level_input.cpp`;
`CCC_MainMenu::Execute` (`console_commands.cpp`); `CUIActorMenu::OnKeyboardAction`/`OnBtnExitClicked`
(`UIActorMenu_action.cpp`); `ShowActorMenu`/`ShowPdaMenu`/`HideActorMenu`/`HidePdaMenu` (`UIGameCustom.cpp`).
All gated to only fire on Escape/I/P keys (cheap enough to leave running). **This logging is still in place in
the current build** — grep `openxray_user0.log` for `[ESCDBG]`.

Reading the **user's own real-session log** (not a synthetic/headless test — important, see the phantom-input
caveat below) showed the precise mechanism at one point in their session (`frame=871`):
```
StartMenu CUIPdaWnd frame=871          <- PDA opens: NO preceding keypress log, NO "ShowPdaMenu called" log
IR_UIOnKeyboardPress dik=41 ...        <- Escape pressed
StopMenu CUIPdaWnd frame=871           <- PDA closes
kQUIT ... now=CUIActorMenu nowShown=1  <- reveals CUIActorMenu (Inventory), open ~50 frames earlier via 'I',
                                            never closed by the user
```
i.e. PDA got shown through **some path that bypasses `CUIGameCustom::ShowPdaMenu()` entirely** (so its
`HideActorMenu()` call never ran), stacking on top of the still-open Inventory instead of replacing it. A single
Escape only pops the top layer (PDA), revealing Inventory underneath — perceived as "the menu" — needing a
second Escape.

Checked one candidate (`D:\DAR3\gamedata\scripts\itms_manager.script:368`, `pda_menu:ShowDialog(true)`) — it's
inside a `---[[ ... --]]` Lua block comment, i.e. **dead code, ruled out**. The actual caller that shows PDA
outside `ShowPdaMenu()` is **still unidentified**.

**Fix applied** (kept, believed still correct): rather than chase the specific unidentified caller, added a
defensive last-line-of-enforcement directly in `CDialogHolder::StartMenu()` (`UIDialogHolder.cpp`): before
adding any new input receiver, if the dialog being started is `CUIPdaWnd` or `CUIActorMenu` (checked via
`GetDebugType()` string — `CDialogHolder` is generic engine code with no compile-time knowledge of these
xrGame UI subclasses) and the OTHER one of that pair is already shown, force `HideDialog()` on it first. This
should make PDA and the actor menu structurally unable to stack, regardless of which code path shows one of
them. Logs `"force-hiding stacked %s first"` when it engages — **grep for that string to check whether it's
even firing** in a fresh repro (if it never fires, the user's PDA/pause-menu symptom is NOT the stacking issue
at all, and is something else entirely — see next section).

### Current state after attempt 2 (symptom #3 above) — what this means

- **Inventory Esc now works.** Good — either the StartMenu guard fixed a real stacking case for Inventory, or
  Inventory's own Escape handling (`CUIActorMenu::OnKeyboardAction`, confirmed correct via `[ESCDBG]` logs:
  matches `kQUIT`/`kUI_BACK` binds and calls `OnBtnExitClicked()` → `HideDialog()`) was actually fine all along
  and the earlier "doesn't close" reports were themselves a symptom of the PDA-stacking-under-Inventory
  situation (now prevented).

- **PDA still shows the pause menu on close.** This is the SAME symptom as originally reported, and the
  StartMenu guard evidently did NOT fix it (or fixed a different instance of it than the one recurring now).
  **Next step: re-add/check the `[ESCDBG]` logging is still active (it should be — was not reverted) and get a
  FRESH log from the user reproducing specifically "open PDA, press Esc once, pause menu appears."** Look for:
  - Does `"force-hiding stacked %s first"` appear right before it? If yes, the guard fired but something else
    is still wrong (maybe the guard closes the wrong one, or a third dialog is involved).
  - If NO stacking-guard message appears, then `TopInputReceiver()` was genuinely `NULL` (not another stacked
    dialog) at the moment `kQUIT`'s `if` condition was checked, meaning **the PDA closed via some path OUTSIDE
    the `IR_UIOnKeyboardPress`/`kQUIT` flow entirely** (e.g. an internal PDA close triggered by something
    unrelated to the keypress, a frame/timing race, or Dead Air's known-broken PDA UI — see §8 point 3 —
    self-closing due to its corrupted window tree) — in which case the *real* bug is "something silently closes
    PDA moments before/independent of the Escape press", and the subsequent Escape correctly (per vanilla
    STALKER semantics: Escape with nothing open = open pause menu) opens the menu, but the user perceives it as
    "closing PDA opened the menu" because of the timing coincidence. **This was already suspected before attempt
    2 and is NOT yet resolved or disproven.**
  - Worth checking: is there a **phantom/ghost 'P' keypress** happening on this machine? The exact same
    signature (`StartMenu CUIPdaWnd` with zero preceding input log) was ALSO seen in an unrelated headless
    automated test that used no real keyboard input at all (see the "test-env note" in §9 / FIX17 in memory) —
    raising the possibility a stray input source (background app, macro tool, second input device, or an
    engine-level input-queue artifact) is opening PDA on its own, independent of the user's real presses, on
    THIS machine. Not confirmed either way. Ask the user directly: does the PDA ever appear to flicker/open with
    no keypress?

- **NEW: Esc does not close the NPC dialogue window.** Not investigated at all yet. The relevant class is
  `CUITalkWnd` (`src/xrGame/ui/UITalkWnd.h`/`.cpp`), shown via `CUIGameSP::StartTalk()` (`UIGameSP.cpp:217`,
  `TalkMenu->ShowDialog(true)`) and referenced in `CUIGameSP::HideShownDialogs()` (`UIGameSP.cpp:38-47`, which
  explicitly special-cases `TalkMenu` — `if (mir && mir == TalkMenu) mir->HideDialog();` — suggesting `TalkMenu`
  needs different handling than PDA/ActorMenu elsewhere too; worth re-reading this function's intent).
  **First things to check next session**:
  1. Does `CUITalkWnd` (or its base) override `OnKeyboardAction` / handle `kQUIT` at all? (Quick grep:
     `IsBinded(kQUIT` / `OnKeyboardAction` in `UITalkWnd.cpp` and its parent classes.) If it has NO escape
     handling of its own (like `CUIPdaWnd`), the generic force-close fallback in `Level_input.cpp`'s `kQUIT` case
     (`if (!handled || (now == tir && tir->IsShown())) tir->HideDialog();`) SHOULD still catch it and force-close
     it — if it's not working, find out why this generic fallback isn't engaging for `CUITalkWnd` specifically
     (maybe `IR_process()` returns false during dialogue — e.g. if talking pauses the device and `TalkMenu->
     m_bWorkInPause` is false, the whole `CDialogHolder::IR_UIOnKeyboardPress` would bail before even reaching
     `TIR->OnKeyboardAction`, and if `TopInputReceiver()` afterward still equals `tir` and `tir->IsShown()`, the
     force-close SHOULD still fire from `Level_input.cpp` regardless — but worth confirming with the `[ESCDBG]`
     logs, which already print `TIR=%s IR_process=%s Paused=%d` for exactly this diagnosis).
  2. Check whether the game/dialogue system expects Escape to be a no-op during NPC conversation by design (some
     STALKER mods intentionally block Escape mid-dialogue to prevent accidentally aborting quest-critical
     conversations) — if so, this may not be a bug at all, and the real ask is "how do you normally exit
     dialogue" (usually a numbered dialogue option like "[End conversation]", not Escape). Check
     `ui_scenes.script` / dialogue manager scripts for the intended exit UX before assuming Escape-to-close is
     even supposed to work here.

### `[ESCDBG]` logging status
Still present and active in the current build (not reverted). Locations: `UIDialogHolder.cpp` (
`IR_UIOnKeyboardPress`, `StartMenu`, `StopMenu`, the new stacking guard), `Level_input.cpp` (`kQUIT` case),
`console_commands.cpp` (`CCC_MainMenu::Execute`), `UIActorMenu_action.cpp` (`OnKeyboardAction`,
`OnBtnExitClicked`), `UIGameCustom.cpp` (`ShowActorMenu`, `ShowPdaMenu`, `HideActorMenu`, `HidePdaMenu`). All
gated to Escape/I/P keys only, so log volume stays low even during normal play — safe to leave in for the next
debugging session. **Next session: add equivalent logging to `CUITalkWnd`'s show/hide/keyboard path** before
investigating symptom 3, following the same pattern (tag `[ESCDBG]`, gate to interesting keys, log
`GetDebugType()`/`Device.dwFrame`/`IsShown()` at entry/exit of the relevant functions).

### Build state
Last rebuild (Release, `xrGame.dll`/`xrEngine.exe` relinked) succeeded clean and sanity-booted into Cordon with
no fatals. Uncommitted local changes (git diff will show `UIDialogHolder.cpp`, `Level_input.cpp`,
`console_commands.cpp`, `UIActorMenu_action.cpp`, `UIGameCustom.cpp`, `UIGameSP.cpp` on top of the §7 patch set).
User has NOT yet independently re-confirmed the PDA/dialogue symptoms are fixed — do not claim they are.
- Desktop shortcut: "Dead Air (OpenXRay)"

---

## 12. Escape render/input-desync fix (2026-07-20 evening) — BUILT, awaiting user confirmation

### The decisive reframing
The user restated the symptom cleanly: **(1) Esc in gameplay does NOT open the pause menu; (2) Esc on an open
PDA opens the pause menu instead of closing the PDA.** This is the *exact inverse* of what the engine intends,
and that inversion is the whole clue.

Verified from source how Escape is dispatched:
- The engine delivers each keypress to EXACTLY ONE receiver: `cbStack.back()->IR_OnKeyboardPress(...)`
  (`xrEngine/xr_input.cpp:302`). During gameplay that receiver is always `CLevel`. In-game screens (PDA,
  inventory, talk) do NOT take over the input stack — they live in the dialog holder's internal
  `m_input_receivers`, and `CLevel` forwards to them. So one Esc = one `CLevel::IR_OnKeyboardPress` = ONE branch
  decision. There is no double-dispatch (the earlier double-dispatch theory is disproven).
- `Level_input.cpp` `kQUIT` branches purely on `TopInputReceiver()`: non-null → dismiss the top screen; null →
  `Console->Execute("main_menu")`.

Therefore both symptoms mean ONE thing: **`TopInputReceiver()` (the input-receiver stack) is out of sync with
what is actually RENDERED on screen.** PDA visibly up but TIR==null → else-branch → pause menu (symptom 2).
Nothing visibly up but a stale TIR!=null → if-branch closes an invisible dialog → no visible change, no menu
(symptom 1). Root cause is Dead Air's broken PDA UI (§8 point 3) desyncing render-state from the receiver stack.

The user CONFIRMED (asked directly) that the PDA only ever appears/disappears on their own keypress — **no
phantom self-flicker** — which rules out the phantom-input-source theory (theory B) for their real play and
pins it on the deterministic desync (theory A). NOTE: the runtime log
(`bin/x64/Release/appdata/logs/openxray_user0.log`) is still heavily polluted by the prior automated test
harness (P/I/Esc/talk injected, often P-open and Esc in the SAME frame) — it CANNOT be used to isolate the
user's real single-press repro. Ignore its machine-cycle tail.

### The fix (trust on-screen state, not the receiver stack)
Files changed (all xrGame; on top of §7 + §11 changes; diff regenerated at
`D:\openxray-build\deadair-openxray-source-patches.diff`):
- `Level_input.cpp` `kQUIT`: capture `tir = TopInputReceiver()` once, and only run the "dismiss top dialog"
  if-branch when `tir && tir->IsShown() && !Paused()` — i.e. the receiver must ACTUALLY be rendered. Else-branch
  now calls `ui->HideShownDialogs()` and only executes `main_menu` if that closed nothing (`closedShown` false).
  So: a screen that is visibly up but off the stack gets closed (fixes symptom 2); a stale non-rendered receiver
  is ignored and Esc opens the menu (fixes symptom 1a); truly-clear screen opens the menu as before.
- `UIGameCustom.h`: `virtual void HideShownDialogs() {}` → `virtual bool HideShownDialogs() { return false; }`
  (base/MP unchanged in behavior, just returns false).
- `UIGameSP.h` / `UIGameSP.cpp`: `HideShownDialogs()` now returns `bool` = "did I close a visibly-shown screen".
  It checks `ActorMenu->IsShown()` / `PdaMenu->IsShown()` / `TalkMenu->IsShown()` (render state) plus the old
  `TopInputReceiver()==TalkMenu` case, closes each shown one, returns whether anything was closed. As a bonus
  this also closes a shown-but-not-TIR Talk window (the §11 "Esc doesn't close NPC dialogue" symptom).

Build: incremental Release succeeded clean; `xrGame.dll` relinked (xrEngine unchanged). `[ESCDBG]` logging is
still in place; the else-branch log now also prints `closedShown=` and whether it went to `main_menu` or
`closed-shown-screen`.

### What to check on the user's next clean manual repro
Grep the fresh log for `[ESCDBG] kQUIT`:
- Symptom 2 gone if, on Esc over an open PDA, you see `kQUIT else-branch ... closedShown=1 -> closed-shown-screen`
  (PDA was shown-but-off-stack and got closed) OR the if-branch fires normally (`tir=CUIPdaWnd ... IsShown`).
- Symptom 1 gone if Esc in real gameplay logs `kQUIT else-branch ... closedShown=0 -> main_menu` and the menu
  appears.
- If symptom 1 persists with a `kQUIT tir=<something> ...` if-branch line (a stale receiver that IS reported
  IsShown()=true but renders nothing), the desync is "on-stack + IsShown-true but not visually drawn" — a
  deeper Dead Air PDA-render problem (§8 point 3); next step there is the PDA UI port, not more input plumbing.
- Once confirmed working, consider removing the `[ESCDBG]` logging (locations listed in §11) for a clean build.

---

## 13. Escape fix ROUND 2 (2026-07-20 late) — both symptoms root-caused via arrival-time snapshot

§12's fix resolved symptom 2 (PDA+Esc no longer opens the menu) but NOT symptom 1 (Esc in gameplay still did
nothing). The `[ESCDBG] IR_OnKeyboardPress ESC entry` log added in §12 cracked it:

```
ESC entry: key=41 action=52(kQUIT=52) screenWasUp=0 ... frame=1069   <- nothing open when Esc arrived
kQUIT tir=CUIPdaWnd handled=1 now=null nowShown=-1 frame=1069        <- but kQUIT finds a PDA to close
```

The ONLY code between the entry snapshot and the `kQUIT` switch is the Lua **eKeyPress callback**
(`Level_input.cpp:134`, `g_actor->callback(GameObject::eKeyPress)(key)`). So:

**ROOT CAUSE (both symptoms): a Dead Air Lua script's key-press callback mutates dialog state on Escape,
and the engine's `kQUIT` then acts on the post-callback state, cancelling out the player's intent.**
- Symptom 1: script OPENS the PDA on Esc (direct `pda:ShowDialog()` — note NO `ShowPdaMenu called` log, it
  bypasses the engine wrapper), then `kQUIT` closes it -> nothing visible, menu never opens.
- Symptom 2: script CLOSES the PDA on Esc, then `kQUIT` sees an empty screen -> opens the pause menu.

### The fix (Level_input.cpp `kQUIT`, replaces the §12 if/else)
Anchor Escape's action to `esc_screen_was_up` — the snapshot taken at the TOP of `IR_OnKeyboardPress`,
BEFORE the eKeyPress callback (`CurrentGameUI()->AnyFullscreenShown()`, a new non-destructive query added in
§12: TopInputReceiver || PDA/inventory/talk IsShown):
- `esc_screen_was_up == true`  -> Escape must DISMISS the screen: let a live top dialog process it then
  force-close if stuck, else `HideShownDialogs()`. Never opens the menu.
- `esc_screen_was_up == false` -> player wants the PAUSE MENU: `HideShownDialogs()` to clear any PDA the
  script spuriously opened during the callback, then `Console->Execute("main_menu")`.

This is mod-agnostic (doesn't depend on identifying the offending script) and symmetric. The DeadZone pause
menu is a `CUIScriptWnd` opened by the `main_menu` console command (NOT a PDA), so closing the script-opened
PDA and running `main_menu` gives the player the screen they expect.

Build: incremental Release, `xrGame.dll` relinked, exit 0. `[ESCDBG]` logging still in place; new tags:
`kQUIT screenWasUp:` and `kQUIT screenWasDown:`.

### STATUS: awaiting user re-test #3
Symptom 2 already confirmed fixed by the user after §12. This round targets symptom 1. NEXT: user does the same
2-step clean repro (Esc in gameplay -> expect pause menu; P then Esc -> expect PDA closes). If symptom 1 is
fixed, remove the `[ESCDBG]` logging (locations in §11+§12) for a clean build. If a PDA still FLASHES for one
frame on gameplay-Esc (script opens it, we close it same frame), that's cosmetic; the deeper fix would be to
stop the Lua script opening the PDA on Esc (grep loose scripts for a key callback doing `pda:ShowDialog` / a
kQUIT/DIK_ESCAPE handler), but the engine anchor already yields correct end-state behavior.

---

## 14. "Crashes ~2–4 min after new game / sooner on later saves" — TWO chained crashes (2026-07-20 late)

### Symptom (as reported)
New game crashes ~4 min in "regardless of what I do"; loading a save does NOT reset the timer, and the interval
SHRINKS as later saves are loaded. **Diagnosis: not one timer — it's A-Life autonomous combat.** NPCs fight and
die on their own a few minutes after the world starts churning; the crash fires on the first NPC death / first
NPC combat, independent of player action, and sooner in later saves because the fighting is already further along.
Two distinct crashes were hiding behind the same symptom; fixing the first revealed the second.

### Crash A — `death_manager.script:438: attempt to call method 'get_weapon_condition_type' (a nil value)`
Fires when `death_manager` processes a dead NPC's dropped weapon (`set_weapon_drop_condition`). Same class as the
§7 missing-export gaps: Dead Air's engine had a per-weapon **u32 "condition type" bitmask** (which parts are
broken: bits ~27–30 = firemode/scope/silencer/GL, bit 2^28 read by `dxr_scopes`) with a getter/setter pair
`get_weapon_condition_type()` / `set_weapon_condition_type(u32)`, called on the CLIENT game object wrapper
(`CScriptGameObject`). Also used by `items_condition.script` (`break_weapon`/`get_break`), `dxr_scopes.script`,
`inventory_upgrades.script`. Default 0 = "no break assigned yet", which is exactly what NPC-dropped weapons need
(0 → enter the randomize-condition branch).

**Fix (source):**
- `inventory_item.h` — added `u32 m_weapon_condition_type{0}` + inline `GetWeaponConditionType()/SetWeaponConditionType(u32)`.
- `script_game_object.cpp` / `.h` — `CScriptGameObject::GetWeaponConditionType()/SetWeaponConditionType()` forwarding
  via `smart_cast<CInventoryItem*>` (mirrors `GetCondition/SetCondition`).
- `script_game_object_script2.cpp` — luabind `.def("get_weapon_condition_type", ...)` / `.def("set_weapon_condition_type", ...)`.
- **LIMITATION: transient, NOT serialized.** Storing it means default 0 on every load, so a weapon's broken-part
  state resets across save/load (cosmetic; e.g. a "broken scope" un-breaks on reload). Chose transient to avoid
  changing the inventory-item save format (would break the user's existing saves). If persistence is wanted later,
  serialize in `CInventoryItem` save/load behind a save-version bump. **CONFIRMED by user: Crash A no longer occurs.**

### Crash B (revealed after A) — GOAP planner abort `!this->solution().empty()`
`FATAL: Expression !this->solution().empty()` in `CActionPlanner<CScriptGameObject,...>::update`
(`action_planner_inline.h:90`), stack `CStalkerCombatPlanner::update` → `CAI_Stalker::Think` → `shedule_Update`.
An NPC entered combat and the GOAP solver found no action sequence to reach its goal this frame (transiently
unsolvable combat graph — common in a partial CoC/Dead Air port). Alundaio had already added a graceful
`if (this->solution().empty()) return;` guard immediately below the `THROW`, but the `THROW` fires FIRST. `THROW`
is only live because this Release build defines `XRAY_EXCEPTIONS` (else `THROW`==`VERIFY`, a NDEBUG no-op — which
is how the retail Dead Air engine tolerated this).

**Fix (source):** `action_planner_inline.h:90` — commented out the `THROW(!this->solution().empty());`; the
existing empty-solution `return` now handles it (NPC simply idles that frame instead of crashing; self-recovers
when world state changes). Mod-agnostic, matches author intent. If combat AI later looks passive/stuck, THAT is
the deeper follow-up (a genuinely missing/mis-registered combat action or evaluator in the GOAP graph), but it is
no longer a crash.

### Build/verify — BOTH FIXED, USER-CONFIRMED (2026-07-20 late)
Both fixes built incrementally clean (Release x64, exit 0), `xrGame.dll` relinked.
- **Crash A** (death_manager) — user-confirmed fixed (error no longer appears).
- **Crash B** (GOAP planner) — first verified via a ~5-min soak (zero `FATAL ERROR` in a 91k-line log, no
  minidump, no Windows Application Error event), then **USER-CONFIRMED**: fresh new game ran **12 minutes
  straight** and was closed cleanly from the menu with no issue. The reported "~2–4 min crash / shrinks with
  later saves" symptom is **RESOLVED**.

Non-fatal log noise observed during play (harmless, do NOT chase as crashes): the `r2_dof_*`/`r2_lens*` "Unknown
command" spam (§8 point 5) and a `CRestrictedObject: you use accessible_nearest when position is already
accessible!` LUA WARNING from `xr_danger.script` corpse-reaction (single `!` = warning, not a fatal).

### Follow-up left open (intentional, not a bug)
Weapon condition-type is **transient (not serialized)** — a weapon's broken-part state resets to 0 on save/load
(cosmetic; never a crash). Chosen to avoid changing the `CInventoryItem` save format and breaking existing saves.
To make it persist: serialize `m_weapon_condition_type` in `CInventoryItem` save/load behind a save-version bump.

---

## 15. Loading-screen progress bar renders as random noise/dots (2026-07-20) — FIXED, user-confirmed visually

### Symptom (as reported)
On every new-game / level load, the loading progress bar shows **artifacts / a random set of colored (black-white)
dots** — the bar's *shape/position is correct* but its pixel content is garbage noise. Everything else on the
loading screen (the S.T.A.L.K.E.R. frame background, the rusty widescreen side panels, the centered level image,
the tips text) renders **correctly**.

### Root cause (fully diagnosed, not guessed)
Dead Air / CoC **ships NO loading-screen config** — there is no `ui_mm_loading_screen.xml` (or any
`loading_progress` / progress-bar definition) anywhere in `$game_config$` (verified by grepping the unpacked loose
configs at `D:\DAR3\gamedata\configs\`, incl. `ui\`). So OpenXRay falls back to its **hardcoded** loading screen in
`src/xrGame/ui/UILoadingScreenHardcoded.h` (see `UILoadingScreen::Initialize()` in `UILoadingScreen.cpp:30` — it
tries to load `ui_mm_loading_screen.xml`, fails, and uses the hardcoded C++ raw-string XML + texture descriptions).

That hardcoded fallback is the **stock Call of Pripyat layout**. Its texture description
(`LoadingScreenXMLTexturesDescription`) maps the progress-bar texture ID `ui_mm_loading_progress_bar` to
**file `ui\ui_actor_loadgame_screen`, atlas sub-rect `x=0 y=772 w=506 h=4`** — i.e. a thin strip *below* the
1024x768 main image, where retail CoP's atlas keeps its progress-bar graphic.

**Dead Air's version of `ui\ui_actor_loadgame_screen.dds` has no valid progress-bar strip at y=772** (that area is
empty/undefined/garbage). Decisive evidence: the loading-screen **background** uses the SAME texture file
(`ui_mm_loading_screen` → `ui\ui_actor_loadgame_screen` rect `0,0,1024,768`) and renders **perfectly**, while the
**progress bar** (rect `0,772,506,4` of the same file) is pure noise. Same file, valid region fine + y=772 region
garbage ⇒ the texture simply doesn't contain a bar there. (On the *retail* Dead Air/CoC engine this never showed
because that engine's built-in loading screen used a different texture/region that matched the shipped asset.)

UV mapping confirmed in `src/xrUICore/XML/UITextureMaster.cpp` (`ParseShTexInfo` stores the rect in absolute
pixels; it becomes UV = pixel/actual-texture-size at draw time) and the bar draw path in
`src/xrUICore/ProgressBar/UIProgressBar.cpp` (`CUIProgressBar::Draw` scissors the fill `m_UIProgressItem` to the
computed bar rect — that's why the *shape* is right but the sampled *content* is garbage).

### The fix (dimension-independent, preserves the working background/panels)
Instead of chasing what Dead Air's atlas actually contains, point ONLY the progress bar at a known-good solid
texture; leave the background/side-panels/level-image untouched (they work).

1. **New loose texture** `D:\DAR3\gamedata\textures\ui\oxr_loading_bar.dds` — an **8x8 solid-white, uncompressed
   32-bit BGRA (A8R8G8B8) DDS** (all bytes 0xFF; 384 bytes total: 128-byte header + 256 bytes pixels). Generated by
   hand (PowerShell byte-writer) — a trivial always-white fill texture, so sampling any sub-rect yields solid white.
   Resolves via `$game_textures$` (loose `gamedata\textures\` overrides archives).
2. **Source patch** `src/xrGame/ui/UILoadingScreenHardcoded.h` — in the two **Call of Pripyat** variants
   (`LoadingScreenXML` = 4:3 and `LoadingScreenXML16x9` = widescreen; **this machine is 1920x1080 → the 16x9 variant
   is the one actually used**), changed the `<loading_progress>` block's `<progress>` and `<background>`
   `<texture>` from `ui_mm_loading_progress_bar` to **`ui\oxr_loading_bar`**. The `<background>` keeps its
   `r=0 g=0 b=0 a=255` tint → black bar backing; the `<progress>` fill is now solid white. (The ClearSky / SoC
   variants in the same header were left unchanged — they use `ui\ui_load` and aren't used in this CoP-mode build.)
   Because the fill samples an all-white texture, the fix does NOT depend on the atlas's real dimensions/content.
3. **Rebuild**: standard incremental Release (`§4` command). `UILoadingScreen.cpp` recompiled, `xrGame.dll` +
   `xrEngine.exe` relinked, exit 0. (`UILoadingScreenHardcoded.h` is `#include`d by `UILoadingScreen.cpp` in
   xrGame — a header-only change, so xrGame rebuilds.)

### Status: FIXED (user-confirmed by screenshot)
After the rebuild, the bar renders as a **clean solid white bar** in the correct position — no more noise/dots.

### Left open (user suspects a SEPARATE bug — NOT yet investigated)
User reports the (now-white) bar **"sometimes glitches out"** intermittently during load. This is almost certainly
unrelated to the palette/noise root cause above (which is deterministic and now gone). Leading hypotheses for next
session, in order:
- The loading render happens on a minimal path (`IGame_Persistent::LoadDraw()` → `RenderBegin/End`, called from
  `CRenderDevice::BeforeFrame` while draining `g_loading_events`), decoupled from the normal render cycle. The
  progress value is pushed via `UILoadingScreen::Update` → `ForceSetProgressPos` under a `ScopeLock` — a race /
  partial-frame between the loader thread updating progress and the draw could momentarily draw a wrong-width or
  stale scissor rect. Check `CUIProgressBar::Draw` vs `Update` threading and the `loadingLock` usage in
  `UILoadingScreen.cpp`.
- A one-off flicker where the fill scissor width is briefly 0/negative or the background static's rect is
  mis-sized on the first frame(s). Add `[LOADDBG]` logging of `m_CurrentLength` / `progress_rect` in
  `CUIProgressBar::Draw` and the pushed `progress` value in `UILoadingScreen::Update` to catch the transient.
- Confirm the intermittency is the BAR specifically (not the whole screen / a load-hitch redraw). Ask the user to
  describe the glitch (does the bar vanish, show noise again for a frame, jump width, or does the whole screen
  tear?).

### Key files touched (regenerate the saved diff — it does NOT yet include these)
- `src/xrGame/ui/UILoadingScreenHardcoded.h` (2 progress-bar texture refs)
- NEW asset: `D:\DAR3\gamedata\textures\ui\oxr_loading_bar.dds` (loose, not in the source diff — it's a content file)
- Diagnostic scratch (screenshots, capture script) lived in the session scratchpad, not the repo.

---

## 16. Drag-ammo-onto-equipped-weapon crash — `is_ammo_suitable` (a nil value) (2026-07-21) — FIXED, USER-CONFIRMED

### Symptom (as reported, with screenshot)
Dragging an ammo box onto an equipped weapon in the inventory fatals:
`itms_manager.script:253: attempt to call method 'is_ammo_suitable' (a nil value)`.

### Root cause
Same class as §7 — a missing engine script export. `itms_manager.script`'s quick-reload path
(lines 242–290, triggered by dropping ammo on a weapon that is in a slot) calls two weapon methods on the
`CScriptGameObject` wrapper that stock OpenXRay does not bind: `is_ammo_suitable(section)` (line 253) and
`get_ammo_name()` (used right after at lines 261/266). Only `is_ammo_suitable` shows in the fatal because it's
the FIRST of the two hit; `get_ammo_name` would have fataled on the very next step. The sibling ammo methods
(`get_ammo_in_magazine`, `get_ammo_type`, `set_ammo_type`, `unload_magazine`, `ammo_*`) were already bound by
earlier sessions in `script_game_object_script2/3.cpp`.

### Fix (source, matches the §7/§9 export pattern)
- `script_game_object.h` — declared `LPCSTR GetAmmoName()` and `bool IsAmmoSuitable(LPCSTR ammo_section)`
  (right after `GetAmmoType()`).
- `script_game_object.cpp` — implemented both via `smart_cast<CWeapon*>(&object())`, reading the public
  `CWeapon::m_ammoTypes` (xr_vector<shared_str> of loadable ammo sections) + `m_ammoType` (current index):
  `GetAmmoName` returns `m_ammoTypes[m_ammoType].c_str()` (nullptr → Lua nil if no weapon / index out of range);
  `IsAmmoSuitable` returns true if the passed section (interned to `shared_str`) is present in `m_ammoTypes`.
  NOTE: get LPCSTR from shared_str via `.c_str()` — shared_str has no `operator*` (a `*deref` C2100'd the build).
- `script_game_object_script2.cpp` — luabind `.def("get_ammo_name", ...)` / `.def("is_ammo_suitable", ...)`
  next to `get_ammo_type`/`set_ammo_type`.

### Build state — USER-CONFIRMED FIXED (2026-07-21)
Incremental Release built clean (exit 0), `xrGame.dll` + `xrEngine.exe` relinked. User tested manually:
matching-caliber ammo box dropped on an equipped weapon quick-reloads as intended; non-matching type does
nothing. No fatal in either case. **RESOLVED.**

---

## 17. Map tablet: dropping a map card onto the tablet did nothing (2026-07-21) — FIXED (awaiting user test)

### Symptom (as reported)
Dragging a map card onto the blank map tablet (planshet) in the inventory did **nothing**. Intended: the tablet
should then display that map when equipped as a weapon (in-hands HUD).

### Item taxonomy (Dead Air planshet system)
- `animation_planshet` — the **blank** tablet. Equippable (slot 2), has HUD `animation_planshet_hud`
  (`item_visual director_ntv\planshet\planshet_hud` = blank tablet in hands). Config:
  `configs\misc\items\items_animations_dar2.ltx:281`.
- `planshet_karta_dar2_*` — a **map card** (19 of them, quest-specific). Plain `:rope` inventory item, no
  functor. Config: `configs\misc\items\items_dar2.ltx:721+`.
- `planshet_map_dar2_*` — the **ready tablet-with-map** (inherits `animation_planshet`; its own `_hud` sets
  `item_visual` to the map-textured tablet model). Has `use1_functor/use1_action_functor` =
  `dar2_planshet_map.script` "Изъять"/extract, which **splits it back** into a blank `animation_planshet` +
  the matching `planshet_karta_*` card. Config: `items_animations_dar2.ltx:310+`.

Quests (`dar2_quest_poboch.script`) hand the player the **blank tablet + a map card SEPARATELY**, so composing
them into the ready tablet is required gameplay — but only the reverse (extract) was scripted.

### Root cause
No loaded Lua script implemented the **compose** direction (card + blank tablet -> `planshet_map_*`).
Confirmed: grep of all loose `gamedata\scripts\*.script` for `planshet` hits only `dar2_planshet_map.script`
(extract only) and `dar2_quest_poboch.script` (quest give). The retail modded `D:\DAR3\xrGame.dll` contains **no**
`planshet`/`karta` strings — item drops are dispatched to the **script** callback `CUIActorMenu_OnItemDropped`
(same path §16's ammo-reload uses, confirmed firing), not handled in C++. So the feature was simply never wired
up on the script side. **This is a content (loose-script) fix, NOT an engine change — no rebuild needed.**

### Fix (loose script, mod-agnostic via naming convention)
`D:\DAR3\gamedata\scripts\itms_manager.script` — added `on_planshet_drag_dropped(itm1,itm2,...)`, called from
`on_item_drag_dropped` alongside the ammo/consumable handlers. When a `planshet_karta_*` card is dropped onto
`animation_planshet` (either drag order accepted), it derives the target section by swapping the section prefix
`planshet_karta_` -> `planshet_map_` (verified: all 19 cards map 1:1 to an existing `planshet_map_*` section),
releases both the card and the blank tablet, and `alife():create`s the ready `planshet_map_*` in the actor's
inventory (mirror of the extract functor). Plays `inv_stack`. The player then equips the composed tablet to see
the map in-hands (matches the "when equipped as a weapon" expectation — no auto-equip, kept minimal/synchronous).

### Status — USER-CONFIRMED FIXED (2026-07-21)
Loose-script change only (no rebuild). After restarting the game, the user dropped a map card onto the blank
tablet, it composed into the ready tablet-with-map, and equipping it displays the map on the in-hands HUD as
intended. **RESOLVED.** (If a composed tablet's HUD map texture ever renders wrong/missing, that's the separate
Dead-Air PDA/texture-porting tail, §8 point 3 — not this compose wiring.)

---

## 18. In-session save-load render corruption (2026-07-21) — UNRESOLVED, RenderDoc capture is the agreed next step

### Symptom
Loading a save **while already in a level** (F9 quickload, console `load`, or the pause-menu Load screen — all
three funnel through the same code path) sometimes leaves the screen visibly wrong: uniformly dim/dark (despite
correct in-game time — confirmed via screenshot showing mid-afternoon `15:13` game clock over a near-black
scene), or a color-scrambled but structurally coherent render of the real scene (looks like a raw G-buffer, not
noise — see below). Persists until a full game restart. **Also confirmed it can happen on the very FIRST level
load of a session** (not exclusively an in-session-quickload bug), and pitch (looking up/down) affects whether
it's visible on a still-corrupted frame, though pitch-dependence is not 100% consistent across repros.

### Investigation summary (this session, exhaustive elimination via real repro + `[PPDBG]` log instrumentation)
Tested and **ruled out**, each with real same-save repro data:
- HDR auto-exposure/tonemap metering (`rt_LUM`/`LUM8` values are byte-identical between clean and corrupted
  loads in every comparison done — this had been the leading theory in earlier sessions and is now dead).
- Ambient/sun/env-color/fog CPU-side shader params (identical across clean/corrupted loads).
- Object/dynamic-light respawn completeness after the soft reload (object count is fully populated from frame 0
  of the post-load window, not still streaming in).
- Torch/flashlight state (user confirmed torch off/unused during a dim repro).
- `r2_mt_render` / `r2_mt_calculate` (X-Ray's render/calculate multithreading) — both forced off in
  `bin\x64\Release\appdata\user.ltx`, bug still reproduces.
- HOM (hierarchical occlusion culling) — relaunched with `-no_hom` (fully disables `CHOM`), bug still
  reproduces "oftenly", both dim and glitched variants seen.

**One real bug found and fixed** (kept, built into `xrRender_R4.dll`): `CRender::Render()`
(`src/Layers/xrRender_R2/r2_R_render.cpp`) has two early-return branches that skip the whole scene/combine
pipeline for a frame. One (`!g_pGameLevel || bMenu`) explicitly rebinds the real backbuffer (`get_base_rt()`)
before returning; the other (`m_bFirstFrameAfterReset`, set once per `IRender::Reset()`/`reset_end()` — see
`r2.cpp:614`) did NOT, so whatever RT was last bound (e.g. a G-buffer target from the scene pass) stayed
presented on screen indefinitely. `[PPDBG]` logging added at the same time (`Render() skip` /
`Render() firstFrameAfterReset skip` / periodic `Render() normal path` heartbeat in `r2_R_render.cpp`, plus
`[PPDBG] CRenderDevice::Reset() called` in `Device_destroy.cpp`) **confirmed** `m_bFirstFrameAfterReset` fired
exactly once during a load that came out dim, and never during a load that came out clean — a real, if
small-sample, correlation. The fix mirrors the sibling branch (adds the missing `u_setrt(...get_base_rt()...)`
call) — safe, minimal, and should stay regardless of further findings.

**But it does NOT fully fix the bug.** Retested after the fix: the pitch-down color-scramble glitch reproduced
again, and this time there was **zero** `Reset()`/`firstFrameAfterReset` activity anywhere near the relevant
load in the log (the only `Reset()` in that entire session was the one-time engine-startup reset). So there is
at least a second, still-unidentified mechanism producing the same class of symptom (or the "wrong RT left
bound" theory is right but via a different code path than the one found and fixed).

### Screenshot evidence (important for whoever picks this up)
Two corrupted screenshots were captured and are NOT random noise — both are clearly coherent renders of the
actual scene geometry with the wrong data mapped to color, strongly suggesting **a raw G-buffer (most likely
the surface-normal target, `rt_Normal` / `r2_RT_N` in `r2_rendertarget.cpp`) is what's ending up on screen
instead of the final lit/tonemapped image**:
- One screenshot: flat cyan/turquoise ground+grass (a uniform up-facing normal maps to a uniform color) with
  noisy red/yellow/green tree canopy (foliage normals point every which way → high-frequency color noise).
  Textbook "normal.xyz shown directly as RGB" look.
- Another screenshot (different repro, also pitch-down): fine radiating swirl/scratch pattern centered
  low-screen. Plausibly the same raw-normal-buffer theory but showing rain-ripple perturbation normals instead
  — `src/Layers/xrRender_R2/r3_rendertarget_draw_rain.cpp:311` writes wet-surface ripple normals directly into
  `rt_Normal` as part of the G-buffer pass (`u_setrt(cmd_list, rt_Normal, nullptr, nullptr, rt_MSAADepth)`),
  and it was raining / ground was wet in that repro. Not confirmed, but a coherent explanation for why the
  pattern differs run to run (whatever was most recently written into `rt_Normal` is what leaks through).

### NEXT STEP (agreed with user): RenderDoc GPU frame capture
Static code reading is exhausted for now — this needs to be seen directly in a GPU capture, not inferred from
source. Steps for the next session:
1. Install RenderDoc (free, https://renderdoc.org/) if not already present on this machine.
2. In RenderDoc's "Launch Application" tab, set:
   - Executable Path: `D:\openxray-build\xray-16\bin\x64\Release\xrEngine.exe`
   - Working Directory: `D:\openxray-build\xray-16\bin\x64\Release`
   - Command-line Arguments: `-fsltx D:\DAR3\fsgame.ltx -r4 -noprefetch -dbg`
   - Defaults are fine otherwise (D3D11 capture — this build already runs DX11 via `-r4`).
3. Click "Launch". The game starts hooked by RenderDoc. Play normally, get into a level.
4. Reproduce the bug the same way as every prior session here: reload the same save repeatedly (F9 or console
   `load`) until dim or color-scrambled appears. The corruption is stable/persists across frames once
   triggered, so there's no tight timing window — press the capture hotkey (default **F12** or Print Screen)
   any time while it's visible.
5. Switch to the RenderDoc window — a new capture thumbnail appears in its capture list. Double-click to open
   the frame analysis UI.
6. In the Texture Viewer, locate the final backbuffer/swapchain output (last entry in "Outputs", or navigate
   the Event Browser to the final `Present`). Right-click the corrupted area and use **"Pixel History"** — it
   lists every draw call that wrote to that pixel this frame, with a thumbnail after each write; the last entry
   before Present is what's actually on screen.
7. Cross-check against the named G-buffer textures in the resource list (`r2_RT_N` = rt_Normal, `r2_RT_generic0`
   /`generic1` = rt_Generic_0/1, `r2_RT_albedo` = rt_Color, `r2_RT_accum` = rt_Accumulator, etc. — names come
   from `r2_rendertarget.cpp`) to see which one visually matches the corrupted screen. This directly confirms
   or refutes the "raw rt_Normal displayed" theory.
8. Also check whether `phase_combine`/`combine_2`-related draw calls are present in that frame's Event Browser
   at all — if they're missing or the event list is unexpectedly short, that's direct proof the compositing
   pipeline didn't run this frame, regardless of which specific flag caused it.
Report back exactly which texture the corrupted pixels match — that points directly at the code path to fix
next instead of more blind guessing.

### Current state for next session
- `[PPDBG]` logging from this whole investigation is still in the build (harmless when idle, all
  gated/rate-limited) — grep `openxray_user0.log` for `[PPDBG]`. Locations: `GamePersistent.cpp` (`OnFrame`,
  post-load object count), `r2_R_render.cpp` (`Render()` skip/heartbeat), `Device_destroy.cpp`
  (`CRenderDevice::Reset()`), plus earlier exposure/env logging in `r2_rendertarget_phase_luminance.cpp` /
  `r4_rendertarget_phase_combine.cpp` / `ColorMapManager.cpp` / `CameraManager.cpp` / `r2.cpp`.
- `r2_mt_calculate` / `r2_mt_render` both `0` in `bin\x64\Release\appdata\user.ltx` — didn't fix the bug, no
  strong reason to revert either.
- Desktop shortcut ("Dead Air (OpenXRay)") args are back to normal; `-no_hom` was tested and reverted.
- The `m_bFirstFrameAfterReset` render-target rebind fix (`r2_R_render.cpp`) is shipped and built into
  `xrRender_R4.dll` — keep it regardless of what RenderDoc finds, it's a real fix for a real (if partial) cause.
- Full memory notes (more granular chronology of every test this session) at the auto-memory file
  `dar3-load-render-corruption` — check there too for anything not repeated here.

---

## 19. Color-scrambled/"confetti" load corruption — ROOT-CAUSED AND FIXED via RenderDoc (2026-07-22)

### What this section covers
§18 left TWO symptoms lumped together under "in-session save-load render corruption": (a) a color-scrambled,
structurally-coherent-but-wrong-colored full-screen glitch, and (b) a uniformly dim/under-lit screen. This
session used an actual RenderDoc GPU frame capture (the plan §18 laid out) to chase symptom (a). **Symptom (a)
is now root-caused and FIXED, user-confirmed clean over 10 consecutive same-save reloads.** Symptom (b), the dim
variant, was NOT touched this session and remains open — see "What's still open" below.

### Methodology (reusable for the dim bug next session)
1. Installed/used RenderDoc (`qrenderdoc.exe`, already present at `C:\Program Files\RenderDoc`), launched
   `xrEngine.exe` via RenderDoc's "Launch Application" tab (`-fsltx D:\DAR3\fsgame.ltx -r4 -noprefetch` — no
   special `-dbg` flag needed; that flag doesn't exist in this engine's arg parsing, RenderDoc hooks D3D11
   regardless of game args).
2. Reproduced the scramble by repeatedly quickloading the same save until the corrupted frame appeared, captured
   it with F12.
3. In the Texture Viewer, right-clicked a pixel **inside** the visibly corrupted region (not a screen corner —
   corners can be outside the affected area) and used **Pixel History** on the swapchain output. This lists every
   draw that touched that pixel, in order, with before/after color.
4. Found the actual value-changing draw (not just alpha-blend no-ops), double-clicked to jump to it in the Event
   Browser, then walked BACKWARDS through the event tree (`combine_1` → `phase_combine_volumetric` → `phase_bloom`
   → `render_distort_objects` → `combine_2` → `LENS_FLARES`/`phase_pp`) checking each draw's bound input textures
   (Pipeline State → Pixel Shader → Resources, hover thumbnails) against the corrupted pattern, until the FIRST
   draw whose output didn't match a clean frame was found.
5. Note: GPU debug markers here are RAII scope guards (`PIX_EVENT` macro, `dxPixEventWrapper` in
   `Layers/xrRender/Debug/dxPixEventWrapper.h`) that only close at the end of their C++ enclosing scope — so a
   marker (e.g. `LENS_FLARES`) can visually contain later, unrelated draws (e.g. `phase_pp`) in the Event Browser
   tree purely because the marker object hadn't gone out of scope yet in the source. Don't assume marker nesting
   reflects logical ownership — verify by checking each draw's actual vertex data / bound resources.

### Root cause
`combine_2` (`r4_rendertarget_phase_combine.cpp`, inside `CRenderTarget::phase_combine`) is the AA/DOF/distortion
"combine" pass. Its pixel shader unconditionally samples a distortion-mask texture (`s_distort`, bound to
`rt_Generic_1_r`) and uses it to offset where it samples the final scene color — i.e. it warps the image using
whatever is in that buffer.

`rt_Generic_1_r` is a **reused scratch render target with two unrelated purposes within the same frame**:
1. Near the top of `phase_combine` (line ~51), it's unconditionally cleared to `{}` (zero) and bound as an MRT
   target for the main scene/lighting/SSAO passes — real G-buffer/lighting content gets written into it.
2. Later (~line 288, inside `if (bDistort)`), it's **repurposed** as the distortion mask: rebound, re-cleared to
   the neutral "no distortion" value `color_rgba(127,127,0,127)`, and anomaly/heat-haze distortion quads
   (`dsgraph.render_distort()`) are drawn into it — but **only if `bDistort` is true**, i.e. only if
   `dsgraph.mapDistort` is non-empty (at least one active distortion-emitting object, e.g. an anomaly).

**The bug**: when `bDistort` is `false` this frame, the rebind + neutral-clear is skipped entirely (it was nested
inside the same `if` as the actual distortion draw). `rt_Generic_1_r` is left holding whatever the earlier
G-buffer/lighting pass wrote into it — real rendered data, just unrelated to distortion — and `combine_2` samples
it anyway, warping the final scene using leftover lighting/SSAO values misread as UV offsets. This produces
exactly the observed symptom: a coherent render of the real scene with color scrambled, not literal noise.

This explains the save-load trigger: after `restart_simulator()` respawns the world, anomaly/distortion-emitting
objects need a few frames to re-register into `dsgraph.mapDistort`, so `bDistort` is very plausibly `false` for
the first frame(s) after a load even if you were standing in an anomaly zone before saving — matching the
intermittent, non-deterministic reproduction seen all session (distorted/normal/normal/dim across repeated
reloads of the same save).

### The fix (shipped, user-confirmed)
`src/Layers/xrRenderPC_R4/r4_rendertarget_phase_combine.cpp` — moved the rebind (`u_setrt`) and neutral clear
(`RCache.ClearRT(rt_Generic_1_r, color_rgba(127,127,0,127))`) OUTSIDE the `if (bDistort)` gate, so they run every
frame unconditionally; only the actual `dsgraph.render_distort()` draw (plus its state setup) stays gated behind
`bDistort`. This guarantees `s_distort` always holds the neutral value when nothing is actively distorting,
eliminating the stale-buffer read. Minimal, safe, same pattern as the already-shipped `m_bFirstFrameAfterReset`
fix from §18 (always establish valid state instead of conditionally skipping it).

Rebuilt (Release x64, standard `§4` command), `r4_rendertarget_phase_combine.cpp` recompiled,
`xrRender_R4.dll` relinked, exit 0. **User-tested: 10 consecutive same-save reloads, zero recurrence of the
color-scramble/glitch symptom.** Keep this fix regardless of the dim-bug follow-up.

Only the R4 (DX11) file was patched — `src/Layers/xrRenderPC_GL/gl_rendertarget_phase_combine.cpp` has the
identical `rt_Generic_1_r`/`bDistort` pattern (same variable names, same structure) and is presumably affected
the same way, but this build only uses `-r4`, so it was left unpatched. Apply the same fix there first if GL
rendering is ever brought up.

### What's still open (for the next session)
Nothing — see §20 below. The dim variant was root-caused and fixed 2026-07-23 using exactly the RenderDoc
methodology this section called for.
- `[PPDBG]` logging from the §18 investigation is still in the build — still safe to leave, still useful for
  cross-referencing frame numbers if resumed.
- `r2_mt_calculate`/`r2_mt_render` still `0` in `user.ltx` (no effect on either bug, no strong reason to revert).

---

## 20. Uniformly-DIM load variant — ROOT-CAUSED AND FIXED via RenderDoc (2026-07-23)

### What this section covers
§18/§19 left the dim variant (uniform under-exposed level after an in-session load, sky and shadow alike, correct
game-time but wrong brightness) as the one still-open piece of the save-load render-corruption bug family. This
session repeated §19's RenderDoc methodology on an actual dim repro capture and found the root cause. **FIXED,
user-confirmed** (level no longer dims itself across repeated reloads after the fix).

### Methodology (same tool, different pixel — see §19 for the general RenderDoc setup/capture steps)
This session's capture was driven collaboratively pixel-by-pixel rather than by a single expert pass, which is
worth recording because two early theories were chased and disproven before the real cause turned up — useful
precedent for not over-trusting a first plausible-looking lead:
1. **False lead 1 — stale `rt_Generic_1_r` write from `phase_combine_volumetric`.** A near-black MRT1 write
   during the volumetric-light combine pass looked suspicious, but tracing the code showed that buffer gets
   unconditionally re-cleared later the same frame (the §19 fix) before `combine_2` ever reads it as `s_distort`
   — a dead end, confirmed via source reading rather than more capturing.
2. **False lead 2 — duality/stereo blend.** `phase_pp`'s shader samples the same render target twice
   (`s_base0`/`s_base1`) via left/right "duality" UVs (`u_calc_tc_duality_ss`, an old split-screen feature driven
   by monster-effector `PPInfo` state, e.g. `bloodsucker.cpp`/`controller.cpp`/`SleepEffector.cpp`). Checking the
   actual per-vertex `TEXCOORD0`/`TEXCOORD1` in RenderDoc's Mesh view (not the raw shared streaming vertex buffer
   — that gave garbage from an unrelated draw and was itself a brief false lead) showed both UV sets identical,
   ruling out a stuck duality offset.
3. **Correcting the search target**: Pixel History's "Tex Before"/"Tex After" columns track the *destination*
   render target's content over time, not what a shader reads as input — don't confuse "swapchain had a dim
   leftover value before this draw" (meaningless; the swapchain is first written by `phase_pp` each frame, so
   "before" is just stale prior-frame garbage) with "this shader's input texture is dim" (the actual signal).
   Once that distinction was clear, the pixel shader **debugger** (RenderDoc: right-click a pixel in Pixel
   History → Debug) gave the real answer directly — stepping one instruction showed `phase_pp`'s very first
   texture sample already reading the dim value, proving `phase_pp` is an innocent passthrough and the real bug
   is upstream, in `combine_2`.
4. **The actual find**: opened `combine_2`'s constant buffer (`$Globals`, PS slot 0) in RenderDoc and found named
   variables — `vibrance`, `lumasharpen`, `aberration`, `lensdirt`, `lenswater`, `temp` — holding either huge
   nonsense floats (`~1E+34`) or literal `NaN`, alongside the legitimately-used, correctly-bound constants
   (`e_barrier`, `e_weights`, `e_kernel`, `dof_params`, `dof_kernel`, `screen_res`). Viewing the shader's
   disassembly (RenderDoc: "View" on the Pixel Shader resource) confirmed `aberration` and `lumasharpen` are
   actually read by `combine_2`'s shader (`Pixel Shader 29993`), while `lensdirt`/`lenswater`/`temp` are declared
   in the same `$Globals` layout but unused by this particular shader variant (harmless).

### Root cause
`combine_2`'s pixel shader (compiled from Dead Air/DeadZone's own shader archive, not stock OpenXRay — the HLSL
source isn't in this repo) implements two DAR2-specific bonus effects, each gated by a runtime toggle:
```hlsl
if (aberration != 0)   { /* ~35 instructions: chromatic-aberration sampling */ }
if (lumasharpen != 0)  { /* ~20 instructions: unsharp-mask sharpening */ }
```
This engine's C++ (`CRenderTarget::phase_combine()`'s `combine_2` setup, `r4_rendertarget_phase_combine.cpp`)
only ever binds the constants stock OpenXRay knows about — `e_barrier`, `e_weights`, `e_kernel`, `m_current`,
`m_previous`, `m_blur`, `dof_params`, `dof_kernel` (lines 419-429 as of §18/§19). **It never sets `aberration`,
`lumasharpen`, or their siblings `vibrance`/`lensdirt`/`lenswater`.** These are Dead Air-added constants the port
was never taught about — the same class of gap as every other "Dead Air added something, the port doesn't know"
issue catalogued in §7-§9, just on the render/shader side instead of Lua/script side. Left unbound, that constant
buffer memory holds whatever was last there — uninitialized/stale GPU memory, which is why the bug is
non-deterministic (varies with driver/allocation state across runs) and why 3 prior sessions' CPU-side
`[PPDBG]` logging never caught it (it only ever logged values this engine's own C++ actually sets — it had no
way to know about GPU-side constants the C++ never touches).

**The actual corruption mechanism**, confirmed by reading `Pixel Shader 29993`'s disassembly line by line:
- D3D shader bytecode's `ne` (not-equal) comparison against `NaN` is defined as **always true** (IEEE-754
  unordered inequality). So when `aberration`/`lumasharpen` happen to be `NaN` (as captured in the dim repro),
  `aberration != 0` and `lumasharpen != 0` both spuriously evaluate true, incorrectly enabling both bonus-effect
  branches even though the code intends `0` to mean "off."
- The aberration branch multiplies its contribution by `aberration` **without a saturate**
  (`mad r0.xyz, aberration.xxxx, r3.xyzx, r0.xyzx`), so multiplying by `NaN` poisons the shader's accumulating
  output color (`r0.xyz`) to `NaN`.
- The sharpen branch's *last* instruction, by contrast, **does** saturate (`add_sat r0.xyz, r0.xyzx, r2.wwww`).
  Per the D3D functional spec, saturating a `NaN` result clamps it to exactly `0` — so this instruction silently
  **resets the entire accumulating output color to pure black**, purely as a side effect of the earlier NaN
  poisoning.
- Everything after that (`combine_2`'s depth-of-field blur accumulation loop, ~135 instructions, the bulk of the
  shader) builds its final weighted-average color on top of that zeroed base instead of the correct scene color.
  DOF blur normally contributes a small correction on top of a dominant in-focus center sample; with the center
  sample zeroed, the result comes out dramatically under-exposed rather than pure black, matching the observed
  "uniformly dim, not black" symptom exactly (~30-70% brightness loss was measured across sampled pixels, both
  sky and shadow, consistent with the effect being upstream of and identical for every pixel).
- `vibrance` is also never bound by this engine, but happened to read a clean `0.0` in the diagnosed capture and
  is correctly inert via its own `if (vibrance != 0)` guard — not implicated this time, but equally unbound, so
  fixed defensively anyway (see below).

### The fix (shipped, user-confirmed)
`src/Layers/xrRenderPC_R4/r4_rendertarget_phase_combine.cpp` — in `CRenderTarget::phase_combine()`'s `combine_2`
setup, right after the existing `dof_kernel` bind (~line 429), added:
```cpp
RCache.set_c("aberration", 0.f, 0.f, 0.f, 0.f);
RCache.set_c("lumasharpen", 0.f, 0.f, 0.f, 0.f);
RCache.set_c("vibrance", 0.f, 0.f, 0.f, 0.f);
```
This guarantees these constants always hold a defined `0` every frame, so the `!= 0` guards correctly disable
both unsupported bonus effects instead of reading garbage. `lensdirt`/`lenswater`/`temp` were left unbound since
confirmed unused by this shader variant (dead in the bytecode) — revisit only if a future shader variant
(colormap path, or a GL equivalent) is found to actually reference them.

Rebuilt (Release x64, standard `§4` command), `r4_rendertarget_phase_combine.cpp` recompiled, `xrRender_R4.dll`
+ `xrEngine.exe` relinked, exit 0. **User-tested: repeated same-save reloads, level no longer dims itself.**
RESOLVED.

Only the R4 (DX11) file was patched — `gl_rendertarget_phase_combine.cpp` was not checked for an equivalent gap
(this build only runs `-r4`); apply the same investigation there first if GL rendering is ever brought up.

### Residual/follow-up notes for next session
- **The §18 `[PPDBG]` logging and the §11/§12 `[ESCDBG]` logging are both still in the build**, un-reverted
  across several sessions now. Neither bug they were added for is still under active investigation (dim/scramble
  both resolved; Escape/dialog-stacking status unknown, see below) — worth a cleanup pass to strip both sets of
  instrumentation for a clean build, now that root causes are found rather than "still hunting."
- **This session's methodology mistakes are worth remembering for next time**: don't chase a render-target's
  content just because it looks visually dark in a thumbnail (bloom targets, shadow regions, etc. can be
  legitimately dark) — check actual numeric values, and prefer the pixel shader *debugger* over hand-tracing
  disassembly by eye once real shader code is available (a hand-traced conclusion was wrong twice this session
  before switching to letting RenderDoc compute the actual register values).
- The Escape-key/dialog-stacking work (§11-§13) remains at "awaiting user re-test #3", untouched this session —
  re-read §11-§13 before resuming it, status still unknown.
- Source patch diff regenerated this session at `D:\openxray-build\deadair-openxray-source-patches.diff`
  (`git diff` in `D:\openxray-build\xray-16`, 51 modified files, 1647 lines) — now current as of end of this
  session, includes everything through this section.

---

## 21. Dynamic lights (headlamp / NPC torches) didn't light the environment (2026-07-23) — ROOT-CAUSED AND FIXED, USER-CONFIRMED

### Symptom (as reported)
"Dynamic lights (player headlamp, NPC headlamp) light up environment and objects only a few steps away from the
source." Both the player's headlamp AND NPC headlamps. Static world lamps looked fine.

### THE ROOT CAUSE (the one that mattered): the spot COOKIE texture zeroes the whole beam
The compiled `accum_spot_unshadowed` pixel shader ends with:

    output = cookie.rgb * (material_diffuse * attenuation) * Ldynamic_color.rgb

i.e. it shapes the beam using the **cookie's RGB**. Dead Air's torch cookie
(`spot_texture = internal\internal_light_torch_r2`, from the visual's `[torch_definition]` userdata) carries its
shape in **ALPHA with RGB left black**, so the entire beam multiplied out to **exactly zero**.

The light was being accumulated correctly every single frame — it just contributed nothing. This is why NOTHING
helped and why the investigation had previously gone in circles for a whole session: boosting color scales zero,
raising range extends zero. It also explains the "both player and NPC headlamps" scope precisely — they share the
same `device_torch` cookie, while static lamps use different textures and worked fine.

**Fix** (`Torch.cpp::net_Spawn`): pass `nullptr` to `light_render->set_texture()` (and skip the script-supplied
`m_torch_texture` in `UpdateCL`'s torch1 block, same reason), so `accum_spot` falls back to the engine's default
spot shader, which produces a correct cone. Beam width still comes from `spot_angle`. **USER-CONFIRMED**: player
headlamp and NPC torches now light the environment properly.

### THREE independent zeroing bugs were stacked — all three fixes are needed and are all in the build
1. **`color_animator = empty` sentinel not honored.** The config comment literally says `empty - not using
   animator`, but `net_Spawn` passed the string straight to `LALib.FindItem("empty")`, and this game's
   `lanims.xr` contains a real item named "empty" whose output is **black**. `UpdateCL` applied that animator's
   color to `light_render` every frame, driving every torch's color to (0,0,0). Fixed by treating "empty" (and
   empty string) as "no animator" so `lanim = nullptr`. This fixed the NPC/world torch colors.
2. **Dead Air's TorchType-0 default headlamp explicitly zeroes torch1.** `xr_actor.script:UpdateTorch`'s `else`
   branch calls `torch_set_color_r/g/b(0.0)` + `torch_set_range(0)` because in Dead Air the headlamp beam is
   supposed to come from **torch2**. With torch2 not enabled, that left the player's beam black. Fixed in
   `UpdateCL`: when torch1's color is all-zero, substitute the baked base color; when torch1 range/radius are 0,
   substitute the baked base range / baked `spot_angle` cone (the cone part also stops one mode inheriting the
   previous mode's beam width, e.g. the headlamp staying at a glowstick's 60 degrees).
3. **The cookie-RGB bug above** — the decisive one, since it zeroed the beam even after 1 and 2 made the color
   correct.

Also reset `TORCH2_BOOST_GAIN`/`TORCH2_RANGE_GAIN` from **60/6 back to 1.0/1.0**. A previous session had escalated
them chasing the "impossibly dim" headlamp; that dimness was the cookie bug (no gain could ever make a
zero-multiplied beam visible). With the real bug fixed, 60x would be blinding. 1.0 uses Dead Air's own intended
values (~1.6 color, range 24).

The earlier `set_shadow(false)` fix on `CTorch`'s spot light remains — it addressed a separate, real sub-bug
(the shadow-map path zeroing contribution for lights that move every frame). Keep it.

**DO NOT re-litigate the old "there must be a clamp on light color" theory.** It is disproven: the shader is
provably LINEAR in `Ldynamic_color` with no `saturate`/clamp anywhere, and `light::set_color` / `accum_spot`
have no clamp either.

### Reusable technique: disassembling this build's compiled shaders WITHOUT RenderDoc
This is what cracked the case, and it is much faster than a GPU capture. OpenXray's shader cache blobs have a
**12-byte header followed by raw `DXBC`**. Blob path:
`bin/x64/Release/appdata/shaders_cache_oxr/r4/<shader>.ps/<hash>`

    tail -c +13 "<blob>" > out.cso

then in PowerShell:

    & "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe" /nologo /dumpbin out.cso /Fc out.asm

`out.asm` contains full cbuffer/resource reflection **and** the disassembly — enough to read the exact lighting
math. Caveat learned: **grepping the `.xdb` archives for texture names does not work** (names aren't stored as
plain strings — known-present textures like `ui_actor_loadgame_screen` also return zero hits), so never conclude
"texture missing" from a grep.

### Diagnostics added and STILL IN THE BUILD (remove for a clean build, or keep the useful one)
- **`dbg_slight_boost <float>`** console var (default 1.0, range 0..1000) — multiplies **every** spot and point
  light's color at accumulation time, live, no rebuild. Declared in `xrRender_console.cpp`/`.h` next to
  `ps_dbg_exposure`; applied in `r3_rendertarget_accum_spot.cpp` and `r3_rendertarget_accum_point.cpp`. This one
  is genuinely useful for light debugging — **consider keeping it.**
- `[PPDBG-ACCUMSPOT]` / `[PPDBG-ACCUMPOINT]` — per-light dumps (color, range, camdist, applied color, boost) in
  the two accum files. `camdist` is what let us tell the player's own lights (camdist ~0.3-0.5) apart from NPC
  and static lights.
- `[PPDBG-TORCHSPAWN]` (`Torch.cpp::net_Spawn`) — logs the animator name and whether `lanim` resolved.
- `[PPDBG-TORCHSTATE]` (`Torch.cpp::UpdateCL`, actor only) — dumps `cust1/spot/color1/range1 |
  cust2/enabled2/color2/range2 | base color/range`. **This is the one that pinpointed bug 2 and will be directly
  useful for the open glowstick issue below — recommend leaving it in until that's resolved.**

### Method note (what actually worked, after a prior session stalled)
The winning move was **isolation, not more measurement**: disabling the torch's omni fill light proved the spot
beam contributed *zero* (turning the headlamp on produced no light at all), which converted a vague "it's dim"
into a precise "this term is zero" — and the shader disassembly then said only `cookie.rgb` could do that.
Earlier sessions kept tuning brightness constants against a light that was mathematically incapable of being
seen. When a value won't respond to a linear multiplier, stop tuning and go find the zero.

### Glowstick (and lighter) only emitted light while the headlamp was enabled — FIXED 2026-07-24, see §22
Resolved in the next session; the subsection below is kept as the record of what was known going in. The
leading hypothesis stated here was correct in substance (the stock `kTORCH` binding hard-toggling the one
shared light) but wrong about the safety net: Dead Air's `enable_torch(true)` force **never actually runs in
this port** — `[PPDBG-TORCHSWITCH]` recorded zero script-driven `Switch` calls across a whole run. Read §22.

### (historical) OPEN / NEXT SESSION — glowstick only emits light while the headlamp is enabled
**Status at the time: UNRESOLVED, user deferred to a future session.** After all the above fixes, the user
reported the glowstick still lights nothing unless the headlamp is switched on.

Key architectural fact to start from: **Dead Air multiplexes ALL handheld light sources — headlamp, hand
flashlight, glowstick, lighter — onto the single `device_torch` `CTorch` object's ONE `light_render`**, switching
their appearance via `itms_manager.TorchType` (0 = default headlamp, 1/2 = hand flashlight, 3..10 = glowstick
variants / lighter; see the big if/elseif in `xr_actor.script:UpdateTorch`, and the TorchType assignment from
`db.actor:active_detector()` in `itms_manager.script:~420`). There is no separate glowstick light object.

Leading hypothesis (NOT yet verified): the engine's stock kTORCH binding fights Dead Air's script-driven design.
`ActorInput.cpp:927` maps `kTORCH` to `CActor::SwitchTorch()` which calls `CTorch::Switch()`, toggling
`m_switched_on` and calling `light_render->set_active(false)`. `CTorch::UpdateCL` also early-returns when
`!m_switched_on`. So pressing L kills the ONE shared light — taking the glowstick with it. Dead Air's own design
appears to be that the shared light is **always on** (`itms_manager.script` `actor_on_update` does
`if (not torch:torch_enabled()) then torch:enable_torch(true) end` every update) and that L should only toggle
**torch2** (the headlamp beam, via `itms_manager.on_key_press` calling `enable_torch2`, gated on
`has_alife_info("enable_device_torch")`). If the retail Dead Air engine dropped or repurposed the stock
`SwitchTorch()` binding, this port re-introducing it would produce exactly this symptom.

Concrete first steps next session:
1. Confirm with `[PPDBG-TORCHSTATE]` (already in the build) whether `m_switched_on` is false while the glowstick
   is out and the headlamp is off. Add `m_switched_on` to that Msg — note the state dump currently sits AFTER the
   `if (!m_switched_on) return;` guard, so move the log ABOVE that guard or it will never print in the failing case.
2. Check whether `itms_manager.actor_on_update`'s force-`enable_torch(true)` is actually running (log inside
   `CTorch::enable_torch`/`Switch`). If it is running, `m_switched_on` should never stay false — if it does,
   something else is turning it off every frame.
3. If confirmed, the fix is likely to make `kTORCH` not hard-toggle the shared light for this mod (let the script
   own it), or to give the glowstick/lighter its own light object rather than sharing `light_render`.
4. Known unrelated stub while you are in here: `torch_set_animation(...)` only stores the string — Dead Air uses
   it to make glowsticks pulse (`empty_green`, `empty_red`, ...), so glowstick colors are correct but **static,
   not animated**. Wiring `CLAItem` animation into `UpdateCL` is a contained piece of work if wanted.

### Files touched this session
- `src/xrGame/Torch.cpp` — "empty" animator sentinel; all-zero torch1 color to base-color fallback; range/cone
  fallbacks; **no spot cookie** (the root-cause fix); torch2 boost gated to the default headlamp mode only;
  `TORCH2_BOOST_GAIN`/`TORCH2_RANGE_GAIN` 60/6 to 1.0/1.0; `[PPDBG-TORCHSPAWN]`/`[PPDBG-TORCHSTATE]` logs.
- `src/xrGame/Torch.h` — added `m_torch_base_cone` (baked `spot_angle`, so a mode passing radius 0 restores its
  own cone instead of inheriting the previous mode's).
- `src/Layers/xrRender/xrRender_console.cpp` / `.h` — `ps_dbg_slight_boost` + `dbg_slight_boost` console command.
- `src/Layers/xrRender_R2/r3_rendertarget_accum_spot.cpp` / `r3_rendertarget_accum_point.cpp` — boost knob +
  `[PPDBG-ACCUMSPOT]` / `[PPDBG-ACCUMPOINT]` logging.
- Rebuild targets used: `/t:xrGame` and `/t:xrRender_R4` (see section 4 for the build command).
- **Regenerate `deadair-openxray-source-patches.diff`** — it does NOT yet include this section's changes.

---

## 22. Glowstick/lighter only lit anything while the headlamp was on (2026-07-24) — FIXED, VERIFIED IN-ENGINE

Follow-up to §21's open item. Fixed engine-side; verified by an automated run that reproduced the bug and then
showed all three states correct. **Awaiting the user's own visual confirmation.**

### Root cause
Dead Air multiplexes *every* handheld light source — headlamp, hand flashlight, all glowstick colours, lighter —
onto the **one** `device_torch` `CTorch` object's single `light_render`, choosing the look via
`itms_manager.TorchType` (see §21). The mod never switches that object off. It expresses "nothing is lit right
now" through **torch1's range being 0**: `xr_actor.script:UpdateTorch`'s default branch (TorchType 0) pairs
`torch_set_range(0)`/`torch_set_radius(0)` with `torch_set_color_r/g/b(0)`, because the real headlamp beam is
supposed to come from torch2 (`enable_torch2`, the L key, handled in `itms_manager.on_key_press`).

The port re-introduced the **stock `kTORCH` binding** (`ActorInput.cpp` → `CActor::SwitchTorch()` →
`CTorch::Switch()`), which hard-toggles `m_switched_on` on that shared object. `CTorch::UpdateCL` early-returns
when `!m_switched_on`, so pressing L switched the *whole multiplexer* off — glowstick and lighter included. Both
therefore appeared to work only while the headlamp happened to be on, in exact lockstep with it.

**The safety net Dead Air relies on does not run in this port.** `itms_manager.actor_on_update` does
`if (not torch:torch_enabled()) then torch:enable_torch(true) end` every frame, which should have re-asserted the
state within one frame. A `[PPDBG-TORCHSWITCH]` probe on `CTorch::Switch` logged **zero** calls for the actor's
torch across an entire run (the only one was `net_Destroy`'s teardown at level unload) — so that line never
reaches `Switch`, even though the *same* script function demonstrably runs (it is the sole writer of
`itms_manager.TorchType`, and torch1 carried live glowstick values). Why the call is lost was not chased further:
the engine now owns the state, which makes it moot. **Do not assume the Lua force works** if you touch this again.

### The fix (three parts, all in the build)
1. **`CTorch::UpdateCL` re-asserts the switch, engine-side** (top of the function, above the `m_switched_on`
   guard): if the Dead Air torch API has been used on an actor-held torch and it is off, switch it back on.
   `m_switched_on` is set to `true` *before* calling `Switch(true)` so the turn-on sound stays silent — this is a
   state re-assert, not a user action. **Do not gate this on `CAttachableItem::enabled()`**: for the actor's own
   torch that flag is false for the entire game (`CActor::can_attach` deliberately omits the `enabled()` test
   that `CAttachmentOwner::can_attach` applies), so the first attempt at this fix was dead code — the
   `en=0` field in `[PPDBG-TORCHSTATE]` is what exposed it.
2. **`CActor::SwitchTorch()` skips script-driven torches** — new `CTorch::script_driven()`
   (`m_torch_customized || m_torch2_customized`). Dead Air handles the same key in Lua and toggles torch2; the
   engine must not also hard-toggle the shared object (that is the bug), and without this the re-assert in (1)
   would fight the key every press (one-frame flicker + on/off sounds).
3. **Range 0 now means "emitting nothing", as Dead Air intends.** §21 had added a "range 0 → substitute the baked
   base range" fallback, which is wrong: it makes the headlamp beam permanently on. It was invisible only because
   the object was *also* being hard-switched off — two bugs cancelling. Replaced with an explicit gate after the
   torch1/torch2 blocks: `emitting = (m_torch_range > 0) || torch2_boost_active`, driving
   `light_render`/`light_omni`/`glow_render` `set_active()`. Applied only when `m_torch_customized`, so stock and
   NPC torches are untouched.
   - The **colour** fallback from §21 (all-zero torch1 colour → baked base colour) **stays** — it is what makes
     TorchType 1/2 (hand flashlight, which sets range/radius but never a colour) visible instead of inheriting
     TorchType 0's zeros.
   - torch2's boost branch now also falls back to the baked range when `torch2_set_range` was never given,
     since torch1's range is 0 in that mode by design.

Net semantics, matching the mod: the shared light object is **always on**; on/off is expressed by range and by
torch2's enable flag.

### Verification (automated, no user playtest needed to confirm the mechanism)
Launched with `-start "server(user0 - quicksave4/single/alife/load)client(localhost)"` (that quicksave has a
glowstick as the active detector). Before the fix, `[PPDBG-TORCHSTATE]` showed the bug verbatim:
`on=0 ... c1(0.00,0.40,0.30) rng1=8.0 | en2=0 ... lr_active=0` — glowstick configured, light hard-off.
After the fix, one run covered all three states:

| state | dump | verdict |
|---|---|---|
| glowstick out, headlamp off | `on=1 spot=0 c1(0,0.40,0.30) rng1=8.0 en2=0` → `lr_active=1` | **fixed** |
| nothing out, headlamp off | `on=1 spot=1 c1(0,0,0) rng1=0.0 en2=0` → `lr_active=0` | still correctly dark |
| nothing out, headlamp on | `on=1 ... en2=1` → `lr_active=1` | headlamp unregressed |

`[PPDBG-TORCHFORCE]` fired **once** (frame 35) and `[PPDBG-TORCHSWITCH]` only at level unload — i.e. nothing
fights the re-assert per frame. No crash, no new errors.

### Diagnostics touched
- `[PPDBG-TORCHSTATE]` **moved above the `m_switched_on` early-return** (the old placement could never observe
  the failing case) and extended with `on=` (`m_switched_on`), `en=` (`enabled()`), `lr_active=`.
- `[PPDBG-TORCHFORCE]` (UpdateCL re-assert) and `[PPDBG-TORCHSWITCH]` (`CTorch::Switch` transitions, actor only)
  added. Both are rare-event logs; TORCHSTATE still prints ~2×/sec. **Remove all three once the user confirms.**

### Files touched this session
- `src/xrGame/Torch.cpp` — re-assert-on at top of `UpdateCL`; `[PPDBG-TORCHSWITCH]` in `Switch(bool)`; removed
  the range-0→base-range fallback; torch2 range fallback; the new "emitting" gate; TORCHSTATE moved/extended.
- `src/xrGame/Torch.h` — `script_driven()`.
- `src/xrGame/ActorInput.cpp` — `CActor::SwitchTorch()` returns early for script-driven torches.
- Rebuild target: `/t:xrGame`.
- Still open from §21: `torch_set_animation(...)` is a store-only stub, so glowstick colours are correct but do
  not pulse (`empty_green`, `empty_red`, …). Contained work if wanted.

### Follow-on (same day, user request): the headlamp now has its OWN light, independent of the hand item
The fix above restored Dead Air's own semantics, which include a consequence the user did not want: because
everything shares `light_render`, equipping a glowstick *replaces* the headlamp beam. Requested behaviour is that
the headlamp works regardless of what is in hand, so torch2 was given a dedicated light object.

- New `ref_light light_torch2` on `CTorch` (SPOT, `set_shadow(false)`, no cookie — same reasons as `light_render`;
  seeded at `net_Spawn` with the visual's baked colour/range/cone). Positioned/rotated every frame from the same
  head transform as `light_render`, right beside it.
- `UpdateCL` no longer writes torch2's boost into `light_render` at all. `light_render` is now purely torch1 (the
  TorchType-selected hand source); `light_torch2` is purely the headlamp, active whenever
  `m_torch2_customized && m_torch2_enabled` — no dependence on TorchType. The old
  `torch1_is_default_headlamp` / `torch2_boost_active` gating is gone, along with `[PPDBG-TORCH]`.
- Beam look is unchanged from the version the user confirmed in §21: colour `(base + torch2) * TORCH2_BOOST_GAIN`,
  range `max(torch2_range, base_range) * TORCH2_RANGE_GAIN`, cone = the baked `spot_angle`. The script's
  `torch2_set_radius(90)` is deliberately ignored (90° is a flood, not a headlamp) — one-line swap in `UpdateCL`
  if a wider beam is ever wanted.
- `light_omni` (the ~1.3 m fill) and `glow_render` follow **either** source (`torch1_emitting || torch2_on`), so
  the headlamp keeps its fill light exactly as before.
- `CTorch::Switch` only ever *deactivates* `light_torch2`; activation is owned by `UpdateCL`, otherwise switching
  the object on would flash the headlamp for a frame with the headlamp disabled.
- NPC torches are unaffected: `m_torch2_customized` is only ever set by `xr_actor.script`, i.e. for the player.

Verified in the same automated way; all four states observed in one run:

| hand item | headlamp | `lr_active` | `t2_active` |
|---|---|---|---|
| none | off | 0 | 0 |
| none | on | 0 | 1 |
| glowstick | off | 1 | 0 |
| glowstick | **on** | **1** | **1** | ← the requested new behaviour

Cost: one extra shadowless spot light while the headlamp is on. **Awaiting the user's visual confirmation.**

---

## 23. `device_lighter` / `device_flashlight` emitted no light at all (2026-07-24) — FIXED

Reported right after §22: glowsticks now work, but the hand flashlight and the lighter still emit nothing, in any
headlamp state. **Not a lighting bug — an engine/script enum-numbering divergence**, and it is almost certainly
not the only thing it was breaking.

### Root cause: OpenXRay renumbered `CHUDState::EHudStates`
`itms_manager.actor_on_update` selects those two modes conditionally, unlike glowsticks:

```lua
if (obj:section() == "device_flashlight") then
    if ((obj:get_state() == 0)) then TorchType = 1 end      -- 0 == eIdle in retail: drawn and idle in hand
...
elseif (obj:section() == "device_lighter") then
    if ((obj:get_state() == 0)) then TorchType = 4 end
```

Retail X-Ray (CoP/CoC 1.4.22 — what Dead Air targets) numbers the base HUD states
`eIdle=0, eShowing=1, eHiding=2, eHidden=3, eBore=4`. This engine (`src/xrGame/HudItem.h`) uses
**`eHidden=0, eIdle=1, eShowing=2, eHiding=3, eBore=4`** — the four base states are permuted. So a drawn device
reports `1` here, the `== 0` test never passed, `TorchType` stayed 0, and neither item ever selected a light
mode. Glowsticks were unaffected because their branch has no state test. `active_detector()` additionally only
returns the item while `IsWorking()`, so the stowed case never reached the test either — hence "no light at all,
regardless".

The mod documents the numbering it expects in its own `xrs_debug_tools.script:1686` (`eIdle --= 0` … `eHidden
--= 3`), so there is no ambiguity about which side is right for this content.

**Why this went unnoticed until now:** `eBore` is 4 in *both* orderings, so everything from there up — all the
weapon states (`eFire=5`, `eFire2=6`, `eReload=7`, …) — lines up. The mod's other numeric `get_state()` tests are
all weapon states (`level_weathers.script:409` and `xr_camper.script:143` use `== 7`;
`xrs_kill_wounded.script:358` uses `== 5 or == 6`) and were always correct. Only comparisons against 0..3 broke.

### Fix: translate at the script boundary, don't renumber the engine enum
`src/xrGame/script_game_object3.cpp` — two small file-local tables applied in `CScriptGameObject::GetState()`
(engine → script) and `CScriptGameObject::SwitchState()` (script → engine); values ≥ `eBore` pass through
untouched. Chosen over renumbering `EHudStates` because that enum is used symbolically in hundreds of C++ places
and `eHidden == 0` is load-bearing for zero-initialised state, whereas the Lua-visible integer is produced and
consumed *only* by those two exports (verified: the only caller of `CScriptGameObject::GetState` is its luabind
`.def`; no `value("eIdle", …)`-style enum is exported to Lua, and no loose script calls `switch_state`).

This fixes the divergence for **every** script at once, including any still living inside the `.xdb` archives —
which a targeted patch of the loose `itms_manager.script` would not have done.

No engine-side lighting work was needed: once `TorchType` is set, §21/§22's existing paths already handle both
modes (flashlight = spot, range 60, no colour → all-zero-colour falls back to the baked 0.6; lighter = point,
range 10, colour (0.40,0.22,0.00)).

### Verification
Same automated run method. Both modes now appear and light, and the headlamp stays independent of them:

| mode | `[PPDBG-TORCHSTATE]` | main light | headlamp |
|---|---|---|---|
| lighter, headlamp off | `spot=0 c1(0.40,0.22,0.00) rng1=10.0` | 1 | 0 |
| lighter, headlamp **on** | same | **1** | **1** |
| flashlight | `spot=1 c1(0,0,0) rng1=60.0` | 1 | 0 |
| nothing out, headlamp on | `rng1=0.0` | 0 | 1 |

The single `bind_gr_gun.script:568 attempt to index global 'ggun_binder'` load-time error in the log is
pre-existing (present in the prior run's `.bkp` too), unrelated to this change.

**Suggested follow-up:** grep the loose scripts for other `get_state()` comparisons against 0..3 that were
silently wrong before this fix and may now change behaviour (in a good way) — and treat "retail enum order vs
OpenXRay enum order" as a standing suspect for any future "script condition never fires" symptom.


---

## 24. Night vision did nothing at all ("scene stays dim, nothing is illuminated") (2026-07-24) — FIXED, VERIFIED IN-ENGINE

Reported right after §23. Pressing the night-vision key (N) changed nothing on screen. **Not a rendering bug at
all — the same "engine key binding fights Dead Air's Lua handler" class as §22's `kTORCH`.**

### Root cause: `kNIGHT_VISION` is handled TWICE per key press, and the two cancel out

Dead Air handles the key itself in `itms_manager.on_key_press`:
```lua
elseif (dik_to_bind(key) == key_bindings.kNIGHT_VISION) and (torch) then
    torch:enable_night_vision(not torch:night_vision_enabled())
```
The port also kept the stock engine binding: `ActorInput.cpp` `case kNIGHT_VISION: SwitchNightVision()` ->
`CActor::SwitchNightVision()` -> `CTorch::SwitchNightVision()` (the no-arg **toggle** overload).

`CLevel::IR_OnKeyboardPress` (`Level_input.cpp:143`) fires the script `eKeyPress` callback **before** dispatching
to the actor's input receiver, so on every press the script toggles first and the engine toggles straight back.
Net effect: **night vision never changes state at all** — it is frozen at whatever it was when the level loaded.
Captured verbatim on the first instrumented run, one key press, same frame:

    [PPDBG-NV] caller: script enable_night_vision(1)
    [PPDBG-NV] SwitchNightVision(on=1 snd=1) f=465 wasOn=0
    [PPDBG-NV]   map='l01_escape' allowed=1 helmet='<no helmet>' outfit='effector_nightvision_3' activeNow=0
    [PPDBG-NV]   Start(sect='effector_nightvision_3') pp_eff_name=nightvision_3.ppe -> effector registered
    [PPDBG-NV] caller: engine kNIGHT_VISION binding (CActor::SwitchNightVision)
    [PPDBG-NV] SwitchNightVision(on=0 snd=1) f=465 wasOn=1
    [PPDBG-NV]   -> Stop()

and the whole-run absence of any `[PPDBG] f=...` line (the `CRender::SetPostProcessParams` dirty-log added in
§18) confirmed the post-process aggregate never once left its neutral identity value.

Note the asymmetry this produces: if the save had NV **off**, it can never be turned on (the reported symptom);
if the save had NV **on** — the mod re-asserts the saved state at load, seen at `f=301` — it can never be turned
off. Both present as "the key does nothing".

### The fix (`ActorInput.cpp` + `Torch.h` + `script_game_object_inventory_owner.cpp`)
Exactly the §22 shape, scoped to night vision:
- New `CTorch::m_night_vision_script_driven` + `set_night_vision_script_driven()` /
  `night_vision_script_driven()`. `CScriptGameObject::enable_night_vision` sets it before delegating.
- `CActor::SwitchNightVision()` returns early when
  `torch->night_vision_script_driven() || torch->script_driven()`. The second term is the §22 flag and makes the
  guard order-independent: Dead Air's `xr_actor.script` drives the torch every frame from spawn, so it is
  already true before any key press — including the very first one, and including the case where the script's
  own NV branch bails out early (PDA open, no torch object, ...).

Stock / non-Dead-Air content is untouched: neither flag is ever set unless a script uses the Dead Air torch API.

### Verification (automated, same method as §22/§23)
`-start "server(user0 - quicksave4/single/alife/load)client(localhost)"`, night save (22:38, Cordon), key
injected with `keybd_event` after forcing the window foreground. **The window must genuinely be focused** — a
`SetForegroundWindow` that Windows silently refuses leaves the game unfocused, and it then renders stale frames
and drops injected keys (three inconclusive runs were lost to this before adding an `AttachThreadInput` +
title-bar-click focus grab and asserting `GetForegroundWindow()` afterwards). Script: `scratchpad/nvrun3.ps1`.

After the fix, one press:

    [PPDBG-NV] caller: script enable_night_vision(1)
    [PPDBG-NV]   Start(sect='effector_nightvision_3') pp_eff_name=nightvision_3.ppe -> effector registered
    [PPDBG-NV] caller: engine kNIGHT_VISION binding -- skipped (script-driven)
    [PPDBG] effectors count=2 types=[56(v=1) 55(v=1) ]
    [PPDBG] f=2683 ... gray=0.660 noise=0.250 base=(0.800,0.700,0.400) add=(0.004,0.005,0.005)
    [PPDBG] f=2743 ... gray=0.684 noise=0.454 base=(1.200,1.300,1.200)
    [PPDBG] f=2803 ... gray=0.709 noise=0.657 base=(1.200,1.300,1.200)
    [PPDBG] f=2863 ... gray=0.734 noise=0.863 base=(1.200,1.300,1.200)

i.e. the effector (type 55 = `effNightvision`) stays registered and its envelopes animate for hundreds of frames
instead of being torn down in the same frame it was created. Screen captures of the *same* camera: NV on =
bright, desaturated, grainy, mean RGB ~ (56,56,53); NV off = dark colour image, mean ~ (18,16,11) — **~3.2x
brighter**, consistent with the ppe's math (`color_base` 1.2/1.3/1.2 clamps to 1.0 in `SPPInfo::SColor::u32()`
-> 2x through the shader's final `*2`, plus `color_gray` 0.993 weights that lift mid-grey further before the
desaturation blend).

### Reference: how night vision actually reaches the screen
Nothing in this chain was broken — recorded so the next session does not have to re-walk it.
`itms_manager.on_key_press` -> `CTorch::SwitchNightVision` -> picks `nightvision_sect` from the **helmet first,
then the outfit** (`outfit_helmet.ltx` -> `effector_nightvision_1`, `outfit.ltx` -> `_2`/`_3`) ->
`CNightVisionEffector::Start` -> `AddEffector(actor, effNightvision, sect)` -> reads `pp_eff_name` from
`configs/misc/postprocess.ltx` (`nightvision_1.ppe` / `_2` / `_3`) -> `CPostprocessAnimator` ->
`CCameraManager::UpdatePPEffectors` -> `CRender::SetPostProcessParams` -> `CRenderTarget::phase_pp` -> the mod's
`postprocess.ps`. The `.ppe` files come from the **base CoC `configs.xdb0`** (verified: no `.ppe` exists in any
DAR2/DeadZone archive, none in loose `gamedata`), and `night_vision_texture` / `night_vision_effector` in
`[device_torch]` are dead CoP-era keys that no engine in this lineage reads.

`postprocess.ps` was disassembled with the §21 blob technique to confirm it is an innocent passthrough: it reads
`c_brightness` (bound) and declares but does not use the Dead Air extras `vibrance`/`lumasharpen`/`aberration`/
`lensdirt`/`lenswater`/`temp` — so the §20 unbound-constant failure mode does not apply to this shader.

### Reading `.xdb` archives from a script (new tooling, reusable)
§21 warned that grepping `.xdb` archives for names does not work. It can now be done properly — the format is
small enough to parse: chunk `1` (usually `0x80000001`, i.e. LZ-compressed) is the file table; entries are
`u16 size, u32 size_real, u32 size_compr, u32 crc, char name[size-16], u32 ptr`. The chunk compression is
xrCore's LZHUF (`src/xrCore/LzHuf.cpp`, N=4096 / F=60 / THRESHOLD=2), ported to Python in
`scratchpad/lzhuf.py` + `scratchpad/xdb.py` (`list` / `extract`), with `scratchpad/ppe.py` decoding a `.ppe`
into readable envelopes. **File bodies with `size_compr != size_real` use `rtc_decompress` and are not
implemented** — but `anims\*.ppe` in `configs.xdb0` are stored uncompressed and extract directly.

### Diagnostics left in the build
`[PPDBG-NV]` — one line per player NV toggle plus the branch it took (`Torch.cpp`), and the caller tag from
`ActorInput.cpp` / `script_game_object_inventory_owner.cpp`. Rare-event and actor-only (NPC torches return
before logging). Remove with the other `[PPDBG-*]` sets in the eventual instrumentation cleanup pass.

### Files touched this session
- `src/xrGame/ActorInput.cpp` — `CActor::SwitchNightVision()` early-out for script-driven torches + caller log.
- `src/xrGame/Torch.h` — `m_night_vision_script_driven`, `set_night_vision_script_driven()`,
  `night_vision_script_driven()`.
- `src/xrGame/Torch.cpp` — `[PPDBG-NV]` toggle/branch logging in `SwitchNightVision` and
  `CNightVisionEffector::Start`.
- `src/xrGame/script_game_object_inventory_owner.cpp` — set the flag in `enable_night_vision` + caller log.
- Rebuild target: `/t:xrGame` (`/t:xrEngine` was used only for temporary diagnostics, since reverted).
- `deadair-openxray-source-patches.diff` **was regenerated at the end of this session** (`git diff` in
  `D:\openxray-build\xray-16`, 56 modified files, 1180 insertions) — it is current through §24, including the
  §21/§22 lighting work that was previously missing from it.

### NEW OPEN ITEM (found while in here, NOT investigated) — `dik_to_bind(key) == 52` in `itms_manager`
`itms_manager.on_key_press` has a third branch comparing the bound action against the **hard-coded number 52**,
and its body opens/closes the PDA (`ActorMenu.get_pda_menu():ShowDialog(true)` / `HideDialog()`). Hard-coded
action ids are a §23-class hazard: OpenXRay's `EGameActions` (`src/xrEngine/xr_level_controller.h`) contains at
least four entries retail CoC 1.4.22 does not (`kLOOK_AROUND`, `kMOVE_AROUND`, `kCAM_4`, `kCAM_AUTOAIM`), which
shifts every id after them. **Counted directly from this engine's enum, action 52 is `kQUIT` — the Escape key**
(for calibration: `kTORCH`=25, `kNIGHT_VISION`=26, `kUSE`=46, `kACTIVE_JOBS`=58). If that branch really fires on
Escape it toggles the PDA on every Escape press, which is a very plausible contributor to the still-unresolved
§11-§13 Escape/dialog-stacking symptoms ("closing the PDA opens the menu"). Cheap first test: add a log line to
that Lua branch and press Escape in game. Named comparisons (`key_bindings.kTORCH`, ...) are safe — only the
bare `52` is suspect. Retail's own numbering was not recovered (the action-name table could not be located in
`D:\DAR3\xrEngine.exe` by string search), so the *intended* action is still unknown; the PDA body suggests
`kACTIVE_JOBS` or a PDA-ish bind.


### Follow-on (same day, user request): per-suit night-vision colour — data-only retint, no engine change

After §24's fix the user's night vision worked but rendered black-and-white, and asked whether the
suit/helmet-defined appearance could be bound properly. **It already is** — nothing was unbound:

- `CTorch::SwitchNightVision` takes `nightvision_sect` from the **helmet first, then the outfit**, and helmet /
  outfit *upgrades* can rewrite it at runtime (`ActorHelmet.cpp:196`, `CustomOutfit.cpp:421` —
  `helm_tactic_up.ltx` upgrades NV `_1` -> `_2` -> `_3`). All of that works in this port.
- Mapping in this install: **all 8 NV helmets** (+ `antirad_outfit`) -> `effector_nightvision_1`, the green one;
  9 outfits (scientific_base, ecolog_orange, all exo suits) -> `_2`; 5 outfits (scientific, ecolog_green, exo,
  wastelander, nosorog) -> `_3`. The monochrome ones are Dead Air's *higher* NV generations, so B&W was the
  authored look for a helmetless character in an NV outfit.

The real wart is that `_2`/`_3` **cannot** show a tint: `SPPInfo::SColor::operator u32()` packs `color_base`
into an 8-bit vertex colour, so anything above 1.0 clips. `_3` was authored (1.20, 1.30, 1.20) — a deliberate
slight green bias — and clips to pure white. `_1` survives only because just its green channel exceeds 1.0.
**The identical clamp exists in retail CoC, so this is not a port regression.** Measured blast radius: of the
116 `.ppe` files in `configs.xdb0`, only 6 have all channels over 1.0 (`nightvision`, `nightvision_2`,
`nightvision_3`, `death`, `eat_drug`, `mosquito_bald`).

Chosen fix (user's call): **data-only loose overrides**, no rebuild, trivially reversible.

- `D:\DAR3\gamedata\anims\nightvision_2.ppe` — `color_base` -> effective **(0.80, 0.92, 1.00)**, cool blue-white
- `D:\DAR3\gamedata\anims\nightvision_3.ppe` — `color_base` -> effective **(1.00, 0.90, 0.68)**, warm amber
- `nightvision_1.ppe` deliberately untouched (the classic green already reads correctly).

Every channel is <= 1.0, so the tint survives the clamp *and* the peak channel still reaches the same 2x boost
the clipped-white version had — brightness is unchanged, only hue is added. "Effective" here means the value
after the engine adds `pp_identity` (0.5) back in `CPostprocessAnimator::Process`, i.e. the files store
`effective - 0.5`; 0.5 is neutral and 1.0 is 2x through the shader's trailing `*2`.

The files are byte-identical to the archived originals apart from the `color_base` key floats (9 and 18 bytes
changed respectively) — same size, all other envelopes (gray, noise, duality, color_add) preserved. Produced by
`scratchpad/ppe_tint.py`, which walks the CEnvelope layout to find each key's `value` offset and patches it in
place; the extraction tooling is the `xdb.py`/`ppe.py` pair from §24.

**To revert:** delete the two files from `D:\DAR3\gamedata\anims\`. The archived originals in `configs.xdb0` are
untouched and take over again. **To retune:**
`py ppe_tint.py ppe_all\nightvision_3.ppe out.ppe <R> <G> <B>` with each value in 0..1.

Verified in-engine: `[PPDBG] f=… base=(1.000,0.900,0.680)` for the gen-3 outfit, i.e. the loose file is picked
up over the archive ($game_anims$ -> `$fs_root$\gamedata\anims\`) and reaches the post-process — it read
`(1.200,1.300,1.200)` before.

---

## 25. Load / quit crash — A-Life registry `THROW2` asserts fire where retail was a no-op (2026-07-24) — FIXED, load path engine-verified

**Reported symptom:** "now when I load the game, it crashes to desktop." Two crashes in the SAME family were
behind this; both are Release-tolerated retail asserts that only fatal because this build sets
`XRAY_EXCEPTIONS=1` (retail Dead Air = `ReleaseMasterGold`, `XRAY_EXCEPTIONS=0`, where `THROW`/`THROW2` collapse
to `VERIFY`/`VERIFY2` = NDEBUG no-ops). This is the exact same mechanism as §14 Crash B (GOAP `THROW`) — **when a
native fatal fingers a `THROW`/`THROW2`/`R_ASSERT` on an A-Life registry op that retail clearly survived, suspect
this class FIRST.** Diagnose which macro via `xrDebug_macros.h:206-226`.

Both fatals originate in the generic `CSafeMapIterator` registry (`safe_map_iterator_inline.h`): `add` line 35
("Specified object **has been already** found in the registry!") and `remove` line 53 ("Specified object
**hasn't been** found in the registry!"). The template param distinguishes the two backing registries: the
LEVEL registry (`CALifeLevelRegistry`) is `CSafeMapIterator<...,1,...>` (use_time_limit=**1**); the per-graph-point
registry (`CGraphPointInfo`) is `CSafeMapIterator<...,0,...>` (use_time_limit=**0**). Read the `0`/`1` in the
crash's `Function:` line to know which one.

### Crash A — teleport, `remove` "hasn't been found" (the one that crashed the user on load)
Stack: `CMovementManager::process_game_path` → `teleport` → server event → `CALifeUpdateManager::teleport_object`
→ `graph().change` → `CALifeGraphRegistry::remove` → `level().remove` → `CSafeMapIterator<...,1,...>::remove`
(level registry). An NPC walking a cross-graph game path teleports its server object; the object's source graph
vertex is on the current level so `no_assert` computed to false, but the object was not in the offline level
registry → fatal on the first A-Life churn after load (fires during `CLevel::OnFrame`, i.e. right after the level
renders — hence "crashes when I load").

**Fix** (`src/xrGame/alife_graph_registry.cpp`, `CALifeGraphRegistry::remove`): force `no_assert=true` on BOTH
removes in the teleport path — the graph-point `objects().remove(object->ID, true)` and `level().remove(object,
true)`. Removing an id that isn't present is inherently a no-op, so tolerating it exactly reproduces retail.

### Crash B — A-Life unload, `add` "already found" (fires on quit-to-menu / load-a-save, which unload the sim first)
Revealed after Crash A was fixed and the game ran ~8500 frames into gameplay, then the test harness did a
quit-to-main-menu. The unload re-registers a monster-group member that is still registered. It surfaced as TWO
consecutive asserts on the SAME `unregister_member` call, fixed together:
- **B1 — graph registry.** Stack: `xrServer::Disconnect` → `CALifeSimulator::destroy` → `CALifeSimulatorBase::unload`
  → per-object `on_unregister` → `CSE_ALifeOnlineOfflineGroup::unregister_member` (`alife_online_offline_group.cpp:124`
  `graph.update(member)`) → `CALifeGraphRegistry::update` → `add` → `CSafeMapIterator<...CSE_ALifeDynamicObject...,0,...>::add`
  (graph-point registry) "already found".
- **B2 — schedule registry.** The very NEXT line (`alife_online_offline_group.cpp:125`
  `alife().scheduled().add(member)`) → `CALifeScheduleRegistry::add` → `CSafeMapIterator<...CSE_ALifeSchedulable...,0,...>::add`
  "already found". Fixing B1 advanced the crash exactly one line to B2 — read the registry's data type in the crash's
  `Function:` line (`CSE_ALifeDynamicObject` = graph, `CSE_ALifeSchedulable` = schedule) to tell them apart.

**Fix B1** (`CALifeGraphRegistry::add`): force `no_assert=true` on the graph-point
`m_objects[game_vertex_id].objects().add(object->ID, object, true)` AND on `level().add(object, true)`. The latter
needed a new optional `bool no_assert=false` param threaded through `CALifeLevelRegistry::add`
(`alife_level_registry.h:31` decl + `alife_level_registry_inline.h` def → `inherited::add(object->ID, object,
no_assert)`); all other `level().add` callers (`setup_current_level`, `attach`) keep the default `false`.
**Fix B2** (`alife_schedule_registry.cpp`, `CALifeScheduleRegistry::add`): `inherited::add(object->ID, schedulable,
true)`. A duplicate add is a no-op (existing entry stays) — exactly retail's behavior.

### Files changed (4)
- `src/xrGame/alife_graph_registry.cpp` — `remove`: both removes `no_assert=true`; `add`: both adds `no_assert=true`.
- `src/xrGame/alife_level_registry.h` — `add(...)` gains `bool no_assert = false`.
- `src/xrGame/alife_level_registry_inline.h` — `CALifeLevelRegistry::add` forwards `no_assert` to `inherited::add`.
- `src/xrGame/alife_schedule_registry.cpp` — `CALifeScheduleRegistry::add` passes `no_assert=true`.

### Build / verify — BOTH paths engine-verified (2026-07-24), NOT yet user-confirmed
Incremental Release built clean (exit 0) each cycle, `xrGame.dll` relinked. **Crash A (load) engine-verified:** the
game loads Cordon and runs into gameplay with **0 fatals** over a full soak (previously fataled on the first
post-load frame of A-Life churn). **Crash B (unload) engine-verified:** a full quit-to-main-menu `- Disconnect`
(the test harness triggers one automatically around frame ~4800–8500) now runs the entire ALife unload
(`unregister_member` → `graph.update` → `scheduled().add`) with **0 registry asserts and 0 FATAL ERROR**, and the
game returns to the main menu cleanly. To re-check any run: `grep -E "already found in the registry|hasn't been
found in the registry|FATAL ERROR"` the log — want 0. **Still NOT user-confirmed** — the user reported the load
CTD; they have not re-tested on the real machine. NOTE: with `-silent_error_mode` a single real assert cascades
into ~50 follow-on `invalid_parameter_handler` "invalid parameter" fatals (GatherInfo's sprintf on corrupted
post-throw state) — those are noise; find the FIRST fatal (the real one) and ignore the rest.

### If MORE of this class surface (likely — §8.1 long tail)
Other A-Life ops (`attach`/`detach`, `switch_online/offline`, level-change object migration) use the same
registries and the same `THROW2`/`R_ASSERT` guards. Fix each the same way: pass/force `no_assert=true` (or guard
the specific op) at the CALLER, never by editing the shared `CSafeMapIterator`/macro (that would silence the whole
engine). Match retail: a duplicate add or a missing remove is a no-op, so tolerating it is correct, not a bandaid.

---

## 26. Console spam `! Unknown command: r2_dof_time / r2_dof_pickable` (2026-07-24) — FIXED, engine-built (NOT yet user-confirmed)

### Symptom (as reported)
Opening the console floods it with `! Unknown command: r2_dof_time` / `r2_dof_pickable` (and the log fills with the
same). Non-fatal (was flagged as harmless noise in §8.5 / §14), but the user wanted it gone.

### Root cause
`gamedata\scripts\level_weathers.script` drives Dead Air's dynamic post-process by calling
`get_console():execute("<cvar> <value>")` **every weather update** (hence the flood). A whole set of the cvars it
sets are Dead Air *engine-build* additions that stock OpenXRay never registered, so `IConsole::Execute` falls
through to the "Unknown command" path. Most standard render cvars it uses (`r2_dof`, `r2_dof_near/focus/far`,
`r2_dof_kernel/sky/enable`, `r2_tonemap_*`, `r2_sun_*`, `r2_ls_*`, `r__detail_*`, `r__dtex_range`, etc.) DO exist
in OpenXRay. Done in **two passes** (only the ones whose script path had actually run showed up each time):

Pass 1 (reported: `r2_dof_time` / `r2_dof_pickable`):
| cvar | registered as |
|------|---------------|
| `r2_dof_pickable` | `CCC_Integer` 0..1 |
| `r2_dof_time` | `CCC_Float` 0..10 |
| `r2_dof_diff_far` | `CCC_Float` |
| `r2_vibrance_val` | `CCC_Float` |
| `r2_aberration_val` | `CCC_Float` |

Pass 2 (reported: `r2_lenswater_val` / `r2_lensdirt_val`; the rest found by auditing ALL `execute("r…")` calls in
`gamedata\scripts\` up front so it wouldn't need a pass 3):
| cvar | registered as |
|------|---------------|
| `r2_vibrance` | `CCC_Integer` 0..1 |
| `r2_lensdirt_val` | `CCC_Float` |
| `r2_lenswater_val` | `CCC_Float` |
| `r2_sss_radius` / `r2_sss_phase1` / `r2_sss_phase2` / `r2_sss_blend` | `CCC_Float` |
| `r__color_base_r/g/b` | `CCC_Float` |
| `r__color_add_r/g/b` | `CCC_Float` |

All the `CCC_Float` ones use a deliberately wide -10000..10000 range so the script-set values never trip a
clamp warning either. **Full audit command** (run this, don't wait for spam to reveal them one at a time):
```
grep -rhoE 'execute\("[a-z0-9_]+' D:\DAR3\gamedata\scripts\ | sed 's/execute("//' | grep -E '^r[0-9_]' | sort -u \
  | while read c; do grep -rq "\"$c\"" <engine>\src\ || echo "MISSING: $c"; done
```
(As of pass 2 that reports **nothing** missing. Note `r2_sss` / `r2_sss_kernel` are NOT called — don't add them.)

### The fix (source, `src/Layers/xrRender/xrRender_console.cpp`)
Two small edits, mirroring the existing `r2_dof_*` block:
1. Declare backing globals next to `ps_r2_dof_kernel_size` (~L245): `ps_r2_dof_pickable/time/diff_far`,
   `ps_r2_vibrance_val/aberration_val`, `ps_r2_vibrance`, `ps_r2_lensdirt_val/lenswater_val`,
   `ps_r2_sss_radius/phase1/phase2/blend`, `ps_r_color_base_r/g/b`, `ps_r_color_add_r/g/b`.
2. Register them right after the `r2_dof_enable` `CMD3` (~L916) with `CMD4(CCC_Integer/CCC_Float, "<name>", &global,
   min, max)`.

They are **storage-only** — the values are accepted, clamped, and remembered, but NOT yet consumed by the render
path (no visual DOF-on-pickup / vibrance / lens-dirt / SSS / color-grade effect). That is the faithful minimum to
kill the spam without porting Dead Air's whole dynamic-post-process feature. To make them functional later: wire
`ps_r2_dof_*` into `GamePersistent`/rendertarget DOF like the stock `ps_r2_dof` vector is, and add the
vibrance/aberration/lens/SSS/color passes to `*_rendertarget_phase_combine.cpp` (note §20 already touched
`combine_2`'s aberration constant).

### Why source, not a loose-script hack
The alternative (swallow the calls in Lua) is fragile: `get_console()` returns a C++ singleton whose `:execute`
can't be overridden per-instance, so you'd have to shadow the global `get_console` — mod-invasive and easy to break
other scripts. Registering the cvars is the same §7-class "restore an engine feature Dead Air's build had" fix and
is mod-agnostic.

### Build / verify
Release built clean (exit 0); `xrRender_R4.dll` (+ `xrRender_GL.dll`, `xrEngine.exe`) relinked. `xrRender_console.cpp`
compiles into every render variant, so R2/R3 get it too if ever used (game launches `-r4`). **Engine-built but NOT
yet user-confirmed** — to verify: launch, open console, load a level so `level_weathers` runs, and confirm zero
`Unknown command: r2_*` lines. Regenerate `deadair-openxray-source-patches.diff` (§10) to include this file.

---

## 27. Grenades (and pistols/binoculars) could not be equipped — Dead Air slot layout not wired into the UI (2026-07-25) — FIXED, engine-built (NOT yet user-confirmed)

### Symptom
Dragging a grenade onto its inventory slot did nothing — the grenade snapped back to the ruck. Same class of enum
divergence as §HUD-state / the torch-key bugs, but for **inventory slots**.

### Root cause (two layers)
1. **Slot count is data-driven** from `system.ltx [inventory]` (`Inventory.cpp:58-89` reads `slot_persistent_%d`
   lines and sizes `m_slots` = count+1). Dead Air defines **15 slots** (base 1..15): knife, rifle1, rifle2,
   **grenades(4)**, **sidearm(5)**, bolt, outfit, pda, detector, torch, artefact, helmet, script-anim(13),
   binocular(14), backpack(15). So base 14/15 are valid array indices — **no crash**, the item just has no UI home.
2. **`CInventoryItem::Load` does `base_slot_id = config_slot + 1`** (`inventory_item.cpp:104`). Bases 1-12 line up
   perfectly with OpenXRay's `inventory_space.h` enum (grenades even land on `GRENADE_SLOT`=4). The breakage was
   purely **UI routing**: `GetSlotList()` (`UIActorMenuInventory.cpp`) only had `case`s for the stock slots, so
   `GRENADE_SLOT`, `BINOCULAR_SLOT` (=Dead Air sidearm), and bases 13-15 fell through to `default → ruck list`. And
   the grenade/binoc **configs actually used `slot = 13` (→ base 14)**, a slot with no enum name at all.

### Fix
Two visible, drag-equippable slots were wired up, and the grenade/binoc configs realigned to the slot
`system.ltx` itself labels "grenades":

**Engine (`xrGame`, rebuilt):**
- `ui/UIActorMenu.h` — added `eInventorySidearmList` + `eInventoryGrenadeList` to `eActorMenuListType`.
- `ui/UIActorMenuInventory.cpp` `GetSlotList()` — `BINOCULAR_SLOT → sidearm list`, `GRENADE_SLOT → grenade list`
  (each falls back to the ruck list if the layout XML lacks the control). **Removed `BINOCULAR_SLOT` from the old
  `default` fall-through group** (that dup caused C2196 "case 5 already used" on the first build).
- `ui/UIActorMenuInitialize.cpp` `InitializeUniversal()` — bound the two lists to the **existing** XML controls
  `dragdrop_sidearm` and `dragdrop_binocular` (+ their highlights), `required=false`; added `BindDragDropListEvents`
  for both. `InitializeUniversal` is the live path for CoC mode (SoC-only code uses `InitializeInventoryMode`).
- `ui/UIActorMenu.cpp` — registered both lists in `GetListType()` (returns `iActorSlot`; it ends in `NODEFAULT`, so
  an unregistered list would assert) and `ClearAllLists()`.
- `ui/UIActorMenuInventory.cpp` — added both to the `InitInventoryMode` show block and the `OnInventoryAction`
  `all_lists[]`. `InitCellForSlot` already covered `GRENADE_SLOT` and `BINOCULAR_SLOT`.

**Config (loose gamedata, no rebuild):**
- `configs/weapons/{w_f1,w_rgd5,w_gd5,w_gsmoke,w_binoc}.ltx` — `slot 13 → slot 3` (base 4 = `GRENADE_SLOT`).
  Grenades now land in the engine's real grenade slot, so they also get the built-in throw/cycle/auto-refill
  (`Actor.cpp:1806`, `ActorInput.cpp SlotsToCheck`). Grenades + binoculars share this slot (top-left
  `dragdrop_binocular` cell) exactly as Dead Air's `slot=13` had them share base 14. **Pistols needed no config
  change** — they were already base 5 (`BINOCULAR_SLOT`); they just lacked a UI list, now the sidearm slot.

### Slot map (config slot → +1 → OpenXRay enum → UI list). Bold = fixed this session.
`0→1 KNIFE→knife · 1→2 INV_SLOT_2→pistol · 2→3 INV_SLOT_3→automatic · `**`3→4 GRENADE_SLOT→grenade(new)`**` ·
`**`4→5 BINOCULAR_SLOT→sidearm(new)`**` · 5→6 BOLT→ruck · 6→7 OUTFIT→outfit · 7→8 PDA→ruck · 8→9 DETECTOR→detector ·
9→10 TORCH→ruck · 10→11 ARTEFACT→belt/ruck · 11→12 HELMET→helmet · 12→13 BACKPACK→backpack(abstract bases only) ·
13→14 (was grenade/binoc, now empty) · 14→15 kit_hunt→ruck`. Note the real script-anim hand items
(`animation_planshet/foto/…`) actually sit at config slot 2 (base 3, the rifle slot) — that's how the §17 planshet
works — **not** at base 13; base 13/15 have no spawnable items, so they were left alone.

### Build / verify
Release built clean (exit 0), `xrGame.dll` relinked to `bin\x64\Release`. Smoke-launch loaded the full Lua suite +
UI subsystem with **zero new errors** (the only FATAL is the pre-existing missing-`quicksave8` auto-load, present 4×
in the pre-change log). **NOT yet user-confirmed in-game** — the actor menu only constructs on level load, which the
auto-loader couldn't reach. To verify: load a real save (quicksave1-10), open inventory, drag a grenade onto the
top-left slot (should stick + show), confirm binoculars go there too and pistols equip to the sidearm slot below-left.
Widescreen uses `actor_menu_16.xml` (has the controls); the 4:3 `actor_menu.xml` lacks `dragdrop_sidearm/binocular`,
so those users keep the old ruck behavior (engine no-ops gracefully) until the controls are added there too.
Regenerate `deadair-openxray-source-patches.diff` (§10) to include the four `ui/UIActorMenu*` files.

**If a future Dead Air key/slot/HUD value "does nothing", suspect a data-vs-enum divergence FIRST (this is now the
4th: HUD-state, torch key, NV key, inventory slots).**

### 27a. Follow-up: double-click equip left the grenade slot visually empty until reopen (2026-07-25) — FIXED, engine-built
After §27, grenades/binoculars *were* equippable but a quirk remained: **double-clicking** one in the ruck left the
slot looking empty; the item only appeared in-hand after closing and showed equipped on the next open. Root cause:
`TryActiveSlot()` (`UIActorMenuInventory.cpp`, called only from `OnItemDbClick`) special-cases `GRENADE_SLOT` and
fires **only** deferred network events (`SendEvent_ActivateSlot`/`Item2Slot`) — no immediate `Slot()` and no UI
`SetItem`, so the cell never moved live (the events apply over the next frames → in-hand on close, cell rebuilt
correctly by `InitCellForSlot` on reopen). The **drag** path was fine (`ToSlot` moves the cell immediately). Fix:
`TryActiveSlot` now `return false` for `GRENADE_SLOT` when a real slot list exists (`m_pLists[eInventoryGrenadeList]`),
so double-click falls through to the standard `ToSlot` machinery (equip / swap / unequip-toggle + live cell refresh,
and `ToSlot` still fires `SendEvent_ActivateSlot` so the grenade is readied). The legacy activate-in-place branch is
kept only for layouts without the slot (4:3). Grenades aren't belt items and `TryActiveSlot` has no other callers, so
the fall-through is safe. Rebuilt clean. **USER-CONFIRMED: equipping (drag + double-click) works.**

### 27b. Empty-slot equip failed for grenades — UI/logical desync in ToSlot swap path (2026-07-25) — FIXED
Symptom: with the grenade slot *looking* empty, some grenades wouldn't equip; once one visibly occupied it, all
swapped fine. Cause: grenades have `default_to_ruck=false` (auto-equip) and the `CGrenade` auto-refill
(`Actor::OnItemDrop`, `Actor.cpp:1806`) can put a grenade in the slot **logically** with no cell in the visible slot
list (the list only syncs on menu open via `InitCellForSlot`). `ToSlot`'s busy-slot branch then bailed at
`if (slot_list->ItemsCount() != 1) return false;`. Fix (`UIActorMenuInventory.cpp`): when `ItemsCount()==0` but
`ItemFromSlot(slot)` returns an item, re-add the missing cell (`slot_list->SetItem(create_cell_item(_iitem))`) before
the count check, so the swap proceeds. **USER-CONFIRMED.**

### 27c. THE BIG TIME-SINK: `user.ltx` corruption + wrong appdata location (2026-07-25) — RESOLVED
Grenade **throwing / G-key** work went sideways and cost hours. Read this before any future keybind/input work.

1. **Attempt (reverted):** to make Dead Air's `G` (bound to `wpn_7` in `default_controls.ltx`) ready a grenade, a
   `kWPN_7` action was added to the `EGameActions` enum + `actions[]` table + Lua registrator + an `Inventory.cpp`
   toggle handler. First build put the `actions[]` row in the wrong position → `R_ASSERT3(actions[idx].id==idx)` in
   `initialize_bindings` fired. **NEVER add to `EGameActions` without adding the matching `actions[]` row in the
   SAME ordinal position.** Fixed the position, rebuilt — but the regression below persisted, so the whole `kWPN_7`
   change was **reverted** (it also never reached the user: their `user.ltx` binds `G` as `bind_sec ui_back kG`,
   not `wpn_7`).
2. **The regression that fooled me for hours:** "inventory key opens the console" + "LMB cycles grenade types
   instead of throwing." Cause was NOT code: when the user clicked **Continue** on the assert dialog, the game ran
   with a scrambled binding table and, on exit, **re-saved `user.ltx` with shifted name↔key pairs** — `bind console
   kI` + `bind inventory kI` (both on I), `bind wpn_next mouse1`, `bind wpn_fire mouse2`. Persisted across every
   rebuild/revert because it's saved DATA. **Any ignored assert in `initialize_bindings` can corrupt `user.ltx` on
   the next clean exit.**
3. **Why I couldn't see it:** the running build (`bin\x64\Release\xrEngine.exe`) resolves `$app_data_root$` relative
   to the exe, so the LIVE config/saves/logs are in **`D:\openxray-build\xray-16\bin\x64\Release\appdata\`**, NOT
   `D:\DAR3\appdata\` (that copy is stale — its `user.ltx` was Jul 22 and never changes). I read the stale one for
   ages. **Always use the bin appdata when debugging the running game.** The full log only flushes on a clean exit;
   `KERNEL:CONSOLE` / `KERNEL:QUIT` mark the fired action + shutdown.
4. **Fix:** restored the bind lines from the clean `D:\DAR3\appdata\user.ltx` into the live bin `user.ltx` (kept the
   live file's video/sound settings); corrupt copy saved as `user.ltx.corrupt_bak`. Engine table is correct now so
   future exits save clean. **USER-CONFIRMED back to normal.** Also note: `quicksave8` is genuinely
   version-mismatched/corrupt (`valid_saved_game`: `save_version < ALIFE_VERSION`) and is the newest save, so the
   auto-continue targets it and FATALs — unrelated to any of this; load a different save.

### 27d. Grenade in-hand toggle on G — SOLVED IN PURE LUA (2026-07-25) — USER-CONFIRMED
After the engine-action route blew up, did it the safe way: a loose-gamedata Lua handler, no engine change, no
rebuild, no corruption risk. In `scripts/itms_manager.script` `on_key_press(key)` (fires for every key with the raw
**SDL scancode** — `Level_input.cpp:143` passes the scancode from `IR_OnKeyboardPress`, confirmed by its
`key == SDL_SCANCODE_ESCAPE` check; `dik_to_bind` is really scancode→action):
```lua
if (key == 10) then -- SDL_SCANCODE_G
    if (db.actor:active_slot() == 4) then
        db.actor:activate_slot(grenade_prev_slot or 0)   -- holster → restore prior weapon / empty
        grenade_prev_slot = nil
    elseif (db.actor:item_in_slot(4)) then               -- grenade equipped in GRENADE_SLOT
        grenade_prev_slot = db.actor:active_slot()
        db.actor:activate_slot(4)                        -- ready to hand
    end
    return
end
```
`item_in_slot`/`activate_slot`/`active_slot` use DATA base-slot ids (actor_effects uses `item_in_slot(7/12/15)` for
outfit/helmet/backpack); slot 4 = grenades in both the data layout and the enum. Scripts load on level load, so a
fresh launch + load picks it up. If a future key needs a handler, THIS is the pattern — never touch the enum.

---

## 28. Backpack equip animation plays but nothing equips / carry weight never changes (2026-07-25) — FIXED, USER-CONFIRMED

### Symptom (as reported)
Equipping a backpack only plays the equip animation — the backpack never actually equips and "equip load" (max
carry weight) stays the same.

### Root cause (two independent gaps, both had to be fixed — the 5th instance of the data-vs-enum divergence class)
1. **`BACKPACK_SLOT` enum (`inventory_space.h`) was 13, but Dead Air's real data lives at base 15.**
   `system.ltx [inventory]` labels its 15th slot `;backpack` (`slot_persistent_15`), every backpack item
   (`items_attachments.ltx [kit_hunt]` and its variants `backpack_light/heavy/travel/stalker`, plus `airtank`,
   `ccrm`, `exobackpack`, `belt_pouch` — all mutually-exclusive "back slot" items sharing one base) declares
   `slot = 14` (config, 0-indexed) → `base_slot_id = 15` (`CInventoryItem::Load`, base = config+1). **20+ Lua
   call sites** (`actor_effects.script`, `xr_conditions.script`, `xr_effects.script`, `itms_manager.script`,
   `dar2_quest_metro.script`, `ui_dar2_game_options.script`) already hardcode `item_in_slot(15)` for backpack and
   work correctly — the item genuinely reaches base 15 at the data layer (that's why the equip animation, gated on
   `actor_effects.script`'s `item_in_slot(15)` polling, correctly fires). But `BACKPACK_SLOT` in the enum was 13
   (the next sequential value after `HELMET_SLOT`), which is Dead Air's OWN "script animation" / hand-item-overlay
   slot (`dar2_animations_hands.script`, `dinamic_hud.script` both read `item_in_slot(13)` directly for that,
   unrelated to backpacks) — so realigning DATA to 13 (the §27 grenade/binoc pattern) would have collided with an
   already-used slot. Went the other way instead: **`BACKPACK_SLOT` explicitly set to `= 15`** in the enum
   (`SLOTS_COUNT` follows automatically). This also upgrades the backpack's UI representation for free — the
   already-existing (pre-built, unused until now) `dragdrop_backpack`/`eInventoryBackpackList` plumbing
   (`GetSlotList()` case, `InitCellForSlot(BACKPACK_SLOT)` at `UIActorMenuInventory.cpp:468`) now points at the
   right slot instead of falling through to the generic "abstract slots past `LAST_SLOT`" ruck-fallback loop
   (`UIActorMenuInventory.cpp:483`) that previously gave it no real UI home.
2. **Even in the right slot, `GetBackpack()` never found it — the items aren't `CBackpack` objects.** All the
   backpack items are `class = SCRPTART` (spawn as `CElectricBall`/script-artefact, `CSE_ALifeItemArtefact`), not
   `CLSID_EQUIPMENT_BACKPACK`/`CBackpack` — they're built on Dead Air's artefact infrastructure (icons, condition/
   degrade, `itms_manager` dismantle menu, `bind_artefact.script`'s `set_artefact_additional_weight` at spawn,
   which is what the §7 `SetAdditionalWeight` export exists for). So `InventoryOwner.cpp`'s
   `GetBackpack()` (`smart_cast<CBackpack*>(ItemFromSlot(BACKPACK_SLOT))`) always returned null for them, and
   `CActor::get_additional_weight()` (`Actor_Movement.cpp` — feeds BOTH `MaxWalkWeight()` and `MaxCarryWeight()`
   in this port, confirmed the single authoritative "equip load" calc) never added their bonus. Changing these
   items' `class` to a real `CBackpack` registration was rejected as too invasive (would drop their artefact
   behavior — condition/degrade, dismantle, radiation immunities via `af_base_absorbation`). Instead, mirrored the
   existing belt-artefact weight-bonus loop a few lines below: in `get_additional_weight()`, when `GetBackpack()`
   is null, `smart_cast<CArtefact*>(inventory().ItemFromSlot(BACKPACK_SLOT))` and add
   `AdditionalInventoryWeight() * GetCondition()` the same way belt artefacts do. This picks up the real,
   spawn-randomized weight value (`bind_artefact.script` → `set_artefact_additional_weight` → `CArtefact::
   SetAdditionalWeight` → `m_additional_weight`), not just the static ltx default.

### Fix (source, 2 files)
- `src/xrServerEntities/inventory_space.h` — `BACKPACK_SLOT` given an explicit `= 15` (was implicit 13);
  documented why 13/14 are skipped (13 = Dead Air's own script-animation slot, 14 = vacated by §27's grenade/
  binoc move, neither is backpack).
- `src/xrGame/Actor_Movement.cpp` `CActor::get_additional_weight()` — added the `else` branch (CArtefact fallback
  in `BACKPACK_SLOT`) described above, right after the existing `CBackpack*` check.

No `gamedata\` / loose-data changes at all — every existing Lua `item_in_slot(15)` reference and every backpack
item's `slot = 14` stay exactly as Dead Air shipped them; only the engine's own slot constant was wrong.

### Build / verify
Release built clean (exit 0) — touching the shared `inventory_space.h` header triggered a full `xrGame` rebuild
(18k/192k functions recompiled), `xrGame.dll` relinked. Smoke-launched with `-silent_error_mode
-no_call_stack_assert`: the automated test-harness soak ran to its own `KERNEL:QUIT` with **zero `FATAL ERROR`,
zero asserts** in the log. `deadair-openxray-source-patches.diff` regenerated (now current through this section).
**USER-CONFIRMED 2026-07-25** — backpack equips correctly in-game and max carry weight increases.
