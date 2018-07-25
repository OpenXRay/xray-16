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
        pInput->ClipCursor(false);
        pInput->unacquire();
    }
    else
    {
        pInput->ClipCursor(true);
        pInput->acquire(true);
    }
}

CInput::CInput(bool exclusive, int deviceForInit)
{
    g_exclusive = exclusive;

    Log("Starting INPUT device...");

    MouseDelta = 25;

    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(keyboardState, sizeof(keyboardState));
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));

    //===================== Dummy pack
    iCapture(&dummyController);

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

void CInput::SetAllAcquire(bool bAcquire) {}

void CInput::SetMouseAcquire(bool bAcquire) {}
void CInput::SetKBDAcquire(bool bAcquire) {}


void CInput::MouseUpdate()
{
    SDL_PumpEvents();

    bool mouse_prev[COUNT_MOUSE_BUTTONS];

    mouse_prev[0] = mouseState[0];
    mouse_prev[1] = mouseState[1];
    mouse_prev[2] = mouseState[2];
    mouse_prev[3] = mouseState[3];
    mouse_prev[4] = mouseState[4];
    mouse_prev[5] = mouseState[5];
    mouse_prev[6] = mouseState[6];
    mouse_prev[7] = mouseState[7];

    bool mouseMoved = false;
    offs[0] = offs[1] = offs[2] = 0;

    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL))
    {
        switch (event.type)
        {
        case SDL_MOUSEMOTION:
            mouseMoved = true;
            mouseTimeStamp[0] = dwCurTime + event.motion.timestamp;
            mouseTimeStamp[1] = dwCurTime + event.motion.timestamp;
            offs[0] += event.motion.xrel;
            offs[1] += event.motion.yrel;
            break;
        case SDL_MOUSEBUTTONUP:
            mouseState[event.button.button - 1] = false;
            cbStack.back()->IR_OnMouseRelease(event.button.button - 1);
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouseState[event.button.button - 1] = true;
            cbStack.back()->IR_OnMousePress(event.button.button - 1);
            break;
        case SDL_MOUSEWHEEL:
            mouseMoved = true;
            mouseTimeStamp[2] = dwCurTime + event.wheel.timestamp;
            mouseTimeStamp[3] = dwCurTime + event.wheel.timestamp;
            offs[2] += event.wheel.y;
            offs[3] += event.wheel.x;
            break;
        }
    }

    auto isButtonOnHold = [&](int i)
    {
        if (mouseState[i] && mouse_prev[i])
            cbStack.back()->IR_OnMouseHold(i);
    };

    isButtonOnHold(0);
    isButtonOnHold(1);
    isButtonOnHold(2);
    isButtonOnHold(3);
    isButtonOnHold(4);
    isButtonOnHold(5);
    isButtonOnHold(6);
    isButtonOnHold(7);

    if (mouseMoved)
    {
        if (offs[0] || offs[1])
            cbStack.back()->IR_OnMouseMove(offs[0], offs[1]);
        if (offs[2] || offs[3])
            cbStack.back()->IR_OnMouseWheel(offs[2], offs[3]);
    }
    else
    {
        if (mouseTimeStamp[1] && dwCurTime - mouseTimeStamp[1] >= MouseDelta)
            cbStack.back()->IR_OnMouseStop(0, mouseTimeStamp[1] = 0);
        if (mouseTimeStamp[0] && dwCurTime - mouseTimeStamp[0] >= MouseDelta)
            cbStack.back()->IR_OnMouseStop(0, mouseTimeStamp[0] = 0);
    }
}

void CInput::KeyUpdate()
{
    SDL_PumpEvents();

    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYMAPCHANGED))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            if (event.key.repeat)
                continue;
            keyboardState[event.key.keysym.scancode] = true;
            cbStack.back()->IR_OnKeyboardPress(event.key.keysym.scancode);
            break;

        case SDL_KEYUP:
            keyboardState[event.key.keysym.scancode] = false;
            cbStack.back()->IR_OnKeyboardRelease(event.key.keysym.scancode);
            break;

        case SDL_TEXTINPUT: 
            Msg("Input: [%s]\n", event.text.text);
            break;
        }
    }

    for (u32 i = 0; i < COUNT_KB_BUTTONS; ++i)
        if (keyboardState[i])
            cbStack.back()->IR_OnKeyboardHold(i);
}

pcstr KeyToMouseButtonName(const int dik)
{
    switch (dik)
    {
    case MOUSE_1: return "Left mouse";
    case MOUSE_2: return "Right mouse";
    case MOUSE_3: return "Center mouse";
    case MOUSE_4: return "Fourth mouse";
    case MOUSE_5: return "Fifth mouse";
    case MOUSE_6: return "Sixth mouse";
    case MOUSE_7: return "Seventh mouse";
    case MOUSE_8: return "Eighth mouse";
    default: return "Unknown mouse";
    }
}

bool CInput::get_dik_name(int dik, LPSTR dest_str, int dest_sz)
{
    pcstr keyname;
    
    if (dik < SDL_NUM_SCANCODES)
        keyname = SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)dik));
    else
        keyname = KeyToMouseButtonName(dik);

    if (0 == strlen(keyname))
    {
        if (dik == SDL_SCANCODE_UNKNOWN)
            keyname = "Unknown";
        else
        {
            Msg("! Can't convert dik_name for dik[%d]", dik);
            return false;
        }
    }

    xr_strcpy(dest_str, dest_sz, keyname);
    return true;
}

bool CInput::iGetAsyncKeyState(int dik)
{
    if (dik < COUNT_KB_BUTTONS)
        return keyboardState[dik];

    if (dik >= MOUSE_1 && dik <= MOUSE_8)
    {
        const int mk = dik - MOUSE_1;
        return iGetAsyncBtnState(mk);
    }

    // unknown key ???
    return false;
}

bool CInput::iGetAsyncBtnState(int btn)
{
    return mouseState[btn];
}
void CInput::ClipCursor(bool clip)
{
    if (clip)
    {
        SDL_SetWindowGrab(Device.m_sdlWnd, SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    else
    {
        SDL_SetWindowGrab(Device.m_sdlWnd, SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_FALSE);
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
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
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
    ZeroMemory(keyboardState, sizeof(keyboardState));
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnAppDeactivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnDeactivate();

    SetAllAcquire(false);
    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(keyboardState, sizeof(keyboardState));
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnFrame(void)
{
    stats.FrameStart();
    stats.FrameTime.Begin();
    dwCurTime = RDEVICE.TimerAsync_MMT();

    if (Device.dwPrecacheFrame == 0)
    {
        KeyUpdate();
        MouseUpdate();
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
