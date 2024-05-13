#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"
#include "GameFont.h"
#include "xrCore/Text/StringConversion.hpp"
#include "xrCore/xr_token.h"

#include <locale>

CInput* pInput = nullptr;
IInputReceiver dummyController;

ENGINE_API float psMouseSens = 1.f;
ENGINE_API float psMouseSensScale = 1.f;
ENGINE_API Flags32 psMouseInvert = {};

ENGINE_API float psControllerStickSens = 1.f;
ENGINE_API float psControllerStickSensScale = 1.f;
ENGINE_API float psControllerStickDeadZone = 0.f;
ENGINE_API float psControllerSensorSens = 1.f;
ENGINE_API float psControllerSensorDeadZone = 0.f;
ENGINE_API Flags32 psControllerInvertY = { false };
ENGINE_API Flags32 psControllerEnableSensors = { true };

ENGINE_API float psControllerCursorAutohideTime = 1.5f;

static bool AltF4Pressed = false;

// Max events per frame
constexpr size_t MAX_KEYBOARD_EVENTS = 64;
constexpr size_t MAX_MOUSE_EVENTS = 256;
constexpr size_t MAX_CONTROLLER_EVENTS = 64;

CInput::CInput(const bool exclusive)
{
    ZoneScoped;

    exclusiveInput = exclusive;

    Log("Starting INPUT device...");

    m_mouseDelta = 25;

    mouseState.reset();
    keyboardState.reset();
    controllerState.reset();
    ZeroMemory(controllerAxisState, sizeof(controllerAxisState));
    last_input_controller = -1;

    //===================== Dummy pack
    iCapture(&dummyController);

    SDL_SetHint(SDL_HINT_WINDOWS_CLOSE_ON_ALT_F4, "0"); // We need to handle it manually

    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH);

    if (strstr(Core.Params, "-no_gamepad"))
        return;

    if (SDL_InitSubSystem(SDL_INIT_GAMEPAD) == 0)
    {
        int num_joysticks;
        SDL_JoystickID *joysticks = SDL_GetJoysticks(&num_joysticks);
        if (joysticks) {
            for (int i = 0; i < num_joysticks; ++i)
                OpenController(i);
        }
        SDL_free(joysticks);
    }
}

CInput::~CInput()
{
    ZoneScoped;

    GrabInput(false);

    for (auto& controller : controllers)
        SDL_CloseGamepad(controller);
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);

    Device.seqFrame.Remove(this);
    Device.seqAppDeactivate.Remove(this);
    Device.seqAppActivate.Remove(this);
}

void CInput::OpenController(int idx)
{
    if (!SDL_IsGamepad(idx))
        return;

    const auto controller = SDL_OpenGamepad(idx);
    if (!controller)
        return;

    if (psControllerEnableSensors.test(1))
        SDL_SetGamepadSensorEnabled(controller, SDL_SENSOR_GYRO, SDL_TRUE);
    controllers.emplace_back(controller);
}

void CInput::EnableControllerSensors(bool enable)
{
    for (auto controller : controllers)
        SDL_SetGamepadSensorEnabled(controller, SDL_SENSOR_GYRO, enable ? SDL_TRUE : SDL_FALSE);
}

//-----------------------------------------------------------------------

void CInput::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** INPUT:    %2.2fms", GetStats().FrameTime.result);
}

void CInput::SetCurrentInputType(InputType type)
{
    currentInputType = type;

    if (type == KeyboardMouse)
        last_input_controller = -1;
}

