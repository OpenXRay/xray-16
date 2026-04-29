// DetailManager.h: interface for the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCore/xrPool.h"
#include "Layers/xrRender/DetailFormat.h"
#include "Layers/xrRender/DetailModel.h"

namespace xray::render::fg
{
#ifdef _EDITOR
//.	#include	"ESceneClassList.h"
const int dm_max_decompress = 14;
class CCustomObject;
typedef u32 ObjClassID;

typedef xr_list<CCustomObject*> ObjectList;
typedef ObjectList::iterator ObjectIt;
typedef xr_map<ObjClassID, ObjectList> ObjectMap;
typedef ObjectMap::iterator ObjectPairIt;

#else
const int dm_max_decompress = 7;
#endif
//const int dm_size = 24;
const int dm_cache1_count = 4;
//const int dm_cache1_line = dm_size * 2 / dm_cache1_count; //! dm_size*2 must be div dm_cache1_count
const int dm_max_objects = 64;
const int dm_obj_in_slot = 4;
//const int dm_cache_line = dm_size + 1 + dm_size;
//const int dm_cache_size = dm_cache_line * dm_cache_line;
//const float dm_fade = float(2 * dm_size) - .5f;
const float dm_slot_size = DETAIL_SLOT_SIZE;

//AVO: detail radius
//const u32 dm_max_cache_size = 62001; // assuming max dm_size = 124
constexpr auto dm_max_cache_size = 62001 * 2; // assuming max dm_size = 248
extern u32 dm_size;
extern u32 dm_cache1_line;
extern u32 dm_cache_line;
extern u32 dm_cache_size;
extern float dm_fade;
extern u32 dm_current_size;// = iFloor((float)ps_r__detail_radius/4)*2; //!
extern u32 dm_current_cache1_line;// = dm_current_size*2/dm_cache1_count; //! dm_current_size*2 must be div dm_cache1_count
extern u32 dm_current_cache_line;// = dm_current_size+1+dm_current_size;
extern u32 dm_current_cache_size;// = dm_current_cache_line*dm_current_cache_line;
extern float dm_current_fade;// = float(2*dm_current_size)-.5f;
extern float ps_current_detail_density;
extern float ps_current_detail_height;

class ECORE_API CDetailManager
{
public:
    // ===================================================================
    // VANILLA CPU PATH MEMBERS
    // These members are from the original X-Ray engine (dev branch)
    // They are used for slot-based CPU visibility and rendering
    // ===================================================================

    float fade_distance = 99999;
    Fvector light_position;
    void details_clear();

    struct SlotItem
    { // один кустик
        float scale;
        float scale_calculated;
        Fmatrix mRotY;
        u32 vis_ID; // индекс в visibility списке он же тип [не качается, качается1, качается2]
        float c_hemi;
        float c_sun;
#if RENDER == R_R1
        Fvector c_rgb;
#endif
    };

    using SlotItemVec = xr_vector<SlotItem*>;

    struct SlotPart
    { //
        u32 id; // ID модельки
        SlotItemVec items; // список кустиков
        SlotItemVec r_items[3]; // список кустиков for render
    };

    enum SlotType : u32
    {
        stReady = 0, // Ready to use
        stPending, // Pending for decompression
    };

    struct Slot
    { // распакованый слот размером DETAIL_SLOT_SIZE
        struct
        {
            u32 empty : 1;
            u32 type : 1;
            u32 frame : 30;
        };
        int sx, sz; // координаты слота X x Y
        vis_data vis; //
        SlotPart G[dm_obj_in_slot]; //

        Slot()
        {
            frame = 0;
            empty = 1;
            type = stReady;
            sx = sz = 0;
            vis.clear();
        }
    };

    struct CacheSlot1
    {
        u32 empty;
        vis_data vis;
        Slot** slots[dm_cache1_count * dm_cache1_count];
        CacheSlot1()
        {
            empty = 1;
            vis.clear();
        }
    };

    typedef xr_vector<xr_vector<SlotItemVec*>> vis_list;
    typedef svector<CDetail*, dm_max_objects> DetailVec;
    typedef DetailVec::iterator DetailIt;
    typedef poolSS<SlotItem, 4096> PSS;

