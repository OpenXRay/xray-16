# Vulkan item 102: implementation boundary

The parsed `shaders.xr` record is not proof that its gameplay pipeline is
implemented. `ShaderMaterialLibrary::find` exposes the serialized properties;
`resolve` admits a record only when the requested context can use its pipeline.
The material name is a lookup key, never evidence that a texture role exists.

| Variant | Vulkan handling | Remaining work |
| --- | --- | --- |
| `BmmDold` / `LmBmmD` four channel terrain | R2-R/G/B/A, mask and terrain detail scale use the terrain descriptor and fragment variant | Confirm bump, gloss, normal and surface parameters against the original R2 shaders and on a device. |
| `BmmDold` / `LmBmmD` / `BmmD` single detail map | Parsed `Detail map / Name` selects a distinct level shader and descriptor, with THM UV scale | Compare each class against its original R1/R2 calculation; support its transform and model path, and validate on device. |
| `LmEbB` / `MODELEbB` | Parsed, explicitly rejected for the missing environment texture and pass | Implement environment descriptor, blend and pass ordering. |
| `LaEmB` | Parsed, explicitly rejected for missing environment texture, constant and multiply pass | Implement both constant and no-constant variants. |
| `S_SET` / `PARTICLE` | Selected blending modes and contexts are checked by `resolve` | Complete the context and flag matrix with pipeline-state tests. |

The host material fixture checks arbitrary names, sparse IDs, serialized
property retention, accepted four channel terrain, single detail selection,
and precise refusal of the environment variants. These checks do not establish image
correctness, device performance or acceptance on SoC, CS and CoP data.

The plan's items 1–11 remain open as a whole. In particular, other texture
roles and multipass classes, complete water and glass behavior,
format combinations, complete interface audit, unified resource diagnostics
and device evidence still require implementation and verification. Do not
mark item 102 complete based on this host fixture or the existing APK.
