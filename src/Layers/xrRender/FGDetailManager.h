#pragma once

#include "xrCore/xrPool.h"
#include "DetailFormat.h"
#include "DetailModel.h"
#include <nvrhi/nvrhi.h>
#include "RenderContext/ResourceHandle.h"
#include "xrEngine/IRenderBackend.h"
#include <memory>

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
        u32 instance_chunk;
        u32 padding2[3];
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
        float grass_color_variation;
        float grass_blade_height;
        u32 buildDetailsIndex;
        u32 buildDetailsPbrIndex;
        Fvector4 interaction_window;
        Fvector4 interaction_window_prev;
        u32 buildDetailsBumpIndex;
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
        u32 visibleBladeCapacity[3];
        u32 totalSlotCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
        float detailDensity;
        float lodDistanceCloseSqr;
        float lodDistanceMidSqr;
        u32 visibleDecalCapacity;
        u32 grassMode;
        u32 visibleBillboardCapacity;
        u32 preparedCapacity[3];
    };
    static_assert(sizeof(DetailCullParams) == 304);

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
        u32 slotOffset;
        u32 slotCount;
        u32 instanceCapacity;
        u32 detailModelCount;
        float detailDensity;
        u32 grassMode;
        u32 padding[2];
    };
    static_assert(sizeof(InstanceGenParams) == 48);

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

    nvrhi::BufferHandle detailModelsBuffer;

    nvrhi::ShaderHandle instanceGenComputeShader;
    nvrhi::BindingLayoutHandle instanceGenBindingLayout;
    nvrhi::ComputePipelineHandle instanceGenPipeline;
    nvrhi::ShaderHandle instanceCountComputeShader;
    nvrhi::BindingLayoutHandle instanceCountBindingLayout;
    nvrhi::ComputePipelineHandle instanceCountPipeline;

    nvrhi::BufferHandle pulledVertexBuffer;
    u32 maxPulledIndexCount = 0;
    xr_vector<DetailModelGPU> cachedModelGPUData;

    nvrhi::BufferHandle bladeIndexBuffer[LOD_COUNT];
    bool bladeIndicesUploaded = false;
    static constexpr u32 PREPARED_BLADE_CAPACITY = 1u << 17;

    nvrhi::TextureHandle buildDetailsTexture;
    u32 buildDetailsBindlessIndex = 0;
    nvrhi::TextureHandle buildDetailsPbrTexture;
    u32 buildDetailsPbrBindlessIndex = 0;
    nvrhi::TextureHandle buildDetailsBumpTexture;
    u32 buildDetailsBumpBindlessIndex = 0;


    nvrhi::ShaderHandle slotCullComputeShader;
    nvrhi::BindingLayoutHandle slotCullBindingLayout;
    nvrhi::ComputePipelineHandle slotCullPipeline;

    nvrhi::ShaderHandle cullComputeShader;
    nvrhi::BindingLayoutHandle computeBindingLayout;
    nvrhi::ComputePipelineHandle computePipeline;
    nvrhi::ShaderHandle slotArgsComputeShader;
    nvrhi::BindingLayoutHandle slotArgsBindingLayout;
    nvrhi::ComputePipelineHandle slotArgsPipeline;
    nvrhi::ShaderHandle visibilityArgsComputeShader;
    nvrhi::BindingLayoutHandle visibilityArgsBindingLayout;
    nvrhi::ComputePipelineHandle visibilityArgsPipeline;

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
        u32 packetCount = 0;
        u32 overflowFlags = 0;

        u32 totalVisible() const { return visibleLOD0Count + visibleLOD1Count + visibleLOD2Count + visibleBillboardCount; }
    };

    class DetailMembershipFingerprint
    {
    public:
        u32 membership0 = 0;
        u32 membership1 = 0;
    };
    static_assert(sizeof(DetailMembershipFingerprint) == 8, "DetailMembershipFingerprint must be 8 bytes");

    static constexpr u32 VISIBILITY_STATUS_BYTES = sizeof(DetailCullingStats) + sizeof(DetailMembershipFingerprint);
    static_assert(VISIBILITY_STATUS_BYTES == 40, "detail visibility status must stay 32-byte stats plus two fingerprint words");

    class InstanceChunk
    {
    public:
        nvrhi::BufferHandle buffer;
        Fbox bounds;
        Fvector2 sourceMin;
        Fvector2 sourceMax;
        u32 firstSlot = 0;
        u32 slotCount = 0;
        u32 instanceCount = 0;
        u32 wavingCount = 0;
        u32 staticCount = 0;
    };

    class InstanceGeneration
    {
    public:
        xr_vector<InstanceChunk> chunks;
        nvrhi::BindingLayoutHandle bindingLayout;
        nvrhi::DescriptorTableHandle descriptorTable;
        nvrhi::BufferHandle slots;
        nvrhi::BufferHandle models;
        nvrhi::BufferHandle pulledVertices;
        InstanceGenParams params = {};
        u32 maxPulledIndexCount = 0;
        u64 id = 0;
        u64 instanceCount = 0;
        u64 bytes = 0;
    };

    class VisibilityFrame
    {
    public:
        std::shared_ptr<const InstanceGeneration> source;
        xr_vector<u32> visibleChunks;
        DetailCullParams cullParams = {};
        nvrhi::BufferHandle visible[VIS_KIND_COUNT];
        nvrhi::BufferHandle drawArgs[VIS_KIND_COUNT];
        nvrhi::BufferHandle prepared[LOD_COUNT];
        nvrhi::BufferHandle visibleSlots;
        nvrhi::BufferHandle visibleSlotCount;
        nvrhi::BufferHandle slotDispatch;
        nvrhi::BufferHandle swDispatch;
        nvrhi::BufferHandle packets;
        nvrhi::BufferHandle workStatus;
        nvrhi::BufferHandle readback;
        DetailCullingStats stats;
        u64 contentSignature = 0;
        u32 visibleCapacity[VIS_KIND_COUNT] = {};
        u32 preparedCapacity[LOD_COUNT] = {};
        u32 entryBase = 0;
        u64 id = 0;
        u64 lease = 0;
        u64 bytes = 0;
        bool statsRecorded = false;
        bool statsReady = false;
    };

    class GenerationWork
    {
    public:
        enum class Stage : u8
        {
            CountReady,
            CountPending,
            EmitReady,
            EmitPending
        };

        std::shared_ptr<InstanceGeneration> source;
        xr_vector<SlotAABB> slots;
        xr_vector<u32> slotOrder;
        nvrhi::BufferHandle counts;
        nvrhi::BufferHandle countReadback;
        nvrhi::BufferHandle localCounters;
        nvrhi::BufferHandle emitSlots;
        nvrhi::BufferHandle status;
        nvrhi::BufferHandle statusReadback;
        InstanceGenParams params = {};
        Stage stage = Stage::CountReady;
        u32 nextChunk = 0;
        u32 submittedChunks = 0;
        u64 lease = 0;
        u64 bytes = 0;
    };

    class InstanceMemoryStats
    {
    public:
        IRenderBackend::MemoryBudget device;
        u64 residentInstances = 0;
        u64 residentBytes = 0;
        u64 activeBytes = 0;
        u64 frameBytes = 0;
        u64 pendingBytes = 0;
        u32 residentChunks = 0;
        u32 generations = 0;
        u32 frames = 0;
    };

    std::shared_ptr<const InstanceGeneration> generatedInstances;
    std::shared_ptr<VisibilityFrame> visibilityFrame;
    std::shared_ptr<GenerationWork> generationWork;
    DetailCullingStats cullingStats;
    InstanceMemoryStats instanceMemoryStats;
    u64 totalGeneratedInstances = 0;

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

    void DispatchCulling(
        nvrhi::ICommandList* cmdList,
        nvrhi::IDevice* device,
        nvrhi::ITexture* hiZPyramid,
        VisibilityFrame& frame,
        const std::shared_ptr<GenerationWork>& generation,
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
    void ClearDrawArgs(nvrhi::ICommandList* cmdList, VisibilityFrame& frame);
    void ComputeSlotAABBs();

    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, VisibilityFrame& frame);
    void ProcessStatsReadback(nvrhi::IDevice* device);
    const DetailCullingStats& GetCullingStats() const { return cullingStats; }

    void PrepareFrame(nvrhi::IDevice* device, u32 entryBase, bool enabled);
    std::shared_ptr<const VisibilityFrame> GetCompletedVisibilityFrame() const;
    void RecordGeneration(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, GenerationWork& work);

