# Vulkan 102: contracts exercised by the renderer

This records the game-path subset of PDF tasks 1–11. `shaders.xr` is read through
the engine VFS. Every property keeps its serialized type, scope and bytes in
`MaterialDefinition`; acceptance depends on its semantics and the consumer's
context. An unfamiliar property, non-default render priority, non-default
base/detail matrix, tessellation selection or tree_s selection fails with the
blender ID, class, version and shader name. A parsed definition alone is not
proof that a level, model, particle or UI pipeline can render it.

## Serialized property schema

All known classes retain `General/Priority` (default 1), `General/Strict sorting`
(default false), and `Base Texture/Name` and `Transform` (default `$base0`
and `$null`). A non-default priority or named base matrix is rejected by the
current game pipeline. The following are the additional properties read by
the original class `Save`/`Load` methods. A property introduced by a newer
version is rejected if inserted into an older record, even if its value is 0.

| Class | Accepted versions | Additional serialized properties and defaults |
| --- | --- | --- |
| `LM`, `V` | 0–1 | `Tessellation` since v1, token 0 (no tessellation) |
| `LM_AREF`, `V_AREF` | 0–1 | `Alpha ref` 32; `Alpha-blend` false since v1 |
| `MODEL` | 0–2 | `Use alpha-channel` false and `Alpha ref` 32 since v1; `Tessellation` token 0 since v2 |
| `D_TREE` | 0–1 | `Alpha-blend` false; `Object LOD` false since v1 |
| `D_STILL` | 0 | `Alpha-blend` false |
| `BmmDold`, `BmmD`, `LmBmmD` | up to 3, 3, 1 respectively | `Detail map/Name` and `Transform` (`$null`); terrain `R2-R/G/B/A` slots in v3 |
| `LmEbB`, `MODELEbB` | 0–1 | `Environment map/Name`, `Transform` (`$null`); `Alpha-Blend` false since v1 |
| `LaEmB` | 0 | `Environment map/Name`, `Transform`, `Constant` (`$null`) |
| `PARTICLE` | 0 | `Blending` token 0, `Texture clamp` true, `Alpha ref` 32 |
| `S_SET` | 0–4 | `Blending` token 0, `Texture clamp` true (omitted by v2 `Load`), `Alpha ref` 32; `Z-test`, `Z-write`, `Lighting`, `Fog` false |

The parser retains raw values and scopes for every property. The Vulkan
consumer rejects nonzero tessellation and Object LOD, unsupported scene flag
combinations, and unknown class properties with the exact class, version,
shader and property path. Synthetic records test these rejection boundaries.

## Material slots and passes

| Blender family | Game path and texture roles | Game pass and limitations |
| --- | --- | --- |
| `LM`, `V`, `LM_AREF`, `V_AREF` | Base; LM also lightmap. Diffuse `.thm` detail and scale are used by opaque LM/V, cutout LM_AREF/V_AREF, and transparent variants with detail. | G-buffer opaque/cutout or depth-tested forward alpha for transparent detail. Bump-detail/normal uses the THM DDS pair and surface descriptor slots. |
| `D_TREE`, `D_STILL`, `MODEL` | Base, cutout threshold; standalone tree models select the two-sided variant by class. | G-buffer or forward by blender state; nonzero `Object LOD` and `Tessellation` rejected. |
| `BmmDold`, `BmmD`, `LmBmmD` | Base, lightmap, explicit detail **or** terrain mask + R/G/B/A textures; `.thm` scale. | One G-buffer draw with detail binding 2 or four terrain bindings 3–6. R/G/B/A bump and bump# pairs contribute tangent normals, gloss and height. |
| `LmEbB`, `MODELEbB` | Base + environment cube; `LmEbB` also lightmap. | Reflection shader; model forward only when blender selects blending. Opaque `MODELEbB` is rejected because its G-buffer reflection variant is absent. Named environment matrix rejected. |
| `LaEmB` | Base, lightmap, 2D environment, optional waveform constant. | Ordered G-buffer base, then multiply-2x with depth read/no write. Named matrix rejected. |
| `PARTICLE` | Base, clamp/wrap sampler by property. | Modes 0–5: set, alpha, add, multiply, multiply-2x, alpha-add. Mode 0 uses original fixed alpha reference 200. |
| `S_SET` | Base, clamp/wrap sampler by property. | UI modes 0–9. World depth attachment requires compatible Z-test/write and flags; unsupported combinations report the precise property/mode. |

