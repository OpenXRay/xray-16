#pragma once

// SDL_NUM_SCANCODES - max vavue in SDL_SCANCODE_* enum
#define MOUSE_1 (SDL_NUM_SCANCODES + SDL_BUTTON_LEFT)
#define MOUSE_2 (SDL_NUM_SCANCODES + SDL_BUTTON_RIGHT)
#define MOUSE_3 (SDL_NUM_SCANCODES + SDL_BUTTON_MIDDLE)

#define MOUSE_4 (SDL_NUM_SCANCODES + SDL_BUTTON_X1)
#define MOUSE_5 (SDL_NUM_SCANCODES + SDL_BUTTON_X2)
#define MOUSE_6 (SDL_NUM_SCANCODES + 6)
#define MOUSE_7 (SDL_NUM_SCANCODES + 7)
#define MOUSE_8 (SDL_NUM_SCANCODES + 8)

constexpr int MouseButtonToKey[] = { MOUSE_1, MOUSE_3, MOUSE_2, MOUSE_4, MOUSE_5, MOUSE_6, MOUSE_7, MOUSE_8 };

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
    case MOUSE_6: return 5 + i;
    case MOUSE_7: return 6 + i;
    case MOUSE_8: return 7 + i;
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
        COUNT_MOUSE_BUTTONS = 8,
        COUNT_MOUSE_AXIS = 4,
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

    bool mouseState[COUNT_MOUSE_BUTTONS];
    bool keyboardState[COUNT_KB_BUTTONS];

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
    void ClipCursor(const bool clip);
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
