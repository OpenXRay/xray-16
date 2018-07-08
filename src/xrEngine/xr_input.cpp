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

    //===================== Dummy pack
    iCapture(&dummyController);

    SDL_SetRelativeMouseMode(SDL_TRUE);

    xrDebug::SetDialogHandler(on_error_dialog);

#ifdef ENGINE_BUILD
    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH);
#endif
}

CInput::~CInput(void)
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
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

void CInput::SetAllAcquire(BOOL bAcquire) {}

void CInput::SetMouseAcquire(BOOL bAcquire) {}
void CInput::SetKBDAcquire(BOOL bAcquire) {}
//-----------------------------------------------------------------------
BOOL b_altF4 = FALSE;
void CInput::KeyUpdate()
{
    if (b_altF4)
        return;

    const Uint8* state = SDL_GetKeyboardState(NULL);
#ifndef _EDITOR
    bool b_alt_tab = false;

    if (!b_altF4 && state[SDL_SCANCODE_F4] && (state[SDL_SCANCODE_RALT] || state[SDL_SCANCODE_LALT]))
    {
        b_altF4 = TRUE;
        Engine.Event.Defer("KERNEL:disconnect");
        Engine.Event.Defer("KERNEL:quit");
        SDL_Event ev;
        ev.type = SDL_QUIT;
        SDL_PushEvent(&ev);
    }
#endif

#ifndef _EDITOR
    if (Device.dwPrecacheFrame == 0)
#endif
    {
#ifndef _EDITOR
        if (state[SDL_SCANCODE_TAB] && (iGetAsyncKeyState(SDL_SCANCODE_LALT) || iGetAsyncKeyState(SDL_SCANCODE_RALT)))
            b_alt_tab = true;
#endif
    }

#ifndef _EDITOR
    if (b_alt_tab)
        SDL_MinimizeWindow(Device.m_sdlWnd);
#endif
}

bool CInput::get_key_name(int dik, LPSTR dest_str, int dest_sz)
{
    if (dik < SDL_NUM_SCANCODES)
    {
        const char* keyname = SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)dik));
        if (0 == strlen(keyname))
        {
            Msg("! cant convert dik_name for dik[%d]", dik);
            return false;
        }
        strcpy_s(dest_str, dest_sz, keyname);
    }

    return true;
}

#define MOUSE_1 (SDL_NUM_SCANCODES + SDL_BUTTON_LEFT)
#define MOUSE_8 (SDL_NUM_SCANCODES + 8)

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
        SDL_ShowCursor(SDL_DISABLE);
    }
    else
    {
        SDL_ShowCursor(SDL_ENABLE);
    }
}

//-------------------------------------------------------
void CInput::iCapture(IInputReceiver* p)
{
    VERIFY(p);

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
#ifndef _EDITOR
        if (Device.dwPrecacheFrame)
            continue;
#endif
        BOOL b_break_cycle = false;
        switch (event.type)
        {
        case SDL_KEYDOWN:
        {
            cbStack.back()->IR_OnKeyboardPress(event.key.keysym.scancode);

            if (0 != event.key.repeat)
                cbStack.back()->IR_OnKeyboardHold(event.key.keysym.scancode);

            KBState[event.key.keysym.scancode] = TRUE;
        }
        break;
        case SDL_KEYUP:
        {
            cbStack.back()->IR_OnKeyboardRelease(event.key.keysym.scancode);
            KBState[event.key.keysym.scancode] = FALSE;
        }
        break;
        case SDL_MOUSEMOTION:
        {
            timeStamp[0] = event.motion.timestamp;
            timeStamp[1] = event.motion.timestamp;
            cbStack.back()->IR_OnMouseMove(event.motion.xrel, event.motion.yrel);
        }
        break;
        case SDL_MOUSEBUTTONUP:
        {
            cbStack.back()->IR_OnMouseRelease(event.button.button);
            mouseState[event.button.button] = FALSE;
        }
        break;
        case SDL_MOUSEBUTTONDOWN:
        {
            cbStack.back()->IR_OnMousePress(event.button.button);

            if (mouseState[event.button.button])
                cbStack.back()->IR_OnMouseHold(event.button.button);

            mouseState[event.button.button] = TRUE;
        }
        break;
        case SDL_MOUSEWHEEL:
        {
            timeStamp[2] = event.wheel.timestamp;
            cbStack.back()->IR_OnMouseWheel(event.wheel.direction);
        }
        break;
        case SDL_QUIT: // go to outside event loop
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
            b_break_cycle = TRUE;
            break;
        default: break;
        }

        if (b_break_cycle)
            break;
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
    // pMouse->SetCooperativeLevel(Device.editor() ? Device.editor()->main_handle() : RDEVICE.m_sdlWnd,
    //    (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND | DISCL_NOWINKEY);
    // pMouse->Acquire();
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