    int dither[16][16];

    // swing values
    struct SSwingValue
    {
        float rot1;
        float rot2;
        float amp1;
        float amp2;
        float speed;
        void lerp(const SSwingValue& v1, const SSwingValue& v2, float factor);
    };
    SSwingValue swing_desc[2];
    SSwingValue swing_current;
    float m_time_rot_1;
    float m_time_rot_2;
    float m_time_pos;
    float m_global_time_old;

    IReader* dtFS;
    DetailHeader dtH;
    DetailSlot* dtSlots; // note: pointer into VFS
    DetailSlot DS_empty;

    DetailVec objects;
    vis_list m_visibles[3]; // 0=still, 1=Wave1, 2=Wave2

#ifndef _EDITOR
    xrXRC xrc;
#endif
    //AVO: detail draw radius
    CacheSlot1** cache_level1;
    Slot*** cache; // grid-cache itself
    svector<Slot*, dm_max_cache_size> cache_task; // non-unpacked slots
    Slot* cache_pool; // just memory for slots

    int cache_cx;
    int cache_cz;

    PSS poolSI; // pool из которого выделяются SlotItem

    Fvector EYE;

    void UpdateVisibleM();
    void UpdateVisibleS();

#ifdef _EDITOR
    virtual ObjectList* GetSnapList() = 0;
#endif

    bool UseVS() const;

    // Software processor
    ref_geom soft_Geom;
    void soft_Load();
    void soft_Unload();
    void soft_Render();

    // Hardware processor
    ref_geom hw_Geom;
    size_t hw_BatchSize;

    VertexStagingBuffer hw_VB;
    IndexStagingBuffer hw_IB;

    ref_constant hwc_consts;
    ref_constant hwc_wave;
    ref_constant hwc_wind;
    ref_constant hwc_array;
    ref_constant hwc_s_consts;
    ref_constant hwc_s_xform;
    ref_constant hwc_s_array;
    void hw_Load();
    void hw_Load_Geom();
    void hw_Load_Shaders();
    void hw_Unload();
    void hw_Render(CBackend& cmd_list);
    void hw_Render_dump(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id);

    // get unpacked slot
    DetailSlot& QueryDB(int sx, int sz);

    void cache_Initialize();
    void cache_Update(int sx, int sz, Fvector& view);
    void cache_Task(int gx, int gz, Slot* D);
    Slot* cache_Query(int sx, int sz);
    void cache_Decompress(Slot* D);
    BOOL cache_Validate();

    // cache grid to world
    int cg2w_X(int x) { return cache_cx - dm_size + x; }
    int cg2w_Z(int z) { return cache_cz - dm_size + (dm_cache_line - 1 - z); }
    // world to cache grid
    int w2cg_X(int x) { return x - cache_cx + dm_size; }
    int w2cg_Z(int z) { return cache_cz - dm_size + (dm_cache_line - 1 - z); }

    void Load();
    void Unload();
    void Render(CBackend& cmd_list);

    /// MT stuff
    Task* m_calc_task{};

    void DispatchMTCalc();

    // ===================================================================
    // GPU COMPUTE PATH ADDITIONS (ps_r__detail_gpu = 1)
    // These members and functions are ONLY used when GPU compute path is active
    // They are added to the bottom to clearly separate from vanilla code
    // ===================================================================

    // Phase 1, Milestone 1.2: Unified geometry per vis_id
    ref_geom vis_unified_geom[3];  // One unified geometry per vis_id
    VertexStagingBuffer vis_unified_VB[3];  // Persistent VBs for unified geometry
    IndexStagingBuffer vis_unified_IB[3];   // Persistent IBs for unified geometry
    xr_vector<u32> vis_geometry_vertex_offsets[3];  // Per-object vertex offsets within unified buffer
    xr_vector<u32> vis_geometry_index_offsets[3];   // Per-object index offsets within unified buffer
    xr_vector<u32> vis_object_indices[3];           // Map from local object index to global object index
    u32 vis_total_vertices[3];     // Total vertices in unified buffer
    u32 vis_total_indices[3];      // Total indices in unified buffer

#ifdef USE_DX11
    // Phase 1, Milestone 1.1: Structured buffers for GPU instancing (one per vis_id: still, wave1, wave2)
    ID3DShaderResourceView* detailSRV_vis[3];
    ID3DBuffer* detailBuffer_vis[3];
    u32 detailBufferSize_vis[3];  // Track allocated size for each buffer

