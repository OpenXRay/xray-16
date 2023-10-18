#pragma once

#include "SoundRender_Effects.h"

#if __has_include(<efx.h>)
#   include <efx.h>
#   include <efx-creative.h>
#   include <efx-presets.h>
#   define XR_HAS_EFX

class CSoundRender_EffectsA_EFX : public CSoundRender_Effects
{
    LPALGENEFFECTS alGenEffects{};
    LPALDELETEEFFECTS alDeleteEffects{};
    LPALISEFFECT alIsEffect{};
    LPALEFFECTF alEffectf{};
    LPALEFFECTI alEffecti{};
    LPALEFFECTFV alEffectfv{};
    LPALGETEFFECTI alGetEffecti{};
    LPALGETEFFECTF alGetEffectf{};
    LPALGETEFFECTFV alGetEffectfv{};
    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots{};
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots{};
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti{};
    LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf{};
    LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv{};
    LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot{};

    ALuint effect{};
    ALuint effectfv{};
    ALuint slot{};

    bool m_is_supported{}; // Boolean variable to indicate presence of EFX Extension

public:
    CSoundRender_EffectsA_EFX();
    ~CSoundRender_EffectsA_EFX() override;

    bool initialized() override;

    auto get_slot() const { return slot; }
    void set_listener(const CSoundRender_Environment& env) override;
    void get_listener(CSoundRender_Environment& env) override;

    void commit() override;
};
#endif // #if __has_include(<efx.h>)
