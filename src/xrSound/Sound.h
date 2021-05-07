#pragma once

#include "xrCore/_types.h"
#include "xrCore/_flags.h"
#include "xrCore/xr_resource.h"
#include "xrCore/_vector3d.h"
#include "xrCommon/xr_vector.h" // DEFINE_VECTOR

#ifdef XRAY_STATIC_BUILD
#   define XRSOUND_API
#else
#   ifdef XRSOUND_EXPORTS
#      define XRSOUND_API XR_EXPORT
#   else
#      define XRSOUND_API XR_IMPORT
#   endif
#endif

constexpr pcstr SNDENV_FILENAME = "sEnvironment.xr";
#define OGG_COMMENT_VERSION 0x0003

// refs
class IGameObject;
class ref_sound;
class ref_sound_data;
class XRSOUND_API CSound_params;
class XRSOUND_API CSound_source;
class XRSOUND_API CSound_emitter;
class XRSOUND_API CSound_stream_interface;
class XRSOUND_API CSound_environment;
class XRSOUND_API SoundEnvironment_LIB; // editor only ref
class XRSOUND_API CSound_stats_ext;
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
XRSOUND_API extern u32 psSoundPrecacheAll;
XRSOUND_API extern xr_token* snd_devices_token;
XRSOUND_API extern u32 snd_device_id;

// Flags
enum
{
    ss_Hardware = 1ul << 1ul, //!< Use hardware mixing only
    ss_EAX = 1ul << 2ul, //!< Use eax
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
    sm_Looped = 1ul << 0ul, //!< Looped
    sm_2D = 1ul << 1ul, //!< 2D mode
    sm_forcedword = u32(-1),
};

enum esound_type
{
    st_Effect = 0,
    st_Music = 1,
    st_forcedword = u32(-1),
};

/// definition (Sound Source)
class XRSOUND_API CSound_source
{
public:
    virtual float length_sec() const = 0;
    virtual u32 game_type() const = 0;
    virtual pcstr file_name() const = 0;
    virtual u16 channels_num() const = 0;
    virtual u32 bytes_total() const = 0;
};

/// definition (Sound Source)
class XRSOUND_API CSound_environment
{};

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
    virtual bool is_2D() = 0;
    virtual void switch_to_2D() = 0;
    virtual void switch_to_3D() = 0;
    virtual void set_position(const Fvector& pos) = 0;
    virtual void set_frequency(float freq) = 0;
    virtual void set_range(float min, float max) = 0;
    virtual void set_volume(float vol) = 0;
    virtual void set_priority(float vol) = 0;
    virtual void stop(bool isDeffered) = 0;
    virtual const CSound_params* get_params() = 0;
    virtual u32 play_time() = 0;
};

/// definition (Sound Stream Interface)
class XRSOUND_API CSound_stream_interface
{};

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

typedef resptr_core<ref_sound_data, resptr_base<ref_sound_data>> ref_sound_data_ptr;

/// definition (Sound Callback)
typedef void __stdcall sound_event(const ref_sound_data_ptr& S, float range);

namespace CDB
{
    class MODEL;
}

/// definition (Sound Manager Interface)
// XXX tamlin: Tag NOVTABLE ?
class XRSOUND_API ISoundManager
{
    virtual void _initialize() = 0;
    virtual void _clear() = 0;

protected:
    friend class ref_sound_data;
    virtual bool _create_data(ref_sound_data& S, pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound = true) = 0;
    virtual void _destroy_data(ref_sound_data& S) = 0;

public:
    virtual ~ISoundManager() {}
    static void _create();
    static void _destroy();

    virtual void _restart() = 0;
    virtual bool i_locked() = 0;

    virtual bool create(ref_sound& S, pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound = true) = 0;
    virtual void attach_tail(ref_sound& S, pcstr fName) = 0;
    virtual void clone(ref_sound& S, const ref_sound& from, esound_type sound_type, int game_type) = 0;
    virtual void destroy(ref_sound& S) = 0;

    virtual void prefetch() = 0;

    virtual void stop_emitters() = 0;
    virtual int pause_emitters(bool val) = 0;

    virtual void play(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f) = 0;
    virtual void play_at_pos(ref_sound& S, IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f) = 0;
    virtual void play_no_feedback(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = nullptr,
        float* vol = nullptr, float* freq = nullptr, Fvector2* range = nullptr) = 0;

    virtual void set_master_volume(float f = 1.f) = 0;
    virtual void set_geometry_env(IReader* I) = 0;
    virtual void set_geometry_som(IReader* I) = 0;
    virtual void set_geometry_occ(CDB::MODEL* M) = 0;
    virtual void set_handler(sound_event* E) = 0;

    virtual void update(const Fvector& P, const Fvector& D, const Fvector& N) = 0;
    virtual void statistic(CSound_stats* s0, CSound_stats_ext* s1) = 0;
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) = 0;

    virtual float get_occlusion_to(const Fvector& hear_pt, const Fvector& snd_pt, float dispersion = 0.2f) = 0;
    virtual float get_occlusion(Fvector& P, float R, Fvector* occ) = 0;

    virtual void object_relcase(IGameObject* obj) = 0;
    virtual const Fvector& listener_position() = 0;

    virtual SoundEnvironment_LIB* get_env_library() = 0;
    virtual void refresh_env_library() = 0;
    virtual void set_user_env(CSound_environment* E) = 0;
    virtual void refresh_sources() = 0;
    virtual void set_environment(u32 id, CSound_environment** dst_env) = 0;
    virtual void set_environment_size(CSound_environment* src_env, CSound_environment** dst_env) = 0;
};

