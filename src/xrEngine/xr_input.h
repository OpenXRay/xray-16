#pragma once

#include "SDL.h"
#include <bitset>

// Mouse2 is a middle button in SDL,
// but in X-Ray this is a right button
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

static_assert(XR_CONTROLLER_BUTTON_COUNT == SDL_CONTROLLER_BUTTON_MAX, "Please, update xr_controller buttons definitions");

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

// SDL has separate axes for X and Y, we don't. Except trigger. Trigger should be separated.
static_assert(XR_CONTROLLER_AXIS_COUNT == 4, "Please, update xr_controller axis definitions");
static_assert(SDL_CONTROLLER_AXIS_MAX == 6, "Please, update xr_controller axis definitions");

constexpr int MouseButtonToKey[] = { MOUSE_1, MOUSE_3, MOUSE_2, MOUSE_4, MOUSE_5 };

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

class ENGINE_API IInputReceiver;

class ENGINE_API CInput
    : public pureFrame,
      public pureAppActivate,
      public pureAppDeactivate
{
public:
    enum
    {
        COUNT_MOUSE_AXIS = 4,
        COUNT_CONTROLLER_AXIS = SDL_CONTROLLER_AXIS_MAX
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
    BENCH_SEC_SCRAMBLEMEMBER1

    u32 mouseTimeStamp[COUNT_MOUSE_AXIS];

    std::bitset<COUNT_MOUSE_BUTTONS> mouseState;
    std::bitset<COUNT_KB_BUTTONS> keyboardState;
    std::bitset<COUNT_CONTROLLER_BUTTONS> controllerState;
    int controllerAxisState[COUNT_CONTROLLER_AXIS];

    xr_vector<IInputReceiver*> cbStack;

    xr_vector<SDL_Joystick*> joysticks;
    xr_vector<SDL_GameController*> controllers;

    void MouseUpdate();
    void KeyUpdate();
    void GameControllerUpdate();

    bool InitJoystick();
    void InitGameController();
    void DisplayDevicesList();

    InputStatistics stats;
    bool exclusiveInput;
    bool inputGrabbed;
    bool availableJoystick;
    bool availableController;

public:
    u32 m_curTime;
    u32 m_mouseDelta;

    const InputStatistics& GetStats() const { return stats; }
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);

    void iCapture(IInputReceiver* pc);
    void iRelease(IInputReceiver* pc);

    bool iGetAsyncKeyState(const int dik);
    bool iGetAsyncBtnState(const int btn);
    bool iGetAsyncGpadBtnState(const int btn);

    void iGetAsyncMousePos(Ivector2& p) const;
    void iSetMousePos(const Ivector2& p) const;

    void GrabInput(const bool grab);
    bool InputIsGrabbed() const;

    CInput(const bool exclusive = true);
    ~CInput();

    virtual void OnFrame();
    virtual void OnAppActivate();
    virtual void OnAppDeactivate();

    IInputReceiver* CurrentIR();

public:
    void ExclusiveMode(const bool exclusive);
    bool IsExclusiveMode() const;
    bool GetKeyName(const int dik, pstr dest, int dest_sz);

    void Feedback(u16 s1, u16 s2, float time);
};

extern ENGINE_API CInput* pInput;
