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
    LOAD_PROC(alGenAuxiliaryEffectSlots, LPALGENAUXILIARYEFFECTSLOTS);
    LOAD_PROC(alDeleteAuxiliaryEffectSlots, LPALDELETEAUXILIARYEFFECTSLOTS);
    LOAD_PROC(alAuxiliaryEffectSloti, LPALAUXILIARYEFFECTSLOTI);
    LOAD_PROC(alIsAuxiliaryEffectSlot, LPALISAUXILIARYEFFECTSLOT);

    alGenEffects(1, &effect);

    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
    alEffectf(effect, AL_REVERB_DENSITY, AL_REVERB_DEFAULT_DENSITY);
    alEffectf(effect, AL_REVERB_DIFFUSION, AL_REVERB_DEFAULT_DIFFUSION);
    alEffectf(effect, AL_REVERB_GAIN, AL_REVERB_DEFAULT_GAIN);
    alEffectf(effect, AL_REVERB_GAINHF, AL_REVERB_DEFAULT_GAINHF);
    alEffectf(effect, AL_REVERB_DECAY_TIME, AL_REVERB_DEFAULT_DECAY_TIME);
    alEffectf(effect, AL_REVERB_DECAY_HFRATIO, AL_REVERB_DEFAULT_DECAY_HFRATIO);
    alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, AL_REVERB_DEFAULT_REFLECTIONS_GAIN);
    alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, AL_REVERB_DEFAULT_REFLECTIONS_DELAY);
    alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, AL_REVERB_DEFAULT_LATE_REVERB_GAIN);
    alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, AL_REVERB_DEFAULT_LATE_REVERB_DELAY);
    alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, AL_REVERB_DEFAULT_AIR_ABSORPTION_GAINHF);
    alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, AL_REVERB_DEFAULT_ROOM_ROLLOFF_FACTOR);
    alEffecti(effect, AL_REVERB_DECAY_HFLIMIT, AL_REVERB_DEFAULT_DECAY_HFLIMIT);

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
    auto mB_to_gain = [](float mb) -> float { return powf(10.0f, mb / 2000.0f); };

    // http://openal.org/pipermail/openal/2014-March/000083.html
    float density = powf(env.EnvironmentSize, 3.0f) / 16.0f;
    if (density > 1.0f)
        density = 1.0f;
    A_CHK(alEffectf(effect, AL_REVERB_DENSITY, density));
    A_CHK(alEffectf(effect, AL_REVERB_DIFFUSION, env.EnvironmentDiffusion));
    A_CHK(alEffectf(effect, AL_REVERB_GAIN, mB_to_gain(env.Room)));
    A_CHK(alEffectf(effect, AL_REVERB_GAINHF, mB_to_gain(env.RoomHF)));
    A_CHK(alEffectf(effect, AL_REVERB_DECAY_TIME, env.DecayTime));
    A_CHK(alEffectf(effect, AL_REVERB_DECAY_HFRATIO, env.DecayHFRatio));
    A_CHK(alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, mB_to_gain(env.Reflections)));
    A_CHK(alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, env.ReflectionsDelay));
    A_CHK(alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, env.ReverbDelay));
    A_CHK(alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, mB_to_gain(env.Reverb)));
    A_CHK(alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, mB_to_gain(env.AirAbsorptionHF)));
    A_CHK(alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, env.RoomRolloffFactor));

}

void CSoundRender_EffectsA_EFX::get_listener(CSoundRender_Environment& env)
{
    VERIFY(!"Not implemented.");
}

void CSoundRender_EffectsA_EFX::commit()
{
    /* Tell the effect slot to use the loaded effect object. Note that the this
     * effectively copies the effect properties. You can modify or delete the
     * effect object afterward without affecting the effect slot.
     */
    A_CHK(alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect));
}
#endif // XR_HAS_EFX
