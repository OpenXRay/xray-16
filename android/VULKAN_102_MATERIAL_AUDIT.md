# Vulkan material compatibility audit (plan 102)

This document describes the implementation, not a device acceptance result. The
game's mounted `shaders.xr`, `.thm`, DDS and level archives remain unmodified.
Names of mod resources are data; the material class and serialized properties
are the authority for surface mode.

The parser retains every serialized property in order as `(scope, name, type,
payload)`, so repeated `Name` and `Transform` fields from different scopes do
not overwrite each other. The public `MaterialDefinition` distinguishes a
parsed record from a renderable pipeline. Unknown classes or newer versions
fail at resolve time with blender ID, class, version and shader name. A missing
library fails explicitly. The environment-map families are parsed but now
reject a draw because the second texture pass is not implemented.

| Blender family | Observed properties / defaults | Current render path | Remaining work |
| --- | --- | --- | --- |
| `LM`, `V` | Base Name/Transform, Priority, Strict sorting, Tessellation (where present) | Opaque; level lightmap when supplied | Validate tessellation token and all non-default variants |
| `LM_AREF`, `V_AREF` | Above plus Alpha ref and Alpha-blend | Cutout or transparent according to saved blend | Validate depth and sorting for each combination |
| `MODEL` | Base, Alpha ref, Use alpha-channel, Tessellation | Model opaque/cutout/forward | Complete bump, detail and material response |
| `D_TREE`, `D_STILL` | Base, Alpha-blend, optional Object LOD | Tree/detail opaque or cutout; transparent detail now uses sorted forward draws | Verify all LOD and blending variants on device |
| `BmmDold`, `LmBmmD`, `BmmD` | Base, optional detail Name/Transform, R2-R/G/B/A | Four-channel terrain if the complete R2 set exists | Single-channel detail, transform, channel surface data |
| `LmEbB`, `MODELEbB` | Base plus Environment map Name/Transform, Alpha-Blend | Rejected with `Environment map / second texture pass` | Reflection texture binding and original multipass blend |
| `PARTICLE` | Base, Blending token, Texture clamp, Alpha ref | Sprite modes 0–5 | Complete scene state matrix and context tests |
| `S_SET` | Base, Blending, Texture clamp, Alpha ref, Z-test/write, Lighting, Fog | Limited UI and scene modes, with explicit errors for unsupported combinations | Additional scene pipelines and state tests |
| `LaEmB` | Class/version parsed | Legacy approximation | Emissive pass and channel contract |

The original blender `Compile` methods under `src/Layers/xrRender/blenders`
are the source of pass semantics. The included exported `shaders.xr` fixture
contains `BmmDold` records with both a single `Detail map/Name` and the four
R2 channels, and `LmEbB`/`MODELEbB` records with an actual environment slot.
The R2 terrain implementation consumes the four channels; merely retaining
the single detail property does not make its earlier-game pass work.

## Evidence and remaining acceptance

The current host tests cover parsing, sparse blender IDs, unsupported classes,
serialized secondary slots, empty particle-group iteration and level archive
decoding. They do not assert a complete blend/depth matrix, shader sampling of
every texture role, reflection, correct colors or device frame time. Both
Android Debug native targets compile, but Android compilation alone cannot
close the visual criteria of plan 102.

The old `GpuLevel::named_surface` water/glass route and lightmap name check
remain name dependent; their removal requires a role derived from serialized
material or level metadata. The global ordering of transparent detail against
other transparent model/particle submissions also remains to be integrated.
