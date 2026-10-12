# Vulkan item 102: material and level format matrix

The renderer resolves the serialized blender class and scoped properties in
`shaders.xr`. Names select records and textures; they do not select rendering
behavior. Missing roles, unknown versions or invalid contexts produce errors
with blender ID, class, version and material name.

| Blender class / state | Texture slots | Geometry / pass | Mode |
| --- | --- | --- | --- |
| `LM`, `V`, `D_STILL`, `D_TREE`, `MODEL` and AREF variants | Base, optional level lightmap | Level or OGF base | Opaque, cutout or transparent from serialized properties |
| `BmmDold`, `LmBmmD` four channel | Base, lightmap, mask, R2-R/G/B/A | Level terrain | Opaque |
| `BmmDold`, `BmmD`, `LmBmmD` single detail | Base, lightmap, `Detail map / Name` | Level detail with THM scale | Opaque |
| `LmEbB` | Base, lightmap, `Environment map / Name` cubemap | Level reflection | Opaque or blended |
| `MODELEbB` | Base, cubemap when blended | OGF static or skinned reflection; opaque deferred base | Opaque or blended |
| `LaEmB` | Base, lightmap, 2D environment, optional named constant | Level lightmap plus environment, then multiply 2x base | Opaque |
| `S_SET`, `PARTICLE` | Base, context-dependent | Scene, screen, HUD or particle blend selection | Serialized blend token and supported flags |

The environment sampler validates DDS cube versus 2D layout. `LaEmB` reads
four constant waveforms from the archive and evaluates them each frame. Its
second pass multiplies G-buffer color and leaves normal, alpha and depth
untouched. `LmEbB` uses the level lightmap UV, and transparent `MODELEbB`
selects the matching static or skinned shader pair.

| Level profile | Declaration / vertex data | Indices and optional fast path | Verification |
| --- | --- | --- | --- |
| SoC | Float and packed static layouts | 16-bit indices, OGF variants and fast buffers | Synthetic v14 fixtures |
| CS | Float and packed static layouts | 16-bit indices, OGF variants and fast buffers | Synthetic v14 fixtures |
| CoP | Float and packed static layouts | 16-bit indices, OGF variants and fast buffers | Synthetic v14 fixtures |
| Mods | Serialized blender IDs and properties, arbitrary names, sparse chunks | Same validated level and OGF formats; unsupported revisions fail with context | Renamed material and malformed input fixtures |

Fast geometry fallback reports the profile, source and exact missing or
invalid vertex/index chunk. Material diagnostics report environment slot,
transform, constant and pass count. Host fixtures check parser routing, level
layout combinations and multiply pipeline state. Real SoC, CS, CoP and mod
archives and device rendering still need verification; synthetic fixtures do
not establish visual parity or acceptance of every custom archive.

**Known format boundary:** named animated `Environment map / Transform` values
are rejected with the property name. The Vulkan path supports the default
`$null` transform. Unknown property layouts, unsupported blend tokens and
missing lightmap UV also fail with the relevant context.
