#include "stdafx.h"

#include "SoundRender_CoreA.h"

XRSOUND_API u32 snd_device_id = u32(-1);

ISoundScene* DefaultSoundScene{};

void CSoundManager::CreateDevicesList()
{
    static bool noSound = strstr(Core.Params, "-nosound");

    SoundRender = xr_new<CSoundRender_CoreA>(*this);

    if (!noSound)
        SoundRender->_initialize_devices_list();

    if (!SoundRender->bPresent)
        soundDevices.emplace_back(nullptr, -1);

    GEnv.Sound = SoundRender;
}

void CSoundManager::Create()
{
    if (SoundRender->bPresent)
    {
        SoundRender->_initialize();
    }
}

void CSoundManager::Destroy()
{
    GEnv.Sound = nullptr;

    SoundRender->_clear();
    xr_delete(SoundRender);

    for (auto& token : soundDevices)
    {
        pstr tokenName = const_cast<pstr>(token.name);
        xr_free(tokenName);
    }
    soundDevices.clear();
}

bool CSoundManager::IsSoundEnabled() const
{
    return SoundRender && SoundRender->bPresent;
}
