# Vulkan 102: contracts exercised by the renderer

This records the game-path subset of PDF tasks 1–11. `shaders.xr` is read through
the engine VFS. Every property keeps its serialized type, scope and bytes in
`MaterialDefinition`; acceptance depends on its semantics and the consumer's
context. An unfamiliar property, non-default render priority, non-default
base/detail matrix, tessellation selection or tree_s selection fails with the
blender ID, class, version and shader name. A parsed definition alone is not
proof that a level, model, particle or UI pipeline can render it.

## Material slots and passes

| Blender family | Game path and texture roles | Game pass and limitations |
| --- | --- | --- |
| `LM`, `V`, `LM_AREF`, `V_AREF` | Base; LM also lightmap. Opaque LM/V can use `.thm` diffuse detail and its scale. | G-buffer opaque/cutout. Bump detail and alpha-tested implicit detail need a separate variant; reject their requests. |
| `D_TREE`, `D_STILL`, `MODEL` | Base, cutout threshold; standalone tree models select the two-sided variant by class. | G-buffer or forward by blender state; nonzero `Object LOD` and `Tessellation` rejected. |
| `BmmDold`, `BmmD`, `LmBmmD` | Base, lightmap, explicit detail **or** terrain mask + R/G/B/A textures; `.thm` scale. | One G-buffer draw with detail binding 2 or four terrain bindings 3–6. Normal/bump/gloss terrain stages are not represented by this path and need further work. |
| `LmEbB`, `MODELEbB` | Base + environment cube; `LmEbB` also lightmap. | Reflection shader; model forward only when blender selects blending. Named environment matrix rejected. |
| `LaEmB` | Base, lightmap, 2D environment, optional waveform constant. | Ordered G-buffer base, then multiply-2x with depth read/no write. Named matrix rejected. |
| `PARTICLE` | Base, clamp/wrap sampler by property. | Modes 0–5: set, alpha, add, multiply, multiply-2x, alpha-add. Mode 0 uses original fixed alpha reference 200. |
| `S_SET` | Base, clamp/wrap sampler by property. | UI modes 0–9. World depth attachment requires compatible Z-test/write and flags; unsupported combinations report the precise property/mode. |

The descriptor cache includes all texture names and the sampler choice. Active
sets retain their image views until idle retirement; both wrap and clamp
descriptors are rebound after texture reload. An absent optional `.thm` keeps
scale 1 and no implicit detail. A present malformed detail chunk fails with
its VFS path. An explicit detail name is not replaced by the base texture.

## Geometry claims (SoC, CS, CoP)

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
negative cases, level layouts, sliding windows, shader assets and selected
pipeline states. The forward transparent queue now interleaves individual
detail placements with models and particles by camera distance, uploading one
buffer per frame slot. Blending of intersecting triangles, lighting, full
terrain normal/gloss, shadows and performance still require image checks on
the target device. A successful parser or pipeline mock cannot establish
pixel correctness on Adreno.
