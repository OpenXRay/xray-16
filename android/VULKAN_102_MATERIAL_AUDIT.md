# Vulkan material compatibility audit (PDF tasks 1–11)

`VULKAN_102_CONTRACT.md` contains the class and version schema, texture roles,
pipeline paths and precise limitations. `VULKAN_PDF_PLAN_PROGRESS.md` tracks the
PDF's completion criteria. This audit reflects the same current tree.

The mounted `shaders.xr`, `.thm`, DDS and level archives are read through VFS.
The parser retains serialized `(scope, name, type, payload)` entries in order;
the resolver distinguishes a parsed definition from a supported game pipeline.
Properties are checked against the original blender's class and version. For
example, `Alpha ref` on `LM`, `Texture clamp` on `LM_AREF`, `Constant` on
`LmEbB`, and a v3 terrain channel in a v2 record cannot silently pass as
ordinary opaque geometry. Their errors identify the blender ID, class,
version, shader and property.

`LM`/`V` diffuse detail, `LM_AREF`/`V_AREF` cutout detail,
transparent diffuse detail, single explicit BmmD detail, four-channel terrain,
and environment bindings have Vulkan paths. Bump/normal/gloss/emissive roles, named texture matrices and
the complete context-specific `S_SET`/`PARTICLE` matrix still need work. The
`LaEmB` base and multiply-2x stages execute in order, but the general
blender pass graph and all original element combinations are unverified.
An opaque `MODELEbB` OGF reports its missing environment pipeline instead of
rendering with the ordinary diffuse model shader.

Surface selection uses serialized blender semantics. The water role uses
saved state, and `Unresolved` is retained until the library supplies a mode;
arbitrary mod names do not select water, glass or a blend equation. Transparent
detail placements join the sorted forward queue. Host tests verify selected
parsing and pipeline contracts, while visual color, lighting, overlap and
performance still require game assets and a target Android device.
