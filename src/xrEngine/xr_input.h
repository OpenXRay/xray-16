#pragma once

#include <bitset>

#include <SDL3/SDL.h>

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(SDL_PLATFORM_APPLE) && TARGET_OS_IOS) && !defined(__amigaos4__)
#   define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE 1
#endif

DECLARE_MESSAGE(KeyMapChanged);

enum EMouseButton
{
    MOUSE_INVALID = SDL_NUM_SCANCODES,
    MOUSE_1, // Left
    MOUSE_2, // Right
    MOUSE_3, // Middle
    MOUSE_4, // X1
    MOUSE_5, // X2
    MOUSE_MAX,
    MOUSE_COUNT = MOUSE_MAX - MOUSE_INVALID - 1
};

enum EControllerButton
{
    XR_CONTROLLER_BUTTON_INVALID = MOUSE_MAX,
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
    XR_CONTROLLER_BUTTON_MISC1,    /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button */
    XR_CONTROLLER_BUTTON_PADDLE1,  /* Xbox Elite paddle P1 */
    XR_CONTROLLER_BUTTON_PADDLE2,  /* Xbox Elite paddle P3 */
    XR_CONTROLLER_BUTTON_PADDLE3,  /* Xbox Elite paddle P2 */
    XR_CONTROLLER_BUTTON_PADDLE4,  /* Xbox Elite paddle P4 */
    XR_CONTROLLER_BUTTON_TOUCHPAD, /* PS4/PS5 touchpad button */
    XR_CONTROLLER_BUTTON_MAX,
    XR_CONTROLLER_BUTTON_COUNT = XR_CONTROLLER_BUTTON_MAX - XR_CONTROLLER_BUTTON_INVALID - 1
};

// SDL has separate axes for X and Y, we don't. Except trigger. Trigger should be separated.
enum EControllerAxis
{
    XR_CONTROLLER_AXIS_INVALID = XR_CONTROLLER_BUTTON_MAX,
    XR_CONTROLLER_AXIS_LEFT,
    XR_CONTROLLER_AXIS_RIGHT,
    XR_CONTROLLER_AXIS_TRIGGER_LEFT,
    XR_CONTROLLER_AXIS_TRIGGER_RIGHT,
    XR_CONTROLLER_AXIS_MAX,
    XR_CONTROLLER_AXIS_COUNT = XR_CONTROLLER_AXIS_MAX - XR_CONTROLLER_AXIS_INVALID - 1
};

class ENGINE_API IInputReceiver;

class ENGINE_API CInput
    : public pureFrame,
      public pureAppActivate,
      public pureAppDeactivate
{
public:
    enum FeedbackType
    {
        FeedbackController, // Entire gamepad
        FeedbackTriggers,
    };

    enum InputType
    {
        KeyboardMouse,
        Controller,
    };

    enum
    {
        COUNT_MOUSE_AXIS = 4,
        COUNT_CONTROLLER_AXIS = SDL_GAMEPAD_AXIS_MAX
    };

    enum
    {
        COUNT_MOUSE_BUTTONS = MOUSE_COUNT,
        COUNT_KB_BUTTONS = SDL_NUM_SCANCODES,
        COUNT_CONTROLLER_BUTTONS = XR_CONTROLLER_BUTTON_COUNT
    };

    struct InputStatistics
    {
        CStatTimer FrameTime;

        void FrameStart() { FrameTime.FrameStart(); }
        void FrameEnd() { FrameTime.FrameEnd(); }
    };

private:
    std::bitset<COUNT_MOUSE_BUTTONS> mouseState;
    std::bitset<COUNT_KB_BUTTONS> keyboardState;
    std::bitset<COUNT_CONTROLLER_BUTTONS> controllerState;
    int mouseAxisState[COUNT_MOUSE_AXIS];
    int controllerAxisState[COUNT_CONTROLLER_AXIS];
    s32 last_input_controller;

    xr_vector<IInputReceiver*> cbStack;

    xr_vector<SDL_Gamepad*> controllers;

    InputType currentInputType{ KeyboardMouse };

    void SetCurrentInputType(InputType type);

    void MouseUpdate();
    void KeyUpdate();
    void ControllerUpdate();

    void OpenController(int idx);

    InputStatistics stats;
    bool exclusiveInput;
    bool inputGrabbed;
    int textInputCounter{};

    MessageRegistry<pureKeyMapChanged> seqKeyMapChanged;

public:
    u32 m_mouseDelta;

    const InputStatistics& GetStats() const { return stats; }
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);

    void iCapture(IInputReceiver* pc);
    void iRelease(IInputReceiver* pc);

    bool iGetAsyncKeyState(const int key);
    bool iAnyMouseButtonDown() const { return mouseState.any(); }
    bool iAnyKeyButtonDown() const { return keyboardState.any(); }
    bool iAnyControllerButtonDown() const { return controllerState.any(); }

    void iGetAsyncScrollPos(Ivector2& p) const;
    bool iGetAsyncMousePos(Ivector2& p, bool global = false) const;
    bool iSetMousePos(const Ivector2& p, bool global = false) const;

    void GrabInput(const bool grab);
    bool InputIsGrabbed() const;

    void EnableTextInput();
    void DisableTextInput();
    bool IsTextInputEnabled() const;

    void RegisterKeyMapChangeWatcher(pureKeyMapChanged* watcher, int priority = REG_PRIORITY_NORMAL);
    void RemoveKeyMapChangeWatcher(pureKeyMapChanged* watcher);

    CInput(const bool exclusive = true);
    ~CInput();

    virtual void OnFrame();
    virtual void OnAppActivate();
    virtual void OnAppDeactivate();

    IInputReceiver* CurrentIR();

    bool IsControllerAvailable() const { return !controllers.empty(); }
    void EnableControllerSensors(bool enable);

    auto GetCurrentInputType() const { return currentInputType; }
    auto IsCurrentInputTypeController() const { return GetCurrentInputType() == InputType::Controller; }
    auto IsCurrentInputTypeKeyboardMouse() const { return GetCurrentInputType() == InputType::KeyboardMouse; }

public:
    void ExclusiveMode(const bool exclusive);
    bool IsExclusiveMode() const;
    bool GetKeyName(const int dik, pstr dest, int dest_sz);

    /**
    *  Start a gamepad vibration effect
    *  Each call to this function cancels any previous vibration effect, and calling it with 0 intensity stops any vibration.
    *
    *  @param type Feedback source
    *  @param s1 The intensity of the low frequency (left) motor or left trigger motor, from 0 to 1
    *  @param s2 The intensity of the high frequency (right) motor or right trigger motor, from 0 to 1
    *  @param duration The duration of the rumble effect, in seconds
    */
    void Feedback(FeedbackType type, float s1, float s2, float duration);
};

extern ENGINE_API CInput* pInput;
