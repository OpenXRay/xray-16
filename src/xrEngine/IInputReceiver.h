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
    static void IR_GetMousePosReal(SDL_Window* m_sdlWnd, Ivector2& p);
    static void IR_GetMousePosReal(Ivector2& p);
    static void IR_GetMousePosIndependent(Fvector2& f);
    static void IR_GetMousePosIndependentCrop(Fvector2& f);
    BOOL IR_GetKeyState(int dik);
    BOOL IR_GetBtnState(int btn);
    void IR_Capture(void);
    void IR_Release(void);

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
};

ENGINE_API extern float psMouseSens;
ENGINE_API extern float psMouseSensScale;
ENGINE_API extern Flags32 psMouseInvert;

#endif
