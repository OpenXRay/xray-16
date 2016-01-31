#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_CoreA.h"

XRSOUND_API xr_token*		snd_devices_token	= NULL;
XRSOUND_API u32				snd_device_id		= u32(-1);
void CSound_manager_interface::_create()
{
    SoundRenderA = new CSoundRender_CoreA();
    SoundRender = SoundRenderA;
    Sound = SoundRender;
    SoundRender->bPresent = strstr(Core.Params, "-nosound")==nullptr;
	if (!SoundRender->bPresent)
        return;
	Sound->_initialize();
}

void CSound_manager_interface::_destroy	()
{
	Sound->_clear		();
    xr_delete			(SoundRender);
    Sound				= 0;
}
