#pragma once

#include "xrCore/xr_types.h"
#include "xrCore/_flags.h"
#include "xrCore/xr_resource.h"
#include "xrCore/_vector3d.h"
#include "xrCore/xr_token.h"
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
struct CSound;
struct resptrcode_sound;
class ISoundScene;
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
XRSOUND_API extern float psSoundVelocityAlpha; // Cribbledirge: Alpha value for moving average.
XRSOUND_API extern float psSoundTimeFactor; //--#SM+#--
XRSOUND_API extern float psSoundLinearFadeFactor; //--#SM+#--
XRSOUND_API extern Flags32 psSoundFlags;
XRSOUND_API extern int psSoundTargets;
XRSOUND_API extern int psSoundCacheSizeMB;
XRSOUND_API extern u32 psSoundPrecacheAll;
XRSOUND_API extern u32 snd_device_id;

XRSOUND_API extern ISoundScene* DefaultSoundScene;

// Flags
enum : u32
{
    ss_Hardware = 1ul << 1ul, //!< Use hardware mixing only
    ss_EFX = 1ul << 2ul, //!< Use efx
};

enum : u32
{
    sq_DEFAULT,
    sq_NOVIRT,
    sq_LIGHT,
    sq_HIGH,
};

enum : u32
{
    sg_Undefined = 0,
    sg_SourceType = u32(-1),
};

enum : u32
{
    sm_Looped = 1ul << 0ul, //!< Looped
    sm_2D = 1ul << 1ul, //!< 2D mode
    sm_IgnoreTimeFactor = 1ul << 2ul
};

enum esound_type : u32
{
    st_Effect = 0,
    st_Music = 1,
};

/// definition (Sound Source)
class XRSOUND_API XR_NOVTABLE CSound_source
{
public:
    virtual ~CSound_source() = 0;
    [[nodiscard]] virtual float length_sec() const = 0;
    [[nodiscard]] virtual u32 game_type() const = 0;
    [[nodiscard]] virtual pcstr file_name() const = 0;
    [[nodiscard]] virtual u16 channels_num() const = 0;
    [[nodiscard]] virtual u32 bytes_total() const = 0;
};

inline CSound_source::~CSound_source() = default;

/// definition (Sound Source)
class XRSOUND_API CSound_environment
{};

/// definition (Sound Params)
class XRSOUND_API CSound_params
{
public:
    Fvector position{};
    Fvector velocity{};  // Cribbledirge.  Added for doppler effect.
    Fvector curVelocity{};  // Current velocity.
    Fvector prevVelocity{};  // Previous velocity.
    Fvector accVelocity{};  // Velocity accumulator (for moving average).
    float base_volume;
    float volume;
    float freq;
    float min_distance;
    float max_distance;
    float max_ai_distance;

    // Functions added by Cribbledirge for doppler effect.
    void update_position(const Fvector& newPosition)
    {
        // If the position has been set already, start getting a moving average of the velocity.
        if (set)
        {
            prevVelocity.set(accVelocity);
            curVelocity.sub(newPosition, position);
            accVelocity.set(curVelocity.mul(psSoundVelocityAlpha).add(prevVelocity.mul(1.f - psSoundVelocityAlpha)));
        }
        else
        {
            set = true;
        }
        position.set(newPosition);
    }

    void update_velocity(const float dt)
    {
        velocity.set(accVelocity).div(dt);
    }

private:
    // A variable in place to determine if the position has been set.  This is to prevent artifacts when
    // the position jumps from its initial position of zero to something greatly different.  This is a big
    // issue in moving average calculation.  We want the velocity to always start at zero for when the sound
    // was initiated, or else things will sound really really weird.
    bool set{};

    // End Cribbledirge.
};

/// definition (Sound Interface)
class XRSOUND_API XR_NOVTABLE CSound_emitter
{
public:
    virtual ~CSound_emitter() = 0;

    virtual bool is_2D() = 0;
    virtual void switch_to_2D() = 0;
    virtual void switch_to_3D() = 0;
    virtual void set_position(const Fvector& pos) = 0;
    virtual void set_frequency(float freq) = 0;
    virtual void set_range(float min, float max) = 0;
    virtual void set_volume(float vol) = 0;
    virtual void set_priority(float vol) = 0;
    virtual void set_time(float t) = 0; //--#SM+#--
    virtual void stop(bool isDeffered) = 0;
    virtual void set_ignore_time_factor(bool ignore) = 0;
    virtual const CSound_params* get_params() = 0;
    virtual u32 play_time() = 0;
};

inline CSound_emitter::~CSound_emitter() = default;

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

