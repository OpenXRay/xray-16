// DetailManager_Compute.h: GPU-driven rendering infrastructure for detail objects
// Implements compute shader-based frustum culling and indirect drawing
//
#pragma once

#include "DetailFormat.h"

namespace xray::render::RENDER_NAMESPACE
{

// ===========================
// GPU Data Structures
// ===========================

// Must match HLSL layout - tightly packed for GPU consumption
// Size: 128 bytes (cache-friendly, 2 per cache line)
#pragma pack(push, 1)
struct DetailInstanceGPU
{
    // Transform data (48 bytes)
    Fvector position;        // World position (12 bytes)
    float scale;             // Uniform scale (4 bytes)
    float rotation_y;        // Y-axis rotation in radians (4 bytes)
    float padding0[7];       // Alignment padding (28 bytes)

    // Rendering data (32 bytes)
    float c_hemi;            // Hemispherical lighting [0..1] (4 bytes)
    float c_sun;             // Sun lighting [0..1] (4 bytes)
    u32 object_id;           // Index into objects array (4 bytes)
    u32 vis_id;              // Animation type: 0=still, 1=wave1, 2=wave2 (4 bytes)
    Fvector color_rgb;       // RGB color (R1 only) (12 bytes)
    float padding1;          // Alignment (4 bytes)

    // Bounding data for culling (32 bytes)
    Fvector bounds_min;      // Local-space AABB min (12 bytes)
    float bounds_radius;     // Bounding sphere radius (4 bytes)
    Fvector bounds_max;      // Local-space AABB max (12 bytes)
    float padding2;          // Alignment (4 bytes)

    // Metadata (16 bytes)
    u32 slot_x;              // Source slot X coordinate (4 bytes)
    u32 slot_z;              // Source slot Z coordinate (4 bytes)
    u32 flags;               // Flags for special rendering (4 bytes)
    float fade_distance_sqr; // Pre-computed squared distance for fading (4 bytes)
};
#pragma pack(pop)

static_assert(sizeof(DetailInstanceGPU) == 128, "DetailInstanceGPU must be 128 bytes");

// GPU-friendly frustum planes (64 bytes)
struct FrustumGPU
{
    Fvector4 planes[6];  // Left, Right, Top, Bottom, Near, Far
};

// Culling parameters passed to compute shader (64 bytes)
struct DetailCullParams
{
    Fvector camera_pos;         // Camera world position (12 bytes)
    float fade_limit_sqr;       // Maximum draw distance squared (4 bytes)

    Fvector camera_dir;         // Camera forward direction (12 bytes)
    float fade_start_sqr;       // Fade start distance squared (4 bytes)

    float r_ssa_discard;        // SSA threshold for culling (4 bytes)
    float r_ssa_cheap;          // SSA threshold for LOD selection (4 bytes)
    u32 instance_count;         // Total number of instances to process (4 bytes)
    u32 frame_number;           // Current frame for temporal throttling (4 bytes)

    float padding[4];           // Alignment to 64 bytes (16 bytes)
};

// Indirect draw arguments structure (must match D3D11_DRAW_INDEXED_ARGUMENTS)
struct IndirectDrawArgs
{
    u32 index_count;      // Number of indices per instance
    u32 instance_count;   // Number of instances to draw (written by compute shader)
    u32 start_index;      // Base index location
    s32 base_vertex;      // Base vertex location
    u32 start_instance;   // Instance index offset
};

// ===========================
// Compute Manager Class
// ===========================

class DetailComputeManager
{
public:
    DetailComputeManager();
    ~DetailComputeManager();

    // Initialization
    void Initialize(u32 max_instances);
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    // Instance management
    void BeginInstanceUpdate();
    void AddInstance(const DetailInstanceGPU& instance);
    void EndInstanceUpdate();
    u32 GetInstanceCount() const { return m_instance_count; }

    // Culling & rendering
    void DispatchCulling(CBackend& cmd_list, const Fmatrix& view_proj);
    void RenderIndirect(CBackend& cmd_list, u32 object_id, u32 vis_id, u32 lod_id);

    // Statistics
    struct Stats
    {
        u32 total_instances;
        u32 culled_instances[3];  // Per vis_id: still, wave1, wave2
        u32 compute_dispatches;
        float cull_time_ms;
    };
    const Stats& GetStats() const { return m_stats; }
    void ResetStats();

private:
    // GPU Resources
    struct GPUResources
    {
        // Instance data buffers
        ID3DBuffer* instance_buffer;                   // All instances (input)
        ID3DShaderResourceView* instance_buffer_srv;   // SRV for reading in compute shader

        ID3DBuffer* visible_indices[3];                // Visible instance indices per vis_id (output)
        ID3DUnorderedAccessView* visible_indices_uav[3]; // UAV for writing
        ID3DShaderResourceView* visible_indices_srv[3];  // SRV for reading during draw

