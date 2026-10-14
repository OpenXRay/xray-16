# PDF 102: context and archive acceptance matrix

This is a test inventory, not a claim that every combination works. A row is
supported only when its serialized state reaches the named consumer, the
pipeline state and shader bindings are asserted, and a representative in-game
scene is accepted. Arbitrary names and VFS overrides must preserve semantics.

## Texture-role families to implement

The original `sload.h` uses a paired bump texture (`s_bump` and `s_bumpX`),
with packed normal `bump.wzy + bumpX.xyz - 1`, gloss `bump.x²` and height
`bumpX.z`; diffuse detail multiplies base RGB and can modulate gloss with its
alpha. Detail-bump uses its own `detail_bump`/`detail_bump#` pair and modifies
normal and gloss. BmmD terrain HQ additionally uses a mask, four diffuse
details and four `*_bump` textures, and optionally four `*_bump#` textures.
The lightmap/hemi roles are separate from these channels. Emission is a
pass/material property (including glow/LaEmB), not a universal DDS suffix.

| Source family | Role combinations to distinguish | Shader/attachment expectation |
| --- | --- | --- |
| LM/V and alpha-ref level | Flat, diffuse detail, bump pair, bump plus diffuse detail, detail bump pair, lightmap/hemi where serialized | Base/normal/gloss/material survive to lighting; alpha/depth consistent across variants |
| BmmD terrain | Mask + four diffuse channels + four normals, optional parallax companions, explicit versus implicit detail | Weighted channels sampled from their respective names and UV scale; no substitution of a detail name for a missing normal |
| MODEL, D_TREE and animated OGF | Flat, bump pair, detail if class permits, cutout/transparent; model lighting and skinning | Tangent basis, alpha/depth, normal/gloss and forward light response for every accepted class |
| Environment, LaEmB and special emission | Base, environment 2D/cube, lightmap, constant, glow/multiply pass where defined | Original pass order and blend equation; color/energy retained through combine |

These rows require extending the G-buffer and its readers together (PDF
12–14). Merely binding extra DDS or adding a shader name would discard gloss
and material before lighting. `THM_CHUNK_BUMP`, `THM_CHUNK_MATERIAL` and
`THM_CHUNK_EXT_NORMALMAP` now have bounded parsing fixtures; rendering their
semantics is still an open implementation item.

## Contexts and modes

| Consumer | Material records to exercise | Required assertions | Current gap |
| --- | --- | --- | --- |
| UI overlay | `S_SET` blending 0–9; clamp on/off; alpha reference; UI texture/video | UI target has no depth attachment; exact blend factors for all ten tokens; sampler addressing; ordering with text and menu highlighting | Host mode fixtures exist, but device UI images and all video/reload combinations remain |
| World static level | `S_SET` 0–9 with each meaningful Z-test, Z-write, lighting, fog and clamp combination; `LM`, `V`, `LM_AREF`, `V_AREF`, BmmD, environment | Reject flags requiring a missing attachment; exact depth/write, sorting, fog/lighting and texture roles; no fallback to alpha blending | Depth-tested unlit world `S_SET` variants 0–9 have distinct blend routes (SET writes depth); depthless and incompatible fog/lighting combinations remain rejected |
| Standalone OGF and animated model | `MODEL`, `D_TREE`, `MODELEbB` and applicable `S_SET`; opaque, cutout, transparent; 1–4 skin weights | Correct pass per shader class and context, bone/UV/normal transfer, cull, blend/depth, LOD and resource lifetime | Static/skinned OGF `S_SET` 0–9 uses the world forward route. Opaque `MODELEbB` follows the original base-only deferred path; its blended path samples the cube. OGF HUD S_SET, water and fog variants are explicitly rejected; full archive and context coverage remains |
| World sprite particle and group | `PARTICLE` 0–5, `S_SET` where referenced; clamp on/off; fixed SET alpha reference and serialized alpha | Exact per-token blend, depth read/no write, fog/lighting where specified, lifetime across reload and frames in flight | Mode host checks exist; real particle effects and image acceptance remain |
| HUD sprite and model | Same particle modes in HUD, model shaders with HUD projection and depth | Separate HUD pass/attachment and projection; ordering against world and UI; alpha and descriptor state | Full material-context Cartesian coverage absent |
| Decal, wallmark and tracer | `S_SET` world modes and flags used by each source; opaque versus multiply-2x and alpha-add | Depth-tested attachment, no unintended depth write, exact source/destination factors, stable order on surfaces | Some special passes exist; the complete token/flag inventory is not exercised |

For each row, fixtures must cover every **accepted** mode and each rejected
combination with its property name. Tests assert shader variant, pass count and
order, texture-role bindings, blend equation, depth state, sorting, fog,
lighting, clamp, reload and two frames in flight. A shader class alone never
grants every mode in every context.

## Actual game/archive profiles

Run the same procedure independently for **SoC, CS and CoP**. For each profile,
record game build and asset manifest hashes, selected quality, renderer mode,
VFS mount order, overridden loose files, `shaders.xr` class/version counts,
level header version, `level.geom` declarations/index ranges, optional
`level.geomx` and fallback reason, OGF type/skin weights/sliding windows,
texture DDS formats and `.thm` flags. The test set needs at least one ordinary
level, terrain, tree vegetation, skeleton NPC, UI/HUD and representative
particle/decal for each game, plus a level with geomX where that profile
provides one. Repeat with a mod override using arbitrary names, a sparse
blender ID and replaced `.thm`/DDS, not with edits to original game assets.

| Gate | Evidence to retain | Failure criterion |
| --- | --- | --- |
| Decode | Resource path, profile, material/visual ID, version, vertex declaration, UV1, index and sliding-window bounds | Lost UV/normal/lightmap or a generic success after an incompatible layout |
| Material resolution | Serialized properties and chosen context, roles, pass graph and unsupported reason | Missing required bump/detail/gloss/emissive/environment channel or a name-derived mode |
| Vulkan host/device | Descriptor images and sampler, SPIR-V bindings, pipeline blend/depth, attachment transitions and validation output | Incorrect role/channel, missing barrier or shader/pipeline mismatch |
| Scene acceptance | Same camera and settings for image capture, visual overlap/lighting/fog and frame-time; level transition and reload | Material absent or visually wrong, validation error, crash, unstable frame time |

The repository contains synthetic decoder and shader fixtures, but **no
licensed SoC/CS/CoP game archives or device captures**. This gate cannot be
marked passed from a host mock or an APK build. The minimal user-provided
input is access to legitimately owned game installations or a bounded,
reproducible set of unmodified archives for each profile, and device output
for visual/frame-time acceptance. Do not publish original assets in the PR.