The descriptor cache includes all texture names and the sampler choice. Active
sets retain their image views until idle retirement; both wrap and clamp
descriptors are rebound after texture reload. An absent optional `.thm` keeps
scale 1 and no implicit detail. A present malformed detail chunk fails with
its VFS path. An explicit detail name is not replaced by the base texture.
Base, lightmap, mask and detail slots require 2D DDS; the environment slot
requires a cube for `LmEbB`/`MODELEbB` and 2D for `LaEmB`. Errors name the
role, slot number and offending resource.

`LevelPassGraph` selects the level shader pair, texture-role mask, attachment,
blend and depth state once per mesh. Its second `LaEmB` node reads the color
written by the first draw in the same ordered geometry render pass. The same graph type is used by level, model, particle, HUD and UI consumers;
level and model nodes also carry the surface role mask. The exact remaining
context and real-archive test gates are in `VULKAN_102_MATRIX_SCOPE.md`.

## Surface descriptor and G-buffer contract

Material set 0 binds base/lightmap/detail or environment at 0–6, base bump and
bump# at 7–8, detail bump and bump# at 9–10, terrain R/G/B/A bump at 11–14,
external normal at 15, and terrain bump# at 16–19. An unused role receives the
base DDS; hot reload rebinds every slot. The fragment push constants at 80–111
carry bump mode, detail/external normal flags, material class and weight, height
scale, emission, and terrain mask. Opaque environment variants use 96–127.

The third RGBA8 color attachment stores `(gloss, material/3, emission, height)`.
Its normal partner stores the mapped normal; sun and local lighting sample both.
Forward lighting applies the same normal and gloss data. Additive glow passes
still use their original forward blend. Host tests cover descriptors, pass graph,
attachment layout and HLSL compilation; rendered reference images remain open.

## Geometry claims (SoC, CS, CoP)

| Profile selected by game | Header | Level geometry | OGF and shader dependency |
| --- | --- | --- | --- |
| SoC | v14, quality 0–2 | 16-bit indices; float3/packed normal and float2/packed UV; optional UV1/color/tangent/binormal | Level static/progressive/tree/LOD; OGF v4; shader entry must resolve by class and properties |
| CS | v14, quality 0–2 | Same bounded decoder; optional geomX progressive fast geometry | Same OGF decoder; fast geometry may fall back to valid `level.geom` with a reason |
| CoP | v14, quality 0–2 | Same bounded decoder, including atlas UV1 | Same OGF decoder and material checks; unsupported type/layout names its visual and profile |

Standalone OGF models use embedded static/skinned vertices (1–4 bone links),
progressive windows and child hierarchies. The renderer does not infer a game
profile from the geometry. These rows describe the exact implemented decoder
contract and synthetic coverage, not validation against every real archive
and blender combination.

The game chooses the profile. The common supported header is level version 14
with quality 0–2. Level vertex declarations require float3 position,
float3/packed normal, float2/packed UV; optional UV1, color, tangent and
binormal are decoded with bounds checks. Indices are 16-bit in level.geom;
sliding windows validate ranges. Level OGF version 4 handles static (0),
hierarchy (1), progressive (2), LOD (6), tree (7), tree with windows (11),
and geomX fast path (22) for progressive visuals. Standalone models also
decode skinned OGF with 1–4 bone links, hierarchy and progressive windows.
Unknown version/layout and malformed chunks fail; a skipped geomX variant
reports the reason and uses valid regular geometry. This is the decoder
matrix, not evidence that all materials or original game scenes display
correctly; no real SoC/CS/CoP game archives are part of the host fixtures.

## Verification boundary

Host tests cover serialized properties, sparse blender IDs, naming invariance,
negative cases, level layouts, sliding windows, shader assets, forward detail scale and selected
pipeline states. The forward transparent queue now interleaves individual
detail placements with models and particles by camera distance, uploading one
buffer per frame slot. Blending of intersecting triangles, terrain normal/gloss parity, shadows and
performance still require image checks on the target device. A successful parser or pipeline mock cannot establish
pixel correctness on Adreno.
