#ifndef SoundRender_CoreAH
#define SoundRender_CoreAH
#pragma once

#include "SoundRender_Core.h"
#include "OpenALDeviceList.h"
#include <eax/eax.h>

#ifdef DEBUG
#define A_CHK(expr)                                                \
    {                                                              \
        alGetError();                                              \
        expr;                                                      \
        ALenum error = alGetError();                               \
        VERIFY2(error == AL_NO_ERROR, (pcstr)alGetString(error)); \
    }
#define AC_CHK(expr)                                                          \
    {                                                                         \
        alcGetError(pDevice);                                                 \
        expr;                                                                 \
        ALCenum error = alcGetError(pDevice);                                 \
        VERIFY2(error == ALC_NO_ERROR, (pcstr)alcGetString(pDevice, error)); \
    }
#else
#define A_CHK(expr) \
    {               \
        expr;       \
    }
#define AC_CHK(expr) \
    {                \
        expr;        \
    }
#endif

class CSoundRender_CoreA : public CSoundRender_Core
{
    typedef CSoundRender_Core inherited;
    EAXSet eaxSet; // EAXSet function, retrieved if EAX Extension is supported
    EAXGet eaxGet; // EAXGet function, retrieved if EAX Extension is supported
    ALCdevice* pDevice;
    ALCcontext* pContext;
    ALDeviceList* pDeviceList;

    struct SListener
    {
        Fvector position;
        Fvector orientation[2];
    };

    SListener Listener;

    bool EAXQuerySupport(bool isDeferred, const GUID* guid, u32 prop, void* val, u32 sz);
    bool EAXTestSupport(bool isDeferred);

protected:
    void i_eax_set(const GUID* guid, u32 prop, void* val, u32 sz) override;
    void i_eax_get(const GUID* guid, u32 prop, void* val, u32 sz) override;
    void update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt) override;

public:
    CSoundRender_CoreA();
    virtual ~CSoundRender_CoreA();

    void _initialize() override;
    void _clear() override;
    void _restart() override;

    void set_master_volume(float f) override;

    const Fvector& listener_position() override { return Listener.position; }
};
extern CSoundRender_CoreA* SoundRenderA;
#endif
