#pragma once

#include "xrCore/xrPool.h"
#include "DetailFormat.h"
#include "DetailModel.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::RENDER_NAMESPACE
{

class FGDetailManager
{
public:

    struct DetailObject
    {
        ref_shader shader;
        ref_geom geometry;
        u32 number_vertices;
        u32 number_indices;
        Fbox bv_bb;
    };

    struct InstanceData
    {
        Fvector pos;
        float scale;
        float rotation;
        float hemi;
        u32 vis_id;
        u32 object_id;
    };
    static_assert(sizeof(InstanceData) == 32, "InstanceData must be 32 bytes");

    struct SlotAABB
    {
        Fvector3 aabb_min;
        float padding0;
        Fvector3 aabb_max;
        float padding1;
        u32 instance_base;
        u32 instance_count;
        int slot_x;
        int slot_z;
        Fvector4 padding2;
    };
    static_assert(sizeof(SlotAABB) == 64, "SlotAABB must be 64 bytes");

    struct BladeVertex
    {
        Fvector pos;
        Fvector2 uv;
        float t;
        float width_scale;
    };
    static_assert(sizeof(BladeVertex) == 28, "BladeVertex must be 28 bytes");

    xr_vector<CDetail*> detail_models;

    xr_vector<DetailObject*> objects;

    xr_vector<InstanceData> all_instances;
    u32 total_instance_count = 0;

    xr_vector<SlotAABB> slot_aabbs;
    u32 slot_count = 0;

    DetailHeader dtH;

    nvrhi::BufferHandle instanceBuffer;

    nvrhi::BufferHandle bladeVertexBuffer;
    nvrhi::BufferHandle bladeIndexBuffer;
    u32 bladeVertexCount = 0;
    u32 bladeIndexCount = 0;
    xr_vector<BladeVertex> bladeVertices;
    xr_vector<u16> bladeIndices;

    nvrhi::BufferHandle visibleIndicesBuffer;
    nvrhi::BufferHandle visibleCountBuffer;
    nvrhi::BufferHandle drawArgsBuffer;
    nvrhi::BufferHandle slotAABBBuffer;

    nvrhi::BufferHandle visibleSlotIDsBuffer;
    nvrhi::BufferHandle visibleSlotCounterBuffer;

    nvrhi::ShaderHandle cullComputeShader;

    nvrhi::ShaderHandle vertexShader;
    nvrhi::ShaderHandle pixelShader;

    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::BindingLayoutHandle graphicsBindingLayout;
    nvrhi::GraphicsPipelineHandle graphicsPipeline;

    nvrhi::BindingLayoutHandle computeBindingLayout;
    nvrhi::ComputePipelineHandle computePipeline;

    u32 visibleBufferCapacity = 0;

    nvrhi::TextureHandle windTexture;
    u32 windTextureBindlessIndex = 0;
    static constexpr u32 WIND_TEXTURE_SIZE = 512;

    nvrhi::ShaderHandle windComputeShader;
    nvrhi::BindingLayoutHandle windBindingLayout;
    nvrhi::ComputePipelineHandle windPipeline;

    Fvector2 windDirection = {1.0f, 0.0f};
    float windSpeed = 0.5f;

    FGDetailManager();
    ~FGDetailManager();

    bool Load();

    void Unload();

    void DecompressAllSlots();

    bool CreateGPUBuffers(nvrhi::IDevice* device);

    void UploadBufferData(nvrhi::ICommandList* cmdList);

    bool LoadCullComputeShader(class framegraph::ShaderLoader* shaderLoader);

    bool LoadGraphicsShaders(class framegraph::ShaderLoader* shaderLoader);
    bool CreateGraphicsPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);

    bool CreateComputePipeline(ng::RenderDevice* device);

    void DispatchCulling(
        nvrhi::ICommandList* cmdList,
        nvrhi::IDevice* device,
        nvrhi::ITexture* hiZPyramid,
        const Fmatrix& viewProj,
        const Fvector4* frustumPlanes,
        u32 frustumPlaneCount,
        const Fvector& cameraPos,
        float fadeDistance,
        u32 hiZWidth,
        u32 hiZHeight,
        u32 hiZMipLevels);

    void DestroyGPUBuffers();

    bool CreateWindTexture(nvrhi::IDevice* device);
    bool LoadWindComputeShader(class framegraph::ShaderLoader* shaderLoader);
    bool CreateWindPipeline(nvrhi::IDevice* device);
    void DispatchWindCompute(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, float time);

    void GenerateBladeGeometry(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments = 8);

    void ComputeSlotAABBs();

private:
    IReader* dtFS = nullptr;

    int dither[16][16];
};

}
