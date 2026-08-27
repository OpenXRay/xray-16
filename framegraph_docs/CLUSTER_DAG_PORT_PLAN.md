# Cluster DAG Port Plan (2026-08-27, from-scratch extraction of OGSR HEAD 166dbb1d8)

Source of truth: `/Users/yohjimane/modding/OGSR-Engine/ogsr_engine/Layers/xrRenderVulkan`
(`vk_world_gpu.h/.cpp`, `vk_meshopt.cpp`, `rvk_loader.cpp`, `shaders/world_cull*.glsl`,
`cluster_meta.glsl`), extracted this session directly from source. Master sequence
(user 2026-08-27): **cluster DAG → VSM → tree wind + tree caster stack.**

## TODO (living checklist)

- [x] M1a: bake harness — range dedup over mega arrays, clod per-mesh path, input validation, threading (`3f56d1976`)
- [x] M1b: component merge + cross-unit seam pins + orphan attach pass (`6ecc75be7`)
- [x] M1c: bake hygiene (self-loop drop) + cut-complementarity check + parentError histogram + timing logs (in M1a/M1b)
- [x] M1d: disk cache — full-snapshot keyed by (params, geom stamp, range-set hash); v1 trades their per-record
      key-matching for whole-bake validity, partial reuse deferred (`8de71b795`)
- [x] M2a: entry table build at static-batch collection (world-space spheres, scale-aware errors, batch/material streams) (`eb8aa9ae4`)
- [x] M2b: cluster cull shader with direct-append cmd emission + per-draw streams (`cluster_cull.cs`)
- [x] M2c: draw integration (prepass + color cluster MDI; `r_cluster` live-toggles via `g_ClusterActive` back to the whole-mesh path) + `r_cluster_debug 1` rainbow
- [x] M3: projErr DAG cut live (`r_cluster_lod`; `0.05` = forced-finest parity mode); health views `r_cluster_debug 2`=level `3`=parentError class `4`=fade viz
- [x] M4: crossfade band + Bayer screen-door in prepass AND color (shared `cluster_fade.h`) + Hi-Z mid-transition exemption (`64d483272`)
- [x] M5: cluster line in rs_stats (entries drawn/total) (`3fadd165d`); in-game measurements pending first playtest

Shipped deviations from §1-§5 (recorded 2026-08-27):
- Unclustered static batches KEEP the existing whole-mesh cull path (no `flags bit1` plain
  entries); SSA (`r_ssa_px`) is therefore inert until plain entries exist. The cull still
  implements the bit1 SSA branch for when they arrive.
- Fade words are fetched by the PS itself via the `drawID` interpolant (added TEXCOORD6 to
  the bindless VS output and its four paired PS inputs), not passed through the VS; the
  encoding is neutral-at-zero (`(63-fA)|fB<<6`) so the shared 1-element dummy buffer bound
  for non-cluster sets decodes as fully opaque, and fade bits 12-17 carry depth/errClass
  debug data.
- Forced-finest for the M2 parity gate is `r_cluster_lod 0.05` (the min clamp), not a flag.
- Cache is a full-bake snapshot keyed by (params, geom stamp, range-set hash) — a superset
  is NOT yet a hit; any eligibility change rebakes (threaded; per-record matching deferred).
- Component entry resolution: each member proto uses its own batch's transform/material; a
  member whose batch never appears falls back to the first present member's batch.
- Variant/blend (`PREPASS_SKIP`) statics never enter the entry table; they continue through
  the variant partition path untouched.

## 0. The system in one paragraph

At level load, eligible static meshes are split into ≤128-triangle clusters and built
into a Nanite-style LOD DAG with meshoptimizer's reference `clusterlod.h` (`clodBuild`):
each simplification level groups clusters, simplifies the group, and records the group's
bounding sphere + world-space error. A cluster carries two (sphere, error) pairs — the
group that PRODUCED it (`lodSelf/selfError`) and the group it is a MEMBER of
(`lodParent/parentError`). Per frame, one compute walks every entry: frustum → SSA (plain
meshes only) → the cut test `projErr(self) <= 1 < projErr(parent)` with
`projErr(s,e) = e * (pxScale/thresholdPx) / max(0.01, viewZ(center) - radius)` — view-Z,
not Euclidean — and appends one indirect command per surviving entry. Because a child's
`(lodParent, parentError)` is bitwise equal to its parent's `(lodSelf, selfError)`, both
sides of every boundary flip on the same float comparison: the cut is crack-free with no
communication. A band around the threshold draws parent and children simultaneously with
complementary 4×4-Bayer screen-door masks (fade values derived from the same floats, so
coverage sums to exactly 1). The color-phase cull adds the Hi-Z test, exempting
mid-transition entries. Terrain and dynamics stay whole-mesh.

