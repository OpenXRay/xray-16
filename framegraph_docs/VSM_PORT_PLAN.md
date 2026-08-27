# VSM Port Plan (2026-08-26, from-scratch extraction of OGSR HEAD 166dbb1d8)

Source of truth: `/Users/yohjimane/modding/OGSR-Engine/ogsr_engine/Layers/xrRenderVulkan`
(`vk_vsm.h/.cpp`, `vk_pass_world.cpp`, `vk_env_light.*`, `vk_console_min.cpp`,
`shaders/vsm_*.glsl`), extracted this session directly from source. No prior docs or
branches were consulted. Port target: our NVRHI framegraph renderer on
`yohji/feat/framegraph`, whose substrate is now proven: same-frame depth prepass
(statics+dynamics+terrain, AT-aware, EQUAL color scheme), same-frame Hi-Z, bindless MDI
culling, reverse-Z engine-wide.

## 0. The system in one paragraph

A 6-level clipmap of virtual shadow pages (4096² virtual texels/level, 128² per page,
32×32 pages/level, 6144 virtual pages) over a sun view built with the eye at the world
origin so light-space XY is world-anchored. A screen-space MARK compute walks prepass
depth and flags which pages this frame's visible pixels need. A residency compute maps
needed pages to physical slots: the STATIC atlas (8192×12288 D16, 6144 slots, fully
resident) uses a toroidal mapping `slot = L*1024 + (absY&31)*32 + (absX&31)` — modulo is
the eviction, no allocator, no atomics on the table — and re-renders only dirty pages
(wrong-tile / round-robin while the sun moves / gaze / forced). The DYNAMIC atlas
(8192×4096 D16, 2048 slots) is bump-allocated and fully re-rendered every update frame
for moving casters (skinned). Bin computes expand casters into (caster, page) draws: one
indirect command per caster with instanceCount = overlapped dirty pages; the page VS
routes each instance into its page's atlas cell and clips to the page rect with 4 clip
distances. A resolve compute converts everything to a screen-space sun mask (RGBA16F: R
lit factor, G camera distance, B static-only, A dyn hit) with 3×3 PCF or stochastic
PCSS, temporal EMA, and a no-page history carry. Receivers pay one texture load.

## 1. Architecture mapping (their dependency → our provider)