using ref_sound = resptr_core<CSound, resptrcode_sound>;

/// definition (Sound Callback)
typedef void sound_event(const ref_sound& S, float range);

namespace CDB
{
    class MODEL;
}

class XRSOUND_API XR_NOVTABLE ISoundScene
{
protected:
    friend struct resptrcode_sound;

public:
    virtual ~ISoundScene() = 0;

    virtual void play(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f) = 0;
    virtual void play_at_pos(ref_sound& S, IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f) = 0;
    virtual void play_no_feedback(ref_sound& S, IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = nullptr,
        float* vol = nullptr, float* freq = nullptr, Fvector2* range = nullptr) = 0;

    virtual void stop_emitters() const = 0;
    virtual int pause_emitters(bool pauseState) = 0;

    virtual void set_handler(sound_event* E) = 0;
    virtual void set_geometry_env(IReader* I) = 0;
    virtual void set_geometry_som(IReader* I) = 0;
    virtual void set_geometry_occ(CDB::MODEL* M) = 0;

    virtual void set_user_env(CSound_environment* E) = 0;
    virtual void set_environment(u32 id, CSound_environment** dst_env) = 0;
    virtual void set_environment_size(CSound_environment* src_env, CSound_environment** dst_env) = 0;
    virtual CSound_environment* get_environment(const Fvector& P) = 0;

    virtual float get_occlusion_to(const Fvector& hear_pt, const Fvector& snd_pt, float dispersion = 0.2f) = 0;
    virtual float get_occlusion(const Fvector& P, float R, Fvector* occ) = 0;

    virtual void object_relcase(IGameObject* obj) = 0;
};

inline ISoundScene::~ISoundScene() = default;

/// definition (Sound Manager Interface)
class XRSOUND_API XR_NOVTABLE ISoundManager
{
protected:
    friend struct CSound;
    friend struct resptrcode_sound;

    virtual CSound* create(pcstr fName, esound_type sound_type, int game_type, bool replaceWithNoSound = true) = 0;
    virtual void destroy(CSound& S) = 0;

    virtual void attach_tail(CSound& S, pcstr fName) = 0;

public:
    virtual ~ISoundManager() = default;

    virtual ISoundScene* create_scene() = 0;
    virtual void destroy_scene(ISoundScene*&) = 0;

    virtual void _restart() = 0;
    virtual bool i_locked() = 0;

    virtual void prefetch() = 0;

    virtual void stop_emitters() = 0;
    virtual int pause_emitters(bool pauseState) = 0;

    virtual void set_master_volume(float f = 1.f) = 0;

    virtual void update(const Fvector& P, const Fvector& D, const Fvector& N) = 0;
    virtual void statistic(CSound_stats* s0, CSound_stats_ext* s1) = 0;
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) = 0;

    virtual const Fvector& listener_position() = 0;

    virtual void refresh_sources() = 0;
};

class XRSOUND_API CSoundManager
{
    xr_vector<xr_token> soundDevices;

    SoundEnvironment_LIB* soundEnvironment{};

public:
    void  CreateDevicesList();
    auto& GetDevicesList() { return soundDevices; }

    void Create();
    void Destroy();

    [[nodiscard]]
    bool IsSoundEnabled() const;

    void env_load();
    void env_unload();
    void refresh_env_library();
    SoundEnvironment_LIB* get_env_library() const;
};

class CSound_UserDataVisitor;

class CSound_UserData : public xr_resource
{
public:
    virtual ~CSound_UserData() = default;
    virtual void accept(CSound_UserDataVisitor*) = 0;
    virtual void invalidate() = 0;
};

using CSound_UserDataPtr = resptr_core<CSound_UserData, resptr_base<CSound_UserData>>;

struct CSound : public xr_resource
{
public:
    //shared_str nm;
    CSound_source* handle{}; //!< Pointer to wave-source interface
    CSound_emitter* feedback{}; //!< Pointer to emitter, automatically clears on emitter-stop

    esound_type s_type{ st_Effect };
    int g_type{}; //!< Sound type, usually for AI

    IGameObject* g_object{}; //!< Game object that emits ref_sound
    CSound_UserDataPtr g_userdata{};
    shared_str fn_attached[2];

    u32 dwBytesTotal{};
    float fTimeTotal{};
};

/*! \class ref_sound
\brief Sound source + control

The main class representing source/emitter interface
This class in fact just hides internals and redirect calls to
specific sub-systems
*/
struct resptrcode_sound : public resptr_base<CSound>
{
    [[nodiscard]]
    ICF CSound_source*     _handle()     const { return p_ ? p_->handle   : nullptr; }