void CInput::MouseUpdate()
{
    ZoneScoped;

    // Mouse2 is a middle button in SDL,
    // but in X-Ray this is a right button
    constexpr int RemapIdx[] = { 0, 2, 1, 3, 4 };
    constexpr int IdxToKey[] = { MOUSE_1, MOUSE_2, MOUSE_3, MOUSE_4, MOUSE_5 };
    static_assert(std::size(RemapIdx) == COUNT_MOUSE_BUTTONS);
    static_assert(std::size(IdxToKey) == COUNT_MOUSE_BUTTONS);

    bool mouseMoved = false;
    int offs[COUNT_MOUSE_AXIS]{};
    const auto mousePrev = mouseState;
    mouseAxisState[2] = 0;
    mouseAxisState[3] = 0;

    SDL_Event events[MAX_MOUSE_EVENTS];
    SDL_PumpEvents();
    const auto count = SDL_PeepEvents(events, MAX_MOUSE_EVENTS,
        SDL_GETEVENT, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_WHEEL);

    if (count)
        SetCurrentInputType(KeyboardMouse);

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event& event = events[i];

        switch (event.type)
        {
        case SDL_EVENT_MOUSE_MOTION:
            mouseMoved = true;
            offs[0] += event.motion.xrel;
            offs[1] += event.motion.yrel;
            mouseAxisState[0] = event.motion.x;
            mouseAxisState[1] = event.motion.y;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            const auto idx = RemapIdx[event.button.button - 1];
            mouseState[idx] = true;
            cbStack.back()->IR_OnMousePress(IdxToKey[idx]);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            const auto idx = RemapIdx[event.button.button - 1];
            mouseState[idx] = false;
            cbStack.back()->IR_OnMouseRelease(IdxToKey[idx]);
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
            mouseMoved = true;
            offs[2] += event.wheel.x;
            offs[3] += event.wheel.y;
            mouseAxisState[2] += event.wheel.x;
            mouseAxisState[3] += event.wheel.y;
            break;
        }
    }

    for (int i = 0; i < MOUSE_COUNT; ++i)
    {
        if (mouseState[i] && mousePrev[i])
            cbStack.back()->IR_OnMouseHold(IdxToKey[i]);
    }

    if (mouseMoved)
    {
        if (offs[0] || offs[1])
            cbStack.back()->IR_OnMouseMove(offs[0], offs[1]);
        if (offs[2] || offs[3])
            cbStack.back()->IR_OnMouseWheel(offs[2], offs[3]);
    }
}

void CInput::KeyUpdate()
{
    ZoneScoped;

    SDL_Event events[MAX_KEYBOARD_EVENTS];
    const auto count = SDL_PeepEvents(events, MAX_KEYBOARD_EVENTS,
        SDL_GETEVENT, SDL_EVENT_KEY_DOWN, SDL_EVENT_KEYMAP_CHANGED);

    // Let iGetAsyncKeyState work correctly during this frame immediately
    for (int i = 0; i < count; ++i)
    {
        const SDL_Event& event = events[i];

        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.repeat)
                continue;
            keyboardState[event.key.keysym.scancode] = true;
            break;

        case SDL_EVENT_KEY_UP:
            keyboardState[event.key.keysym.scancode] = false;
            break;
        }
    }

    if (keyboardState[SDL_SCANCODE_F4] && (keyboardState[SDL_SCANCODE_LALT] || keyboardState[SDL_SCANCODE_RALT]))
    {
        AltF4Pressed = true;
        Engine.Event.Defer("KERNEL:disconnect");
        Engine.Event.Defer("KERNEL:quit");
        return;
    }

    if (count)
        SetCurrentInputType(KeyboardMouse);

    // If textInputCounter has changed,
    // we assume that text input target changed.
    // Theoretically, this is not always true, though.
    // But we always can change the solution.
    // If we find out something not work as expected.
    const auto cnt = textInputCounter;

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event& event = events[i];

        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.repeat)
                continue;
            cbStack.back()->IR_OnKeyboardPress(event.key.keysym.scancode);
            break;

        case SDL_EVENT_KEY_UP:
            cbStack.back()->IR_OnKeyboardRelease(event.key.keysym.scancode);
            break;

        case SDL_EVENT_TEXT_INPUT:
            if (cnt != textInputCounter)
                continue; // if input target changed, skip this frame
            cbStack.back()->IR_OnTextInput(event.text.text);
            break;

        case SDL_EVENT_KEYMAP_CHANGED:
            seqKeyMapChanged.Process();
            break;
        }
    }

    for (u32 i = 0; i < COUNT_KB_BUTTONS; ++i)
        if (keyboardState[i])
            cbStack.back()->IR_OnKeyboardHold(i);
}

