#pragma once

#include "SoundRender_Core.h"

#include <al.h>
#include <alc.h>
#include <alext.h>

#ifdef DEBUG
#define A_CHK(expr)                                             \
    do                                                          \
    {                                                           \
        alGetError();                                           \
        expr;                                                   \
        ALenum err_ = alGetError();                             \
        VERIFY2(err_ == AL_NO_ERROR, (pcstr)alGetString(err_)); \
    } while (false)

#define AC_CHK(expr)                                                       \
    do                                                                     \
    {                                                                      \
        alcGetError(pDevice);                                              \
        expr;                                                              \
        ALCenum err_ = alcGetError(pDevice);                               \
        VERIFY2(err_ == ALC_NO_ERROR, (pcstr)alcGetString(pDevice, err_)); \
    } while (false)

#else
#define A_CHK(expr) \
    do              \
    {               \
        expr;       \
    } while (false)

#define AC_CHK(expr) \
    do               \
    {                \
        expr;        \
    } while (false)
#endif

class ALDeviceList;

class CSoundRender_CoreA : public CSoundRender_Core
{
    typedef CSoundRender_Core inherited;

    ALCdevice* pDevice;
    ALCcontext* pContext;
    ALDeviceList* pDeviceList;

    struct SListener
    {
        Fvector position{};
        Fvector velocity{};
        Fvector curVelocity{};
        Fvector prevVelocity{};
        Fvector accVelocity{};
        Fvector orientation[2]{};
    };

    SListener Listener;

protected:
    void update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt) override;

public:
    CSoundRender_CoreA(CSoundManager& p);

    void _initialize_devices_list() override;
    void _initialize() override;
    void _clear() override;
    void _restart() override;

    void set_master_volume(float f) override;

    const Fvector& listener_position() override { return Listener.position; }
};