    [[nodiscard]]
    ICF CSound_emitter*    _feedback()   const { return p_ ? p_->feedback : nullptr; }

    [[nodiscard]]
    ICF IGameObject*       _g_object()   const { VERIFY(p_); return p_ ? p_->g_object : nullptr; }

    [[nodiscard]]
    ICF int                _g_type()     const { VERIFY(p_); return p_ ? p_->g_type : 0; }

    [[nodiscard]]
    ICF esound_type        _sound_type() const { VERIFY(p_); return p_ ? p_->s_type : st_Effect; }

    [[nodiscard]]
    ICF CSound_UserDataPtr _g_userdata() const { VERIFY(p_); return p_ ? p_->g_userdata : nullptr; }

    ICF bool create(pcstr name, esound_type sound_type, int game_type, bool replaceWithNoSound = true)
    {
        VerSndUnlocked();
        _set(GEnv.Sound->create(name, sound_type, game_type, replaceWithNoSound));
        return _get();
    }

    ICF void destroy()
    {
        if (!p_)
            return;
        VerSndUnlocked();
        GEnv.Sound->destroy(*p_);
        _set(nullptr);
    }

    ICF void attach_tail(pcstr name) const
    {
        VerSndUnlocked();
        if (!p_)
            return;
        GEnv.Sound->attach_tail(*p_, name);
    }

    void clone(const ref_sound& from, esound_type sound_type, int game_type)
    {
        if (!from._get())
            return;
        _set(xr_new<CSound>());
        p_->handle = from->handle;
        p_->dwBytesTotal = from->dwBytesTotal;
        p_->fTimeTotal = from->fTimeTotal;
        p_->fn_attached[0] = from->fn_attached[0];
        p_->fn_attached[1] = from->fn_attached[1];
        p_->g_type = (game_type == sg_SourceType) ? p_->handle->game_type() : game_type;
        p_->s_type = sound_type;
    }

    ICF void play(IGameObject* O, u32 flags = 0, float delay = 0.f)
    {
        if (!p_ || !DefaultSoundScene)
            return;
        VerSndUnlocked();
        DefaultSoundScene->play(static_cast<ref_sound&>(*this), O, flags, delay);
    }

    ICF void play_at_pos(IGameObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f)
    {
        if (!p_ || !DefaultSoundScene)
            return;
        VerSndUnlocked();
        DefaultSoundScene->play_at_pos(static_cast<ref_sound&>(*this), O, pos, flags, delay);
    }

    ICF void play_no_feedback(IGameObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = nullptr, float* vol = nullptr, float* freq = nullptr, Fvector2* range = nullptr)
    {
        if (!p_ || !DefaultSoundScene)
            return;
        VerSndUnlocked();
        DefaultSoundScene->play_no_feedback(static_cast<ref_sound&>(*this), O, flags, delay, pos, vol, freq, range);
    }

    ICF void stop()                           const { VerSndUnlocked(); if (_feedback()) _feedback()->stop(false); }
    ICF void stop_deferred()                  const { VerSndUnlocked(); if (_feedback()) _feedback()->stop(true ); }

    ICF void set_position(const Fvector& pos) const { VerSndUnlocked(); if (_feedback()) _feedback()->set_position(pos); }
    ICF void set_frequency(float freq)        const { VerSndUnlocked(); if (_feedback()) _feedback()->set_frequency(freq); }
    ICF void set_range(float min, float max)  const { VerSndUnlocked(); if (_feedback()) _feedback()->set_range(min, max); }
    ICF void set_volume(float vol)            const { VerSndUnlocked(); if (_feedback()) _feedback()->set_volume(vol); }
    ICF void set_priority(float p)            const { VerSndUnlocked(); if (_feedback()) _feedback()->set_priority(p); }
    ICF void set_time(float t)                const { VerSndUnlocked(); if (_feedback()) _feedback()->set_time(t); }; //--#SM+#--

    [[nodiscard]]
    ICF const CSound_params* get_params() const
    {
        VerSndUnlocked();
        return _feedback() ? _feedback()->get_params() : nullptr;
    }

    ICF void set_params(CSound_params* p) const
    {
        VerSndUnlocked();
        if (CSound_emitter* const feedback = _feedback())
        {
            feedback->set_position(p->position);
            feedback->set_frequency(p->freq);
            feedback->set_range(p->min_distance, p->max_distance);
            feedback->set_volume(p->volume);
        }
    }

    [[nodiscard]]
    ICF float get_length_sec() const { return p_ ? p_->fTimeTotal : 0.0f; }

    static void VerSndUnlocked()
    {
        VERIFY(!GEnv.Sound->i_locked());
    }
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
