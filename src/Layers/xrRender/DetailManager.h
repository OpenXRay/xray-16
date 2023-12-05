// DetailManager.h: interface for the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef DetailManagerH
#define DetailManagerH
#pragma once

#include "xrCore/xrPool.h"
#include "DetailFormat.h"
#include "DetailModel.h"

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
        float distance;
        Fvector position;
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
    typedef xr_vector<CDetail*> DetailVec;
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
#if defined(USE_DX9)
    void hw_Render_dump(CBackend& cmd_list, ref_constant array, u32 var_id, u32 lod_id, u32 c_base);
#elif defined(USE_DX11) || defined(USE_OGL)
    void hw_Render_dump(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id);
#if RENDER == R_R4
    void draw(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id);
    void draw_instances(CBackend& cmd_list, const Fvector4& consts, const Fvector4& wave, const Fvector4& wind, u32 var_id, u32 lod_id);
    Fvector4 *upload_buffer[R__NUM_CONTEXTS];
    ref_texture t_draw_matrices;
#endif
#else
#   error No graphics API selected or enabled!
#endif

    // get unpacked slot
    DetailSlot& QueryDB(int sx, int sz);

    void cache_Initialize();
    void cache_Update(int sx, int sz, Fvector& view, int limit);
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
    Lock MT;
    volatile u32 m_frame_calc;
    volatile u32 m_frame_rendered;

    void MT_CALC();
    ICF void MT_SYNC()
    {
        if (m_frame_calc == Device.dwFrame)
            return;

        MT_CALC();
    }

    CDetailManager();
    virtual ~CDetailManager();
};

#endif // DetailManagerH
