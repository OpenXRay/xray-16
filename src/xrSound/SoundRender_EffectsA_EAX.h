#pragma once

#include "SoundRender_Effects.h"

#if __has_include(<eax/eax.h>)
#   define OPENAL
#   include <eax/eax.h>

#   define XR_HAS_EAX

class CSoundRender_EffectsA_EAX : public CSoundRender_Effects
{
    EAXSet m_eax_set_fn; // EAXSet function, retrieved if EAX Extension is supported
    EAXGet m_eax_get_fn; // EAXGet function, retrieved if EAX Extension is supported

    bool m_is_supported; // Boolean variable to indicate presence of EAX Extension
    bool m_is_deferred;

private:
    ICF ALenum eax_set(const GUID* guid, u32 prop, void* val, u32 sz) const;
    ICF ALenum eax_get(const GUID* guid, u32 prop, void* val, u32 sz) const;

    [[nodiscard]] bool query_support(bool isDeferred, const GUID* guid, u32 prop, void* val, u32 sz) const;
    [[nodiscard]] bool test_support(bool isDeferred) const;

public:
    CSoundRender_EffectsA_EAX();

    bool initialized() override;

    void set_listener(const CSoundRender_Environment& env) override;
    void get_listener(CSoundRender_Environment& env) override;

    void commit() override;
};
#endif // #if __has_include(<eax/eax.h>)