    // Phase 2.0: Full level decompression
    struct SlotItemWithObject
    {
        SlotItem item;      // Original slot item data
        u32 object_id;      // Which grass object type (0-63)
    };

    // Instance data structure matching shader
    struct InstanceData
    {
        Fvector pos;     // Position (12 bytes)
        float scale;     // Scale factor (4 bytes)
        float hemi;      // Hemisphere lighting (4 bytes)
        u32 vis_id;      // Visibility/animation type (4 bytes)
        u32 object_id;   // Which grass object type (4 bytes)
        float padding;   // Padding to align to 16 bytes (4 bytes) = 32 bytes total
    };
    static_assert(sizeof(InstanceData) == 32, "InstanceData must be 32 bytes");

    xr_vector<SlotItemWithObject> all_level_instances;  // ALL instances for entire level
    u32 total_instance_count;
    bool full_level_loaded;

    // Phase 2.0.3: Persistent GPU buffers
    ID3DBuffer* persistent_instance_buffer;
    ID3DShaderResourceView* persistent_instance_srv;
    u32 persistent_buffer_capacity;

    // Phase 2.1: GPU culling infrastructure
    ref_cs cull_compute_shader;  // detail_cull.cs compute shader
    ID3DBuffer* cull_constant_buffer;  // Culling parameters (camera, frustum, etc)

    // UNIFIED: Single buffer for all visible instances (all grass types use same shader)
    ID3DBuffer* gpu_visible_unified_buffer;
    ID3DShaderResourceView* gpu_visible_unified_srv;
    ID3DUnorderedAccessView* gpu_visible_unified_uav;
    u32 gpu_visible_buffer_capacity;  // Max total visible instances

    // UNIFIED: Single atomic counter for all visible instances
    ID3DBuffer* gpu_visible_count_buffer;
    ID3DUnorderedAccessView* gpu_visible_count_uav;
    ID3DBuffer* gpu_visible_count_readback;  // CPU readback for count

    // UNIFIED: Single indirect draw args buffer
    ID3DBuffer* gpu_indirect_args_unified;
    ID3DUnorderedAccessView* gpu_indirect_args_unified_uav;

    // Phase 4A: Spatial culling with BVH
    struct SlotAABB
    {
        Fvector3 aabb_min;          // Minimum corner of bounding box
        float padding0;             // Align to 16 bytes
        Fvector3 aabb_max;          // Maximum corner of bounding box
        float padding1;             // Align to 16 bytes
        u32 instance_base;          // First instance index in persistent buffer
        u32 instance_count;         // Number of instances in this slot
        int slot_x;                 // Grid X coordinate (for debugging)
        int slot_z;                 // Grid Z coordinate (for debugging)
        Fvector4 padding2;
    };
    static_assert(sizeof(SlotAABB) == 64, "SlotAABB must be 64 bytes for GPU alignment");

    xr_vector<SlotAABB> slot_aabbs;     // CPU copy of slot AABBs
    u32 slot_count;                     // Total number of slots with instances
    ID3DBuffer* slot_aabb_buffer;       // GPU buffer for slot AABBs
    ID3DShaderResourceView* slot_aabb_srv;  // SRV for slot AABB buffer

    // Phase 5: Interactive Grass System - Milestone 5.1: Interaction texture atlas
    ID3D11Texture2D* interaction_atlas;           // 2048x2048, stores displacement per slot
    ID3D11RenderTargetView* interaction_rtv;
    ID3D11ShaderResourceView* interaction_srv;
    ID3D11UnorderedAccessView* interaction_uav;
    u32 atlas_width;                              // Default: 2048
    u32 atlas_height;                             // Default: 2048
    u32 slot_texture_size;                        // Default: 32 (32x32 per slot)
    ID3DBuffer* slot_atlas_uv_buffer;             // GPU buffer for UV mapping
    ID3DShaderResourceView* slot_atlas_uv_srv;
    ID3D11SamplerState* interaction_sampler;      // Sampler for interaction/wind textures

