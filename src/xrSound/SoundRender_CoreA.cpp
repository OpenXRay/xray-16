#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_CoreA.h"
#include "SoundRender_TargetA.h"

CSoundRender_CoreA* SoundRenderA = nullptr;

CSoundRender_CoreA::CSoundRender_CoreA() : CSoundRender_Core()
{
    pDevice = nullptr;
    pDeviceList = nullptr;
    pContext = nullptr;
#if defined(XR_PLATFORM_WINDOWS)
    eaxSet = nullptr;
    eaxGet = nullptr;
#endif
}

CSoundRender_CoreA::~CSoundRender_CoreA() {}
#if defined(XR_PLATFORM_WINDOWS)
bool CSoundRender_CoreA::EAXQuerySupport(bool isDeferred, const GUID* guid, u32 prop, void* val, u32 sz)
{
    if (AL_NO_ERROR != eaxGet(guid, prop, 0, val, sz))
        return false;
    if (AL_NO_ERROR != eaxSet(guid, (isDeferred ? DSPROPERTY_EAXLISTENER_DEFERRED : 0) | prop, 0, val, sz))
        return false;
    return true;
}

bool CSoundRender_CoreA::EAXTestSupport(bool isDeferred)
{
    EAXLISTENERPROPERTIES ep;
    if (!EAXQuerySupport(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom, sizeof(ep.lRoom)))
        return false;
    if (!EAXQuerySupport(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF, sizeof(ep.lRoomHF)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
                         &ep.flRoomRolloffFactor, sizeof(float)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYTIME,
                         &ep.flDecayTime, sizeof(float)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYHFRATIO,
                         &ep.flDecayHFRatio, sizeof(float)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONS,
                         &ep.lReflections, sizeof(ep.lReflections)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
                         &ep.flReflectionsDelay, sizeof(float)))
        return false;
    if (!EAXQuerySupport(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb, sizeof(ep.lReverb)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_REVERBDELAY,
                         &ep.flReverbDelay, sizeof(float)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
                         &ep.flEnvironmentDiffusion, sizeof(float)))
        return false;
    if (!EAXQuerySupport(isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
                         &ep.flAirAbsorptionHF, sizeof(float)))
        return false;
    if (!EAXQuerySupport(
        isDeferred, &DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags, sizeof(u32)))
        return false;
    return true;
}
#endif

void CSoundRender_CoreA::_restart() { inherited::_restart(); }
void CSoundRender_CoreA::_initialize()
{
    pDeviceList = xr_new<ALDeviceList>();

    if (0 == pDeviceList->GetNumDevices())
    {
        CHECK_OR_EXIT(0, "OpenAL: Can't create sound device.");
        xr_delete(pDeviceList);
    }
    pDeviceList->SelectBestDevice();
    R_ASSERT(snd_device_id >= 0 && snd_device_id < pDeviceList->GetNumDevices());
    const ALDeviceDesc& deviceDesc = pDeviceList->GetDeviceDesc(snd_device_id);
    // OpenAL device
    pDevice = alcOpenDevice(deviceDesc.name);
    if (pDevice == nullptr)
    {
        CHECK_OR_EXIT(0, "SOUND: OpenAL: Failed to create device.");
        bPresent = FALSE;
        return;
    }

    // Get the device specifier.
    const ALCchar* deviceSpecifier = alcGetString(pDevice, ALC_DEVICE_SPECIFIER);

    // Create context
    pContext = alcCreateContext(pDevice, nullptr);
    if (nullptr == pContext)
    {
        CHECK_OR_EXIT(0, "SOUND: OpenAL: Failed to create context.");
        bPresent = FALSE;
        alcCloseDevice(pDevice);
        pDevice = nullptr;
        return;
    }

    // clear errors
    alGetError();
    alcGetError(pDevice);

    // Set active context
    AC_CHK(alcMakeContextCurrent(pContext));

    // initialize listener
    A_CHK(alListener3f(AL_POSITION, 0.f, 0.f, 0.f));
    A_CHK(alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f));
    Fvector orient[2] = {{0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}};
    A_CHK(alListenerfv(AL_ORIENTATION, (const ALfloat*)&orient[0].x));
    A_CHK(alListenerf(AL_GAIN, 1.f));
#if defined(XR_PLATFORM_WINDOWS)
    // Check for EAX extension
    bEAX = deviceDesc.props.eax && !deviceDesc.props.eax_unwanted;

    eaxSet = (EAXSet)alGetProcAddress((pcstr)"EAXSet");
    if (eaxSet == nullptr)
        bEAX = false;
    eaxGet = (EAXGet)alGetProcAddress((pcstr)"EAXGet");
    if (eaxGet == nullptr)
        bEAX = false;

    if (bEAX)
    {
        bDeferredEAX = EAXTestSupport(true);
        bEAX = EAXTestSupport(false);
    }
#endif
    inherited::_initialize();

    // Pre-create targets
    CSoundRender_Target* T = nullptr;
    for (u32 tit = 0; tit < u32(psSoundTargets); tit++)
    {
        T = xr_new<CSoundRender_TargetA>();
        if (T->_initialize())
        {
            s_targets.push_back(T);
        }
        else
        {
            Log("! SOUND: OpenAL: Max targets - ", tit);
            T->_destroy();
            xr_delete(T);
            break;
        }
    }
}

void CSoundRender_CoreA::set_master_volume(float f)
{
    if (bPresent)
        A_CHK(alListenerf(AL_GAIN, f));
}

void CSoundRender_CoreA::_clear()
{
    inherited::_clear();
    // remove targets
    CSoundRender_Target* T = nullptr;
    for (u32 tit = 0; tit < s_targets.size(); tit++)
    {
        T = s_targets[tit];
        T->_destroy();
        xr_delete(T);
    }
    // Reset the current context to NULL.
    alcMakeContextCurrent(nullptr);
    // Release the context and the device.
    alcDestroyContext(pContext);
    pContext = nullptr;
    alcCloseDevice(pDevice);
    pDevice = nullptr;
    xr_delete(pDeviceList);
}

#if defined(XR_PLATFORM_WINDOWS)
void CSoundRender_CoreA::i_eax_set(const GUID* guid, u32 prop, void* val, u32 sz) { eaxSet(guid, prop, 0, val, sz); }
void CSoundRender_CoreA::i_eax_get(const GUID* guid, u32 prop, void* val, u32 sz) { eaxGet(guid, prop, 0, val, sz); }
#endif
void CSoundRender_CoreA::update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt)
{
    inherited::update_listener(P, D, N, dt);

    if (!Listener.position.similar(P))
    {
        Listener.position.set(P);
        bListenerMoved = TRUE;
    }
    Listener.orientation[0].set(D.x, D.y, -D.z);
    Listener.orientation[1].set(N.x, N.y, -N.z);

    A_CHK(alListener3f(AL_POSITION, Listener.position.x, Listener.position.y, -Listener.position.z));
    A_CHK(alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f));
    A_CHK(alListenerfv(AL_ORIENTATION, (const ALfloat*)&Listener.orientation[0].x));
}