private:
    IReader* dtFS = nullptr;
    int dither[16][16];

    xr_vector<DecalPulledVertex> pulledVertexData;

    void AllocateGeneration(nvrhi::IDevice* device, GenerationWork& work);
    void AllocateVisibilityFrame(nvrhi::IDevice* device, VisibilityFrame& frame);
    void RequireInstanceMemory(u64 bytes, const char* purpose);
    void UpdateInstanceMemoryStats();
    void DestroyInstanceStorage();
    void RecordVisibilityWork(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, VisibilityFrame& frame, bool slots);
    static u64 CombineContentHash(u64 hash, u64 value);
    static u64 FinalizeContentHash(u64 hash);
    static u64 ComposeContentSignature(u64 sourceId, const DetailCullingStats& stats,
        const DetailMembershipFingerprint& fingerprint);
    nvrhi::BufferHandle CreateInstanceBuffer(nvrhi::IDevice* device, u64 bytes, u32 stride,
        const char* name, bool readback = false, bool indirect = false);
    bool IsChunkVisible(const InstanceChunk& chunk, const DetailCullParams& params) const;
    xr_vector<std::shared_ptr<VisibilityFrame>> m_visibilityFrames;
    xr_vector<std::weak_ptr<const InstanceGeneration>> m_instanceGenerations;
    std::shared_ptr<VisibilityFrame> m_completedVisibilityFrame;
    nvrhi::BindingLayoutHandle m_cullSourceLayout;
    u64 m_instanceGenerationId = 0;
    u64 m_visibilityFrameId = 0;
    bool m_instancesNeedRegeneration = true;
    bool m_detailsEnabled = false;
    void BuildDetailModelGPUData();
};

} // namespace xray::render::fg