    // Milestone 5.2: Entity tracking
    struct InteractiveEntity
    {
        Fvector position;
        float radius;              // Interaction radius
        Fvector velocity;          // Movement direction/speed
        float weight;              // 0-1, affects displacement strength
        Fvector direction;         // Entity facing direction (normalized)
        float padding;             // Align to 48 bytes
    };
    xr_vector<InteractiveEntity> interactive_entities;
    u32 max_entities;                              // Default: 256
    ID3DBuffer* entity_buffer;
    ID3DShaderResourceView* entity_srv;
    u32 entity_count_this_frame;

    // Milestone 5.3: Interaction compute shader
    ref_cs interaction_compute_shader;             // detail_interaction.cs
    ID3DBuffer* interaction_constant_buffer;

    // Phase 5 Persistence: Dirty bits buffer for GPU to mark modified pages
    ID3D11Buffer* page_dirty_bits_buffer;          // One uint per slot (0=clean, 1=dirty)
    ID3D11UnorderedAccessView* page_dirty_bits_uav;
    ID3D11ShaderResourceView* page_dirty_bits_srv;

    // Milestone 5.4: Wind system
    ID3D11Texture2D* wind_texture;                 // 512x512 FBM wind field
    ID3D11ShaderResourceView* wind_srv;
    ID3D11UnorderedAccessView* wind_uav;
    u32 wind_texture_size;                         // Default: 512
    ref_cs wind_compute_shader;                    // detail_wind_fbm.cs
    ID3DBuffer* wind_constant_buffer;
    Fvector2 wind_direction;                       // Current wind direction
    float wind_speed;                              // Current wind speed

    struct WindParams
    {
        float time;              // 0-3
        Fvector2 wind_direction; // 4-11
        float wind_speed;        // 12-15
        u32 octaves;             // 16-19
        float lacunarity;        // 20-23
        float gain;              // 24-27
        float padding1;          // 28-31 (align scroll_speed to 16-byte boundary)
        Fvector2 scroll_speed;   // 32-39
        u32 texture_size;        // 40-43
        u32 pad[3];              // 44-55
    };

    // Phase 6: Virtual texturing page table structures
    struct PageTableEntry {
        uint16_t physical_page;      // 0-4095 or 0xFFFF (not resident)
        uint8_t  mip_level;          // Reserved for future LOD (default: 0)
        uint8_t  reference_bit : 1;  // For Clock algorithm
        uint8_t  dirty_bit : 1;      // Needs writeback
        uint8_t  locked_bit : 1;     // In-flight upload, can't evict
        uint8_t  padding : 5;
        uint64_t last_access_frame;  // For age tracking
    };
    static_assert(sizeof(PageTableEntry) == 16, "PageTableEntry packing check");

    struct PhysicalPageInfo {
        uint32_t logical_slot;       // Which world slot occupies this page (UINT32_MAX = free)
        uint8_t  reference_bit;      // For Clock algorithm
        uint8_t  locked;             // Can't evict (in-flight upload)
        uint16_t padding;
    };
    static_assert(sizeof(PhysicalPageInfo) == 8, "PhysicalPageInfo packing check");

    // Phase 6: Virtual Texturing System
    static const uint16_t PHYSICAL_PAGES = 4096;       // 64×64 atlas slots
    static const uint16_t INVALID_PAGE = 0xFFFF;

    xr_vector<PageTableEntry> page_table;              // [slot_count] - dynamically sized
    std::array<PhysicalPageInfo, PHYSICAL_PAGES> physical_pages;
    uint16_t clock_hand;                                // Current position for Clock algorithm
    uint32_t resident_page_count;                       // How many pages currently in use