void CInput::ControllerUpdate()
{
    ZoneScoped;

    constexpr int ControllerButtonToKey[] =
    {
        XR_CONTROLLER_BUTTON_A,
        XR_CONTROLLER_BUTTON_B,
        XR_CONTROLLER_BUTTON_X,
        XR_CONTROLLER_BUTTON_Y,
        XR_CONTROLLER_BUTTON_BACK,
        XR_CONTROLLER_BUTTON_GUIDE,
        XR_CONTROLLER_BUTTON_START,
        XR_CONTROLLER_BUTTON_LEFTSTICK,
        XR_CONTROLLER_BUTTON_RIGHTSTICK,
        XR_CONTROLLER_BUTTON_LEFTSHOULDER,
        XR_CONTROLLER_BUTTON_RIGHTSHOULDER,
        XR_CONTROLLER_BUTTON_DPAD_UP,
        XR_CONTROLLER_BUTTON_DPAD_DOWN,
        XR_CONTROLLER_BUTTON_DPAD_LEFT,
        XR_CONTROLLER_BUTTON_DPAD_RIGHT,
        XR_CONTROLLER_BUTTON_MISC1,
        XR_CONTROLLER_BUTTON_PADDLE1,
        XR_CONTROLLER_BUTTON_PADDLE2,
        XR_CONTROLLER_BUTTON_PADDLE3,
        XR_CONTROLLER_BUTTON_PADDLE4,
        XR_CONTROLLER_BUTTON_TOUCHPAD,
    };

    SDL_Event events[MAX_CONTROLLER_EVENTS];
    auto count = SDL_PeepEvents(events, MAX_CONTROLLER_EVENTS,
        SDL_GETEVENT, SDL_EVENT_GAMEPAD_ADDED, SDL_EVENT_GAMEPAD_ADDED);

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event& event = events[i];
        OpenController(event.cdevice.which);
    }

    if (!IsControllerAvailable())
        return;

    const int controllerDeadZone = int(psControllerStickDeadZone * (SDL_JOYSTICK_AXIS_MAX / 100.f)); // raw

    const auto controllerPrev = controllerState;
    decltype(controllerAxisState) controllerAxisStatePrev;
    CopyMemory(controllerAxisStatePrev, controllerAxisState, sizeof(controllerAxisState));

    constexpr SDL_EventType MAX_EVENT = SDL_EVENT_GAMEPAD_SENSOR_UPDATE;

    count = SDL_PeepEvents(events, MAX_CONTROLLER_EVENTS,
        SDL_GETEVENT, SDL_EVENT_GAMEPAD_AXIS_MOTION, MAX_EVENT);

    for (int i = 0; i < count; ++i)
    {
        const SDL_Event& event = events[i];

        switch (event.type)
        {
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            if (event.jaxis.axis >= COUNT_CONTROLLER_AXIS)
                break; // SDL added new axis, not supported by engine yet

            if (last_input_controller != event.jaxis.which) // don't write if don't really need to
                last_input_controller = event.jaxis.which;

            if (std::abs(event.jaxis.value) < controllerDeadZone)
                controllerAxisState[event.jaxis.axis] = 0;
            else
            {
                controllerAxisState[event.jaxis.axis] = event.jaxis.value;
                SetCurrentInputType(Controller);
            }
            break;
        }

        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            if (event.button.button >= XR_CONTROLLER_BUTTON_COUNT)
                break; // SDL added new button, not supported by engine yet

            if (last_input_controller != event.button.which) // don't write if don't really need to
                last_input_controller = event.button.which;
            SetCurrentInputType(Controller);

            controllerState[event.button.button] = true;
            cbStack.back()->IR_OnControllerPress(ControllerButtonToKey[event.button.button], 1.f, 0.f);
            break;

        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (event.button.button >= XR_CONTROLLER_BUTTON_COUNT)
                break; // SDL added new button, not supported by engine yet

            if (last_input_controller != event.button.which) // don't write if don't really need to
                last_input_controller = event.button.which;
            SetCurrentInputType(Controller);

            controllerState[event.button.button] = false;
            cbStack.back()->IR_OnControllerRelease(ControllerButtonToKey[event.button.button], 0.f, 0.f);
            break;

        case SDL_EVENT_GAMEPAD_ADDED:
            OpenController(event.cdevice.which);
            break;

        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            const auto controller = SDL_GetGamepadFromInstanceID(event.cdevice.which);
            const auto it = std::find(controllers.begin(), controllers.end(), controller);
            if (it != controllers.end())
                controllers.erase(it);
            break;
        }

        case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
        {
            if (last_input_controller != event.gsensor.which) // only use data from the recently used controller
                break;
            if (event.gsensor.sensor != SDL_SENSOR_GYRO)
                break;

            const auto gyro = Fvector { -event.gsensor.data[1], -event.gsensor.data[0], -event.gsensor.data[2] };
            if (!gyro.similar(Fvector{ 0.f, 0.f, 0.f }, psControllerSensorDeadZone))
                cbStack.back()->IR_OnControllerAttitudeChange(gyro);
            break;
        }
        } // switch (event.type)
    }

    for (int i = 0; i < COUNT_CONTROLLER_BUTTONS; ++i)
    {
        if (controllerState[i] && controllerPrev[i])
            cbStack.back()->IR_OnControllerHold(ControllerButtonToKey[i], 1.f, 0.f);
    }

    const auto checkAxis = [this](int axis, int rawX, int rawY, int prevRawX, int prevRawY)
    {
        const auto quantize = [](int value)
        {
            return value / (SDL_JOYSTICK_AXIS_MAX / 100.f);
        };

        const auto x = quantize(rawX), y = quantize(rawY), prevX = quantize(prevRawX), prevY = quantize(prevRawY);
        const bool xActive = !fis_zero(x), yActive = !fis_zero(y), prevXActive = !fis_zero(prevX), prevYActive = !fis_zero(prevY);

        if ((xActive && prevXActive) || (yActive && prevYActive))
            cbStack.back()->IR_OnControllerHold(axis, x, y);
        else if (xActive || yActive)
            cbStack.back()->IR_OnControllerPress(axis, x, y);
        else if (prevXActive || prevYActive)
            cbStack.back()->IR_OnControllerRelease(axis, 0.f, 0.f);
    };

    checkAxis(XR_CONTROLLER_AXIS_LEFT,          controllerAxisState[0], controllerAxisState[1], controllerAxisStatePrev[0], controllerAxisStatePrev[1]);
    checkAxis(XR_CONTROLLER_AXIS_RIGHT,         controllerAxisState[2], controllerAxisState[3], controllerAxisStatePrev[2], controllerAxisStatePrev[3]);
    checkAxis(XR_CONTROLLER_AXIS_TRIGGER_LEFT,  controllerAxisState[4], 0,                      controllerAxisStatePrev[4], 0);
    checkAxis(XR_CONTROLLER_AXIS_TRIGGER_RIGHT, controllerAxisState[5], 0,                      controllerAxisStatePrev[5], 0);
}

