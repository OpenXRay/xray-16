#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"
#include "Include/editor/ide.hpp"
#include "GameFont.h"
#include "PerformanceAlert.hpp"
#include "xrCore/Text/StringConversion.hpp"

CInput* pInput = NULL;
IInputReceiver dummyController;

ENGINE_API float psMouseSens = 1.f;
ENGINE_API float psMouseSensScale = 1.f;
ENGINE_API Flags32 psMouseInvert = {FALSE};

// Max events per frame
constexpr size_t MAX_KEYBOARD_EVENTS = 64;
constexpr size_t MAX_MOUSE_EVENTS = 256;

float stop_vibration_time = flt_max;

static void on_error_dialog(bool before)
{
    if (!pInput || !pInput->IsExclusiveMode() || Device.editor())
        return;

    if (before)
        pInput->GrabInput(false);
    else
        pInput->GrabInput(true);
}

CInput::CInput(const bool exclusive)
{
    exclusiveInput = exclusive;

    Log("Starting INPUT device...");

    MouseDelta = 25;

    mouseState.reset();
    keyboardState.reset();
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));

    //===================== Dummy pack
    iCapture(&dummyController);

    xrDebug::SetDialogHandler(on_error_dialog);

    SDL_StopTextInput(); // sanity

    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH);
}

CInput::~CInput()
{
    GrabInput(false);
    Device.seqFrame.Remove(this);
    Device.seqAppDeactivate.Remove(this);
    Device.seqAppActivate.Remove(this);
}

//-----------------------------------------------------------------------

void CInput::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** INPUT:    %2.2fms", pInput->GetStats().FrameTime.result);
}

void CInput::MouseUpdate()
{
    SDL_PumpEvents();

    const auto mousePrev = mouseState;

    bool mouseMoved = false;
    offs[0] = offs[1] = offs[2] = 0;

    SDL_Event events[MAX_MOUSE_EVENTS];
    const auto count = SDL_PeepEvents(events, MAX_MOUSE_EVENTS,
        SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event event = events[i];

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

    for (int i = 0; i < MOUSE_COUNT; ++i)
        if (mouseState[i] && mousePrev[i])
            cbStack.back()->IR_OnMouseHold(i);

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

    SDL_Event events[MAX_KEYBOARD_EVENTS];
    const auto count = SDL_PeepEvents(events, MAX_KEYBOARD_EVENTS,
        SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP);

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event event = events[i];

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
    default: return "Unknown mouse";
    }
}

bool CInput::get_dik_name(int dik, LPSTR dest_str, int dest_sz)
{
    xr_string keyname;
    static std::locale locale("");

    if (dik < SDL_NUM_SCANCODES)
        keyname = StringFromUTF8(SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)dik)), locale);
    else
        keyname = KeyToMouseButtonName(dik);

    if (keyname.empty())
    {
        if (dik == SDL_SCANCODE_UNKNOWN)
            keyname = "Unknown";
        else
            return false;
    }

    xr_strcpy(dest_str, dest_sz, keyname.c_str());
    return true;
}

bool CInput::iGetAsyncKeyState(int dik)
{
    if (dik < COUNT_KB_BUTTONS)
        return keyboardState[dik];

    if (dik >= MOUSE_1 && dik < MOUSE_MAX)
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

void CInput::GrabInput(const bool grab)
{
    // Self descriptive
    SDL_ShowCursor(grab ? SDL_FALSE : SDL_TRUE);

    // Clip cursor to the current window
    // If SDL_HINT_GRAB_KEYBOARD is set then the keyboard will be grabbed too
    SDL_SetWindowGrab(Device.m_sdlWnd, grab ? SDL_TRUE : SDL_FALSE);

    // Grab the mouse
    if (exclusiveInput)
        SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);

    // We're done here.
    inputGrabbed = grab;
}

bool CInput::InputIsGrabbed() const
{
    return inputGrabbed;
}

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

    mouseState.reset();
    keyboardState.reset();
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnAppDeactivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnDeactivate();

    mouseState.reset();
    keyboardState.reset();
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnFrame(void)
{
    stats.FrameStart();
    stats.FrameTime.Begin();
    dwCurTime = RDEVICE.TimerAsync_MMT();

    if (Device.dwPrecacheFrame == 0 && !Device.IsAnselActive)
    {
        KeyUpdate();
        MouseUpdate();
    }
    else
    {
        SDL_FlushEvents(SDL_KEYDOWN, SDL_MOUSEWHEEL);
    }

    stats.FrameTime.End();
    stats.FrameEnd();
}

IInputReceiver* CInput::CurrentIR()
{
    if (cbStack.size())
        return cbStack.back();
    return nullptr;
}

void CInput::ExclusiveMode(const bool exclusive)
{
    GrabInput(false);

    // Original CInput was using DirectInput in exclusive mode
    // In which keyboard was grabbed with the mouse.
    // Uncomment it below, if you want.
    //SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, exclusive ? "1" : "0");
    exclusiveInput = exclusive;

    GrabInput(true);
}

bool CInput::IsExclusiveMode() const { return exclusiveInput; }

void CInput::feedback(u16 s1, u16 s2, float time)
{
    stop_vibration_time = RDEVICE.fTimeGlobal + time;
#ifndef _EDITOR
//. set_vibration (s1, s2);
#endif
}