    // Indirection buffer (replaces slot_atlas_uvs)
    ID3DBuffer* indirection_buffer;
    ID3DShaderResourceView* indirection_srv;

    // Reverse mapping: physical page → logical slot (for compute shader)
    ID3DBuffer* physical_to_logical_buffer;
    ID3DShaderResourceView* physical_to_logical_srv;

    // Statistics
    struct PageTableStats {
        uint64_t total_requests;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t evictions;
        uint32_t current_resident;
    } page_table_stats;

    // Phase 6B: Visibility-driven page requests
    ID3D11Buffer* visible_slots_readback;
    ID3D11Query* readback_query;

    static const uint32_t MAX_VISIBLE_SLOTS = 8192;  // Conservative estimate
    xr_vector<uint32_t> visible_slots_cache;         // CPU copy of visible slots
    uint32_t visible_slot_count;                      // How many visible this frame

    // Upload queue for page promotion
    struct PageUploadRequest {
        uint32_t logical_slot;
        uint8_t priority;           // 0-255, higher = more urgent
        uint64_t request_frame;

        bool operator<(const PageUploadRequest& other) const {
            return priority < other.priority;  // Priority queue sorts by highest
        }
    };
    std::priority_queue<PageUploadRequest> upload_queue;
    uint32_t max_uploads_per_frame;  // Default: 16

    // GPU-side visible slot tracking
    ID3D11Buffer* visible_slot_ids_gpu;          // GPU output buffer
    ID3D11UnorderedAccessView* visible_slot_ids_uav;
    ID3D11Buffer* visible_slot_counter_gpu;      // Atomic counter
    ID3D11UnorderedAccessView* visible_slot_counter_uav;

    // Upload pipeline (data transfer CPU→GPU)
    static const uint32_t STAGING_BUFFER_SIZE = 2 * 1024 * 1024;  // 2 MB
    ID3D11Buffer* upload_staging_buffer;
    uint8_t* staging_buffer_mapped;  // Persistent map pointer

    // Phase 3 (Week 5-6): A-Life integration with deferred updates
    struct PendingInteractionUpdate {
        uint32_t logical_slot;        // 4 bytes
        Fvector world_position;       // 12 bytes (3 floats)
        float radius;                 // 4 bytes
        float strength;               // 4 bytes
        uint64_t timestamp;           // 8 bytes (8-byte aligned)
        uint8_t interaction_type;     // 1 byte
        uint8_t padding[7];           // Pad to 40 bytes (8-byte alignment)
    };
    static_assert(sizeof(PendingInteractionUpdate) == 40, "PendingInteractionUpdate size check");

    xr_vector<PendingInteractionUpdate> pending_updates;
    uint32_t max_pending_updates;    // Default: 4096

    // Thread-safe queue for A-Life requests from other threads
    struct ThreadSafeInteractionRequest {
        Fvector world_position;
        float radius;
        float strength;
        uint8_t type;
        uint8_t padding[3];
    };
    xr_vector<ThreadSafeInteractionRequest> threadsafe_request_queue;
    Lock threadsafe_queue_lock;

    // Priority thresholds
    static const uint8_t PRIORITY_VISIBLE = 255;      // Visible slots
    static const uint8_t PRIORITY_DIRTY = 128;        // Slots with pending updates
    static const uint8_t PRIORITY_PREFETCH = 64;      // Adjacent to visible
    static const uint8_t PRIORITY_ALIFE = 32;         // Generic A-Life activity

    // Statistics
    struct ALifeUpdateStats {
        uint64_t total_updates_requested;
        uint64_t immediate_updates;      // Slot was resident
        uint64_t deferred_updates;       // Slot not resident
        uint64_t applied_deferred;       // Deferred update later applied
        uint64_t expired_updates;        // Too old, discarded
    } alife_stats;

    // Interaction update compute shader
    ref_cs interaction_update_cs;

    struct InteractionUpdateCB {
        Fvector2 world_center;        // 8 bytes (2 floats)
        float radius;                 // 4 bytes
        float strength;               // 4 bytes
        uint32_t physical_page;       // 4 bytes
        float slot_size;              // 4 bytes
        float atlas_width;            // 4 bytes
        float slot_texture_size;      // 4 bytes
    };
    static_assert(sizeof(InteractionUpdateCB) == 32, "InteractionUpdateCB size check");

