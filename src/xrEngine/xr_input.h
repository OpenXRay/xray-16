#pragma once

#include "SDL.h"
#include <bitset>

// Mouse2 is a middle button in SDL,
// but in X-Ray this is a right button
enum xrMouse
{
    MOUSE_1 = SDL_NUM_SCANCODES + SDL_BUTTON_LEFT,
    MOUSE_2 = SDL_NUM_SCANCODES + SDL_BUTTON_RIGHT,
    MOUSE_3 = SDL_NUM_SCANCODES + SDL_BUTTON_MIDDLE,
    MOUSE_4 = SDL_NUM_SCANCODES + SDL_BUTTON_X1,
    MOUSE_5 = SDL_NUM_SCANCODES + SDL_BUTTON_X2,
    MOUSE_MAX,
    MOUSE_COUNT = MOUSE_5 - SDL_NUM_SCANCODES
};

enum xrController
{
    XR_CONTROLLER_BUTTON_INVALID = -1,
    XR_CONTROLLER_BUTTON_A = SDL_CONTROLLER_BUTTON_A + MOUSE_MAX,
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
    XR_CONTROLLER_BUTTON_MAX
};

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
    XR_CONTROLLER_BUTTON_DPAD_RIGHT
};

inline int KeyToMouseButton(const int dik, const bool fromZero = true)
{
    int i = 0;
    if (!fromZero)
        ++i;

    switch (dik)
    {
    case MOUSE_1: return 0 + i;
    case MOUSE_2: return 1 + i;
    case MOUSE_3: return 2 + i;
    case MOUSE_4: return 3 + i;
    case MOUSE_5: return 4 + i;
    default: return dik - SDL_NUM_SCANCODES + i;
    }
}

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
        COUNT_CONTROLLER_AXIS = 4
    };

    enum
    {
        COUNT_MOUSE_BUTTONS = MOUSE_COUNT,
        COUNT_KB_BUTTONS = SDL_NUM_SCANCODES
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

    int offs[COUNT_MOUSE_AXIS];

    std::bitset<COUNT_MOUSE_BUTTONS> mouseState;
    std::bitset<COUNT_KB_BUTTONS> keyboardState;

    xr_vector<IInputReceiver*> cbStack;

    void MouseUpdate();
    void KeyUpdate();

    InputStatistics stats;
    bool exclusiveInput;
    bool inputGrabbed;

public:
    u32 dwCurTime;
    u32 MouseDelta;

    const InputStatistics& GetStats() const { return stats; }
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);

    void iCapture(IInputReceiver* pc);
    void iRelease(IInputReceiver* pc);
    bool iGetAsyncKeyState(int dik);
    bool iGetAsyncBtnState(int btn);
    void iGetLastMouseDelta(Ivector2& p) { p.set(offs[0], offs[1]); }
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
    bool get_dik_name(int dik, LPSTR dest, int dest_sz);

    void feedback(u16 s1, u16 s2, float time);
};

extern ENGINE_API CInput* pInput;
