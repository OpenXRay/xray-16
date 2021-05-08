#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"
#include "GameFont.h"
#include "xrCore/Text/StringConversion.hpp"
#include "xrCore/xr_token.h"

CInput* pInput = nullptr;
IInputReceiver dummyController;

xr_vector<xr_token> JoysticksToken;
xr_vector<xr_token> ControllersToken;

ENGINE_API float psMouseSens = 1.f;
ENGINE_API float psMouseSensScale = 1.f;
ENGINE_API Flags32 psMouseInvert = {false};

// Max events per frame
constexpr size_t MAX_KEYBOARD_EVENTS = 64;
constexpr size_t MAX_MOUSE_EVENTS = 256;
constexpr size_t MAX_CONTROLLER_EVENTS = 64;

float stop_vibration_time = flt_max;

static void OnErrorDialog(bool before)
{
    if (!pInput || !pInput->IsExclusiveMode() || Device.editor())
        return;

    if (before)
        pInput->GrabInput(false);
    else
        pInput->GrabInput(true);
}

bool CInput::InitJoystick()
{
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0)
    {
        SDL_Joystick* joystick;
        int count = SDL_NumJoysticks();
        for (int i = 0; i < count; ++i)
        {
            joystick = SDL_JoystickOpen(i);
            if (joystick)
            {
                JoysticksToken.emplace_back(xr_strdup(SDL_JoystickName(joystick)), i);
                joysticks.emplace_back(joystick);
                continue;
            }

            Log("SDL_JoystickOpen failed: ", SDL_GetError());
            return false;
        }

        if (joysticks.empty())
        {
            Log("No joysticks available");
            JoysticksToken.emplace_back(nullptr, -1);
            return false;
        }

        availableJoystick = true;
    }
    else
    {
        Log("Joystick SDL_InitSubSystem failed: ", SDL_GetError());
        return false;
    }

    return true;
}

void CInput::InitGameController()
{
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0)
    {
        SDL_GameController* controller;
        int count = SDL_NumJoysticks();
        for (int i = 0; i < count; ++i)
        {
            if (SDL_IsGameController(i))
            {
                controller = SDL_GameControllerOpen(i);
                if (controller)
                {
                    ControllersToken.emplace_back(xr_strdup(SDL_GameControllerName(controller)), i);
                    controllers.emplace_back(controller);
                    continue;
                }

                Log("SDL_GameControllerOpen failed: ", SDL_GetError());
                return;
            }
        }

        availableController = true;
    }
    else
    {
        Log("Game Controller SDL_InitSubSystem failed: ", SDL_GetError());
        return;
    }
}

void CInput::DisplayDevicesList()
{
    if (availableController && !controllers.empty())
    {
        Msg("Available game controllers[%d]:", controllers.size());

        for (auto& token : ControllersToken)
            if (token.name)
                Log(token.name);
    }
    else
        Log("No game controllers available");

    if (joysticks.size() > controllers.size())
    {
        Msg("Available joysticks[%d]:", joysticks.size() - controllers.size());

        if(controllers.size() > 0)
        {
            size_t it = 0;
            for (auto& token : JoysticksToken)
            {
                if (it < ControllersToken.size())
                {
                    if (token.id == ControllersToken[it].id)
                    {
                        ++it;
                        continue;
                    }
                }

                if (token.name)
                    Log(token.name);
            }
        }
    }

    ControllersToken.emplace_back(nullptr, -1);
    JoysticksToken.emplace_back(nullptr, -1);
}

CInput::CInput(const bool exclusive): availableJoystick(false), availableController(false)
{
    exclusiveInput = exclusive;

    Log("Starting INPUT device...");

    if (CInput::InitJoystick())
    {
        CInput::InitGameController();
        CInput::DisplayDevicesList();
    }

    m_mouseDelta = 25;

    mouseState.reset();
    keyboardState.reset();
    controllerState.reset();
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));

    //===================== Dummy pack
    iCapture(&dummyController);

    xrDebug::SetDialogHandler(OnErrorDialog);

    SDL_StopTextInput(); // sanity

    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH);
}