bool KbdKeyToButtonName(const int dik, xr_string& result)
{
    static std::locale locale("");

    if (dik >= 0)
    {
        cpcstr name = SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)dik));
        if (name && name[0])
        {
            result = StringFromUTF8(name, locale);
            return true;
        }
    }

    return false;
}

bool OtherDevicesKeyToButtonName(const int btn, xr_string& /*result*/)
{
    if (btn > CInput::COUNT_KB_BUTTONS)
    {
        // XXX: Not implemented
        return false;
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

bool CInput::iGetAsyncKeyState(const int key)
{
    if (key < COUNT_KB_BUTTONS)
        return keyboardState[key];

    if (key > MOUSE_INVALID && key < MOUSE_MAX)
    {
        const int idx = key - (MOUSE_INVALID + 1);
        return mouseState[idx];
    }

    if (key > XR_CONTROLLER_BUTTON_INVALID && key < XR_CONTROLLER_BUTTON_MAX)
    {
        const int idx = key - (XR_CONTROLLER_BUTTON_INVALID + 1);
        return controllerState[idx];
    }

    if (key > XR_CONTROLLER_AXIS_INVALID && key < XR_CONTROLLER_AXIS_MAX)
    {
        switch (static_cast<EControllerAxis>(key))
        {
        case XR_CONTROLLER_AXIS_LEFT:
            return controllerAxisState[SDL_GAMEPAD_AXIS_LEFTX] || controllerAxisState[SDL_GAMEPAD_AXIS_LEFTY];
        case XR_CONTROLLER_AXIS_RIGHT:
            return controllerAxisState[SDL_GAMEPAD_AXIS_RIGHTX] || controllerAxisState[SDL_GAMEPAD_AXIS_RIGHTY];
        case XR_CONTROLLER_AXIS_TRIGGER_LEFT:
            return controllerAxisState[SDL_GAMEPAD_AXIS_LEFT_TRIGGER];
        case XR_CONTROLLER_AXIS_TRIGGER_RIGHT:
            return controllerAxisState[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER];
        }
    }

    // unknown key ???
    return false;
}

void CInput::iGetAsyncScrollPos(Ivector2& p) const
{
    p = { mouseAxisState[2], mouseAxisState[3] };
}

bool CInput::iGetAsyncMousePos(Ivector2& p, bool global /*= false*/) const
{
    float x, y;
    if (global)
    {
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
        SDL_GetGlobalMouseState(&x, &y);
        p.x = static_cast<int>(x);
        p.y = static_cast<int>(y);
        return true;
#endif
        // if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE unavailable
        // fallback to SDL_GetMouseState
        // but report false
    }
    SDL_GetMouseState(&x, &y);
    p.x = static_cast<int>(x);
    p.y = static_cast<int>(y);
    return !global;
}

bool CInput::iSetMousePos(const Ivector2& p, bool global /*= false*/) const
{
    if (global)
    {
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
        SDL_WarpMouseGlobal(p.x, p.y);
        return true;
#endif
        // if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE unavailable
        // fallback to SDL_WarpMouseInWindow
        // but report false
    }

    SDL_WarpMouseInWindow(Device.m_sdlWnd, p.x, p.y);
    return !global;
}

void CInput::GrabInput(const bool grab)
{
    // Self descriptive
    if(grab)
    {
        SDL_HideCursor();
    }
    else
    {
        SDL_ShowCursor();
    }

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

void CInput::EnableTextInput()
{
    ++textInputCounter;

    if (textInputCounter == 1)
        SDL_StartTextInput();

    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_TEXT_EDITING, SDL_EVENT_TEXT_INPUT);
}

void CInput::DisableTextInput()
{
    --textInputCounter;
    if (textInputCounter < 0)
        textInputCounter = 0;

    if (textInputCounter == 0)
        SDL_StopTextInput();

    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_TEXT_EDITING, SDL_EVENT_TEXT_INPUT);
}

bool CInput::IsTextInputEnabled() const
{
    return textInputCounter > 0;
}

void CInput::RegisterKeyMapChangeWatcher(pureKeyMapChanged* watcher, int priority /*= REG_PRIORITY_NORMAL*/)
{
    seqKeyMapChanged.Add(watcher, priority);
}

void CInput::RemoveKeyMapChangeWatcher(pureKeyMapChanged* watcher)
{
    seqKeyMapChanged.Remove(watcher);
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
    ZeroMemory(controllerAxisState, sizeof(controllerAxisState));
    last_input_controller = -1;
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
    ZeroMemory(controllerAxisState, sizeof(controllerAxisState));
    last_input_controller = -1;
}

void CInput::OnAppDeactivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnDeactivate();

    mouseState.reset();
    keyboardState.reset();
    controllerState.reset();
    ZeroMemory(controllerAxisState, sizeof(controllerAxisState));
    last_input_controller = -1;
}

