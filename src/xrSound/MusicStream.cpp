// MusicStream.cpp: implementation of the CMusicStream class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MusicStream.h"
#include "xr_streamsnd.h"

#if defined(WINDOWS)
#include "x_ray.h"
#endif
#include "GameFont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMusicStream::CMusicStream() {}
CMusicStream::~CMusicStream() {}
int CMusicStream::FindEmptySlot()
{
    for (u32 i = 0; i < streams.size(); i++)
    {
        if (streams[i] == 0)
            return i;
    }
    return -1;
}

CSoundStream* CMusicStream::CreateSound(LPCSTR name)
{
    int slot;
    CSoundStream* pSnd = xr_new<CSoundStream>();
    pSnd->Load(name);

    if ((slot = FindEmptySlot()) >= 0)
    {
        streams[slot] = pSnd;
        return pSnd;
    }

    streams.push_back(pSnd);
    return pSnd;
}

void CMusicStream::DeleteSound(CSoundStream* pSnd)
{
    int slot = -1;
    for (u32 i = 0; i < streams.size(); i++)
    {
        if (streams[i] == pSnd)
        {
            slot = i;
            break;
        }
    }

    if (slot >= 0)
    {
        xr_delete(streams[slot]);
        pSnd = NULL;
    }
}

void CMusicStream::OnMove()
{
    for (u32 i = 0; i < streams.size(); i++)
        streams[i]->OnMove();
    /*	if (psDeviceFlags&rsStatistic)
        {
            int cnt = 0;
            for(int i=0; i<streams.size(); i++) cnt+=streams[i]->isPlaying()?1:0;
            pApp->pFont->Out(0,0.5f,"%d / %d",cnt,streams.size());
        }
    */
}

void CMusicStream::Reload()
{
    // ...
}

void CMusicStream::Update()
{
    for (u32 i = 0; i < streams.size(); i++)
        if (streams[i])
            streams[i]->bNeedUpdate = true;
}
