#pragma once
#ifndef SoundH
#define SoundH

#include "Common/Platform.hpp"
#include "xrCore/_types.h"
#include "xrCore/_flags.h"
#include "xrCore/xr_resource.h"
#include "xrCore/_vector3d.h"
#include "xrCommon/xr_vector.h" // DEFINE_VECTOR

#ifdef XRSOUND_EXPORTS
#define XRSOUND_API XR_EXPORT
#else
#define XRSOUND_API XR_IMPORT
#endif

#ifdef __BORLANDC__
#define XRSOUND_EDITOR_API XRSOUND_API

// editor only refs
class XRSOUND_EDITOR_API SoundEnvironment_LIB;
#else
#define XRSOUND_EDITOR_API
#endif

#define SNDENV_FILENAME "sEnvironment.xr"
#define OGG_COMMENT_VERSION 0x0003

// refs
class IGameObject;
class XRSOUND_API CSound_params;
class XRSOUND_API CSound_source;
class XRSOUND_API CSound_emitter;
class XRSOUND_API CSound_stream_interface;
class XRSOUND_API CSound_environment;
struct xr_token;
class IReader;
template <class T>
struct _vector2;
using Fvector2 = _vector2<float>;

XRSOUND_API extern u32 psSoundModel;
XRSOUND_API extern float psSoundVEffects;
XRSOUND_API extern float psSoundVFactor;
XRSOUND_API extern float psSoundVMusic;
XRSOUND_API extern float psSoundRolloff;
XRSOUND_API extern float psSoundOcclusionScale;
XRSOUND_API extern Flags32 psSoundFlags;
XRSOUND_API extern int psSoundTargets;
XRSOUND_API extern int psSoundCacheSizeMB;
XRSOUND_API extern xr_token* snd_devices_token;
XRSOUND_API extern u32 snd_device_id;

// Flags
enum
{
    ss_Hardware = (1ul << 1ul), //!< Use hardware mixing only
    ss_EAX = (1ul << 2ul), //!< Use eax
    ss_forcedword = u32(-1)
};

enum
{
    sq_DEFAULT,
    sq_NOVIRT,
    sq_LIGHT,
    sq_HIGH,
    sq_forcedword = u32(-1)
};
enum
{
    sg_Undefined = 0,
    sg_SourceType = u32(-1),
    sg_forcedword = u32(-1),
};
enum
{
    sm_Looped = (1ul << 0ul), //!< Looped
    sm_2D = (1ul << 1ul), //!< 2D mode
    sm_forcedword = u32(-1),
};
enum esound_type
{
    st_Effect = 0,
    st_Music = 1,
    st_forcedword = u32(-1),
};

class CSound_UserDataVisitor;

class CSound_UserData : public xr_resource
{
public:
    virtual ~CSound_UserData() {}
    virtual void accept(CSound_UserDataVisitor*) = 0;
    virtual void invalidate() = 0;
};
typedef resptr_core<CSound_UserData, resptr_base<CSound_UserData>> CSound_UserDataPtr;

class ref_sound_data : public xr_resource
{
public:
    //shared_str nm;
    CSound_source* handle; //!< Pointer to wave-source interface
    CSound_emitter* feedback; //!< Pointer to emitter, automaticaly clears on emitter-stop
    esound_type s_type;
    int g_type; //!< Sound type, usually for AI
    IGameObject* g_object; //!< Game object that emitts ref_sound
    CSound_UserDataPtr g_userdata;
    shared_str fn_attached[2];

    u32 dwBytesTotal;
    float fTimeTotal;

public:
    ref_sound_data() throw();
    ref_sound_data(LPCSTR fName, esound_type sound_type, int game_type);
    virtual ~ref_sound_data();
    float get_length_sec() const { return fTimeTotal; };
};
typedef resptr_core<ref_sound_data, resptr_base<ref_sound_data>> ref_sound_data_ptr;
/*! \class ref_sound
\brief Sound source + control

The main class respresenting source/emitter interface
This class infact just hides internals and redirect calls to
specific sub-systems
*/
struct ref_sound
{
    ref_sound_data_ptr _p;

public:
    ref_sound() {}
    ~ref_sound() {}
    CSound_source* _handle() const;
    CSound_emitter* _feedback();
    IGameObject* _g_object();
    int _g_type();
    esound_type _sound_type();
    CSound_UserDataPtr _g_userdata();

    void create(LPCSTR name, esound_type sound_type, int game_type);
    void attach_tail(LPCSTR name);

    void clone(const ref_sound& from, esound_type sound_type, int game_type);

    void destroy();

    void play(IGameObject* O, u32 flags = 0, float delay = 0.f);
    void play_at_pos(IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f);
    void play_no_feedback(IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = 0, float* vol = 0,
        float* freq = 0, Fvector2* range = 0);

    void stop();
    void stop_deferred();
    void set_position(const Fvector& pos);
    void set_frequency(float freq);
    void set_range(float min, float max);
    void set_volume(float vol);
    void set_priority(float vol);