CInput::~CInput()
{
    GrabInput(false);

    for (auto& joystick : joysticks)
        SDL_JoystickClose(joystick);

    for (auto& controller : controllers)
        SDL_GameControllerClose(controller);

    for (auto& token : JoysticksToken)
        xr_free(token.name);
    JoysticksToken.clear();

    for (auto& token : ControllersToken)
        xr_free(token.name);
    ControllersToken.clear();

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
    const auto mousePrev = mouseState;

    bool mouseMoved = false;
    offs[0] = offs[1] = offs[2] = 0;

    SDL_Event events[MAX_MOUSE_EVENTS];
    SDL_PumpEvents();
    const auto count = SDL_PeepEvents(events, MAX_MOUSE_EVENTS,
        SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event event = events[i];

        switch (event.type)
        {
        case SDL_MOUSEMOTION:
            mouseMoved = true;
            mouseTimeStamp[0] = m_curTime + event.motion.timestamp;
            mouseTimeStamp[1] = m_curTime + event.motion.timestamp;
            offs[0] += event.motion.xrel;
            offs[1] += event.motion.yrel;
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouseState[event.button.button - 1] = true;
            cbStack.back()->IR_OnMousePress(event.button.button - 1);
            break;
        case SDL_MOUSEBUTTONUP:
            mouseState[event.button.button - 1] = false;
            cbStack.back()->IR_OnMouseRelease(event.button.button - 1);
            break;
        case SDL_MOUSEWHEEL:
            mouseMoved = true;
            mouseTimeStamp[2] = m_curTime + event.wheel.timestamp;
            mouseTimeStamp[3] = m_curTime + event.wheel.timestamp;
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
        if (mouseTimeStamp[1] && m_curTime - mouseTimeStamp[1] >= m_mouseDelta)
            cbStack.back()->IR_OnMouseStop(0, mouseTimeStamp[1] = 0);
        if (mouseTimeStamp[0] && m_curTime - mouseTimeStamp[0] >= m_mouseDelta)
            cbStack.back()->IR_OnMouseStop(0, mouseTimeStamp[0] = 0);
    }
}

void CInput::KeyUpdate()
{
    SDL_Event events[MAX_KEYBOARD_EVENTS];
    const auto count = SDL_PeepEvents(events, MAX_KEYBOARD_EVENTS,
        SDL_GETEVENT, SDL_KEYDOWN, SDL_TEXTINPUT);

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

        case SDL_TEXTINPUT:
            cbStack.back()->IR_OnTextInput(event.text.text);
            break;

        default:
            // Nothing here
            break;
        }
    }

    for (u32 i = 0; i < COUNT_KB_BUTTONS; ++i)
        if (keyboardState[i])
            cbStack.back()->IR_OnKeyboardHold(i);
}

void CInput::GameControllerUpdate()
{
    const auto controllerPrev = controllerState;

    SDL_Event events[MAX_CONTROLLER_EVENTS];
    const auto count = SDL_PeepEvents(events, MAX_CONTROLLER_EVENTS,
        SDL_GETEVENT, SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERDEVICEREMAPPED);

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event event = events[i];

        switch (event.type)
        {
        case SDL_CONTROLLERAXISMOTION:
            Log("Controller do axis motion");
            
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            controllerState[event.cbutton.button] = true;
            cbStack.back()->IR_OnControllerPress(event.cbutton.button);
            break;
        case SDL_CONTROLLERBUTTONUP:
            controllerState[event.cbutton.button] = false;
            cbStack.back()->IR_OnControllerRelease(event.cbutton.button);
            break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERDEVICEREMAPPED:
            break;
        }
    }

    for (int i = 0; i < COUNT_CONTROLLER_BUTTONS; ++i)
        if (controllerState[i] && controllerPrev[i])
            cbStack.back()->IR_OnKeyboardHold(ControllerButtonToKey[i]);
}

bool KbdKeyToButtonName(const int dik, xr_string& name)
{
    static std::locale locale("");

    if (dik >= 0)
    {
        name = StringFromUTF8(SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)dik)), locale);
        return true;
    }

    return false;
}

bool OtherDevicesKeyToButtonName(const int btn, xr_string& name)
{
    int idx = btn - MOUSE_1;

    if (idx >= 0)
    {
        name = keyboards[idx].key_local_name;
        return true;
    }

    return false;
}

bool CInput::GetKeyName(const int dik, pstr dest_str, int dest_sz)
{
    xr_string keyname;
    bool result;

    if (dik < COUNT_KB_BUTTONS)
        result = KbdKeyToButtonName(dik, keyname);
    else
        result = OtherDevicesKeyToButtonName(dik, keyname);

    if (keyname.empty())
        return false;

    xr_strcpy(dest_str, dest_sz, keyname.c_str());
    return result;
}

bool CInput::iGetAsyncKeyState(const int dik)
{
    if (dik < COUNT_KB_BUTTONS)
        return keyboardState[dik];

    if (dik >= MOUSE_1 && dik < MOUSE_MAX)
    {
        const int mk = dik - MOUSE_1;
        return iGetAsyncBtnState(mk);
    }

    if (dik >= XR_CONTROLLER_BUTTON_A && dik < XR_CONTROLLER_BUTTON_MAX)
    {
        const int mk = dik - XR_CONTROLLER_BUTTON_A;
        return iGetAsyncGpadBtnState(mk);
    }

    // unknown key ???
    return false;
}

bool CInput::iGetAsyncBtnState(const int btn)
{
    return mouseState[btn];
}

bool CInput::iGetAsyncGpadBtnState(const int btn)
{
    return controllerState[btn];
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
        for (size_t cnt = cbStack.size(); cnt > 0; --cnt)
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
    controllerState.reset();
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnAppDeactivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnDeactivate();

    mouseState.reset();
    keyboardState.reset();
    controllerState.reset();
    ZeroMemory(mouseTimeStamp, sizeof(mouseTimeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnFrame(void)
{
    stats.FrameStart();
    stats.FrameTime.Begin();
    m_curTime = RDEVICE.TimerAsync_MMT();

    if (Device.dwPrecacheFrame == 0 && !Device.IsAnselActive)
    {
        KeyUpdate();
        MouseUpdate();

        if (availableController)
            GameControllerUpdate();
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

bool CInput::IsExclusiveMode() const 
{
    return exclusiveInput;
}

void CInput::Feedback(u16 s1, u16 s2, float time)
{
    stop_vibration_time = RDEVICE.fTimeGlobal + time;
}
