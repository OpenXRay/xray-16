#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"
#include "Include/editor/ide.hpp"
#include "GameFont.h"
#include "PerformanceAlert.hpp"

#ifndef _EDITOR
#include "xr_input_xinput.h"
#endif
CInput* pInput = NULL;
IInputReceiver dummyController;

ENGINE_API float psMouseSens = 1.f;
ENGINE_API float psMouseSensScale = 1.f;
ENGINE_API Flags32 psMouseInvert = {FALSE};

float stop_vibration_time = flt_max;

#define MOUSEBUFFERSIZE 64
#define KEYBOARDBUFFERSIZE 64

static bool g_exclusive = true;
static void on_error_dialog(bool before)
{
    if (!pInput || !g_exclusive || Device.editor())
        return;

    if (before)
    {
        pInput->unacquire();
        return;
    }

    pInput->acquire(true);
}

CInput::CInput(BOOL bExclusive, int deviceForInit)
{
    g_exclusive = !!bExclusive;

    Log("Starting INPUT device...");

    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(KBState, sizeof(KBState));
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));

    xrDebug::SetDialogHandler(on_error_dialog);

#ifdef ENGINE_BUILD
    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH);
#endif
}

CInput::~CInput(void)
{
#ifdef ENGINE_BUILD
    Device.seqFrame.Remove(this);
    Device.seqAppDeactivate.Remove(this);
    Device.seqAppActivate.Remove(this);
#endif
}

//-----------------------------------------------------------------------

void CInput::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** INPUT:    %2.2fms", pInput->GetStats().FrameTime.result);
}

void CInput::SetAllAcquire(BOOL bAcquire)
{
}

void CInput::SetMouseAcquire(BOOL bAcquire)
{
}
void CInput::SetKBDAcquire(BOOL bAcquire) {}
//-----------------------------------------------------------------------
void CInput::KeyUpdate(SDL_Event* event)
{
    bool b_dik_pause_was_pressed = false;

    if (SDL_SCANCODE_PAUSE == event->key.keysym.scancode)
    {
        if (SDL_KEYDOWN == event->key.type)
            b_dik_pause_was_pressed = true;
    }

#ifndef _EDITOR
    bool b_alt_tab = false;

    if (Device.dwPrecacheFrame == 0)
#endif
    {
        const Uint8* state = SDL_GetKeyboardState(NULL);

        if (SDL_KEYDOWN == event->key.type)
            cbStack.back()->IR_OnKeyboardPress(event->key.keysym.scancode);
        else if (SDL_KEYUP == event->key.type)
        {
            cbStack.back()->IR_OnKeyboardRelease(event->key.keysym.scancode);
#ifndef _EDITOR
            if (SDL_SCANCODE_TAB == event->key.keysym.scancode && KMOD_ALT == SDL_GetModState())
                b_alt_tab = true;
#endif
        }

        for (u32 i = 0; i < COUNT_KB_BUTTONS; i++)
            if (state[i])
                cbStack.back()->IR_OnKeyboardHold(i);
    }

#ifndef _EDITOR
    if (b_alt_tab)
        SDL_MinimizeWindow(Device.m_sdlWnd);
#endif
    /*
    #ifndef _EDITOR
    //update xinput if exist
    for( DWORD iUserIndex=0; iUserIndex<DXUT_MAX_CONTROLLERS; iUserIndex++ )
    {
    DXUTGetGamepadState( iUserIndex, &g_GamePads[iUserIndex], true, false );

    if( !g_GamePads[iUserIndex].bConnected )
    continue; // unplugged?

    bool new_b, old_b;
    new_b = !!(g_GamePads[iUserIndex].wPressedButtons & XINPUT_GAMEPAD_A);
    old_b = !!(g_GamePads[iUserIndex].wLastButtons & XINPUT_GAMEPAD_A);

    if(new_b != old_b)
    {
    if(old_b)
    cbStack.back()->IR_OnMousePress(0);
    else
    cbStack.back()->IR_OnMouseRelease(0);
    }
    int dx,dy;
    dx = iFloor(g_GamePads[iUserIndex].fThumbRX*6);
    dy = iFloor(g_GamePads[iUserIndex].fThumbRY*6);
    if(dx || dy)
    cbStack.back()->IR_OnMouseMove ( dx, dy );
    }

    if(Device.fTimeGlobal > stop_vibration_time)
    {
    stop_vibration_time = flt_max;
    set_vibration (0, 0);
    }
    //xinput
    #endif
    */
}

bool CInput::get_key_name(SDL_Scancode dik, LPSTR dest_str, int dest_sz)
{
    const char* keyname = SDL_GetKeyName(SDL_GetKeyFromScancode(dik));
    if (0 == strlen(keyname))
    {
        Msg("! cant convert dik_name for dik[%d]", dik);
        return false;
    }
    strcpy_s(dest_str, dest_sz, keyname);

    return true;
}