    const CSound_params* get_params();
    void set_params(CSound_params* p);
    float get_length_sec() const { return _p ? _p->get_length_sec() : 0.0f; };
};

/// definition (Sound Source)
class XRSOUND_API CSound_source
{
public:
    virtual float length_sec() const = 0;
    virtual u32 game_type() const = 0;
    virtual LPCSTR file_name() const = 0;
    virtual u16 channels_num() const = 0;
    virtual u32 bytes_total() const = 0;
};

/// definition (Sound Source)
class XRSOUND_API CSound_environment
{
public:
};

/// definition (Sound Params)
class XRSOUND_API CSound_params
{
public:
    Fvector position;
    float base_volume;
    float volume;
    float freq;
    float min_distance;
    float max_distance;
    float max_ai_distance;
};

/// definition (Sound Interface)
class XRSOUND_API CSound_emitter
{
public:
    virtual BOOL is_2D() = 0;
    virtual void switch_to_2D() = 0;
    virtual void switch_to_3D() = 0;
    virtual void set_position(const Fvector& pos) = 0;
    virtual void set_frequency(float freq) = 0;
    virtual void set_range(float min, float max) = 0;
    virtual void set_volume(float vol) = 0;
    virtual void set_priority(float vol) = 0;
    virtual void stop(BOOL bDeffered) = 0;
    virtual const CSound_params* get_params() = 0;
    virtual u32 play_time() = 0;
};

/// definition (Sound Stream Interface)
class XRSOUND_API CSound_stream_interface
{
public:
};

/// definition (Sound Stream Interface)
class XRSOUND_API CSound_stats
{
public:
    u32 _rendered;
    u32 _simulated;
    u32 _cache_hits;
    u32 _cache_misses;
    u32 _events;
};

class XRSOUND_API CSound_stats_ext
{
public:
    struct SItem
    {
        shared_str name;
        CSound_params params;
        float volume;
        esound_type type;
        int game_type;
        IGameObject* game_object;
        struct
        {
            u32 _3D : 1;
            u32 _rendered : 1;
        };
    };
    DEFINE_VECTOR(SItem, item_vec, item_vec_it);
    item_vec items;

public:
    void clear() { items.clear(); }
    void append(const SItem& itm) { items.push_back(itm); }
};

/// definition (Sound Callback)
typedef void __stdcall sound_event(ref_sound_data_ptr S, float range);

namespace CDB
{
class MODEL;
}

/// definition (Sound Manager Interface)
// XXX tamlin: Tag NOVTABLE ?
class XRSOUND_API CSound_manager_interface
{
    virtual void _initialize() = 0;
    virtual void _clear() = 0;

protected:
    friend class ref_sound_data;
    virtual void _create_data(ref_sound_data& S, LPCSTR fName, esound_type sound_type, int game_type) = 0;
    virtual void _destroy_data(ref_sound_data& S) = 0;

public:
    virtual ~CSound_manager_interface() {}
    static void _create();
    static void _destroy();

    virtual void _restart() = 0;
    virtual BOOL i_locked() = 0;

    virtual void create(ref_sound& S, LPCSTR fName, esound_type sound_type, int game_type) = 0;
    virtual void attach_tail(ref_sound& S, LPCSTR fName) = 0;
    virtual void clone(ref_sound& S, const ref_sound& from, esound_type sound_type, int game_type) = 0;
    virtual void destroy(ref_sound& S) = 0;
    virtual void stop_emitters() = 0;
    virtual int pause_emitters(bool val) = 0;

    virtual void play(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f) = 0;
    virtual void play_at_pos(ref_sound& S, IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f) = 0;
    virtual void play_no_feedback(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = 0,
        float* vol = 0, float* freq = 0, Fvector2* range = 0) = 0;

    virtual void set_master_volume(float f = 1.f) = 0;
    virtual void set_geometry_env(IReader* I) = 0;
    virtual void set_geometry_som(IReader* I) = 0;
    virtual void set_geometry_occ(CDB::MODEL* M) = 0;
    virtual void set_handler(sound_event* E) = 0;

    virtual void update(const Fvector& P, const Fvector& D, const Fvector& N) = 0;
    virtual void statistic(CSound_stats* s0, CSound_stats_ext* s1) = 0;
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) = 0;

    virtual float get_occlusion_to(const Fvector& hear_pt, const Fvector& snd_pt, float dispersion = 0.2f) = 0;

    virtual void object_relcase(IGameObject* obj) = 0;
    virtual const Fvector& listener_position() = 0;
#ifdef __BORLANDC__
    virtual SoundEnvironment_LIB* get_env_library() = 0;
    virtual void refresh_env_library() = 0;
    virtual void set_user_env(CSound_environment* E) = 0;
    virtual void refresh_sources() = 0;
    virtual void set_environment(u32 id, CSound_environment** dst_env) = 0;
    virtual void set_environment_size(CSound_environment* src_env, CSound_environment** dst_env) = 0;
#endif
};

class CSound_manager_interface;
extern XRSOUND_API CSound_manager_interface* Sound;

#endif // include guard
