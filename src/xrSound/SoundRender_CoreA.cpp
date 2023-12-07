#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_CoreA.h"
#include "SoundRender_TargetA.h"
#include "OpenALDeviceList.h"
#include "SoundRender_EffectsA_EAX.h"
#include "SoundRender_EffectsA_EFX.h"

CSoundRender_CoreA::CSoundRender_CoreA(CSoundManager& p)
    : CSoundRender_Core(p)
{
    pDevice = nullptr;
    pDeviceList = nullptr;
    pContext = nullptr;
}

void CSoundRender_CoreA::_restart() { inherited::_restart(); }

void CSoundRender_CoreA::_initialize_devices_list()
{
    pDeviceList = xr_new<ALDeviceList>();

    if (0 == pDeviceList->GetNumDevices())
    {
        Log("! SOUND: OpenAL: No sound devices found.");
        bPresent = false;
        xr_delete(pDeviceList);
    }
    bPresent = true;
}

void CSoundRender_CoreA::_initialize()
{
    if (!pDeviceList)
    {
        VERIFY2(pDeviceList, "Probably incorrect initialization order. Make sure to call _initialize_devices_list() first.");
        bPresent = false;
        return;
    }

    pDeviceList->SelectBestDevice();
    R_ASSERT(snd_device_id >= 0 && snd_device_id < pDeviceList->GetNumDevices());
    const ALDeviceDesc& deviceDesc = pDeviceList->GetDeviceDesc(snd_device_id);

    // OpenAL device
    pDevice = alcOpenDevice(deviceDesc.name);
    if (!pDevice)
    {
        Log("! SOUND: OpenAL: Failed to create device.");
        bPresent = false;
        return;
    }

    // Get the device specifier.
    //const ALCchar* deviceSpecifier = alcGetString(pDevice, ALC_DEVICE_SPECIFIER);

    // Create context
    pContext = alcCreateContext(pDevice, nullptr);
    if (!pContext)
    {
        Log("! SOUND: OpenAL: Failed to create context.");
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

#if AL_EXT_float32
    supports_float_pcm = alIsExtensionPresent("AL_EXT_FLOAT32")  // first is OpenAL Soft,
                      || alIsExtensionPresent("AL_EXT_float32"); // second is macOS
#endif

    auto auxSlot = ALuint(-1);
#if defined(XR_HAS_EAX)
    // Check for EAX extension
    if (deviceDesc.props.eax && !m_effects)
    {
        m_effects = xr_new<CSoundRender_EffectsA_EAX>();
        if (!m_effects->initialized())
        {
            Log("SOUND: OpenAL: Failed to initialize EAX.");
            xr_delete(m_effects);
        }
    }
#elif defined(XR_HAS_EFX)
    // Check for EFX extension
    if (deviceDesc.props.efx && !m_effects)
    {
        m_effects = xr_new<CSoundRender_EffectsA_EFX>();
        if (m_effects->initialized())
            auxSlot = ((CSoundRender_EffectsA_EFX*)m_effects)->get_slot();
        else
        {
            Log("SOUND: OpenAL: Failed to initialize EFX.");
            xr_delete(m_effects);
        }
    }
#endif
    inherited::_initialize();

    // Pre-create targets
    CSoundRender_Target* T = nullptr;
    for (u32 tit = 0; tit < u32(psSoundTargets); tit++)
    {
        T = xr_new<CSoundRender_TargetA>(auxSlot);
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
    xr_delete(m_effects);
    // remove targets
    CSoundRender_Target* T = nullptr;
    for (auto& sr_target : s_targets)
    {
        T = sr_target;
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

void CSoundRender_CoreA::update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt)
{
    inherited::update_listener(P, D, N, dt);

    // Use exponential moving average for a nice smooth doppler effect.
    Listener.prevVelocity.set(Listener.accVelocity);
    Listener.curVelocity.sub(P, Listener.position);
    Listener.accVelocity.set(Listener.curVelocity.mul(psSoundVelocityAlpha).add(Listener.prevVelocity.mul(1.f - psSoundVelocityAlpha)));
    Listener.prevVelocity.set(Listener.accVelocity).div(dt);

    if (!Listener.position.similar(P))
    {
        Listener.position.set(P);
        bListenerMoved = TRUE;
    }
    Listener.orientation[0].set(D.x, D.y, -D.z);
    Listener.orientation[1].set(N.x, N.y, -N.z);

    A_CHK(alListener3f(AL_POSITION, Listener.position.x, Listener.position.y, -Listener.position.z));
    A_CHK(alListener3f(AL_VELOCITY, Listener.prevVelocity.x, Listener.prevVelocity.y, -Listener.prevVelocity.z));
    A_CHK(alListenerfv(AL_ORIENTATION, (const ALfloat*)&Listener.orientation[0].x));
}
