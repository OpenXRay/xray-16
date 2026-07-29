# FrameGraph Lighting & ReSTIR RT

English notes for the FoxConED lighting / ReSTIR / upscaling work on top of `yohji/feat/framegraph`.

## Scope

- Target PR: `OpenXRay/xray-16` base branch `yohji/feat/framegraph`
- Head: FoxConED fork branch carrying this stack
- Out of scope for the commit set: local `Externals/` dumps, `run/`, build logs, onnx tarballs, `tmp/`, `tools/`

## Historical FoxConED commits (already on the branch)

These landed earlier as larger sync commits. Content map for review (not rewritten in place, to avoid touching other authors' history):

| Commit theme | What it contains |
|---|---|
| FrameGraph lighting stack (CSM / local atlas / postFX) | Sun CSM + spot/omni local shadow atlas into forward+, sticky tile assignment, classic-style PCF; supporting FG passes (SSR/SSGI/AO/bloom/TAA/volumetrics/rain) |
| Sync lighting / RT / upscaling stack | ReSTIR DI/GI path, NRD wiring, DLSS/Streamline hooks, RT BLAS/TLAS integration, skinned RT paths, console knobs |
| Restore lighting after OpenXRay FG merge | Re-apply lighting stack after merging newer OpenXRay framegraph tip |
| Merge OpenXRay framegraph tip / dev mainline | Integration merges only |
| Small build / engine fixes | GCC16 `_Countof`, Thunderbolt ED duplicate, `session_name.c_str()`, ancestry note |

## Split follow-up commits (this PR tip)

| Commit | Intent |
|---|---|
| stabilize ReSTIR temporal on camera motion | Motion-vector history sample uses `currUV + mv` (`mv = prev - curr`); relaxed turn rejection gates |
| light CHAR mutants in ReSTIR like world geometry | `SURF_MARK_CHAR` skinned self-shadow soft-skip; VariantPSO remap to lit `bindless_skinned*` |
| improve RT water reflections lighting and gloss | Terrain/lmap albedo path, GGX glossy, lower roughness blur |
| fix DLAA with RT and tighten DLSS feature recreate | Force DLAA render scale 1; after feature recreate evaluate with `reset` instead of skipping the frame |
| add skinned velocity shaders and every-frame BLAS | Skinned velocity VS/PS set; per-frame skinned BLAS updates |
| harden particle occlusion and glow forward paths | Particle occlusion PS + glow forward shaders / pass wiring |
| extend ReSTIR GI stack and RT quality console knobs | GI pass setup, `r_rt_quality` presets, related constants |
| integrate FrameGraph lighting passes and shared shaders | Remaining FG pass / shared shader / culling / material glue |

## Runtime notes

- DLSS Frame Generation display FPS is not the same as the rendered/base FPS counter; FG doubles presented frames, RT cost still hits the base path.
- `r_rt_quality` is console-driven; it is intentionally not exposed in the options menu.
- CHAR/mutant darkness was caused by skinned hits zeroing shadow attenuation; self-skip now allows partial lighting on character surfaces.

## Suggested review order

1. ReSTIR temporal filter MV sign + turn gates
2. `rt_visibility.h` skinned self-max + VariantPSO skinned remap
3. `restir_water_rt.cs`
4. `UpscaleState` / `NgxDLSS` DLAA + feature recreate
5. Skinned velocity + BLAS update path
6. Broader FG / console integration
