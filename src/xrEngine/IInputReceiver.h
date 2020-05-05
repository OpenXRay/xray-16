// IInputReceiver.h: interface for the IInputReceiver class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#ifndef IINPUTRECEIVERH
#define IINPUTRECEIVERH
#include "xrCore/_flags.h"
#include "xrGame/xr_level_controller.h"

// fwd. decl.
template <class T>
struct _vector2;
using Fvector2 = _vector2<float>;
using Ivector2 = _vector2<int>;

class ENGINE_API IInputReceiver
{
public:
    virtual ~IInputReceiver() = default;
    static void IR_GetLastMouseDelta(Ivector2& p);
    static void IR_GetMousePosScreen(Ivector2& p);
    static void IR_GetMousePosWindow(SDL_Window* sdlWnd, Ivector2& p);
    static void IR_GetMousePosWindow(Ivector2& p);
    static void IR_GetMousePosIndependent(Fvector2& f);
    static void IR_GetMousePosIndependentCrop(Fvector2& f);
    bool IR_GetKeyState(int dik);
    bool IR_GetBtnState(int btn);
    virtual void IR_Capture(void);
    virtual void IR_Release(void);

    virtual void IR_OnDeactivate(void);
    virtual void IR_OnActivate(void);

    virtual void IR_OnMousePress(int /*btn*/) {}
    virtual void IR_OnMouseRelease(int /*btn*/) {}
    virtual void IR_OnMouseHold(int /*btn*/) {}
    virtual void IR_OnMouseWheel(int /*x*/, int /*y*/) {}
    virtual void IR_OnMouseMove(int /*x*/, int /*y*/) {}
    virtual void IR_OnMouseStop(int /*x*/, int /*y*/) {}

    virtual void IR_OnKeyboardPress(int /*dik*/) {}
    virtual void IR_OnKeyboardRelease(int /*dik*/) {}
    virtual void IR_OnKeyboardHold(int /*dik*/) {}
    virtual void IR_OnTextInput(pcstr text) {}

    virtual void IR_OnJoystickMove(int /*axis*/, int /*value*/) {}
    virtual void IR_OnJoystickPress(int /*dik*/) {}
    virtual void IR_OnJoystickRelease(int /*dik*/) {}

    virtual void IR_OnControllerMove(int /*axis*/, int /*value*/) {}
    virtual void IR_OnControllerPress(int /*dik*/) {}
    virtual void IR_OnControllerRelease(int /*dik*/) {}
};

ENGINE_API extern float psMouseSens;
ENGINE_API extern float psMouseSensScale;
ENGINE_API extern Flags32 psMouseInvert;

#endif
