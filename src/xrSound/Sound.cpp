#include "stdafx.h"

#include "SoundRender_CoreA.h"

XRSOUND_API u32 snd_device_id = u32(-1);

void CSoundManager::CreateDevicesList()
{
    SoundRender = xr_new<CSoundRender_CoreA>(*this);
    SoundRender->bPresent = strstr(Core.Params, "-nosound") == nullptr;
    if (SoundRender->bPresent)
        SoundRender->_initialize_devices_list();
    else
        soundDevices.emplace_back("null", -1);

    GEnv.Sound = SoundRender;
}

void CSoundManager::Create()
{
    if (SoundRender->bPresent)
    {
        env_load();
        SoundRender->_initialize();
    }
}

void CSoundManager::Destroy()
{
    GEnv.Sound = nullptr;

    SoundRender->_clear();
    xr_delete(SoundRender);

    env_unload();

    for (auto& token : soundDevices)
    {
        pstr tokenName = const_cast<pstr>(token.name);
        xr_free(tokenName);
    }
    soundDevices.clear();
}

void CSoundManager::env_load()
{
    string_path fn;
    if (FS.exist(fn, "$game_data$", SNDENV_FILENAME))
    {
        soundEnvironment = xr_new<SoundEnvironment_LIB>();
        soundEnvironment->Load(fn);
    }
}

void CSoundManager::env_unload()
{
    if (soundEnvironment)
        soundEnvironment->Unload();
    xr_delete(soundEnvironment);
}

SoundEnvironment_LIB* CSoundManager::get_env_library() const
{
    return soundEnvironment;
}
