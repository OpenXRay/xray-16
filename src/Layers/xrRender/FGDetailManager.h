#pragma once

#include "xrCore/xrPool.h"
#include "DetailFormat.h"
#include "DetailModel.h"
#include <nvrhi/nvrhi.h>
#include "RenderContext/ResourceHandle.h"

namespace xray::profiler { class GPUProfiler; }

namespace xray::render::fg
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
        u32 packed;
    };
    static_assert(sizeof(InstanceData) == 16, "InstanceData must be 16 bytes");

    struct GPUSlotData
    {
        float world_min_x;
        float world_min_z;
        float y_base;
        float y_height;
        u32 packed_ids;
        u32 packed_palette_01;
        u32 packed_palette_23;
        float hemi;
    };
    static_assert(sizeof(GPUSlotData) == 32, "GPUSlotData must be 32 bytes");

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

    struct DetailFrameConstants
    {
        Fvector4 consts;
        Fvector4 wave;
        Fvector4 dir2D;
        Fvector4 dir2D_2;
        Fmatrix viewProj;
        Fvector4 detail_params;
        Fvector4 g_wind_direction;
        float grass_wind_displacement;
        float grass_interaction_displacement;
        u32 interaction_atlas_index;
        u32 perlin4d_texture_index;
        Fvector4 grass_color_tip;
        Fvector4 grass_color_base;
        Fvector4 grass_sss_color;
        float grass_color_variation;
        float grass_blade_height;
        u32 buildDetailsIndex;
        u32 buildDetailsPbrIndex;
    };

    struct DetailCullParams
    {
        Fmatrix viewProj;
        Fmatrix prevViewProj;
        Fvector3 cameraPos;
        float fadeDistanceSqr;
        Fvector4 frustumPlanes[6];
        u32 visibleBladeCapacity;
        u32 totalSlotCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
        float lodDistanceCloseSqr;
        float lodDistanceMidSqr;
        float detailDensity;
        u32 visibleDecalCapacity;
        u32 grassMode;
        u32 visibleBillboardCapacity;
        u32 cullPad2;
    };

    struct GrassObjectTint { float r, g, b, pad; };

    struct DecalPulledVertex
    {
        float px, py, pz;
        float u, v;
    };
    static_assert(sizeof(DecalPulledVertex) == 20, "DecalPulledVertex must be 20 bytes");

    struct DetailModelGPU
    {
        float minScale;
        float maxScale;
        float flags;
        float geomExtentX;
        float geomExtentZ;
        float uv_min_x;
        float uv_min_y;
        float uv_max_x;
        float uv_max_y;
        u32 pulledVertexBase;
        u32 pulledIndexCount;
        float geomExtentY;
    };
    static_assert(sizeof(DetailModelGPU) == 48, "DetailModelGPU must be 48 bytes");

    struct InstanceGenParams
    {
        float heightmapWorldMinX;
        float heightmapWorldMinZ;
        float heightmapTexelSize;
        float detailHeightMultiplier;
        u32 genMode;
        u32 prefixSumBlockSize;
        u32 prefixSumTotalBlocks;
        u32 instanceCapacity;
        u32 detailModelCount;
        u32 pad0, pad1, pad2;
    };

    xr_vector<CDetail*> detail_models;
    xr_vector<DetailObject*> objects;
    xr_vector<SlotAABB> slot_aabbs;
    u32 slot_count = 0;
    DetailHeader dtH;

    static constexpr u32 LOD_COUNT = 3;
    static constexpr u32 LOD_SEGMENTS[LOD_COUNT] = {9, 4, 2};

    nvrhi::BufferHandle slotDataBuffer;
    xr_vector<GPUSlotData> slotDataCPU;

    nvrhi::BufferHandle generatedInstancesBuffer;
    nvrhi::BufferHandle detailModelsBuffer;
    u32 generatedInstancesCapacity = 0;

    nvrhi::ShaderHandle instanceGenComputeShader;
    nvrhi::BindingLayoutHandle instanceGenBindingLayout;
    nvrhi::ComputePipelineHandle instanceGenPipeline;

    nvrhi::BufferHandle bladeVertexBuffer[LOD_COUNT];
    nvrhi::BufferHandle bladeIndexBuffer[LOD_COUNT];
    u32 bladeVertexCount[LOD_COUNT] = {0, 0, 0};
    u32 bladeIndexCount[LOD_COUNT] = {0, 0, 0};
    xr_vector<BladeVertex> bladeVertices[LOD_COUNT];
    xr_vector<u16> bladeIndices[LOD_COUNT];

    nvrhi::BufferHandle pulledVertexBuffer;
    nvrhi::BufferHandle pulledIndexBuffer;
    u32 maxPulledIndexCount = 0;
    xr_vector<DetailModelGPU> cachedModelGPUData;

    nvrhi::BufferHandle visibleInstancesBuffer[LOD_COUNT];
    nvrhi::BufferHandle drawArgsBuffer[LOD_COUNT];
    nvrhi::BufferHandle visibleDecalInstancesBuffer;
    nvrhi::BufferHandle decalDrawArgsBuffer;
    nvrhi::BufferHandle visibleBillboardInstancesBuffer;
    nvrhi::BufferHandle billboardDrawArgsBuffer;
    nvrhi::BufferHandle slotAABBBuffer;

    nvrhi::TextureHandle buildDetailsTexture;
    u32 buildDetailsBindlessIndex = 0;
    nvrhi::TextureHandle buildDetailsPbrTexture;
    u32 buildDetailsPbrBindlessIndex = 0;

    nvrhi::BufferHandle visibleSlotIDsBuffer;
    nvrhi::BufferHandle visibleSlotCounterBuffer;
    nvrhi::BufferHandle slotVisibilityBuffer;

    nvrhi::ShaderHandle slotCullComputeShader;
    nvrhi::BindingLayoutHandle slotCullBindingLayout;
    nvrhi::ComputePipelineHandle slotCullPipeline;

    nvrhi::ShaderHandle cullComputeShader;
    nvrhi::BindingLayoutHandle computeBindingLayout;
    nvrhi::ComputePipelineHandle computePipeline;

    nvrhi::ShaderHandle vertexShader;
    nvrhi::ShaderHandle pixelShader;
    nvrhi::ShaderHandle decalVertexShader;
    nvrhi::ShaderHandle decalPixelShader;
    nvrhi::ShaderHandle billboardVertexShader;
    nvrhi::ShaderHandle billboardPixelShader;

    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::BindingLayoutHandle graphicsBindingLayout;
    nvrhi::BindingLayoutHandle decalBindingLayout;
    nvrhi::BindingLayoutHandle billboardBindingLayout;
    nvrhi::GraphicsPipelineHandle graphicsPipeline;
    nvrhi::GraphicsPipelineHandle decalGraphicsPipeline;
    nvrhi::GraphicsPipelineHandle billboardGraphicsPipeline;

    u32 visibleBufferCapacity = 0;

    nvrhi::TextureHandle perlin4dTexture;  // 3D volume (RGBA16F, 32³)
    u32 perlin4dBindlessIndex = 0;         // not used for 3D — bound directly at t12
    static constexpr u32 PERLIN4D_TEXTURE_SIZE = 32;

    nvrhi::ShaderHandle perlin4dComputeShader;
    nvrhi::BindingLayoutHandle perlin4dBindingLayout;
    nvrhi::ComputePipelineHandle perlin4dPipeline;
    fg::BufferHandle perlin4dCB;

    // Wind parameters (set from environment, consumed by detail/grass passes)
    Fvector2 windDirection = {1.0f, 0.0f};
    float windSpeed = 0.5f;

    struct DetailCullingStats
    {
        u32 visibleSlotsCount = 0;
        u32 visibleLOD0Count = 0;
        u32 visibleLOD1Count = 0;
        u32 visibleLOD2Count = 0;
        u32 visibleDecalCount = 0;
        u32 visibleBillboardCount = 0;

        u32 totalVisible() const { return visibleLOD0Count + visibleLOD1Count + visibleLOD2Count + visibleBillboardCount; }
    };

    DetailCullingStats cullingStats;
    nvrhi::BufferHandle statsReadbackBuffer;
    bool statsReadbackPending = false;
    u32 statsFrameCounter = 0;

    nvrhi::SamplerHandle cachedSmp_LinearWrap;
    nvrhi::SamplerHandle cachedSmp_PointClamp;
    nvrhi::SamplerHandle cachedSmp_LinearClamp;
    nvrhi::SamplerHandle cachedSmp_AnisoWrap;

    nvrhi::BufferHandle cachedDummySlotIndirection;

    fg::BufferHandle cachedCullParamsCB;
    fg::BufferHandle cachedInstanceGenParamsCB;

    nvrhi::BufferHandle cachedGrassTintsBuffer;

    bool cachedResourcesInitialized = false;

    static constexpr u32 HEIGHTMAP_TEXELS_PER_SLOT = 4;
    static constexpr float HEIGHTMAP_NO_TERRAIN = -1e10f;

    u32 heightmapWidth = 0;
    u32 heightmapHeight = 0;
    float heightmapWorldMinX = 0.f;
    float heightmapWorldMinZ = 0.f;
    float heightmapTexelSize = 0.f;

    nvrhi::TextureHandle heightmapTexture;

    float m_lastDensity = -1.0f;
    bool m_instancesNeedRegeneration = true;

    nvrhi::BufferHandle instanceCounterBuffer;
    nvrhi::BufferHandle instanceCountReadbackBuffer;
    u32 totalGeneratedInstances = 0;
    bool instanceCountReadbackPending = false;
    nvrhi::BufferHandle perSlotCountsBuffer;

    nvrhi::BufferHandle perSlotPrefixBuffer;
    nvrhi::BufferHandle blockTotalsBuffer;
    nvrhi::BufferHandle perSlotLocalCountersBuffer;

    static constexpr u32 PREFIX_SUM_BLOCK_SIZE = 256;
    nvrhi::ShaderHandle prefixSumScanShader;
    nvrhi::ShaderHandle prefixSumTopShader;
    nvrhi::BindingLayoutHandle prefixSumBindingLayout;
    nvrhi::ComputePipelineHandle prefixSumScanPipeline;
    nvrhi::ComputePipelineHandle prefixSumTopPipeline;

    FGDetailManager();
    ~FGDetailManager();

    bool Load();
    void Unload();
    bool BakeHeightmap();
    bool LoadHeightmapTexture(nvrhi::IDevice* device);
    bool LoadBuildDetailsTexture(nvrhi::IDevice* device);
    void PackSlotData();
    bool CreateGPUBuffers(nvrhi::IDevice* device);
    bool CreateCachedResources(nvrhi::IDevice* device);
    void UploadBufferData(nvrhi::ICommandList* cmdList);
    bool LoadCullComputeShader(class framegraph::ShaderLoader* shaderLoader);
    bool LoadInstanceGenShader(class framegraph::ShaderLoader* shaderLoader);
    bool LoadGraphicsShaders(class framegraph::ShaderLoader* shaderLoader);
    bool CreateGraphicsPipeline(fg::RenderDevice* device, const nvrhi::FramebufferInfo& fbInfo);
    bool CreateComputePipeline(fg::RenderDevice* device);
    bool CreateInstanceGenPipeline(fg::RenderDevice* device);
    bool LoadPrefixSumShaders(class framegraph::ShaderLoader* shaderLoader);
    bool CreatePrefixSumPipeline(fg::RenderDevice* device);

    void DispatchCulling(
        nvrhi::ICommandList* cmdList,
        nvrhi::IDevice* device,
        nvrhi::ITexture* hiZPyramid,
        const Fmatrix& prevViewProj,
        float fadeDistance,
        u32 hiZWidth,
        u32 hiZHeight,
        u32 hiZMipLevels,
        xray::profiler::GPUProfiler* gpuProfiler = nullptr);

    void DestroyGPUBuffers();
    void InvalidateShadersAndPipelines();

    bool CreatePerlin4DTexture(nvrhi::IDevice* device);
    bool LoadPerlin4DComputeShader(class framegraph::ShaderLoader* shaderLoader);
    bool CreatePerlin4DPipeline(nvrhi::IDevice* device);
    void DispatchPerlin4DCompute(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, float time);

    void GenerateBladeGeometry(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments = 8);
    void RegenerateBladeGeometry(nvrhi::ICommandList* cmdList);
    void ComputeSlotAABBs();

    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device);
    void ProcessStatsReadback(nvrhi::IDevice* device);
    const DetailCullingStats& GetCullingStats() const { return cullingStats; }

    void RegenerateAllInstances(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device,
        xray::profiler::GPUProfiler* gpuProfiler = nullptr);
    void ResizeVisibleBuffersIfNeeded(nvrhi::IDevice* device);

private:
    IReader* dtFS = nullptr;
    int dither[16][16];

    xr_vector<DecalPulledVertex> pulledVertexData;

    nvrhi::BindingSetHandle CreateInstanceGenBindingSet(nvrhi::IDevice* device) const;
    void BuildDetailModelGPUData();
};

} // namespace xray::render::fg