void CInput::OnFrame(void)
{
    ZoneScoped;

    if (AltF4Pressed)
        return;

    stats.FrameStart();
    stats.FrameTime.Begin();

    if (Device.dwPrecacheFrame == 0 && !Device.IsAnselActive)
    {
        ControllerUpdate();
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

    return nullptr;
}

void CInput::ExclusiveMode(const bool exclusive)
{
    GrabInput(false);

    // Original CInput was using DirectInput in exclusive mode
    // In which keyboard was grabbed with the mouse.
    // It produces problems on Linux, so it's disabled by default.
    if (strstr(Core.Params, "-grab_keyboard"))
        SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, exclusive ? "1" : "0");
    exclusiveInput = exclusive;

    GrabInput(true);
}

bool CInput::IsExclusiveMode() const
{
    return exclusiveInput;
}

void CInput::Feedback(FeedbackType type, float s1, float s2, float duration)
{
    const u16 s1_rumble = iFloor(u16(-1) * clampr(s1, 0.0f, 1.0f));
    const u16 s2_rumble = iFloor(u16(-1) * clampr(s2, 0.0f, 1.0f));
    const u32 duration_ms = duration < 0.f ? 0 : iFloor(duration * 1000.f);

    switch (type)
    {
    case FeedbackController:
    {
        if (last_input_controller != -1)
        {
            const auto controller = SDL_GetGamepadFromInstanceID(last_input_controller);
            SDL_RumbleGamepad(controller, s1_rumble, s2_rumble, duration_ms);
        }
        break;
    }

    case FeedbackTriggers:
    {
        if (last_input_controller != -1)
        {
            const auto controller = SDL_GetGamepadFromInstanceID(last_input_controller);
            SDL_RumbleGamepadTriggers(controller, s1_rumble, s2_rumble, duration_ms);
        }
        break;
    }

    default: NODEFAULT;
    }
}
