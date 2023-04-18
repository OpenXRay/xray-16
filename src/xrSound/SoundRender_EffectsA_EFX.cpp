#include "stdafx.h"

#include "SoundRender_EffectsA_EFX.h"

#ifdef XR_HAS_EFX
#   include "SoundRender_CoreA.h"
#   include "SoundRender_Environment.h"

#define LOAD_PROC(x, type) \
    do \
    { \
        ((x) = (type)alGetProcAddress(#x)); \
        if (!(x)) \
            return; \
    } while (false)

CSoundRender_EffectsA_EFX::CSoundRender_EffectsA_EFX()
{
    LOAD_PROC(alGenEffects, LPALGENEFFECTS);
    LOAD_PROC(alDeleteEffects, LPALDELETEEFFECTS);
    LOAD_PROC(alIsEffect, LPALISEFFECT);
    LOAD_PROC(alEffecti, LPALEFFECTI);
    LOAD_PROC(alEffectf, LPALEFFECTF);
    LOAD_PROC(alEffectfv, LPALEFFECTFV);
    LOAD_PROC(alGetEffectf, LPALGETEFFECTF);
    LOAD_PROC(alGetEffectfv, LPALGETEFFECTFV);
    LOAD_PROC(alGetEffecti, LPALGETEFFECTI);
    LOAD_PROC(alGenAuxiliaryEffectSlots, LPALGENAUXILIARYEFFECTSLOTS);
    LOAD_PROC(alDeleteAuxiliaryEffectSlots, LPALDELETEAUXILIARYEFFECTSLOTS);
    LOAD_PROC(alAuxiliaryEffectSloti, LPALAUXILIARYEFFECTSLOTI);
    LOAD_PROC(alAuxiliaryEffectSlotf, LPALAUXILIARYEFFECTSLOTF);
    LOAD_PROC(alIsAuxiliaryEffectSlot, LPALISAUXILIARYEFFECTSLOT);

    alGenEffects(1, &effect);

    constexpr ALfloat f3[3] = { 0.f, 0.f, 0.f };
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
    alEffectf(effect, AL_EAXREVERB_DENSITY, AL_EAXREVERB_DEFAULT_DENSITY);
    alEffectf(effect, AL_EAXREVERB_DIFFUSION, AL_EAXREVERB_DEFAULT_DIFFUSION);
    alEffectf(effect, AL_EAXREVERB_GAIN, AL_EAXREVERB_DEFAULT_GAIN);
    alEffectf(effect, AL_EAXREVERB_GAINHF, AL_EAXREVERB_DEFAULT_GAINHF);
    alEffectf(effect, AL_EAXREVERB_GAINLF, AL_EAXREVERB_DEFAULT_GAINLF);
    alEffectf(effect, AL_EAXREVERB_DECAY_TIME, AL_EAXREVERB_DEFAULT_DECAY_TIME);
    alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, AL_EAXREVERB_DEFAULT_DECAY_HFRATIO);
    alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, AL_EAXREVERB_DEFAULT_DECAY_LFRATIO);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, AL_EAXREVERB_DEFAULT_REFLECTIONS_GAIN);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, AL_EAXREVERB_DEFAULT_REFLECTIONS_DELAY);
    alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, f3);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, AL_EAXREVERB_DEFAULT_LATE_REVERB_GAIN);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, AL_EAXREVERB_DEFAULT_LATE_REVERB_DELAY);
    alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, f3);
    alEffectf(effect, AL_EAXREVERB_ECHO_TIME, AL_EAXREVERB_DEFAULT_ECHO_TIME);
    alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, AL_EAXREVERB_DEFAULT_ECHO_DEPTH);
    alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, AL_EAXREVERB_DEFAULT_MODULATION_TIME);
    alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, AL_EAXREVERB_DEFAULT_MODULATION_DEPTH);
    alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, AL_EAXREVERB_DEFAULT_AIR_ABSORPTION_GAINHF);
    alEffectf(effect, AL_EAXREVERB_HFREFERENCE, AL_EAXREVERB_DEFAULT_HFREFERENCE);
    alEffectf(effect, AL_EAXREVERB_LFREFERENCE, AL_EAXREVERB_DEFAULT_LFREFERENCE);
    alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, AL_EAXREVERB_DEFAULT_ROOM_ROLLOFF_FACTOR);
    alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, AL_EAXREVERB_DEFAULT_DECAY_HFLIMIT);

    /* Check if an error occured, and clean up if so. */
    if (const ALenum err = alGetError(); err == AL_NO_ERROR)
        m_is_supported = true;
    else
    {
        Log("SOUND: OpenAL: Failed to init EFX:", alGetString(err));
        if (alIsEffect(effect))
            alDeleteEffects(1, &effect);
    }

    alGenAuxiliaryEffectSlots(1, &slot);
    if (const ALenum err = alGetError(); err != AL_NO_ERROR)
        Log("! SOUND: OpenAL: failed to generate auxiliary slot:", alGetString(err));

    Log("* SOUND: EFX extension:", m_is_supported ? "present" : "absent");
}