## 1. Architecture mapping (their dependency → our provider)

| Their contract | Our provider | Delta |
|---|---|---|
| Level pool VBs read back from GPU (staged copy + guards) | mega VB/IB **CPU-resident at `EndLevelLoad`** (`RegisterVBPool` converts at load; `GetMeshAllocation` is arithmetic) | no readback at all; bake reads `UnifiedVertex` arrays directly |
| Per-visual `DrawnSlice` (finest SWI slice) | per-visual `GetMeshAllocation` ranges after `LoadVisuals` | dedupe by (vertexOffset, indexOffset, indexCount) — instanced mu-models cluster ONCE |
| World-baked geometry, one entry set | model-space bake → **per-batch entry instantiation** at static-batch collection: sphere/lodSelf/lodParent transformed by batch world, radii+errors × maxScale | scaling by one factor preserves bitwise complementarity within a batch; shader math identical to theirs |
| Per-material groups + per-group descriptor binds | bindless: 2 regions (static, terrain-whole-mesh); materialID per entry | their group loop collapses; direct-append replaces our stamp+compact for the cluster path |
| `firstInstance = id \| fA<<20 \| fB<<26` (20-bit ceiling, D3D12-hostile) | per-draw streams via the DRAWINDEX instance-rate pattern: cull writes `outBatchIndex[slot]`, `outMaterialID[slot]`, `outFade[slot]`, `startInstanceLocation = slot` | no bit budget, no `SV_InstanceID` hazard; forward VS keeps its existing reads + one fade fetch |
| `u16` cluster indices relative to `vBase` (+ u32 component pools) | all cluster indices **u32 appended to the mega IB** at bake, stored mesh-local in the cache, rebased by `alloc.vertexOffset` at append | one index path; draw args same shape as today |
| Their attribute encoding: normal bytes/255 @12, raw SHORT2 UVs | UnifiedVertex: D3DCOLOR normal @12 (same /255), float2 UV0 @24, float2 UV1 @32 | same 7-float layout, 3 weighted (normal), 4 protect-only (both UV sets) |
| `world_cull` (frustum set) / `world_cull_hzb` (Hi-Z set) | our phase A (prepass cull) / phase B (color cull) — the two-phase structure already matches | replace the static set's per-batch test with the entry cut; A emits prepass cmds, B re-emits with Hi-Z |
| Hi-Z test w/ focal scale | our `cull_utils.h` HiZTestSphereEx (already the ported OGSR test) | none |
| `vkCmdDrawIndexedIndirectCount` per group region | our `drawIndexedIndirectCount` w/ count buffer (existing) | none |
| Pool compaction + AdoptFrom | our mega arrays: free HOST copies after upload (bake done); GPU dedup later | different mechanism, simpler |

## 2. Not ported v1 (with reasons)

