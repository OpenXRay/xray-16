#pragma once

#include "xrCore/xrPool.h"
#include "Layers/xrRender/DetailFormat.h"
#include "Layers/xrRender/DetailModel.h"

namespace xray::render::fg
{
#ifdef _EDITOR
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
const int dm_cache1_count = 4;
const int dm_max_objects = 64;
const int dm_obj_in_slot = 4;
const float dm_slot_size = DETAIL_SLOT_SIZE;

constexpr auto dm_max_cache_size = 62001 * 2;
extern u32 dm_size;
extern u32 dm_cache1_line;
extern u32 dm_cache_line;
extern u32 dm_cache_size;
extern float dm_fade;
extern u32 dm_current_size;
extern u32 dm_current_cache1_line;
extern u32 dm_current_cache_line;
extern u32 dm_current_cache_size;
extern float dm_current_fade;
extern float ps_current_detail_density;
extern float ps_current_detail_height;

class ECORE_API CDetailManager
{
public:

    float fade_distance = 99999;
    Fvector light_position;
    void details_clear();

    struct SlotItem
    {
        float scale;
        float scale_calculated;
        Fmatrix mRotY;
        u32 vis_ID;
        float c_hemi;
        float c_sun;
#if RENDER == R_R1
        Fvector c_rgb;
#endif
    };

    using SlotItemVec = xr_vector<SlotItem*>;

    struct SlotPart
    {
        u32 id;
        SlotItemVec items;
        SlotItemVec r_items[3];
    };

    enum SlotType : u32
    {
        stReady = 0,
        stPending,
    };

    struct Slot
    {
        struct
        {
            u32 empty : 1;
            u32 type : 1;
            u32 frame : 30;
        };
        int sx, sz;
        vis_data vis;
        SlotPart G[dm_obj_in_slot];

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
    DetailSlot* dtSlots;
    DetailSlot DS_empty;

    DetailVec objects;
    vis_list m_visibles[3];

#ifndef _EDITOR
    xrXRC xrc;
#endif
    CacheSlot1** cache_level1;
    Slot*** cache;
    svector<Slot*, dm_max_cache_size> cache_task;
    Slot* cache_pool;

    int cache_cx;
    int cache_cz;

    PSS poolSI;

    Fvector EYE;

    void UpdateVisibleM();
    void UpdateVisibleS();

#ifdef _EDITOR
    virtual ObjectList* GetSnapList() = 0;
#endif

    bool UseVS() const;

    ref_geom soft_Geom;
    void soft_Load();
    void soft_Unload();
    void soft_Render();

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

    DetailSlot& QueryDB(int sx, int sz);

    void cache_Initialize();
    void cache_Update(int sx, int sz, Fvector& view);
    void cache_Task(int gx, int gz, Slot* D);
    Slot* cache_Query(int sx, int sz);
    void cache_Decompress(Slot* D);
    BOOL cache_Validate();

    int cg2w_X(int x) { return cache_cx - dm_size + x; }
    int cg2w_Z(int z) { return cache_cz - dm_size + (dm_cache_line - 1 - z); }
    int w2cg_X(int x) { return x - cache_cx + dm_size; }
    int w2cg_Z(int z) { return cache_cz - dm_size + (dm_cache_line - 1 - z); }

    void Load();
    void Unload();
    void Render(CBackend& cmd_list);

    Task* m_calc_task{};

    void DispatchMTCalc();

    ref_geom vis_unified_geom[3];
    VertexStagingBuffer vis_unified_VB[3];
    IndexStagingBuffer vis_unified_IB[3];
    xr_vector<u32> vis_geometry_vertex_offsets[3];
    xr_vector<u32> vis_geometry_index_offsets[3];
    xr_vector<u32> vis_object_indices[3];
    u32 vis_total_vertices[3];
    u32 vis_total_indices[3];

#ifdef USE_DX11
    ID3DShaderResourceView* detailSRV_vis[3];
    ID3DBuffer* detailBuffer_vis[3];
    u32 detailBufferSize_vis[3];

    struct SlotItemWithObject
    {
        SlotItem item;
        u32 object_id;
    };

    struct InstanceData
    {
        Fvector pos;
        float scale;
        float hemi;
        u32 vis_id;
        u32 object_id;
        float padding;
    };
    static_assert(sizeof(InstanceData) == 32, "InstanceData must be 32 bytes");

    xr_vector<SlotItemWithObject> all_level_instances;
    u32 total_instance_count;
    bool full_level_loaded;

    ID3DBuffer* persistent_instance_buffer;
    ID3DShaderResourceView* persistent_instance_srv;
    u32 persistent_buffer_capacity;

    ref_cs cull_compute_shader;
    ID3DBuffer* cull_constant_buffer;