    ID3D11Buffer* interaction_update_cb;

    // GPU rendering shader (separate from vanilla)
    IBlender* b_detail_gpu{nullptr};    // GPU blender class
    ref_shader gpu_detail_shader;       // GPU shader (detail_gpu.vs + detail_gpu.ps)

    // Phase 5: Warm cache (RAM-based recently-accessed slots)
    struct WarmCacheEntry {
        uint32_t world_slot_id;          // Which logical slot
        uint64_t last_access_frame;      // For LRU eviction
        uint16_t dirty_flag;             // Needs writeback to disk?
        uint16_t compressed_size;        // Actual size of compressed_data
        uint8_t compressed_data[1024];   // BC3 compressed 32×32 RGBA (max size)
    };
    static_assert(sizeof(WarmCacheEntry) <= 1064, "WarmCacheEntry size check");

    static const uint32_t WARM_CACHE_SLOTS = 65536;  // 64K slots
    xr_vector<WarmCacheEntry> warm_cache;
    xr_map<uint32_t, uint32_t> world_to_warm_index;  // world_slot → warm_cache index
    xr_vector<uint32_t> warm_cache_free_list;        // Free slots in warm cache

    // Compression buffer (reused for compress/decompress)
    uint8_t* compression_work_buffer;  // 32×32×4 = 4KB uncompressed
    static const uint32_t UNCOMPRESSED_SLOT_SIZE = 32 * 32 * 4;  // RGBA

    // GPU readback (dirty pages from atlas → CPU)
    static const uint32_t MAX_READBACKS_PER_FRAME = 8;
    struct ReadbackRequest {
        uint32_t logical_slot;
        uint16_t physical_page;
        uint64_t request_frame;
    };
    xr_vector<ReadbackRequest> readback_queue;

    // Double-buffered readback staging
    ID3D11Buffer* readback_staging_buffers[2];
    uint32_t readback_buffer_index;
    ID3D11Query* readback_queries[2];

    // Track which slots were read back (for processing when data ready)
    struct PendingReadback {
        uint32_t logical_slot;
        uint16_t physical_page;
    };
    xr_vector<PendingReadback> pending_readbacks[2];  // One per staging buffer

    // GPU readback staging texture (texture→texture copy, then texture→buffer)
    ID3D11Texture2D* readback_staging_texture;

    // Phase 5: Disk persistence (simple file-based)
    string_path db_path;          // Path to database file

    // Background save thread
    std::thread save_thread;
    std::atomic<bool> save_thread_running;
    std::mutex save_mutex;
    xr_vector<uint32_t> slots_pending_save;

    // GPU shader constants (from gpu_detail_shader)
    ref_constant gpu_wind2; // dir2 for vis_id=2 (wave2)
    ref_constant gpu_detail_params;  // Phase 5: slot grid parameters (x_size, z_size, x_offs, z_offs)
    ref_constant gpu_grass_wind_displacement;  // Phase 5: wind displacement strength
    ref_constant gpu_grass_interaction_displacement;  // Phase 5: interaction displacement strength
    ref_constant g_wind_direction;  // Phase 6: Global wind direction (XY normalized) - matches compute shader

    // GPU-only function declarations
    void hw_Render_object(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 object_id);
    void hw_Render_FullLevel(CBackend& cmd_list);

    void DecompressAllSlots();
    void CreatePersistentInstanceBuffer();

    void CreateGPUCullingBuffers();
    void DispatchGPUCulling(CBackend& cmd_list);

    void ComputeSlotAABBs();
    void CreateSlotAABBBuffer();
    void DestroySlotAABBBuffer();
    void ValidateSlotAABBs();

    void CreateInteractionAtlas();
    void DestroyInteractionAtlas();
    void CreateEntityTrackingBuffers();
    void DestroyEntityTrackingBuffers();
    void UpdateInteractiveEntities(CBackend& cmd_list);
    void CreateWindTexture();
    void DestroyWindTexture();
    void RenderInteractions(CBackend& cmd_list);
    void UpdateWind(CBackend& cmd_list);