- **Stage-B page streaming** (`ClusterStream`, residency bits, requests, touched-LRU,
  pinned roots): even their eager mode zeroes it out cleanly ("hard-wire sb=1, drop
  bindings 6/7; nothing else changes"). v1 is eager: cluster index data resident
  (~+40-60% mega IB). Streaming is its own later phase; the cull keeps the seam (a
  constant `drawable=1` in place of the bits fetch).
- **Shadow cull targets** (`world_cull_shadow`, 5-target regions): no consumers until
  VSM; ports as VSM's cluster bin (ortho constant-error cut) in the VSM project.
- **Compose clone re-cull** machinery: no compose feature here.
- **Pool compaction**: replaced by host-copy free; GPU-side geometry dedup can come later.
- **Tessellated-material routing** (`r_tess` interplay): we have no tess; all bump
  materials are clusterable.
- **SWI finest-slice extraction**: our allocations already reference the drawn slice.

## 3. Bake pipeline (ours; constants theirs unless noted)

Hook: end of level load, after `LoadVisuals`, inside `EndLevelLoad` before mega upload.

1. **Collect ranges**: walk static-eligible visuals (MT_NORMAL/MT_PROGRESSIVE + trees'
   MT_TREE_* ride the static set here) → `GetMeshAllocation`; dedupe by range key.
   Eligibility: valid allocation, not terrain material, not water/wallmark/blend-variant
   (bindless MaterialSystem flags), `indexCount >= 3`. Size gate: merge candidates
   (solid) any size; AT and merge-off solids need `>= 3*minTris`,
   `minTris = max(r_cluster_tris, 128)`.
2. **Component merge** (`r_cluster_merge`, M1b): union-find over dilated AABB overlap
   (`kDilate 0.5`), bucketed by (stride-class — ours is uniform — so bucket by nothing;
   AT excluded); recursive split while `tris > 200000 || extent > 48 m` (gap-preferred
   cut, `bestGap` seed 0.75, else tri median); orphan attach pass (host = least extent
   growth, solid-only). Their measured reasons: unbounded components froze the cut
   (parent spheres avg 187 m / max 1197 m); unattached orphans never dissolve (~1500
   frames stuck at parentError INF).
3. **Seam pins**: FNV-1a64 over 12 position bytes; only component↔component positions
   pin (`kShared`); component protect = lmap-UV discontinuity vs position-canonical
   vertex WITHIN the same mesh only. Their frozen-DAG numbers: pinning vs AT/plain froze
   16% of verts; diffuse-UV protects froze 40%.
4. **clodBuild config** (verbatim): `clodDefaultConfig(128)`, `optimize_bounds=true`,
   `simplify_fallback_sloppy=false` (under-reports error on low-poly), 
   `simplify_prune=true` (what dissolves window frames — disconnected shells),
   `attribute_weights={0.5,0.5,0.5}`, `attribute_count=3` over a 7-float array,
   per-mesh `attribute_protect_mask=0x78` (both UV sets), component protect self-computed.
   Validate every clod input first (their heap-corruption guard: meshopt release builds
   compile out bounds checks): counts nonzero, index_count%3==0, arrays sized, all
   indices < vertex_count.
5. **Meta emission** (proto, model-space): sphere = cluster bounds; `lodParent` =
   member-group sphere, `parentError = min(err, 1e30)`; `refined >= 0` →
   `lodSelf/selfError` = producing group, else self bounds + 0. Component path bins
   triangles per source mesh (stable_sort) — one entry per (cluster × mesh) sharing the
   cluster's spheres. Plain (unclusterable) meshes are NOT baked — they stay whole-mesh
   batches with `flags bit1` (SSA-cullable) at entry build.
6. **Bake hygiene**: drop self-loop entries (`lodSelf==lodParent && selfError==parentError`
   bitwise, selfError>0, parent finite) unless a mesh would lose all entries.
7. **Diagnostics** (M1c, always on): parentError histogram (INF / >100 / 10-100 / 1-10 /
   0.1-1 / <0.1), level0 count, cut-complementarity: sorted-vector FNV over
   (lodSelf,selfError) 5 raw words; count parent keys not found = holes (small + stable
   = pruned shells; a jump after a change = popping holes, bisect). Timing log per stage.
8. **Threading**: work-stealing atomic cursor, biggest units first, ≤ min(8, hw) threads.
9. **Disk cache** (M1d): magic `'VCLF'`, version 1, params hash (version, 128,
   r_cluster_tris, r_cluster_merge), geom stamp = size ^ crc32(first 64K of level.geom),
   key per range {vertexOffset, indexOffset, indexCount, vertexCount}, records
   key-matched (not positional), **superset tolerated as HIT** (their 27 s / 18 s rebake
   lessons), stdio not FS reader, `r_cluster_cache 0` = repro switch that must NOT save.
   Layout: header | records+entry lists | meta protos | u32 index blob.

## 4. Runtime data (ours)

- **Entry buffer** (built once per level at static-batch collection, world-space):
  their 80-byte Meta verbatim — sphere(16) lodSelf(16) lodParent(16) indexCount ibFirst
  firstVertex group selfError parentError flags reserved — with our semantics:
  `ibFirst/firstVertex` absolute mega offsets (eager: no pageSlotBase add), `group` ∈
  {0 static, 1 terrain}, `flags bit0` = AT hard-cut, `bit1` = plain whole mesh,
  `reserved` = batchIndex (their `_p1` page id slot — we have no pages; batchIndex needs
  its own word and this is it). Entry count guard: warn at > 4M (no packing ceiling).
- **Output buffers per phase-consumable set**: cmds (20 B × entries, exact prefix
  regions per group), counts (u32 × groups), and the three per-draw streams
  (batchIndex, materialID, fadeWord) sized like cmds. Phase A writes the prepass set;
  phase B the color set — reusing the same buffers sequentially exactly like our
  current two-phase scheme (WAR/WAW edges already ordered it).
- **projErr inputs**: `lodParams = {pxScale/max(0.05, r_cluster_lod), 0.01, fadeBand, unused}`
  with `pxScale = height / (2 tan(fovY/2))`; `ssaCull = 2*pxScale / r_ssa_px`.
- **Cvars** (their names/defaults): `r_cluster 1`, `r_cluster_tris 256`,
  `r_cluster_lod 4.0` (live), `r_cluster_fade 0.25` (live), `r_cluster_merge 1`,
  `r_cluster_cache 1`, `r_cluster_debug 0..4` (live), `r_ssa_px 2.0` (live).

## 5. The cull (per phase; deltas from theirs marked)

```
entry e:
  frustum: 6 planes, reject if dist < -r          (same extraction both phases — bit-identical)
  SSA (flags bit1 only): reject if r*ssaCull < max(0.01, viewZ - r)
  [streaming residency: eager v1 — skipped]
  sp = projErr(lodSelf, selfError); pp = projErr(lodParent, parentError)
  hard cut (band<=0 or flags bit0): draw iff pp > 1 && sp <= 1
  banded: reject if pp <= 1 or sp > 1+band
          fA = round(clamp((pp-1)/band,0,1)*63); fB = round(clamp((sp-1)/band,0,1)*63)
          reject if fA == 0
  [phase B only] if fA==63 && fB==0 && HiZTestSphere(...) reject     ← mid-transition EXEMPT
  slot = atomicAdd(counts[group]); base = groupBase[group]
  cmds[base+slot] = {indexCount, 1, ibFirst, firstVertex, base+slot}  ← startInstance = slot (DRAWINDEX)
  outBatch[base+slot] = batchIndex; outMat[base+slot] = materialID    ← OUR streams, not bit-packing
  outFade[base+slot]  = fA | (fB<<6)
```

Fragment side (prepass AT PS + forward PS + terrain n/a), shared include:
`d = (kBayer4x4[(y&3)*4+(x&3)] + 0.5)/16; if (fA<63 || fB>0) { if (d >= fA/63 || d < fB/63) discard; }`
Same table + decode in prepass and color — with our EQUAL color scheme a mismatch is
instantly visible (holes), which is a tripwire, not a risk.

## 6. Milestones with visible gates

**M1 — bake (log-verified, zero rendering change).** Gate: bake report on a real level —
units/clusters/entries, level histogram populated beyond level0, holes small and stable
across two identical bakes, complementarity clean; second load hits the cache with
identical counts and ~0 bake ms. `r_cluster 0` skips everything.

**M2 — entries drawn at forced-finest (visual parity).** Gate: `r_cluster 1` vs `0`
pixel-identical scenes (the finest cut ≡ original geometry by construction);
`r_cluster_debug 1` rainbow (PCG hash of entryId) paints the world in cluster-sized
confetti; stats overlay shows entry/draw counts and the region-overflow counter at 0.

**M3 — live LOD.** Gate: sliding `r_cluster_lod` 0.5→16 in console visibly coarsens
distant facades and drops draw counts in the overlay; strafing shows no popping holes
(complementarity); `r_cluster_debug 4` health view shows green/yellow dominant, red
(never-coarsens) only on legitimately plain meshes.

**M4 — crossfade.** Gate: `r_cluster_fade 0 ↔ 0.25` A/B turns LOD pops into short
dissolves; no black/flickering facades near transitions with Hi-Z on (exemption
working); no holes at transitions (prepass/color Bayer parity — EQUAL scheme enforces).

**M5 — measurements.** Draw-count ×N reduction recorded (theirs: 11118 → 3704 with
Hi-Z on Pripyat), bake time per level, cache hit time, mega IB growth, host memory
freed after upload.

## 7. Hazard checklist (their recorded bugs that apply here)

1. Validate clod inputs or risk heap corruption (release meshopt has no bounds checks).
2. Complementarity is bitwise: any transform applied to spheres/errors at entry build
   must be the SAME operation on both pair members (one scale factor per batch).
3. Prepass/color cut must agree exactly: one shader file, two entry points or a
   permutation whose frustum/cut code is verifiably identical; same plane array.
4. Mid-transition entries exempt from Hi-Z or facades flicker black.
5. Fade gate: never widen the cut unless BOTH draw sides can dither.
6. Bayer table + decode identical in prepass and color (shared include; include-hash
   cache fix makes this safe to iterate).
7. SSA never applies to clusters — only plain whole meshes (bit1).
8. Cache: superset = hit; key-matched not positional; repro switch must not save.
9. Self-loop entries dropped at bake (they can never produce pixels; they starve DAGs).
10. Region overflow structurally impossible (exact prefix sums) — still audit-count it.
11. Progressive meshes: cluster the finest slice only (coarse slices poke through).
12. Terrain never clusters (separate material path; spheres would lie under displacement).
13. Free the bake's host memory before spawn (their 3.6 MB leaf list lesson, scaled up).
