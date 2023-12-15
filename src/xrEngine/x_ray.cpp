//-----------------------------------------------------------------------------
// File: x_ray.cpp
//
// Programmers:
// Oles - Oles Shishkovtsov
// AlexMX - Alexander Maksimchuk
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "XR_IOConsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "xrCDB/ISpatial.h"
#include "xrSASH.h"
#include "xr_input.h"

//---------------------------------------------------------------------

ENGINE_API CApplication* pApp = nullptr;

//////////////////////////////////////////////////////////////////////////
struct _SoundProcessor : public pureFrame
{
    virtual void OnFrame()
    {
        // Msg ("------------- sound: %d [%3.2f,%3.2f,%3.2f]",u32(Device.dwFrame),VPUSH(Device.vCameraPosition));
        GEnv.Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
    }
} SoundProcessor;

CApplication::CApplication()
{
    // events
    eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
    eConsole = Engine.Event.Handler_Attach("KERNEL:console", this);

    // Register us
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

    if (psDeviceFlags.test(mtSound))
        Device.seqFrameMT.Add(&SoundProcessor);
    else
        Device.seqFrame.Add(&SoundProcessor);
}

CApplication::~CApplication()
{
    Console->Hide();

    Device.seqFrameMT.Remove(&SoundProcessor);
    Device.seqFrame.Remove(&SoundProcessor);
    Device.seqFrame.Remove(this);

    // events
    Engine.Event.Handler_Detach(eQuit, this);
    Engine.Event.Handler_Detach(eConsole, this);
}

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
    if (E == eQuit)
    {
        if (pInput != nullptr)
            pInput->GrabInput(false);

        g_SASH.EndBenchmark();

        SDL_Event quit = { SDL_QUIT };
        SDL_PushEvent(&quit);
    }
    else if (E == eConsole)
    {
        pstr command = (pstr)P1;
        Console->ExecuteCommand(command, false);
        xr_free(command);
    }
}

// Sequential
void CApplication::OnFrame()
{
    Engine.Event.OnFrame();
}
