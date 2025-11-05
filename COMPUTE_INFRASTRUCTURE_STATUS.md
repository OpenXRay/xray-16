# Detail Manager Compute Infrastructure - Implementation Status

## Completed Components

### 1. GPU Data Structures (`DetailManager_Compute.h`)
✅ **DetailInstanceGPU** (128 bytes, cache-aligned)
- Transform data: position, scale, rotation_y
- Rendering data: lighting (c_hemi, c_sun), object_id, vis_id, color_rgb
- Bounding data: AABB min/max, sphere radius
- Metadata: slot coordinates, flags, fade distance

✅ **FrustumGPU** (64 bytes)
- 6 frustum planes extracted from view-projection matrix

✅ **DetailCullParams** (64 bytes)
- Camera position/direction
- Fade limits (start/end squared distances)
- SSA thresholds for culling/LOD
- Frame number for temporal effects

✅ **IndirectDrawArgs**
- Matches D3D11_DRAW_INDEXED_ARGUMENTS layout
- `instance_count` written by compute shader

### 2. Compute Shader (`detail_cull.cs`)
✅ **Culling Pipeline** (256 threads per group)
1. Distance culling (fade_limit_sqr)
2. Frustum culling (sphere vs 6 planes)
3. SSA (Screen Space Area) culling
4. Sort into 3 visibility lists by vis_id (still/wave1/wave2)
5. Atomic counter updates
6. Output to UAV buffers

✅ **Functions:**
- `SphereInsidePlane()` - Plane/sphere intersection
- `FrustumCullSphere()` - 6-plane frustum test
- `ComputeSSA()` - Screen-space area for LOD

### 3. Manager Class (`DetailManager_Compute.h`)
✅ **DetailComputeManager**
- Instance management (Begin/Add/End)
- GPU buffer allocation (structured buffers, UAVs, SRVs)
- Compute dispatch (culling pass)
- Indirect rendering support
- Statistics tracking

✅ **GPU Resources:**
- Instance buffer (all instances) + SRV
- Visible indices buffers [3] + UAVs + SRVs
- Counter buffer (atomic) + UAV
- Indirect args buffers [3] + UAVs
- Constant buffer (cull params)
- Compute shader reference

✅ **Utility Functions:**
- `BuildFrustumGPU()` - Extract planes from VP matrix
- `ConvertToGPUInstance()` - CPU SlotItem → GPU format

## Next Steps

### Phase 1: Implementation (Current)
- [ ] Create `DetailManager_Compute.cpp` with buffer creation/destruction
- [ ] Implement `CreateBuffers()` - allocate structured buffers
- [ ] Implement `UploadInstances()` - CPU→GPU transfer
- [ ] Implement `DispatchCulling()` - bind resources & dispatch compute
- [ ] Implement `RenderIndirect()` - DrawIndexedInstancedIndirect calls
- [ ] Compile shader: `detail_cull.cs` → compiled bytecode

### Phase 2: Integration
- [ ] Add compute path toggle to `DetailManager.h`
- [ ] Integrate with existing `cache_Decompress()` to build instance list
- [ ] Replace `UpdateVisibleM()` with compute dispatch
- [ ] Hook into `hw_Render()` for indirect draws
- [ ] Add console commands for enable/disable/stats

### Phase 3: Testing & Optimization
- [ ] Verify visual parity with CPU path
- [ ] Profile GPU/CPU times
- [ ] Benchmark instance counts (10K, 100K, 500K)
- [ ] Tune thread group size (current: 256)
- [ ] Add GPU timestamps for perf tracking

## Architecture Benefits

### Memory Efficiency
- CPU: ~80 bytes per SlotItem (pointers, matrices, scattered data)
- GPU: 128 bytes per instance (cache-aligned, contiguous)
- Structured buffers avoid pre-multiplied VB/IB waste

### Performance Gains
- **Culling**: GPU parallel > CPU serial
- **Draw Calls**: 3 indirect draws vs 100s of batched draws
- **Bandwidth**: No constant buffer spam (64×4 float4s per batch)
- **Scalability**: 100K+ instances possible

### Flexibility
- Easy to add Hi-Z occlusion culling (future)
- Temporal anti-aliasing support (frame counter)
- GPU-driven LOD selection
- Foundation for grass state texture

## File Structure

```
src/Layers/xrRender/
├── DetailManager_Compute.h       ✅ Created (GPU structs, manager class)
└── DetailManager_Compute.cpp     ⏳ Next (implementation)

res/gamedata/shaders/r5/
└── detail_cull.cs                ✅ Created (frustum culling)

Documentation/
├── GRASS_RENDERING_ARCHITECTURE.md   ✅ Created (design doc)
└── COMPUTE_INFRASTRUCTURE_STATUS.md  ✅ This file
```

## Shader Compilation

The compute shader needs to be compiled:
```
input:  res/gamedata/shaders/r5/detail_cull.cs
output: <shader_cache>/detail_cull_cs.cso
```

Engine should auto-compile on first load, or use shader compiler tool.

## Configuration

Future console variables:
```
r_detail_compute       1/0   - Enable GPU compute path
r_detail_compute_stats 1/0   - Show culling statistics
r_detail_max_instances 100000 - Max instances to allocate
```

## Notes

- All structures are tightly packed for GPU efficiency
- Frustum extraction follows standard VP matrix decomposition
- SSA calculation matches existing CPU logic for consistency
- Vis_id splitting (0/1/2) preserves animation system
- Indirect args allow GPU to control draw count dynamically

---

**Status**: Infrastructure complete, ready for implementation phase
**Branch**: `yohji/feat/mt-detailmanager`
**Last Updated**: 2025-10-09
