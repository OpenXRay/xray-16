#pragma once

#include "DetailFormat.h"

namespace xray::render::RENDER_NAMESPACE
{

#pragma pack(push, 1)
struct DetailInstanceGPU
{
    Fvector position;
    float scale;
    float rotation_y;
    float padding0[3];

    float c_hemi;
    float c_sun;
    u32 object_id;
    u32 vis_id;
    Fvector color_rgb;
    float padding1;

    Fvector bounds_min;
    float bounds_radius;
    Fvector bounds_max;
    float padding2;

    u32 slot_x;
    u32 slot_z;
    u32 flags;
    float fade_distance_sqr;
};
#pragma pack(pop)

static_assert(sizeof(DetailInstanceGPU) == 128, "DetailInstanceGPU must be 128 bytes");

struct FrustumGPU
{
    Fvector4 planes[6];
};

struct DetailCullParams
{
    Fvector camera_pos;
    float fade_limit_sqr;

    Fvector camera_dir;
    float fade_start_sqr;

    float r_ssa_discard;
    float r_ssa_cheap;
    u32 instance_count;
    u32 frame_number;

    float padding[4];
};

struct IndirectDrawArgs
{
    u32 index_count;
    u32 instance_count;
    u32 start_index;
    s32 base_vertex;
    u32 start_instance;
};

class DetailComputeManager
{
public:
    DetailComputeManager();
    ~DetailComputeManager();

    void Initialize(u32 max_instances);
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    void BeginInstanceUpdate();
    void AddInstance(const DetailInstanceGPU& instance);
    void EndInstanceUpdate();
    u32 GetInstanceCount() const { return m_instance_count; }

    void DispatchCulling(CBackend& cmd_list, const Fmatrix& view_proj);
    void RenderIndirect(CBackend& cmd_list, u32 object_id, u32 vis_id, u32 lod_id);

    struct Stats
    {
        u32 total_instances;
        u32 culled_instances[3];
        u32 compute_dispatches;
        float cull_time_ms;
    };
    const Stats& GetStats() const { return m_stats; }
    void ResetStats();

private:
    struct GPUResources
    {
        ID3DBuffer* instance_buffer;
        ID3DShaderResourceView* instance_buffer_srv;

        ID3DBuffer* visible_indices[3];
        ID3DUnorderedAccessView* visible_indices_uav[3];
        ID3DShaderResourceView* visible_indices_srv[3];

        ID3DBuffer* counter_buffer;
        ID3DUnorderedAccessView* counter_buffer_uav;

        ID3DBuffer* indirect_args[3];
        ID3DUnorderedAccessView* indirect_args_uav[3];

        ConstantBufferHandle cull_params_cb;

        ref_cs cull_shader;

        HostBufferHandle counter_readback;
    };

    GPUResources m_gpu;

    xr_vector<DetailInstanceGPU> m_instance_staging;
    u32 m_instance_count;
    u32 m_max_instances;

    bool m_initialized;
    bool m_needs_upload;

    Stats m_stats;

    void CreateBuffers(u32 max_instances);
    void DestroyBuffers();
    void UploadInstances(CBackend& cmd_list);
    void CompileShaders();
};

inline FrustumGPU BuildFrustumGPU(const Fmatrix& view_proj)
{
    FrustumGPU frustum;

    frustum.planes[0].set(
        view_proj._14 + view_proj._11,
        view_proj._24 + view_proj._21,
        view_proj._34 + view_proj._31,
        view_proj._44 + view_proj._41
    );

    frustum.planes[1].set(
        view_proj._14 - view_proj._11,
        view_proj._24 - view_proj._21,
        view_proj._34 - view_proj._31,
        view_proj._44 - view_proj._41
    );

    frustum.planes[2].set(
        view_proj._14 - view_proj._12,
        view_proj._24 - view_proj._22,
        view_proj._34 - view_proj._32,
        view_proj._44 - view_proj._42
    );

    frustum.planes[3].set(
        view_proj._14 + view_proj._12,
        view_proj._24 + view_proj._22,
        view_proj._34 + view_proj._32,
        view_proj._44 + view_proj._42
    );

    frustum.planes[4].set(
        view_proj._14 + view_proj._13,
        view_proj._24 + view_proj._23,
        view_proj._34 + view_proj._33,
        view_proj._44 + view_proj._43
    );

    frustum.planes[5].set(
        view_proj._14 - view_proj._13,
        view_proj._24 - view_proj._23,
        view_proj._34 - view_proj._33,
        view_proj._44 - view_proj._43
    );

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

inline DetailInstanceGPU ConvertToGPUInstance(
    const CDetailManager::SlotItem& item,
    u32 object_id,
    const CDetail& detail_object,
    int slot_x,
    int slot_z)
{
    DetailInstanceGPU gpu_inst = {};

    gpu_inst.position.set(item.mRotY._41, item.mRotY._42, item.mRotY._43);
    gpu_inst.scale = item.scale;

    gpu_inst.rotation_y = atan2f(item.mRotY._31, item.mRotY._33);

    gpu_inst.c_hemi = item.c_hemi;
    gpu_inst.c_sun = item.c_sun;
    gpu_inst.object_id = object_id;
    gpu_inst.vis_id = item.vis_ID;

#if RENDER == R_R1
    gpu_inst.color_rgb = item.c_rgb;
#endif

    gpu_inst.bounds_min = detail_object.bv_bb.vMin;
    gpu_inst.bounds_max = detail_object.bv_bb.vMax;
    gpu_inst.bounds_radius = detail_object.bv_sphere.R;

    gpu_inst.slot_x = slot_x;
    gpu_inst.slot_z = slot_z;
    gpu_inst.flags = 0;
    gpu_inst.fade_distance_sqr = item.distance;

    return gpu_inst;
}

}