    ID3DBuffer* gpu_visible_unified_buffer;
    ID3DShaderResourceView* gpu_visible_unified_srv;
    ID3DUnorderedAccessView* gpu_visible_unified_uav;
    u32 gpu_visible_buffer_capacity;

    ID3DBuffer* gpu_visible_count_buffer;
    ID3DUnorderedAccessView* gpu_visible_count_uav;
    ID3DBuffer* gpu_visible_count_readback;

    ID3DBuffer* gpu_indirect_args_unified;
    ID3DUnorderedAccessView* gpu_indirect_args_unified_uav;

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
    static_assert(sizeof(SlotAABB) == 64, "SlotAABB must be 64 bytes for GPU alignment");

    xr_vector<SlotAABB> slot_aabbs;
    u32 slot_count;
    ID3DBuffer* slot_aabb_buffer;
    ID3DShaderResourceView* slot_aabb_srv;

    ID3D11Texture2D* interaction_atlas;
    ID3D11RenderTargetView* interaction_rtv;
    ID3D11ShaderResourceView* interaction_srv;
    ID3D11UnorderedAccessView* interaction_uav;
    u32 atlas_width;
    u32 atlas_height;
    u32 slot_texture_size;
    ID3DBuffer* slot_atlas_uv_buffer;
    ID3DShaderResourceView* slot_atlas_uv_srv;
    ID3D11SamplerState* interaction_sampler;

    struct InteractiveEntity
    {
        Fvector position;
        float radius;
        Fvector velocity;
        float weight;
        Fvector direction;
        float padding;
    };
    xr_vector<InteractiveEntity> interactive_entities;
    u32 max_entities;
    ID3DBuffer* entity_buffer;
    ID3DShaderResourceView* entity_srv;
    u32 entity_count_this_frame;

    ref_cs interaction_compute_shader;
    ID3DBuffer* interaction_constant_buffer;

    ID3D11Buffer* page_dirty_bits_buffer;
    ID3D11UnorderedAccessView* page_dirty_bits_uav;
    ID3D11ShaderResourceView* page_dirty_bits_srv;

    ID3D11Texture2D* wind_texture;
    ID3D11ShaderResourceView* wind_srv;
    ID3D11UnorderedAccessView* wind_uav;
    u32 wind_texture_size;
    ref_cs wind_compute_shader;
    ID3DBuffer* wind_constant_buffer;
    Fvector2 wind_direction;
    float wind_speed;

    struct WindParams
    {
        float time;
        Fvector2 wind_direction;
        float wind_speed;
        u32 octaves;
        float lacunarity;
        float gain;
        float padding1;
        Fvector2 scroll_speed;
        u32 texture_size;
        u32 pad[3];
    };

    struct PageTableEntry {
        uint16_t physical_page;
        uint8_t  mip_level;
        uint8_t  reference_bit : 1;
        uint8_t  dirty_bit : 1;
        uint8_t  locked_bit : 1;
        uint8_t  padding : 5;
        uint64_t last_access_frame;
    };
    static_assert(sizeof(PageTableEntry) == 16, "PageTableEntry packing check");

    struct PhysicalPageInfo {
        uint32_t logical_slot;
        uint8_t  reference_bit;
        uint8_t  locked;
        uint16_t padding;
    };
    static_assert(sizeof(PhysicalPageInfo) == 8, "PhysicalPageInfo packing check");

    static const uint16_t PHYSICAL_PAGES = 4096;
    static const uint16_t INVALID_PAGE = 0xFFFF;

    xr_vector<PageTableEntry> page_table;
    std::array<PhysicalPageInfo, PHYSICAL_PAGES> physical_pages;
    uint16_t clock_hand;
    uint32_t resident_page_count;

    ID3DBuffer* indirection_buffer;
    ID3DShaderResourceView* indirection_srv;

    ID3DBuffer* physical_to_logical_buffer;
    ID3DShaderResourceView* physical_to_logical_srv;

    struct PageTableStats {
        uint64_t total_requests;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t evictions;
        uint32_t current_resident;
    } page_table_stats;

    ID3D11Buffer* visible_slots_readback;
    ID3D11Query* readback_query;

    static const uint32_t MAX_VISIBLE_SLOTS = 8192;
    xr_vector<uint32_t> visible_slots_cache;
    uint32_t visible_slot_count;

    struct PageUploadRequest {
        uint32_t logical_slot;
        uint8_t priority;
        uint64_t request_frame;

        bool operator<(const PageUploadRequest& other) const {
            return priority < other.priority;
        }
    };
    std::priority_queue<PageUploadRequest> upload_queue;
    uint32_t max_uploads_per_frame;

    ID3D11Buffer* visible_slot_ids_gpu;
    ID3D11UnorderedAccessView* visible_slot_ids_uav;
    ID3D11Buffer* visible_slot_counter_gpu;
    ID3D11UnorderedAccessView* visible_slot_counter_uav;