#define MOUSE_1 (0xED + 100)
#define MOUSE_8 (0xED + 107)

BOOL CInput::iGetAsyncKeyState(int dik)
{
    if (dik < COUNT_KB_BUTTONS)
        return !!KBState[dik];
    else if (dik >= MOUSE_1 && dik <= MOUSE_8)
    {
        int mk = dik - MOUSE_1;
        return iGetAsyncBtnState(mk);
    }
    else
        return FALSE; // unknown key ???
}

BOOL CInput::iGetAsyncBtnState(int btn) { return !!mouseState[btn]; }
void CInput::ClipCursor(bool clip)
{
    if (clip)
    {
        //::ClipCursor(&Device.m_rcWindowClient);
        SDL_ShowCursor(SDL_DISABLE);
    }
    else
    {
        //::ClipCursor(nullptr);
        SDL_ShowCursor(SDL_ENABLE);
    }
}

// void CInput::MouseUpdate(SDL_Event *event)
void CInput::MouseUpdate()
{
#ifndef _EDITOR
    if (Device.dwPrecacheFrame)
        return;
#endif
    BOOL mouse_prev[COUNT_MOUSE_BUTTONS];

    mouse_prev[0] = mouseState[0];
    mouse_prev[1] = mouseState[1];
    mouse_prev[2] = mouseState[2];
    mouse_prev[3] = mouseState[3];
    mouse_prev[4] = mouseState[4];
    mouse_prev[5] = mouseState[5];
    mouse_prev[6] = mouseState[6];
    mouse_prev[7] = mouseState[7];
}

//-------------------------------------------------------
void CInput::iCapture(IInputReceiver* p)
{
    VERIFY(p);
    
    MouseUpdate();

    // change focus
    if (!cbStack.empty())
        cbStack.back()->IR_OnDeactivate();
    cbStack.push_back(p);
    cbStack.back()->IR_OnActivate();

    // prepare for _new_ controller
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::iRelease(IInputReceiver* p)
{
    if (p == cbStack.back())
    {
        cbStack.back()->IR_OnDeactivate();
        cbStack.pop_back();

        if (!cbStack.empty())
            cbStack.back()->IR_OnActivate();
    }
    else
    {
        // we are not topmost receiver, so remove the nearest one
        u32 cnt = cbStack.size();
        for (; cnt > 0; --cnt)
            if (cbStack[cnt - 1] == p)
            {
                xr_vector<IInputReceiver*>::iterator it = cbStack.begin();
                std::advance(it, cnt - 1);
                cbStack.erase(it);
                break;
            }
    }
}

void CInput::OnAppActivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnActivate();

    SetAllAcquire(true);
    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(KBState, sizeof(KBState));
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnAppDeactivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnDeactivate();

    SetAllAcquire(false);
    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(KBState, sizeof(KBState));
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnFrame(void)
{
    SDL_Event event;

    stats.FrameStart();
    stats.FrameTime.Begin();
    dwCurTime = RDEVICE.TimerAsync_MMT();

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP: KeyUpdate(&event); continue;

        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEWHEEL:
            // MouseUpdate(&event);
            //MouseUpdate();
            continue;
//        case SDL_WINDOWEVENT:
//            switch (event.window.event)
//            {
//            case SDL_WINDOWEVENT_CLOSE:
//                event.type = SDL_QUIT;
//                SDL_PushEvent(&event);
//                continue;
//            case SDL_WINDOWEVENT_ENTER:
//#if SDL_VERSION_ATLEAST(2, 0, 5)
//            case SDL_WINDOWEVENT_TAKE_FOCUS:
//                RDEVICE.OnWM_Activate(event.window.data1, event.window.data2);
//                continue;
//#endif
//            default: SDL_Log("Window %d got unknown event %d", event.window.windowID, event.window.event); continue;
//            }
//            continue;
        case SDL_QUIT:
            Engine.Event.Defer("KERNEL:disconnect");
            Engine.Event.Defer("KERNEL:quit");
            break;

        default: continue;
        }
    }

    stats.FrameTime.End();
    stats.FrameEnd();
}

IInputReceiver* CInput::CurrentIR()
{
    if (cbStack.size())
        return cbStack.back();
    else
        return NULL;
}

void CInput::unacquire() {}

void CInput::acquire(const bool& exclusive)
{
    //pMouse->SetCooperativeLevel(Device.editor() ? Device.editor()->main_handle() : RDEVICE.m_sdlWnd,
    //    (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND | DISCL_NOWINKEY);
    //pMouse->Acquire();
}

void CInput::exclusive_mode(const bool& exclusive)
{
    g_exclusive = exclusive;
    unacquire();
    acquire(exclusive);
}
bool CInput::get_exclusive_mode() { return g_exclusive; }
void CInput::feedback(u16 s1, u16 s2, float time)
{
    stop_vibration_time = RDEVICE.fTimeGlobal + time;
#ifndef _EDITOR
//. set_vibration (s1, s2);
#endif
}