| Their contract | Our provider | Delta |
|---|---|---|
| ShadowGPU caster meta (world-baked spheres + draw ranges + group) for the FULL static set (never camera-culled) | `GPUCullingManager` raw sets: `m_staticSet.objectBuffer` (`GPUObjectData{pos,radius,batchIndex,flags}`), `drawArgsBuffer` templates, `materialIDBuffer`, `instanceBuffer`; terrain equivalents | add public accessors for the RAW buffers + counts; bin reads the raw set, not the compacted one |
| One VB/IB per ShadowGPU group | mega VB (UnifiedVertex 48 B) + mega IB (u32); terrain shares them | their per-group loop collapses to 2 regions: static, terrain |
| World-baked vertex positions (identity transforms) | model-space mega VB + per-batch `g_InstanceData[batch].world` | page VS applies the batch world matrix; batchIndex packed into the page stream (see §4) |
| Per-material AT descriptor sets + push aref | bindless: `g_Materials[materialID]` + `clip(a - alphaRef)` | one AT-aware page PSO replaces their material rebind machinery; also upgrades their per-mesh path, which drew AT casters solid |
| `Skinned_CollectCasters` (leaf spheres, bone base) + bone SSBO | `UploadSkinnedObjects` buckets: spheres + `SkinnedDrawRecord{world,boneOffset}` + `g_BoneMatrices` | skinned page VS = our `skinned_common.h` prologue + route epilogue; collect adds `r_vsm_npc_dist` cull |
| `gl_InstanceIndex` includes firstInstance (Vulkan) | D3D12 `SV_InstanceID` is zero-based → use an instance-rate vertex stream (our proven DRAWINDEX pattern): page stream bound at slot 1, `startInstanceLocation` = region base | semantics identical on both APIs by IA spec |
| Hand-placed Vulkan barriers (37 recorded hazards, incl. 3 WAW sagas) | NVRHI auto state tracking + framegraph RAW/WAR/WAW edges | their gotchas #3/#4/#5/#6/#7/#31 become non-events; never copy-into-live-indirect regardless |
| `vkCmdSetDepthBias` dynamic state (their clobber saga, gotcha #2) | NVRHI PSO-baked depth bias | bias is static per PSO → the clobber class cannot exist; cvar change ⇒ PSO rebuild + `InvalidateCache()` |
| EnvLight set bindings 14/15/16/23/24/25 | reflection-driven binding sets per pass; receivers need ONLY the mask (`Load`, no sampler); grass-direct (later) gets atlases/tables in the detail pass | 1×1 white fallback until `MaskReady()` |
| `Prof` zones World/VSMmark/render/resolve | framegraph per-pass GPU timers; throttle reads `GPUProfiler` timing of the atlas pass | keep their zone granularity as pass names |
| `invert_44(viewProj)` (their costliest bug: affine-only `Fmatrix::invert`) | `Device.mInvFullTransform` — already consumed successfully by our MV/ReSTIR world reconstruction | none |
| sun dir/color | `CurrentEnv->sun_dir/sun_color` via existing `GetSunLightData()` path | add thunderbolt hold (last non-bolt dir) |
| Cascade fallback + HUD-on-cascade | none — VSM is the only sun-shadow path; `r_vsm 0` = no sun shadows (current state); HUD PS (`bindless_skinned_hud.ps`) skips the mask tap | their `vsmActive` −2.7 ms lesson inverts: we never pay a cascade |

## 2. Not ported, with reasons (absent inputs, not redesigns)

- **Entire tree caster stack** (`r_vsm_tree_*`, wind hybrid, invalidation circles, near-set,
  meshlets, hulls, voxel bricks, impostors, tree VRS): predicated on animated trees; ours are
  rigid members of the static set. The residency shader keeps the 4-circle input (zeroed).
- **Cluster bin** (`vsm_bin_cluster`, candidates, combos): no cluster DAG here; their
  per-mesh path is their own shipped fallback and is what we port — upgraded with bindless AT.
- **Receiver mask (`r_vsm_rmask`)**: its only consumers are the tree/voxel bins we don't
  port. Omit the buffer and the mark work entirely.
- **Shadow-HZB (`r_vsm_hzb`)**: default OFF for them, "measured no net gain". Skip.
- **`r_vsm_ta_blend_dlss`**: no DLSS.
- **Fog sampling of the atlas**: no volumetrics yet; the atlas accessors should exist for it.

Revisit triggers (the conditions that flip each cut, decided 2026-08-27):

| Cut feature | Adopt when | Shape of the adoption |
|---|---|---|
| Tree wind hybrid + invalidation circles + near-set | tree wind animation is restored — MUST land in the same milestone as visual wind (wind in color without it = self-shadow banding + frozen shadows, their documented failure) | tree-dyn bin beside the skinned bin; shared wind include between color VS and page VS; the residency 4-circle input is already kept warm |
| Tree LOD tiers (hulls, voxels, meshlets, impostors) | only after the hybrid exists AND profiling shows dyn crown fill cost — they default most of these off themselves | measured-need, one tier at a time |
| VSM cluster bin | the world cluster-LOD DAG ships (its own post-VSM project) | days of work; unlocks ortho-constant shadow LOD with zero cache invalidations |
| Receiver mask (rmask) | together with the tree hybrid, never alone — its only consumers are tree/voxel bins | mark-side cell bits + bin-side test; NEVER applied to static cached pages |

## 3. Resources (ours; sizes from their constants)

Persistent (created once, imported into the framegraph each frame):

| Resource | Format / size | Notes |
|---|---|---|
| static atlas | D16, 8192×12288 (192 MiB) | depth attachment + SRV; first frame CLEAR(0.0), then LOAD |
| dynamic atlas | D16, 8192×4096 (64 MiB) | CLEAR(0.0) every dyn-update frame |
| mask ×2 | RGBA16F, screen | ping-pong like `m_normals`; UAV (resolve) + SRV Load (receivers) |
| pageTable / dynPageTable | u32 ×6144 | static: rewritten by resid; dyn: filled UNMAPPED then alloc |
| pageList / dynPageList | uint4 ×6144 / ×2048 | slot → (level, windowPage) |
| physTile | uint2 ×6144 | THE cache state; filled 0xFFFFFFFF on init/invalidate only |
| slotDirty, dirtyList(+2 tail), drawClear(16 B), needed, pageHits, counters, readbacks | u32 arrays per spec | dirtyList tail dwords = wrong/gaze counters (never inside the live indirect) |
| casterPages (page stream) | u32 × kMaxCasters×kPagesCap | entry = `(batchIndex<<13) | slot` (13 bits: 6144 slots; batch ≤ 512k↛ — see §4 packing note) |
| vsmIndirect + groupCount | 20 B × 2 regions × kGroupStride; u32×2 | regions: static, terrain |
| skinMeta / skinCasterPages / skinIndirect / dynUsed / dynAllocInfo | per spec (`kSkinnedCap=kMaxSkinned=256`) | |
| VsmParams CB 176 B + ResolveParams CB 240 B | volatile CBs | layouts byte-for-byte from the spec |

Retuned constants (config, not logic): `kMaxCasters` = our static registry ceiling
(start 16384, clamp+warn like theirs); `kGroupStride` = kMaxCasters (2 regions only);
everything else verbatim: kLevels 6, kVirtualRes 4096, kPageSize 128, kMaxPhys 2048,
kMaxPhysS 6144, kPagesCap 1024, kZNear/kZFar ∓1000, kZSnap 256, kPrimeFrames 8,
kRejectTol 0.05, kSkinnedCap 256.

**Packing note:** their casterPages entry is a bare slot. Ours must also carry batchIndex
for the world matrix + material fetch. 32 bits = slot(13) + batch(19) → batch cap 524 288 ✓
(registry cap 65 536). Page VS: `slot = e & 0x1FFF; batch = e >> 13`.

## 4. Framegraph passes (placement on our frame)

```
GPU Culling A (async) → Depth Prepass → Hi-Z Build → GPU Culling B
                                  ↘ VSM Prepare (compute)      [reads rt_Depth]
                                     VSM Atlas (graphics)      [reads bins, writes atlases]
                                     VSM Resolve (compute)     [reads atlases+rt_Depth+prevMask, writes curMask]
        → Sky → Sun → ClusterAssign → Forward+ Color [reads curMask] → …
```

- **VSM Prepare**: frame fills (their disjoint-fill discipline kept for clarity), MARK
  dispatch (8×8 groups over prepass depth at `markStep`), residency (96 groups), dyn alloc
  (96 groups, non-skip frames), static bin, terrain bin, skinned bin. One pass; internal
  ordering via NVRHI auto barriers.
- **VSM Atlas**: static pass = indirect clear quad (vertexCount 6, instanceCount from
  resid; depth 0.0, compare Always) → static region MDI (`drawIndexedIndirectCount`,
  page-stream at slot 1) → terrain region MDI; dynamic pass (skip on cadence) = CLEAR(0.0)
  → per-leaf skinned draws (`drawIndexedIndirect`, one cmd per leaf). Page PSOs: same VS
  position path discipline as color (`precise` already engine-wide), depth Greater,
  write on, **negative PSO-baked raster bias**, cull None, 4 clip distances.
- **VSM Resolve**: 8×8 over screen; writes mask[cur], reads mask[prev]; rotates with our
  existing ping-pong index. Both filter paths ported at once (PCF and PCSS are branches on
  `params3.x`); temporal EMA + neighborhood clamp + motion fade + no-page carry included
  from day one (the sun direction interpolates every frame in this engine, so round-robin
  refresh boundaries shimmer without the EMA — not deferrable).
- **BeginFrame** (CPU, before pass setup): sun view (eye at origin, up flip at |sd.y|>0.99),
  thunderbolt hold, per-level window with page-lattice snap (NO jitter, ever — their code
  comments claiming jitter are stale; the lattice snap is what keeps the cache valid),
  zCentre snap to 256 m + `InvalidateCache()` on change, sunMoving flag, night/load
  freezes, cadence gates, throttle controller, prime countdown.

## 5. Reverse-Z flip table (complete; everything else ports byte-for-byte)

| Site | Theirs (forward) | Ours (reverse) |
|---|---|---|
| page VS depth out | `nz=(lp.z-zp.x)*zp.y` (0 near) | `1 - nz` (1 near) |
| atlas clear / clear-quad z | 1.0 | 0.0 |
| page PSO depth func | LessEqual | **Greater** (explicit; NVRHI default is a forward-Z trap) |
| atlas sampler border | opaque white (=lit) | opaque **black** (=lit at 0.0) — we use `Load` + table checks instead where possible |
| receiver compare | `zHere - bias > occ` ⇒ shadowed | `zHere + bias < occ` ⇒ shadowed |
| static/dyn merge | logical OR of the two compares (unchanged structure) | same |
| PCSS blocker accept | `d < zS - bias` | `d > zS + bias` |
| blocker nearest merge | `min`, sentinel 2.0, accept <1.5 | `max`, sentinel −1.0, accept >−0.5 |
| blocker distance | `(zS − avg)/zScale` | `(avg − zS)/zScale` |
| mark/resolve sky reject | `zndc >= 0.99999` | `zndc <= 0.0` **exactly** (sky clears to 0.0; an epsilon eats far geometry) |
| raster (write-side) bias | positive | **negative**, PSO-baked |
| degenerate-instance cull `gl_Position=(2,2,2,1)` | outside [0,1] | still outside — unchanged |
| mask sky sentinel `(1, 1e6, 1, 0)` | unchanged | unchanged |
| dyn debug presence `d < 0.999` | | `d > 0.001` |

## 6. Shader manifest (theirs → ours, HLSL sm_6_6)

| Theirs | Ours | Notes |
|---|---|---|
| vsm_common.glsl | `shared/vsm_common.h` | pageIndex/toroidalSlot/select + VSM_PAGE_RANGE as a **macro** (their SPIR-V-parity rationale holds for us as DXIL/SPIRV parity across 4 bins) |
| vsm_params.glsl | fields inside a `VsmParams` cbuffer header | one declaration, included everywhere (our include-hashing cache fix makes shared headers safe) |
| vsm_mark.comp | vsm_mark.cs | subgroup ops → Wave intrinsics (`WaveReadLaneFirst`, `WaveIsFirstLane`, `WaveActiveBallot`+`CountBits`); rmask block omitted |
| vsm_resid.comp | vsm_resid.cs | 128 B push → cbuffer (no push limit pressure in NVRHI, keep layout anyway) |
| vsm_alloc.comp | vsm_alloc.cs | verbatim |
| vsm_bin.comp | vsm_bin.cs | + batch packing; group regions = {static, terrain}; camera + lodDist kept |
| vsm_skinned_bin.comp | vsm_skinned_bin.cs | verbatim + dynUsed |
| vsm_page_route.glsl | route block in `vsm_page_common.h` | clip via `float4 : SV_ClipDistance`; NDC y negated to match compute-side atlas addressing (their BeginPlain-vs-flipped-viewport distinction lands on us as: our raster y-flip must be cancelled so `(slot + pageLocal)/dim` sampling in resolve matches) |
| vsm_page.vert / _at.vert/.frag | vsm_page.vs + vsm_page.ps (AT-aware bindless) | one PSO family; PS clips only when MAT_FLAG_ALPHA_TEST |
| vsm_skinned_page.vert | vsm_skinned_page_{2w,3w,4w,hq}.vs | skinning prologue from `skinned_common.h` — must stay bit-identical with the color path |
| vsm_clear.vert | vsm_clear.vs | depth 0.0 |
| vsm_resolve.comp | vsm_resolve.cs | both filter paths + temporal, full port |
| vsm_sample.glsl | one `Load` line in `common_functions.h` (`sunLight *= mask`) gated by a `vsm_ready` constant; grass-direct variant deferred to M4 | receiver cost: one Load |

## 7. Milestones with in-game gates

**M0 (hours).** Enable `shaderClipDistance` in `VulkanBackend` (CLAMP pattern; D3D12
needs nothing). Clip-distance smoke test on both backends. Register the full cvar block
(names + defaults verbatim, minus tree/cluster/rmask/hzb/dlss families; bias signs per §5).

**M1 — clipmap + MARK (days).** `vsm_common.h` (unit-test the pure math CPU-side:
toroidal bijection, select nesting, page range vs a brute-force reference), `VSMSystem`
BeginFrame (full §4 CPU block), resources, MARK pass + counter readback + a page-grid
debug overlay mode. Gate: uniquePages plausible (hundreds, not 6144); page grid glued to
the world while strafing; all-L0 histogram = inverse bug.

**M2 — static cache + atlas + resolve + receivers (1–2 weeks).** Residency (full dirty
law incl. budget + prime + forceDirty A/B), clear quad, static+terrain bins, atlas pass,
resolve (both filters + EMA + carry), receiver tap in `output_forward_pbr`, night/load
freezes, `InvalidateCache` on level unload + zCentre + bias-cvar change. Gate (their
measured economics): standing still with static sun ⇒ dirty ≈ 1–2/6144 and atlas pass
≈ 0 ms; `r_vsm_cache 0` reproduces render-all cost; camera sprint ⇒ wrong-tile burst then
settle; `r_vsm 0/1` A/B; no page seams (half-texel inset); shadows don't crawl under sun
rotation (lattice snap); night ⇒ VSM passes absent from the profiler.

**M3 — dynamic atlas + skinned (1 week).** Alloc, skinned collect (+`r_vsm_npc_dist`),
skinned bin + page VS family, dyn cadence + all three alignment gates (motion, window
snap, sun drift — shipping cadence without the gates = their measured one-frame shadow
dropouts), split receiver biases (their shared-bias bug deleted every below-knee shadow),
dynUsed gate, `r_vsm_debug_dyn` overlay. Gate: NPC shadows track animation; frozen dyn
atlas doesn't misalign on weather sun-steps.

**M4 — perf war + grass (as measured).** In their order, each A/B'd: mark-half (default
on), throttle controller, gaze refresh, dirty-budget tuning; then grass casting (bin over
FGDetailManager's instance pool with wind parity) + grass direct receivers (mask is
wrong-by-construction for grass — it describes the surface behind the blade).