class CSound_UserDataVisitor;

class CSound_UserData : public xr_resource
{
public:
    virtual ~CSound_UserData() {}
    virtual void accept(CSound_UserDataVisitor*) = 0;
    virtual void invalidate() = 0;
};

using CSound_UserDataPtr = resptr_core<CSound_UserData, resptr_base<CSound_UserData>>;

class ref_sound_data : public xr_resource
{
public:
    //shared_str nm;
    CSound_source* handle; //!< Pointer to wave-source interface
    CSound_emitter* feedback; //!< Pointer to emitter, automatically clears on emitter-stop
    esound_type s_type;
    int g_type; //!< Sound type, usually for AI
    IGameObject* g_object; //!< Game object that emits ref_sound
    CSound_UserDataPtr g_userdata;
    shared_str fn_attached[2];

    u32 dwBytesTotal;
    float fTimeTotal;

    ref_sound_data() noexcept
        : handle(0), feedback(0), s_type(st_Effect), g_type(0), g_object(0), dwBytesTotal(0), fTimeTotal(0)
    {
    }

    ref_sound_data(pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound = true)
    {
        GEnv.Sound->_create_data(*this, fName, sound_type, game_type, replaceWithNoSound);
    }

    virtual ~ref_sound_data() { GEnv.Sound->_destroy_data(*this); }
    float get_length_sec() const { return fTimeTotal; }
};

inline void VerSndUnlocked() { VERIFY(!GEnv.Sound->i_locked()); }
/*! \class ref_sound
\brief Sound source + control

The main class representing source/emitter interface
This class in fact just hides internals and redirect calls to
specific sub-systems
*/
class ref_sound
{
public:
    ref_sound_data_ptr _p;

    ref_sound() = default;
    ~ref_sound() = default;

    CSound_source*     _handle() const { return _p ? _p->handle   : nullptr; }
    CSound_emitter*    _feedback() const { return _p ? _p->feedback : nullptr; }
    IGameObject*       _g_object()   { VERIFY(_p); return _p->g_object;   }
    int                _g_type()     { VERIFY(_p); return _p->g_type;     }
    esound_type        _sound_type() { VERIFY(_p); return _p->s_type;     }
    CSound_UserDataPtr _g_userdata() { VERIFY(_p); return _p->g_userdata; }

    bool create(pcstr name, esound_type sound_type, int game_type, bool replaceWithNoSound = true)
    { VerSndUnlocked(); return GEnv.Sound->create(*this, name, sound_type, game_type, replaceWithNoSound); }

    void attach_tail(pcstr name)
    { VerSndUnlocked(); GEnv.Sound->attach_tail(*this, name); }

    void clone(const ref_sound& from, esound_type sound_type, int game_type)
    { VerSndUnlocked(); GEnv.Sound->clone(*this, from, sound_type, game_type); }

    void destroy()
    { VerSndUnlocked(); GEnv.Sound->destroy(*this); }

    void play(IGameObject* O, u32 flags = 0, float delay = 0.f)
    { VerSndUnlocked(); GEnv.Sound->play(*this, O, flags, delay); }

    void play_at_pos(IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f)
    { VerSndUnlocked(); GEnv.Sound->play_at_pos(*this, O, pos, flags, delay); }

    void play_no_feedback(IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = nullptr, float* vol = nullptr, float* freq = nullptr, Fvector2* range = nullptr)
    { VerSndUnlocked(); GEnv.Sound->play_no_feedback(*this, O, flags, delay, pos, vol, freq, range); }

    void stop()                           { VerSndUnlocked(); if (_feedback()) _feedback()->stop(false); }
    void stop_deferred()                  { VerSndUnlocked(); if (_feedback()) _feedback()->stop(true ); }

    void set_position(const Fvector& pos) { VerSndUnlocked(); if (_feedback()) _feedback()->set_position(pos); }
    void set_frequency(float freq)        { VerSndUnlocked(); if (_feedback()) _feedback()->set_frequency(freq); }
    void set_range(float min, float max)  { VerSndUnlocked(); if (_feedback()) _feedback()->set_range(min, max); }
    void set_volume(float vol)            { VerSndUnlocked(); if (_feedback()) _feedback()->set_volume(vol); }
    void set_priority(float p)            { VerSndUnlocked(); if (_feedback()) _feedback()->set_priority(p); }

    const CSound_params* get_params()
    {
        VerSndUnlocked();
        return _feedback() ? _feedback()->get_params() : 0;
    }

    void set_params(CSound_params* p)
    {
        VerSndUnlocked();
        CSound_emitter* const feedback = _feedback();
        if (feedback)
        {
            feedback->set_position(p->position);
            feedback->set_frequency(p->freq);
            feedback->set_range(p->min_distance, p->max_distance);
            feedback->set_volume(p->volume);
        }
    }

    float get_length_sec() const { return _p ? _p->get_length_sec() : 0.0f; }
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

    using item_vec = xr_vector<SItem>;
    item_vec items;

    void clear() { items.clear(); }
    void append(const SItem& itm) { items.push_back(itm); }
};
