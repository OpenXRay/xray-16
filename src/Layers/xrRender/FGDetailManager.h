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
        float pad;
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
        float grass_interaction_max_angle;
        float grass_blade_width;
        Fvector4 grass_color_tip;
        Fvector4 grass_color_base;
        Fvector4 grass_sss_color;
        float grass_color_variation;
        float grass_blade_height;
        u32 buildDetailsIndex;
        u32 buildDetailsPbrIndex;
        Fvector4 interaction_window;
        Fvector4 interaction_window_prev;
        float grass_normal_bend;
        float grass_pad0;
        float grass_pad1;
        float grass_pad2;
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

    struct InteractionEntity
    {
        Fvector pos;
        float radius;
        Fvector vel;
        float weight;
    };
    static_assert(sizeof(InteractionEntity) == 32, "InteractionEntity must be 32 bytes");

    struct InteractionParams
    {
        Fvector2 originCur;
        Fvector2 originPrev;
        int prevShiftX;
        int prevShiftY;
        float texelSize;
        u32 size;
        Fvector4 spring;
        float contactRate;
        float dt;
        float maxVelocity;
        u32 entityCount;
        float heightmapMinX;
        float heightmapMinZ;
        float heightmapTexelSize;
        u32 prevValid;
    };
    static_assert(sizeof(InteractionParams) == 80, "InteractionParams must be 80 bytes");

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
    static constexpr u32 LOD_TRIANGLES[LOD_COUNT] = {17, 7, 3};
    static constexpr u32 VIS_KIND_MESH = 3;
    static constexpr u32 VIS_KIND_DECAL = 4;
    static constexpr u32 VIS_KIND_COUNT = 5;
    static constexpr u32 MAX_PULLED_TRIANGLES = 127;

    nvrhi::BufferHandle slotDataBuffer;
    xr_vector<GPUSlotData> slotDataCPU;

    nvrhi::BufferHandle generatedInstancesBuffer;
    nvrhi::BufferHandle detailModelsBuffer;
    u32 generatedInstancesCapacity = 0;

    nvrhi::ShaderHandle instanceGenComputeShader;
    nvrhi::BindingLayoutHandle instanceGenBindingLayout;
    nvrhi::ComputePipelineHandle instanceGenPipeline;

    nvrhi::BufferHandle pulledVertexBuffer;
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

    nvrhi::ShaderHandle slotCullComputeShader;
    nvrhi::BindingLayoutHandle slotCullBindingLayout;
    nvrhi::ComputePipelineHandle slotCullPipeline;

    nvrhi::ShaderHandle cullComputeShader;
    nvrhi::BindingLayoutHandle computeBindingLayout;
    nvrhi::ComputePipelineHandle computePipeline;

    u32 visibleBufferCapacity = 0;

    nvrhi::TextureHandle perlin4dTexture;  // 3D volume (RGBA16F, 32³)
    static constexpr u32 PERLIN4D_TEXTURE_SIZE = 32;

    nvrhi::ShaderHandle perlin4dComputeShader;
    nvrhi::BindingLayoutHandle perlin4dBindingLayout;
    nvrhi::ComputePipelineHandle perlin4dPipeline;
    fg::BufferHandle perlin4dCB;

    static constexpr u32 INTERACTION_TEXTURE_SIZE = 1024;
    static constexpr float INTERACTION_TEXEL_SIZE = 0.0625f;
    static constexpr u32 INTERACTION_MAX_ENTITIES = 64;

    nvrhi::TextureHandle interactionTexture[2];
    Fvector2 interactionOrigin[2] = {};
    bool interactionValid[2] = {false, false};
    u32 interactionCurrent = 0;
    u32 interactionDispatchFrame = 0;
    nvrhi::BufferHandle interactionEntityBuffer;
    xr_vector<InteractionEntity> interactionEntities;

    nvrhi::ShaderHandle interactionComputeShader;
    nvrhi::BindingLayoutHandle interactionBindingLayout;
    nvrhi::ComputePipelineHandle interactionPipeline;
    fg::BufferHandle interactionCB;

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
    static constexpr u32 STATS_READBACK_SLOTS = 6;
    nvrhi::BufferHandle statsReadbackBuffers[STATS_READBACK_SLOTS];
    u32 statsWriteSlot = 0;
    u32 statsScheduled = 0;
    u32 statsFrameCounter = 0;

    nvrhi::SamplerHandle cachedSmp_LinearWrap;
    nvrhi::SamplerHandle cachedSmp_PointClamp;
    nvrhi::SamplerHandle cachedSmp_LinearClamp;
    nvrhi::SamplerHandle cachedSmp_AnisoWrap;

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
    u32 m_lastGrassMode = ~0u;
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

    bool CreateInteractionResources(nvrhi::IDevice* device);
    bool LoadInteractionComputeShader(class framegraph::ShaderLoader* shaderLoader);
    bool CreateInteractionPipeline(nvrhi::IDevice* device);
    void DispatchInteraction(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device);

    void FillFrameConstants(DetailFrameConstants& out);
    void UploadGrassTints(nvrhi::ICommandList* cmdList);
    void ClearDrawArgs(nvrhi::ICommandList* cmdList);
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