#undef LOAD_PROC

CSoundRender_EffectsA_EFX::~CSoundRender_EffectsA_EFX()
{
    if (m_is_supported)
    {
        alDeleteEffects(1, &effect);
        if (alIsAuxiliaryEffectSlot(slot))
            alDeleteAuxiliaryEffectSlots(1, &slot);
    }
}

bool CSoundRender_EffectsA_EFX::initialized()
{
    return m_is_supported;
}

void CSoundRender_EffectsA_EFX::set_listener(const CSoundRender_Environment& env)
{
    A_CHK(alEffectf(effect, AL_EAXREVERB_DENSITY, env.Density));
    A_CHK(alEffectf(effect, AL_EAXREVERB_DIFFUSION, env.EnvironmentDiffusion));
    A_CHK(alEffectf(effect, AL_EAXREVERB_GAIN, env.Room));
    A_CHK(alEffectf(effect, AL_EAXREVERB_GAINHF, env.RoomHF));
    A_CHK(alEffectf(effect, AL_EAXREVERB_DECAY_TIME, env.DecayTime));
    A_CHK(alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, env.DecayHFRatio));
    A_CHK(alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, env.DecayLFRatio));
    A_CHK(alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, env.Reflections));
    A_CHK(alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, env.ReflectionsDelay));
    //A_CHK(alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, &env.ReflectionsPan[0]));
    A_CHK(alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, env.Reverb));
    A_CHK(alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, env.ReverbDelay));
    //A_CHK(alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, &env.ReverbPan[0]));
    A_CHK(alEffectf(effect, AL_EAXREVERB_ECHO_TIME, env.EchoTime));
    A_CHK(alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, env.EchoDepth));
    A_CHK(alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, env.ModulationTime));
    A_CHK(alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, env.ModulationDepth));
    A_CHK(alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, env.AirAbsorptionHF));
    A_CHK(alEffectf(effect, AL_EAXREVERB_HFREFERENCE, env.HFReference));
    A_CHK(alEffectf(effect, AL_EAXREVERB_LFREFERENCE, env.LFReference));
    A_CHK(alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, env.RoomRolloffFactor));
    A_CHK(alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, env.DecayHFLimit));
}

void CSoundRender_EffectsA_EFX::get_listener(CSoundRender_Environment& env)
{
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_DENSITY, &env.Density));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_DIFFUSION, &env.EnvironmentDiffusion));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_GAIN, &env.Room));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_GAINHF, &env.RoomHF));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_DECAY_TIME, &env.DecayTime));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, &env.DecayHFRatio));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, &env.DecayLFRatio));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, &env.Reflections));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, &env.ReflectionsDelay));
    //A_CHK(alGetEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, (float*)&env.ReflectionsPan));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, &env.Reverb));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, &env.ReverbDelay));
    //A_CHK(alGetEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, (float*)&env.ReverbPan));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_ECHO_TIME, &env.EchoTime));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, &env.EchoDepth));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_MODULATION_TIME, &env.ModulationTime));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, &env.ModulationDepth));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, &env.AirAbsorptionHF));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_HFREFERENCE, &env.HFReference));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_LFREFERENCE, &env.LFReference));
    A_CHK(alGetEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, &env.RoomRolloffFactor));
    A_CHK(alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, (ALint)env.DecayHFLimit));
}

void CSoundRender_EffectsA_EFX::commit()
{
    /* Tell the effect slot to use the loaded effect object. Note that the this
     * effectively copies the effect properties. You can modify or delete the
     * effect object afterward without affecting the effect slot.
     */
    A_CHK(alAuxiliaryEffectSlotf(slot, AL_EFFECTSLOT_GAIN, 1.f));
    A_CHK(alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, false));
    A_CHK(alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect));
}
#endif // XR_HAS_EFX
