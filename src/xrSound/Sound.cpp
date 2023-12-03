#include "stdafx.h"

#include "SoundRender_CoreA.h"

XRSOUND_API xr_token* snd_devices_token = nullptr;
XRSOUND_API u32 snd_device_id = u32(-1);

void CSoundManager::CreateDevicesList()
{
    SoundRender = xr_new<CSoundRender_CoreA>();
    SoundRender->bPresent = strstr(Core.Params, "-nosound") == nullptr;
    if (SoundRender->bPresent)
        SoundRender->_initialize_devices_list();

    GEnv.Sound = SoundRender;
}

void CSoundManager::Create()
{
    if (SoundRender->bPresent)
        SoundRender->_initialize();
}

void CSoundManager::Destroy()
{
    SoundRender->_clear();
    xr_delete(SoundRender);

    GEnv.Sound = nullptr;
}
