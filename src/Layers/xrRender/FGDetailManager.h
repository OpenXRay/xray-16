// FGDetailManager.h - Framegraph detail manager (data/resources only, no rendering)
#pragma once

#include "xrCore/xrPool.h"
#include "DetailFormat.h"
#include "DetailModel.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::RENDER_NAMESPACE
{

// ═══════════════════════════════════════════════════════
//  FG DETAIL MANAGER
// ═══════════════════════════════════════════════════════
// Clean framegraph detail manager:
// - Loads level.details file and decompresses full level
// - Creates GPU buffers for rendering
// - NO rendering logic (handled by DetailPassSetup)
// - NO old CBackend dependencies (uses ng::RenderDevice abstraction)

class FGDetailManager
{
public:
    // ═══════════════════════════════════════════════════════
    //  CORE DATA STRUCTURES
    // ═══════════════════════════════════════════════════════

    // Detail object (grass mesh type)
    struct DetailObject
    {
        ref_shader shader;           // Material shader
        ref_geom geometry;           // Legacy geometry (for reference)
        u32 number_vertices;
        u32 number_indices;
        Fbox bv_bb;                  // Bounding box
        // Note: We don't store CPU-side vertex/index data - only GPU buffers matter
    };

    // Instance data matching GPU shader (32 bytes)
    struct InstanceData
    {
        Fvector pos;       // World position (12 bytes)
        float scale;       // Scale factor (4 bytes)
        float rotation;    // Y-axis rotation in radians (4 bytes)
        float hemi;        // Hemisphere lighting (4 bytes)
        u32 vis_id;        // Visibility/animation type (0=still, 1=wave1, 2=wave2) (4 bytes)
        u32 object_id;     // Which grass object type (0-63) (4 bytes)
    };
    static_assert(sizeof(InstanceData) == 32, "InstanceData must be 32 bytes");

    // Slot AABB for spatial culling (64 bytes)
    struct SlotAABB
    {
        Fvector3 aabb_min;      // Minimum corner (12 bytes)
        float padding0;         // Align to 16 bytes
        Fvector3 aabb_max;      // Maximum corner (12 bytes)
        float padding1;         // Align to 16 bytes
        u32 instance_base;      // First instance index (4 bytes)
        u32 instance_count;     // Number of instances (4 bytes)
        int slot_x;             // Grid X coordinate (4 bytes)
        int slot_z;             // Grid Z coordinate (4 bytes)
        Fvector4 padding2;      // Pad to 64 bytes
    };
    static_assert(sizeof(SlotAABB) == 64, "SlotAABB must be 64 bytes");

    // Blade vertex for procedural grass (28 bytes)
    struct BladeVertex
    {
        Fvector pos;         // Local position Y-up (12 bytes)
        Fvector2 uv;         // Texture coordinates (8 bytes)
        float t;             // Height parameter 0=base, 1=tip (4 bytes)
        float width_scale;   // Width at this height (4 bytes)
    };
    static_assert(sizeof(BladeVertex) == 28, "BladeVertex must be 28 bytes");

    // GPU constant buffer for detail rendering (must match HLSL cbuffer in detail_gpu.vs/ps)
    struct DetailFrameConstants
    {
        Fvector4 consts;                  // scale, scale, aniso, ambient
        Fvector4 wave;                    // wave animation params
        Fvector4 dir2D;                   // wind direction 1 (XY = direction)
        Fvector4 dir2D_2;                 // wind direction 2 (perpendicular)
        Fmatrix viewProj;                 // View-projection matrix
        Fvector4 detail_params;           // slot size/offset (unused for now)
        Fvector4 g_wind_direction;        // x=angle degrees, y=speed, z/w=unused
        float grass_wind_displacement;    // Wind strength
        float grass_interaction_displacement;  // Interaction strength
        u32 interaction_atlas_index;      // Bindless index for interaction atlas
        u32 wind_texture_index;           // Bindless index for wind texture
        Fvector4 grass_color_tip;         // RGB + padding (blade tip color)
        Fvector4 grass_color_base;        // RGB + padding (blade base color)
        Fvector4 grass_sss_color;         // RGB + intensity (subsurface scattering)
        float grass_color_variation;      // Per-blade color variation amount
        float grass_blade_height;         // Blade height multiplier
        float pad0, pad1;                 // Padding to 16-byte alignment
    };

    // GPU constant buffer for detail culling (must match HLSL cbuffer in detail_cull.cs)
    struct DetailCullParams
    {
        Fmatrix viewProj;        // Current frame (for frustum culling)
        Fmatrix prevViewProj;    // Previous frame (for temporal Hi-Z sampling)
        Fvector3 cameraPos;
        float fadeDistanceSqr;
        Fvector4 frustumPlanes[6];
        u32 totalInstanceCount;
        u32 totalSlotCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
        float lodDistanceCloseSqr;
        float lodDistanceMidSqr;
        u32 pad;
    };

    // Per-object grass tint color
    struct GrassObjectTint { float r, g, b, pad; };

    // ═══════════════════════════════════════════════════════
    //  PUBLIC MEMBERS
    // ═══════════════════════════════════════════════════════

    // Detail models (CDetail* for decompression - matching original)
    xr_vector<CDetail*> detail_models;

    // Detail objects (mesh types - legacy, keep for compatibility)
    xr_vector<DetailObject*> objects;

    // All level instances (decompressed)
    xr_vector<InstanceData> all_instances;
    u32 total_instance_count = 0;

    // Slot AABBs for spatial culling
    xr_vector<SlotAABB> slot_aabbs;
    u32 slot_count = 0;

    // Level header
    DetailHeader dtH;

    // ═══════════════════════════════════════════════════════
    //  GPU RESOURCES (NVRHI)
    // ═══════════════════════════════════════════════════════

    // LOD system constants
    static constexpr u32 LOD_COUNT = 3;
    static constexpr u32 LOD_SEGMENTS[LOD_COUNT] = {9, 4, 2};  // Close, mid, far

    // Instance data buffer (all level instances)
    nvrhi::BufferHandle instanceBuffer;

    // Blade geometry per LOD (procedural grass mesh)
    nvrhi::BufferHandle bladeVertexBuffer[LOD_COUNT];
    nvrhi::BufferHandle bladeIndexBuffer[LOD_COUNT];
    u32 bladeVertexCount[LOD_COUNT] = {0, 0, 0};
    u32 bladeIndexCount[LOD_COUNT] = {0, 0, 0};
    xr_vector<BladeVertex> bladeVertices[LOD_COUNT];  // Cached for upload
    xr_vector<u16> bladeIndices[LOD_COUNT];           // Cached for upload

    // GPU culling buffers (per-LOD)
    nvrhi::BufferHandle visibleInstancesBuffer[LOD_COUNT];  // Output: visible instances per LOD
    nvrhi::BufferHandle drawArgsBuffer[LOD_COUNT];          // Output: indirect draw args per LOD
    nvrhi::BufferHandle slotAABBBuffer;                     // Input: slot bounding boxes (shared)

    // Phase 6B: Visible slot tracking (for page table system)
    nvrhi::BufferHandle visibleSlotIDsBuffer;   // visible slot IDs
    nvrhi::BufferHandle visibleSlotCounterBuffer; // visible slot counter

    // Two-pass culling buffers
    nvrhi::BufferHandle slotVisibilityBuffer;    // Per-slot visibility flag (u32 per slot, written by Pass 1)
    nvrhi::BufferHandle instanceToSlotBuffer;    // Maps instance_idx -> slot_idx (u32 per instance)
    xr_vector<u32> instanceToSlotMapping;        // CPU-side copy for upload

    // Pass 1: Slot culling compute shader + pipeline
    nvrhi::ShaderHandle slotCullComputeShader;
    nvrhi::BindingLayoutHandle slotCullBindingLayout;
    nvrhi::ComputePipelineHandle slotCullPipeline;

    // Pass 2: Instance culling compute shader + pipeline
    nvrhi::ShaderHandle cullComputeShader;
    nvrhi::BindingLayoutHandle computeBindingLayout;
    nvrhi::ComputePipelineHandle computePipeline;

    // Graphics shaders
    nvrhi::ShaderHandle vertexShader;
    nvrhi::ShaderHandle pixelShader;

    // Graphics pipeline
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::BindingLayoutHandle graphicsBindingLayout;
    nvrhi::GraphicsPipelineHandle graphicsPipeline;

    // Max capacity for culling buffers
    u32 visibleBufferCapacity = 0;

    // ═══════════════════════════════════════════════════════
    //  WIND SYSTEM (FBM procedural wind)
    // ═══════════════════════════════════════════════════════

    // Wind texture (512x512 R16G16B16A16_FLOAT)
    // RGB = wind direction (encoded), A = wind strength
    nvrhi::TextureHandle windTexture;
    u32 windTextureBindlessIndex = 0;  // Bindless descriptor index for VS access
    static constexpr u32 WIND_TEXTURE_SIZE = 512;

    // Wind compute shader and pipeline
    nvrhi::ShaderHandle windComputeShader;
    nvrhi::BindingLayoutHandle windBindingLayout;
    nvrhi::ComputePipelineHandle windPipeline;

    // Wind parameters (updated per frame)
    Fvector2 windDirection = {1.0f, 0.0f};  // Normalized XZ direction
    float windSpeed = 0.5f;                  // Base wind speed

    // ═══════════════════════════════════════════════════════
    //  CULLING STATS (GPU readback)
    // ═══════════════════════════════════════════════════════

    struct DetailCullingStats
    {
        u32 visibleSlotsCount = 0;      // Slots that passed frustum culling
        u32 visibleLOD0Count = 0;       // Instances in LOD0 (close, 9 segments)
        u32 visibleLOD1Count = 0;       // Instances in LOD1 (mid, 4 segments)
        u32 visibleLOD2Count = 0;       // Instances in LOD2 (far, 2 segments)

        u32 totalVisible() const { return visibleLOD0Count + visibleLOD1Count + visibleLOD2Count; }
    };

    DetailCullingStats cullingStats;
    nvrhi::BufferHandle statsReadbackBuffer;  // CPU-readable buffer for stats
    bool statsReadbackPending = false;
    u32 statsFrameCounter = 0;

    // ═══════════════════════════════════════════════════════
    //  CACHED PER-FRAME RESOURCES
    // ═══════════════════════════════════════════════════════
    // Objects with constant parameters, created once and reused every frame.
    // Avoids per-frame createSampler/createBuffer overhead.

    // Samplers (4 unique objects covering 7 binding slots)
    nvrhi::SamplerHandle cachedSmp_LinearWrap;    // s0, s3, s5 (Linear, Wrap)
    nvrhi::SamplerHandle cachedSmp_PointClamp;    // s1, compute point sampler (Point, Clamp)
    nvrhi::SamplerHandle cachedSmp_LinearClamp;   // s2 (Linear, Clamp)
    nvrhi::SamplerHandle cachedSmp_AnisoWrap;     // s4 (Linear, Wrap, 16x Aniso)

    // Dummy buffers (constant placeholders, never modified)
    nvrhi::BufferHandle cachedDummyMaterials;         // t8 placeholder (32 bytes)
    nvrhi::BufferHandle cachedDummySlotIndirection;   // t32 placeholder (4 bytes)

    // Volatile constant buffers (created once, writeBuffer'd per frame)
    nvrhi::BufferHandle cachedDynTransformsCB;    // b0: DynamicTransforms
    nvrhi::BufferHandle cachedShaderParamsCB;     // b1: ShaderParams (dummy)
    nvrhi::BufferHandle cachedStaticGlobalsCB;    // b2: StaticGlobals
    nvrhi::BufferHandle cachedDetailGlobalsCB;    // b3: DetailFrameConstants
    nvrhi::BufferHandle cachedDynLightCB;         // b4: DynamicLight (dummy)
    nvrhi::BufferHandle cachedCullParamsCB;       // b5: DetailCullParams

    // Grass object tint buffer (created once, written per frame)
    nvrhi::BufferHandle cachedGrassTintsBuffer;

    bool cachedResourcesInitialized = false;

    // ═══════════════════════════════════════════════════════
    //  PUBLIC METHODS
    // ═══════════════════════════════════════════════════════

    FGDetailManager();
    ~FGDetailManager();

    // Load level.details file
    bool Load();

    // Unload all data
    void Unload();

    // Decompress entire level into all_instances
    void DecompressAllSlots();

    // Create all GPU buffers (call after DecompressAllSlots)
    bool CreateGPUBuffers(nvrhi::IDevice* device);

    // Create cached per-frame resources (samplers, dummy buffers, volatile CBs)
    bool CreateCachedResources(nvrhi::IDevice* device);

    // Upload buffer data to GPU (call once during first frame)
    void UploadBufferData(nvrhi::ICommandList* cmdList);

    // Load compute shader for GPU culling
    bool LoadCullComputeShader(class framegraph::ShaderLoader* shaderLoader);

    // Load graphics shaders and create pipeline
    bool LoadGraphicsShaders(class framegraph::ShaderLoader* shaderLoader);
    bool CreateGraphicsPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);

    // Create compute pipeline for GPU culling
    bool CreateComputePipeline(ng::RenderDevice* device);

    // Dispatch GPU culling compute shader
    void DispatchCulling(
        nvrhi::ICommandList* cmdList,
        nvrhi::IDevice* device,
        nvrhi::ITexture* hiZPyramid,
        const Fmatrix& viewProj,
        const Fmatrix& prevViewProj,  // Previous frame's viewProj for temporal Hi-Z
        const Fvector4* frustumPlanes,
        u32 frustumPlaneCount,
        const Fvector& cameraPos,
        float fadeDistance,
        u32 hiZWidth,
        u32 hiZHeight,
        u32 hiZMipLevels);

    // Destroy GPU buffers
    void DestroyGPUBuffers();

    // Wind system
    bool CreateWindTexture(nvrhi::IDevice* device);
    bool LoadWindComputeShader(class framegraph::ShaderLoader* shaderLoader);
    bool CreateWindPipeline(nvrhi::IDevice* device);
    void DispatchWindCompute(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, float time);

    // Generate procedural blade geometry
    void GenerateBladeGeometry(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments = 8);

    // Regenerate blade geometry and upload to GPU (for runtime parameter changes)
    void RegenerateBladeGeometry(nvrhi::ICommandList* cmdList);

    // Compute slot AABBs from instances
    void ComputeSlotAABBs();

    // Stats readback (call after DispatchCulling)
    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device);
    void ProcessStatsReadback(nvrhi::IDevice* device);
    const DetailCullingStats& GetCullingStats() const { return cullingStats; }

private:
    // File stream for level.details
    IReader* dtFS = nullptr;

    // Dither matrix for detail placement
    int dither[16][16];
};

} // namespace xray::render::RENDER_NAMESPACE
