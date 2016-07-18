// Implementation of Sound functions not exported from xrSound
#include "xrSound/Sound.h"
#include "xrCore/xrDebug_macros.h"

namespace
{
static inline void VerSndUnlocked() { VERIFY(!::Sound->i_locked()); }
}

/// ********* Sound ********* (utils, accessors, helpers)
ref_sound_data::ref_sound_data() throw() :
    handle(0),
    feedback(0),
    s_type(st_Effect),
    g_type(0),
    g_object(0)
{}

ref_sound_data::ref_sound_data(LPCSTR fName, esound_type sound_type, int game_type)
{ ::Sound->_create_data(*this, fName, sound_type, game_type); }

ref_sound_data::~ref_sound_data()
{ ::Sound->_destroy_data(*this); }

void ref_sound::create(LPCSTR name, esound_type sound_type, int game_type)
{
    VerSndUnlocked();
    ::Sound->create(*this, name, sound_type, game_type);
}

void ref_sound::attach_tail(LPCSTR name)
{
    VerSndUnlocked();
    ::Sound->attach_tail(*this, name);
}

CSound_source* ref_sound::_handle() const { return _p ? _p->handle : 0; }

CSound_emitter* ref_sound::_feedback() { return _p ? _p->feedback : 0; }

IGameObject* ref_sound::_g_object()
{
    VERIFY(_p);
    return _p->g_object;
}

int ref_sound::_g_type()
{
    VERIFY(_p);
    return _p->g_type;
}

esound_type ref_sound::_sound_type()
{
    VERIFY(_p);
    return _p->s_type;
}

CSound_UserDataPtr ref_sound::_g_userdata()
{
    VERIFY(_p);
    return _p->g_userdata;
}

void ref_sound::clone(const ref_sound& from, esound_type sound_type, int game_type)
{
    VerSndUnlocked();
    ::Sound->clone(*this, from, sound_type, game_type);
}

void ref_sound::destroy()
{
    VerSndUnlocked();
    ::Sound->destroy(*this);
}

void ref_sound::play(IGameObject* O, u32 flags, float d)
{
    VerSndUnlocked();
    ::Sound->play(*this, O, flags, d);
}

void ref_sound::play_at_pos(IGameObject* O, const Fvector& pos, u32 flags, float d)
{
    VerSndUnlocked();
    ::Sound->play_at_pos(*this, O, pos, flags, d);
}

void ref_sound::play_no_feedback(
    IGameObject* O, u32 flags, float d, Fvector* pos, float* vol, float* freq, Fvector2* range)
{
    VerSndUnlocked();
    ::Sound->play_no_feedback(*this, O, flags, d, pos, vol, freq, range);
}

void ref_sound::set_position(const Fvector& pos)
{
    VerSndUnlocked();
    VERIFY(_feedback());
    _feedback()->set_position(pos);
}

void ref_sound::set_frequency(float freq)
{
    VerSndUnlocked();
    if (_feedback())
        _feedback()->set_frequency(freq);
}

void ref_sound::set_range(float min, float max)
{
    VerSndUnlocked();
    if (_feedback())
        _feedback()->set_range(min, max);
}

void ref_sound::set_volume(float vol)
{
    VerSndUnlocked();
    if (_feedback())
        _feedback()->set_volume(vol);
}

void ref_sound::set_priority(float p)
{
    VerSndUnlocked();
    if (_feedback())
        _feedback()->set_priority(p);
}

void ref_sound::stop()
{
    VerSndUnlocked();
    if (_feedback())
        _feedback()->stop(FALSE);
}

void ref_sound::stop_deferred()
{
    VerSndUnlocked();
    if (_feedback())
        _feedback()->stop(TRUE);
}

const CSound_params* ref_sound::get_params()
{
    VerSndUnlocked();
    return _feedback() ? _feedback()->get_params() : 0;
}

void ref_sound::set_params(CSound_params* p)
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
