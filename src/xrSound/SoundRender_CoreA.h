#pragma once

#include "SoundRender_Core.h"

#include <al.h>
#include <alc.h>

#if __has_include(<alext.h>)
#include <alext.h>
#endif

#ifndef AL_EXT_float32
#define AL_EXT_float32 1
#define AL_FORMAT_MONO_FLOAT32                   0x10010
#define AL_FORMAT_STEREO_FLOAT32                 0x10011
#endif

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

protected:
    void update_listener(const Fvector& P, const Fvector& D, const Fvector& N, const Fvector& R, float dt) override;

public:
    CSoundRender_CoreA(CSoundManager& p);

    void _initialize_devices_list() override;
    void _initialize() override;
    void _clear() override;
    void _restart() override;

    void set_master_volume(float f) override;

};