## 8. Bring-up hazard checklist (their recorded bugs that apply to us)

1. Full 4×4 inverse for world reconstruction (use `m_InvVP`; level histogram catches it).
2. zCentre slab: without the 256 m snap+invalidate, distant-camera Z leaves the ±1000 m
   window → all shadows vanish the moment r_vsm turns on.
3. Sun-below-horizon guard must not freeze before the first atlas/mask exists.
4. Thunderbolt sun-step: hold last non-bolt direction or ~500 pages go wrong-tile per bolt.
5. Prime must re-arm on the load-screen falling edge (it burns down behind the loader).
6. Counters never live inside a live indirect command (dirtyList tail dwords).
7. Page-range macro shared verbatim by every bin, or shadows end at page-grid lines.
8. Static cached pages: no partial-page tricks ever (stale when camera samples other cells).
9. Split static/dyn receiver biases; dyn is an epsilon (casters-only atlas can't acne).
10. Soft-angle default 0.2° (their 2.0° default made shadows invisible: 7.5× the real sun).
11. Y orientation: atlas is addressed directly by compute — the page/clear VS must negate
    NDC y relative to our flipped raster convention; wrong sign = coherent shadows in
    wrong atlas rows (L0 reads landing on L5).
12. Register collisions are silent in Slang reflection — verify the full reflected map of
    every opted PS when choosing the mask slot (t17 currently free).
13. Shader include edits: covered by our include-hashing cache fix (keep it).
14. Skinned page skinning must be bit-identical to the color path (shared include).
15. Receivers walk to the finest MAPPED level; MARK marks one level per pixel, so the
    walk is not a general fallback — the no-page carry is (default 0.98).

## 9. Per-milestone verification deliverables (contractual, written before the code)

| Milestone | Visible deliverable | Instrument | Pass / fail, stated in advance |
|---|---|---|---|
| M0 | Animated square window cut from a fullscreen triangle by 4 clip planes | debug pass behind `r_vsm_debug 9` | crisp square on VK **and** Metal = pass; whole triangle = feature dead; nothing = Slang rejected `SV_ClipDistance` |
| M1a | `vsm_selftest` console command: toroidal bijection, select nesting, PAGE_RANGE vs brute force on CPU | log PASS/FAIL per property | any FAIL blocks GPU work |
| M1b | Level-tint + page-grid overlay on the scene | `r_vsm_debug 1` + uniquePages/per-level counters in StatsOverlay | grid world-glued while strafing; level rings at 12/24/48 m; sky untinted; uniquePages in the hundreds (6144-pegged or single-color = inverse/select bug) |
| M2a | Static atlas live view | inspector `rt_VSM_AtlasS`; dirty/wrong/deferred counters | tiles fill in while moving; image FREEZES standing still; `r_vsm_cache 0` churns everything but changes no content while stationary (ground-truth comparator) |
| M2b | Screen mask view | inspector `rt_VSM_Mask` (R channel) | recognizable scene shadows before receivers exist; misplaced-but-coherent shadows here = atlas Y-orientation bug, caught pre-receiver |
| M2c | Shadows in scene | `r_vsm 0/1` live A/B; GPU timings | dirty ≈ 1–2/6144 still + atlas ≈ 0 ms; sprint burst then settle; no page-grid seams; no crawl under time_factor; night ⇒ VSM passes absent from timings |
| M3 | Dyn atlas view + red dyn overlay | inspector `rt_VSM_AtlasD`; `r_vsm_debug_dyn 1/2/3`; dyn alloc counters | NPC pages re-render per frame while static pages sit frozen; shadow tracks animation; `r_vsm_cadence 3` + strafe ⇒ no one-frame dropouts; NPC feet still cast (split-bias check) |
| M4 | Self-demonstrating knobs | overlay ms + lodBias readout; `time_factor`; `r_vsm_grass 0/1` | `throttle_budget 0.5` ⇒ bias climbs, shadows coarsen, ms falls; gaze kills accelerated-sun vibration; grass shadows sway with wind parity |

Cross-cutting rules: (1) criteria in this table precede implementation; (2) every layer
has a console off-switch so regressions bisect from the console (`r_vsm`, `r_vsm_cache`,
`r_vsm_temporal`, `r_vsm_soft`, `r_vsm_cadence`, debug modes); (3) gate-feeding counters
stay ungated — their lesson: gated telemetry "prints zeros that mean 'not measured', not
'nothing wrong'"; (4) inside each milestone, the visible artifact is built FIRST
(inspector registration ships the same day a resource is created).
