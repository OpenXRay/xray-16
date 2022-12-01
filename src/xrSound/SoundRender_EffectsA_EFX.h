#pragma once

#include "SoundRender_Effects.h"

#if __has_include(<openal/efx.h>)
#   include <openal/efx.h>
#   define XR_HAS_EFX

class CSoundRender_EffectsA_EFX : public CSoundRender_Effects
{
    LPALGENEFFECTS alGenEffects{};
    LPALDELETEEFFECTS alDeleteEffects{};
    LPALISEFFECT alIsEffect{};
    LPALEFFECTF alEffectf{};
    LPALEFFECTI alEffecti{};
    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots{};
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots{};
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti{};
    LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot{};

    ALuint effect{};
    ALuint slot{};

    bool m_is_supported{}; // Boolean variable to indicate presence of EFX Extension

public:
    CSoundRender_EffectsA_EFX();
    ~CSoundRender_EffectsA_EFX() override;

    bool initialized() override;

    void set_listener(const CSoundRender_Environment& env) override;
    void get_listener(CSoundRender_Environment& env) override;

    void commit() override;

public:
    auto get_slot() const { return slot; }
};
#endif // #if __has_include(<openal/efx.h>)