    void InitializePageTable();
    void ShutdownPageTable();
    void UpdatePageTable();  // Called each frame
    uint16_t RequestPage(uint32_t logical_slot, uint8_t priority);
    uint16_t FindVictimPage();
    void EvictPage(uint16_t physical_page);
    void PromotePage(uint32_t logical_slot, uint16_t physical_page);
    bool IsPageResident(uint32_t logical_slot) const;
    void UpdateIndirectionBuffer(CBackend& cmd_list);
    void ResetPageTableStats();
    void PrintPageTableStats();

    void InitializeVisibilityReadback();
    void ShutdownVisibilityReadback();
    void ReadVisibleSlotsFromGPU();
    void ProcessUploadQueue();
    void RequestVisiblePages();
    void RequestPageWithPriority(uint32_t logical_slot, uint8_t priority);

    void ProcessThreadSafeRequests(); // Called on render thread
    void ProcessPendingUpdates();
    void ApplyPendingUpdatesToSlot(uint32_t logical_slot);
    void ExpireOldPendingUpdates(uint64_t max_age_frames);
    void ApplyInteractionUpdateGPU(uint32_t physical_page, const Fvector& world_center, float radius, float strength);
    bool IsSlotDirty(uint32_t logical_slot) const;
    void ResetALifeStats();
    void PrintALifeStats();

    void InitializeWarmCache();
    void ShutdownWarmCache();
    bool LoadSlotFromWarmCache(uint32_t logical_slot, uint8_t* out_data);
    void SaveSlotToWarmCache(uint32_t logical_slot, const uint8_t* data, size_t size);
    void EvictFromWarmCache(uint32_t logical_slot);
    uint32_t FindWarmCacheLRUVictim();
    void CompressSlotData(const uint8_t* uncompressed, uint8_t* compressed, size_t* out_size);
    void DecompressSlotData(const uint8_t* compressed, size_t compressed_size, uint8_t* uncompressed);

    void InitializeReadbackPipeline();
    void ShutdownReadbackPipeline();
    void RequestSlotReadback(uint32_t logical_slot);
    void ProcessReadbackQueue();
    void ReadbackSlotFromGPU(uint32_t logical_slot, uint16_t physical_page);
    float HalfToFloat(uint16_t h);
    uint16_t FloatToHalf(float f);

    void InitializePersistence();
    void ShutdownPersistence();
    void SaveSlotToDisk(uint32_t logical_slot, const uint8_t* data, size_t size);
    bool LoadSlotFromDisk(uint32_t logical_slot, uint8_t* out_data, size_t* out_size);
    void BackgroundSaveThread();  // Thread function
    void QueueSlotForSave(uint32_t logical_slot);
    void FlushPendingSaves();
    void UploadSlotDataToGPU(uint32_t physical_page, const uint8_t* data);


#endif
    void RequestInteractionUpdate(const Fvector& world_pos, float radius, float strength, uint8_t type = 0); // Main thread only
    void RequestInteractionUpdateThreadSafe(const Fvector& world_pos, float radius, float strength, uint8_t type = 0); // Thread-safe version

    // Phase 6: Bezier curve blademesh generation
    struct BladeVertex
    {
        Fvector pos;         // Local position (Y-up)
        Fvector2 uv;         // Texture coordinates
        float t;             // Height parameter (0=base, 1=tip) for AO
        float width_scale;   // Width at this height (for debugging)
    };
    void CreateSDF_BladeGeometry();
    void GenerateGrassBlade(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments = 6);

    ref_geom blade_geom;                // Bezier curve bladegeometry
    VertexStagingBuffer blade_vb;       // Blade vertex buffer
    IndexStagingBuffer blade_ib;        // Blade index buffer
    u32 blade_vertex_count;             // Number of vertices in blade mesh
    u32 blade_index_count;              // Number of indices in blade mesh

    CDetailManager();
    virtual ~CDetailManager();
};
} // namespace xray::render::fg
