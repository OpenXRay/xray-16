#include "stdafx.h"

#include "SoundRender_EffectsA_EAX.h"

#ifdef XR_HAS_EAX
#   include "SoundRender_Environment.h"

CSoundRender_EffectsA_EAX::CSoundRender_EffectsA_EAX()
{
    m_eax_set_fn = (EAXSet)alGetProcAddress((pcstr)"EAXSet");
    if (!m_eax_set_fn)
        return;

    m_eax_get_fn = (EAXGet)alGetProcAddress((pcstr)"EAXGet");
    if (!m_eax_get_fn)
        return;

    m_is_deferred = test_support(true);
    m_is_supported = test_support(false);

    Log("* SOUND: Creative EAX extension:", m_is_supported ? "present" : "absent");
    Log("* SOUND: Creative EAX deferred:", m_is_deferred ? "present" : "absent");
}

bool CSoundRender_EffectsA_EAX::initialized()
{
    return m_eax_set_fn && m_eax_get_fn && m_is_supported;
}

ICF ALenum CSoundRender_EffectsA_EAX::eax_set(const GUID* guid, u32 prop, void* val, u32 sz) const
{
    return m_eax_set_fn(guid, prop, 0, val, sz);
}

ICF ALenum CSoundRender_EffectsA_EAX::eax_get(const GUID* guid, u32 prop, void* val, u32 sz) const
{
    return m_eax_get_fn(guid, prop, 0, val, sz);
}

bool CSoundRender_EffectsA_EAX::query_support(bool isDeferred, const GUID* guid, u32 prop, void* val, u32 sz) const
{
    if (AL_NO_ERROR != eax_get(guid, prop, val, sz))
        return false;
    if (AL_NO_ERROR != eax_set(guid, (isDeferred ? DSPROPERTY_EAXLISTENER_DEFERRED : 0) | prop, val, sz))
        return false;
    return true;
}

bool CSoundRender_EffectsA_EAX::test_support(bool isDeferred) const
{
    EAXLISTENERPROPERTIES ep;
    if (!query_support(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom, sizeof(ep.lRoom)))
        return false;
    if (!query_support(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF, sizeof(ep.lRoomHF)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
        &ep.flRoomRolloffFactor, sizeof(float)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYTIME,
        &ep.flDecayTime, sizeof(float)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYHFRATIO,
        &ep.flDecayHFRatio, sizeof(float)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONS,
        &ep.lReflections, sizeof(ep.lReflections)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
        &ep.flReflectionsDelay, sizeof(float)))
        return false;
    if (!query_support(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb, sizeof(ep.lReverb)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERBDELAY,
        &ep.flReverbDelay, sizeof(float)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
        &ep.flEnvironmentDiffusion, sizeof(float)))
        return false;
    if (!query_support(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
        &ep.flAirAbsorptionHF, sizeof(float)))
        return false;
    if (!query_support(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags, sizeof(u32)))
        return false;
    return true;
}

void CSoundRender_EffectsA_EAX::set_listener(const CSoundRender_Environment& env)
{
    EAXLISTENERPROPERTIES ep;
    ep.lRoom                    = iFloor(env.Room); // room effect level at low frequencies
    ep.lRoomHF                  = iFloor(env.RoomHF); // room effect high-frequency level re. low frequency level
    ep.flRoomRolloffFactor      = env.RoomRolloffFactor; // like DS3D flRolloffFactor but for room effect
    ep.flDecayTime              = env.DecayTime; // reverberation decay time at low frequencies
    ep.flDecayHFRatio           = env.DecayHFRatio; // high-frequency to low-frequency decay time ratio
    ep.lReflections             = iFloor(env.Reflections); // early reflections level relative to room effect
    ep.flReflectionsDelay       = env.ReflectionsDelay; // initial reflection delay time
    ep.lReverb                  = iFloor(env.Reverb); // late reverberation level relative to room effect
    ep.flReverbDelay            = env.ReverbDelay; // late reverberation delay time relative to initial reflection
    ep.dwEnvironment            = EAXLISTENER_DEFAULTENVIRONMENT; // sets all listener properties
    ep.flEnvironmentSize        = env.EnvironmentSize; // environment size in meters
    ep.flEnvironmentDiffusion   = env.EnvironmentDiffusion; // environment diffusion
    ep.flAirAbsorptionHF        = env.AirAbsorptionHF; // change in level per meter at 5 kHz
    ep.dwFlags                  = EAXLISTENER_DEFAULTFLAGS; // modifies the behavior of properties

    const u32 deferred = m_is_deferred ? DSPROPERTY_EAXLISTENER_DEFERRED : 0;

    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom, sizeof(ep.lRoom));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF, sizeof(ep.lRoomHF));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
        &ep.flRoomRolloffFactor, sizeof(float));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYTIME, &ep.flDecayTime,
        sizeof(float));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYHFRATIO, &ep.flDecayHFRatio,
        sizeof(float));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONS, &ep.lReflections,
        sizeof(ep.lReflections));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
        &ep.flReflectionsDelay, sizeof(float));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb, sizeof(ep.lReverb));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERBDELAY, &ep.flReverbDelay,
        sizeof(float));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
        &ep.flEnvironmentDiffusion, sizeof(float));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
        &ep.flAirAbsorptionHF, sizeof(float));
    eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags, sizeof(u32));
}

void CSoundRender_EffectsA_EAX::get_listener(CSoundRender_Environment& env)
{
    EAXLISTENERPROPERTIES ep;
    eax_get(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ALLPARAMETERS,
        &ep, sizeof(EAXLISTENERPROPERTIES));
    env.Room                    = (float)ep.lRoom;
    env.RoomHF                  = (float)ep.lRoomHF;
    env.RoomRolloffFactor       = (float)ep.flRoomRolloffFactor;
    env.DecayTime               = (float)ep.flDecayTime;
    env.DecayHFRatio            = (float)ep.flDecayHFRatio;
    env.Reflections             = (float)ep.lReflections;
    env.ReflectionsDelay        = (float)ep.flReflectionsDelay;
    env.Reverb                  = (float)ep.lReverb;
    env.ReverbDelay             = (float)ep.flReverbDelay;
    env.EnvironmentSize         = (float)ep.flEnvironmentSize;
    env.EnvironmentDiffusion    = (float)ep.flEnvironmentDiffusion;
    env.AirAbsorptionHF         = (float)ep.flAirAbsorptionHF;
}

void CSoundRender_EffectsA_EAX::commit()
{
    // commit eax
    if (m_is_deferred)
    {
        eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS, nullptr, 0);
    }
}
#endif // XR_HAS_EAX