    static const uint32_t STAGING_BUFFER_SIZE = 2 * 1024 * 1024;
    ID3D11Buffer* upload_staging_buffer;
    uint8_t* staging_buffer_mapped;

    struct PendingInteractionUpdate {
        uint32_t logical_slot;
        Fvector world_position;
        float radius;
        float strength;
        uint64_t timestamp;
        uint8_t interaction_type;
        uint8_t padding[7];
    };
    static_assert(sizeof(PendingInteractionUpdate) == 40, "PendingInteractionUpdate size check");

    xr_vector<PendingInteractionUpdate> pending_updates;
    uint32_t max_pending_updates;

    struct ThreadSafeInteractionRequest {
        Fvector world_position;
        float radius;
        float strength;
        uint8_t type;
        uint8_t padding[3];
    };
    xr_vector<ThreadSafeInteractionRequest> threadsafe_request_queue;
    Lock threadsafe_queue_lock;

    static const uint8_t PRIORITY_VISIBLE = 255;
    static const uint8_t PRIORITY_DIRTY = 128;
    static const uint8_t PRIORITY_PREFETCH = 64;
    static const uint8_t PRIORITY_ALIFE = 32;

    struct ALifeUpdateStats {
        uint64_t total_updates_requested;
        uint64_t immediate_updates;
        uint64_t deferred_updates;
        uint64_t applied_deferred;
        uint64_t expired_updates;
    } alife_stats;

    ref_cs interaction_update_cs;

    struct InteractionUpdateCB {
        Fvector2 world_center;
        float radius;
        float strength;
        uint32_t physical_page;
        float slot_size;
        float atlas_width;
        float slot_texture_size;
    };
    static_assert(sizeof(InteractionUpdateCB) == 32, "InteractionUpdateCB size check");

    ID3D11Buffer* interaction_update_cb;

    IBlender* b_detail_gpu{nullptr};
    ref_shader gpu_detail_shader;

    struct WarmCacheEntry {
        uint32_t world_slot_id;
        uint64_t last_access_frame;
        uint16_t dirty_flag;
        uint16_t compressed_size;
        uint8_t compressed_data[1024];
    };
    static_assert(sizeof(WarmCacheEntry) <= 1064, "WarmCacheEntry size check");

    static const uint32_t WARM_CACHE_SLOTS = 65536;
    xr_vector<WarmCacheEntry> warm_cache;
    xr_map<uint32_t, uint32_t> world_to_warm_index;
    xr_vector<uint32_t> warm_cache_free_list;

    uint8_t* compression_work_buffer;
    static const uint32_t UNCOMPRESSED_SLOT_SIZE = 32 * 32 * 4;

    static const uint32_t MAX_READBACKS_PER_FRAME = 8;
    struct ReadbackRequest {
        uint32_t logical_slot;
        uint16_t physical_page;
        uint64_t request_frame;
    };
    xr_vector<ReadbackRequest> readback_queue;

    ID3D11Buffer* readback_staging_buffers[2];
    uint32_t readback_buffer_index;
    ID3D11Query* readback_queries[2];

    struct PendingReadback {
        uint32_t logical_slot;
        uint16_t physical_page;
    };
    xr_vector<PendingReadback> pending_readbacks[2];

    ID3D11Texture2D* readback_staging_texture;

    string_path db_path;

    std::thread save_thread;
    std::atomic<bool> save_thread_running;
    std::mutex save_mutex;
    xr_vector<uint32_t> slots_pending_save;

    ref_constant gpu_wind2;
    ref_constant gpu_detail_params;
    ref_constant gpu_grass_wind_displacement;
    ref_constant gpu_grass_interaction_displacement;
    ref_constant g_wind_direction;

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
    void UpdatePageTable();
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

    void ProcessThreadSafeRequests();
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
    void BackgroundSaveThread();
    void QueueSlotForSave(uint32_t logical_slot);
    void FlushPendingSaves();
    void UploadSlotDataToGPU(uint32_t physical_page, const uint8_t* data);


#endif
    void RequestInteractionUpdate(const Fvector& world_pos, float radius, float strength, uint8_t type = 0);
    void RequestInteractionUpdateThreadSafe(const Fvector& world_pos, float radius, float strength, uint8_t type = 0);

    struct BladeVertex
    {
        Fvector pos;
        Fvector2 uv;
        float t;
        float width_scale;
    };
    void CreateSDF_BladeGeometry();
    void GenerateGrassBlade(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments = 6);

    ref_geom blade_geom;
    VertexStagingBuffer blade_vb;
    IndexStagingBuffer blade_ib;
    u32 blade_vertex_count;
    u32 blade_index_count;

    CDetailManager();
    virtual ~CDetailManager();
};
}
