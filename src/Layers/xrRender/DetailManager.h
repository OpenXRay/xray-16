// DetailManager.h: interface for the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DetailFormat.h"
#include "DetailModel.h"

namespace xray::render::RENDER_NAMESPACE
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
    size_t hw_BatchSize;

    ref_geom hw_Geom;
    VertexStagingBuffer hw_VB;
    IndexStagingBuffer hw_IB;

#ifdef USE_DX11
    // Structured buffers for instancing (one per vis_id: still, wave1, wave2)
    // Phase 1, Milestone 1.1: Organize buffers by vis_id instead of size
    ID3DShaderResourceView* detailSRV_vis[3];
    ID3DBuffer* detailBuffer_vis[3];
    u32 detailBufferSize_vis[3];  // Track allocated size for each buffer

    // Phase 1, Milestone 1.2: Unified geometry per vis_id
    ref_geom vis_unified_geom[3];  // One unified geometry per vis_id
    VertexStagingBuffer vis_unified_VB[3];  // Persistent VBs for unified geometry
    IndexStagingBuffer vis_unified_IB[3];   // Persistent IBs for unified geometry
    xr_vector<u32> vis_geometry_vertex_offsets[3];  // Per-object vertex offsets within unified buffer
    xr_vector<u32> vis_geometry_index_offsets[3];   // Per-object index offsets within unified buffer
    xr_vector<u32> vis_object_indices[3];           // Map from local object index to global object index
    u32 vis_total_vertices[3];     // Total vertices in unified buffer
    u32 vis_total_indices[3];      // Total indices in unified buffer

    // Phase 2.0: Full level decompression
    struct SlotItemWithObject
    {
        SlotItem item;      // Original slot item data
        u32 object_id;      // Which grass object type (0-63)
    };
    // Instance data structure matching shader
    struct InstanceData
    {
        Fvector m0;  // First column of rotation matrix (X-axis)
        float scale;
        Fvector m1;  // Second column of rotation matrix (Y-axis)
        float hemi;
        Fvector m2;  // Third column of rotation matrix (Z-axis)
        u32 vis_id;
        Fvector pos; // Position
        u32 object_id;
    };
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

    // Per-object visible instance buffers (output from compute shader)
    static const u32 max_gpu_culled_objects = 16;  // Hardcoded for first 16 objects
    ID3DBuffer* gpu_visible_buffers[max_gpu_culled_objects];
    ID3DShaderResourceView* gpu_visible_srvs[max_gpu_culled_objects];
    ID3DUnorderedAccessView* gpu_visible_uavs[max_gpu_culled_objects];
    u32 gpu_visible_buffer_capacity;  // Max instances per object buffer

    // Counter buffer (one u32 per object type)
    ID3DBuffer* gpu_visible_counts_buffer;
    ID3DUnorderedAccessView* gpu_visible_counts_uav;
    ID3DBuffer* gpu_visible_counts_readback;  // CPU readback for counts

    // Phase 2.2: Indirect draw args buffers (one per object)
    ID3DBuffer* gpu_indirect_args[max_gpu_culled_objects];
    ID3DUnorderedAccessView* gpu_indirect_args_uavs[max_gpu_culled_objects];

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

    xr_vector<SlotAABB> slot_aabbs;     // CPU copy of slot AABBs
    u32 slot_count;                     // Total number of slots with instances
    ID3DBuffer* slot_aabb_buffer;       // GPU buffer for slot AABBs
    ID3DShaderResourceView* slot_aabb_srv;  // SRV for slot AABB buffer

    // Phase 5: Interactive Grass System
    // Milestone 5.1: Interaction texture atlas
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
        float padding[2];          // Align to 32 bytes
    };
    xr_vector<InteractiveEntity> interactive_entities;
    u32 max_entities;                              // Default: 256
    ID3DBuffer* entity_buffer;
    ID3DShaderResourceView* entity_srv;
    u32 entity_count_this_frame;

    // Milestone 5.3: Interaction compute shader
    ref_cs interaction_compute_shader;             // detail_interaction.cs
    ID3DBuffer* interaction_constant_buffer;

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

#endif

    ref_constant hwc_consts;
    ref_constant hwc_wave;
    ref_constant hwc_wind;  // dir1 for vis_id=1 (wave1)
    ref_constant hwc_wind2; // dir2 for vis_id=2 (wave2)
    ref_constant hwc_array;
    ref_constant hwc_s_consts;
    ref_constant hwc_s_xform;
    ref_constant hwc_s_array;
    ref_constant hwc_detail_params;  // Phase 5: slot grid parameters (x_size, z_size, x_offs, z_offs)
    ref_constant hwc_grass_wind_displacement;  // Phase 5: wind displacement strength
    ref_constant hwc_grass_interaction_displacement;  // Phase 5: interaction displacement strength
    void hw_Load();
    void hw_Load_Geom();
    void hw_Load_Shaders();
    void hw_Unload();
    void hw_Render(CBackend& cmd_list);
    void hw_Render_dump(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id);
    void hw_Render_object(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 object_id);

#ifdef USE_DX11
    // Phase 2.0.4: Render from full level decompression
    void hw_Render_FullLevel(CBackend& cmd_list);
#endif

    // get unpacked slot
    DetailSlot& QueryDB(int sx, int sz);

    void cache_Initialize();
    void cache_Update(int sx, int sz, Fvector& view);
    void cache_Task(int gx, int gz, Slot* D);
    Slot* cache_Query(int sx, int sz);
    void cache_Decompress(Slot* D);
    BOOL cache_Validate();

#ifdef USE_DX11
    // Phase 2.0: Full level decompression
    void DecompressAllSlots();
    void CreatePersistentInstanceBuffer();

    // Phase 2.1: GPU culling
    void CreateGPUCullingBuffers();
    void DispatchGPUCulling(CBackend& cmd_list);

    // Phase 4A: BVH spatial culling
    void ComputeSlotAABBs();
    void CreateSlotAABBBuffer();
    void DestroySlotAABBBuffer();
    void ValidateSlotAABBs();

    // Phase 5: Interactive Grass System
    void CreateInteractionAtlas();
    void DestroyInteractionAtlas();
    void CreateEntityTrackingBuffers();
    void DestroyEntityTrackingBuffers();
    void UpdateInteractiveEntities(CBackend& cmd_list);
    void CreateWindTexture();
    void DestroyWindTexture();
    void RenderInteractions(CBackend& cmd_list);
    void UpdateWind(CBackend& cmd_list);

    // Phase 6: Virtual texturing management
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

    // Phase 6B: Visibility integration
    void InitializeVisibilityReadback();
    void ShutdownVisibilityReadback();
    void ReadVisibleSlotsFromGPU();
    void ProcessUploadQueue();
    void RequestVisiblePages();
    void RequestPageWithPriority(uint32_t logical_slot, uint8_t priority);
#endif
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

    CDetailManager();
    virtual ~CDetailManager();
};
} // namespace xray::render::RENDER_NAMESPACE