        ID3DBuffer* counter_buffer;                    // Atomic counters for visible instances
        ID3DUnorderedAccessView* counter_buffer_uav;   // UAV for atomic operations

        // Indirect draw arguments
        ID3DBuffer* indirect_args[3];                  // One per vis_id (D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
        ID3DUnorderedAccessView* indirect_args_uav[3]; // UAV for compute shader writes

        // Constants
        ConstantBufferHandle cull_params_cb;           // Culling parameters

        // Compute shader
        ref_cs cull_shader;                            // Frustum culling compute shader

        // Staging for readback (debug/stats only)
        HostBufferHandle counter_readback;
    };

    GPUResources m_gpu;

    // CPU-side data
    xr_vector<DetailInstanceGPU> m_instance_staging;  // CPU staging for uploads
    u32 m_instance_count;
    u32 m_max_instances;

    // State
    bool m_initialized;
    bool m_needs_upload;  // Instances changed, need GPU upload

    // Statistics
    Stats m_stats;

    // Internal helpers
    void CreateBuffers(u32 max_instances);
    void DestroyBuffers();
    void UploadInstances(CBackend& cmd_list);
    void CompileShaders();
};

// ===========================
// Utility Functions
// ===========================

// Build frustum from view-projection matrix
inline FrustumGPU BuildFrustumGPU(const Fmatrix& view_proj)
{
    FrustumGPU frustum;

    // Extract frustum planes from view-projection matrix
    // Left: row4 + row1
    frustum.planes[0].set(
        view_proj._14 + view_proj._11,
        view_proj._24 + view_proj._21,
        view_proj._34 + view_proj._31,
        view_proj._44 + view_proj._41
    );

    // Right: row4 - row1
    frustum.planes[1].set(
        view_proj._14 - view_proj._11,
        view_proj._24 - view_proj._21,
        view_proj._34 - view_proj._31,
        view_proj._44 - view_proj._41
    );

    // Top: row4 - row2
    frustum.planes[2].set(
        view_proj._14 - view_proj._12,
        view_proj._24 - view_proj._22,
        view_proj._34 - view_proj._32,
        view_proj._44 - view_proj._42
    );

    // Bottom: row4 + row2
    frustum.planes[3].set(
        view_proj._14 + view_proj._12,
        view_proj._24 + view_proj._22,
        view_proj._34 + view_proj._32,
        view_proj._44 + view_proj._42
    );

    // Near: row4 + row3
    frustum.planes[4].set(
        view_proj._14 + view_proj._13,
        view_proj._24 + view_proj._23,
        view_proj._34 + view_proj._33,
        view_proj._44 + view_proj._43
    );

    // Far: row4 - row3
    frustum.planes[5].set(
        view_proj._14 - view_proj._13,
        view_proj._24 - view_proj._23,
        view_proj._34 - view_proj._33,
        view_proj._44 - view_proj._43
    );

    // Normalize planes
    for (int i = 0; i < 6; ++i)
    {
        float len = _sqrt(frustum.planes[i].x * frustum.planes[i].x +
                          frustum.planes[i].y * frustum.planes[i].y +
                          frustum.planes[i].z * frustum.planes[i].z);
        if (len > EPS)
        {
            frustum.planes[i].x /= len;
            frustum.planes[i].y /= len;
            frustum.planes[i].z /= len;
            frustum.planes[i].w /= len;
        }
    }

    return frustum;
}

// Convert CPU SlotItem to GPU instance
inline DetailInstanceGPU ConvertToGPUInstance(
    const CDetailManager::SlotItem& item,
    u32 object_id,
    const CDetail& detail_object,
    int slot_x,
    int slot_z)
{
    DetailInstanceGPU gpu_inst = {};

    // Extract position from transform matrix
    gpu_inst.position.set(item.mRotY._41, item.mRotY._42, item.mRotY._43);
    gpu_inst.scale = item.scale;

    // Extract rotation (assuming rotation around Y axis)
    // atan2(m31, m33) gives Y rotation
    gpu_inst.rotation_y = atan2f(item.mRotY._31, item.mRotY._33);

    // Rendering data
    gpu_inst.c_hemi = item.c_hemi;
    gpu_inst.c_sun = item.c_sun;
    gpu_inst.object_id = object_id;
    gpu_inst.vis_id = item.vis_ID;

#if RENDER == R_R1
    gpu_inst.color_rgb = item.c_rgb;
#endif

    // Bounding data (from detail object)
    gpu_inst.bounds_min = detail_object.bv_bb.vMin;
    gpu_inst.bounds_max = detail_object.bv_bb.vMax;
    gpu_inst.bounds_radius = detail_object.bv_sphere.R;

    // Metadata
    gpu_inst.slot_x = slot_x;
    gpu_inst.slot_z = slot_z;
    gpu_inst.flags = 0;
    gpu_inst.fade_distance_sqr = item.distance;  // Already squared by CPU culling

    return gpu_inst;
}

} // namespace xray::render::RENDER_NAMESPACE
