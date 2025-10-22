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
    ID3DShaderResourceView* detailSRV_vis[3];
    ID3DBuffer* detailBuffer_vis[3];
    u32 detailBufferSize_vis[3];

    ref_geom vis_unified_geom[3];
    VertexStagingBuffer vis_unified_VB[3];
    IndexStagingBuffer vis_unified_IB[3];
    xr_vector<u32> vis_geometry_vertex_offsets[3];
    xr_vector<u32> vis_geometry_index_offsets[3];
    xr_vector<u32> vis_object_indices[3];
    u32 vis_total_vertices[3];
    u32 vis_total_indices[3];

    struct SlotItemWithObject
    {
        SlotItem item;
        u32 object_id;
    };
    struct InstanceData
    {
        Fvector m0;
        float scale;
        Fvector m1;
        float hemi;
        Fvector m2;
        u32 vis_id;
        Fvector pos;
        u32 object_id;
    };
    xr_vector<SlotItemWithObject> all_level_instances;
    u32 total_instance_count;
    bool full_level_loaded;

    ID3DBuffer* persistent_instance_buffer;
    ID3DShaderResourceView* persistent_instance_srv;
    u32 persistent_buffer_capacity;

    ref_cs cull_compute_shader;
    ID3DBuffer* cull_constant_buffer;

    static const u32 max_gpu_culled_objects = 16;
    ID3DBuffer* gpu_visible_buffers[max_gpu_culled_objects];
    ID3DShaderResourceView* gpu_visible_srvs[max_gpu_culled_objects];
    ID3DUnorderedAccessView* gpu_visible_uavs[max_gpu_culled_objects];
    u32 gpu_visible_buffer_capacity;

    ID3DBuffer* gpu_visible_counts_buffer;
    ID3DUnorderedAccessView* gpu_visible_counts_uav;
    ID3DBuffer* gpu_visible_counts_readback;

    ID3DBuffer* gpu_indirect_args[max_gpu_culled_objects];
    ID3DUnorderedAccessView* gpu_indirect_args_uavs[max_gpu_culled_objects];

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
        float padding[2];
    };
    xr_vector<InteractiveEntity> interactive_entities;
    u32 max_entities;
    ID3DBuffer* entity_buffer;
    ID3DShaderResourceView* entity_srv;
    u32 entity_count_this_frame;

    ref_cs interaction_compute_shader;
    ID3DBuffer* interaction_constant_buffer;

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

#endif

    ref_constant hwc_consts;
    ref_constant hwc_wave;
    ref_constant hwc_wind;
    ref_constant hwc_wind2;
    ref_constant hwc_array;
    ref_constant hwc_s_consts;
    ref_constant hwc_s_xform;
    ref_constant hwc_s_array;
    ref_constant hwc_detail_params;
    ref_constant hwc_grass_wind_displacement;
    void hw_Load();
    void hw_Load_Geom();
    void hw_Load_Shaders();
    void hw_Unload();
    void hw_Render(CBackend& cmd_list);
    void hw_Render_dump(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id);
    void hw_Render_object(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 object_id);

#ifdef USE_DX11
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
    void UpdateInteractiveEntities();
    void CreateWindTexture();
    void DestroyWindTexture();
    void RenderInteractions(CBackend& cmd_list);
    void UpdateWind(CBackend& cmd_list);
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
