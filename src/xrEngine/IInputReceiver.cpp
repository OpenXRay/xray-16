#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"
#include "SDL_syswm.h"

void IInputReceiver::IR_Capture(void)
{
    VERIFY(pInput);
    pInput->iCapture(this);
}

void IInputReceiver::IR_Release(void)
{
    VERIFY(pInput);
    pInput->iRelease(this);
}

void IInputReceiver::IR_GetLastMouseDelta(Ivector2& p)
{
    VERIFY(pInput);
    pInput->iGetLastMouseDelta(p);
}

void IInputReceiver::IR_OnDeactivate(void)
{
    int i;
    for (i = 0; i < CInput::COUNT_KB_BUTTONS; i++)
        if (IR_GetKeyState(i))
            IR_OnKeyboardRelease(i);

    for (i = 0; i < CInput::COUNT_MOUSE_BUTTONS; i++)
        if (IR_GetBtnState(i))
            IR_OnMouseRelease(i);
    //IR_OnMouseStop(DIMOFS_X, 0);
    //IR_OnMouseStop(DIMOFS_Y, 0);
}

void IInputReceiver::IR_OnActivate(void) {}
BOOL IInputReceiver::IR_GetKeyState(int dik)
{
    VERIFY(pInput);
    return pInput->iGetAsyncKeyState(dik);
}

BOOL IInputReceiver::IR_GetBtnState(int btn)
{
    VERIFY(pInput);
    return pInput->iGetAsyncBtnState(btn);
}

void IInputReceiver::IR_GetMousePosScreen(Ivector2& p) 
{ 
    SDL_GetMouseState(&p.x, &p.y);
}

void IInputReceiver::IR_GetMousePosReal(SDL_Window* m_sdlWnd, Ivector2& p)
{
    IR_GetMousePosScreen(p);
}
void IInputReceiver::IR_GetMousePosReal(Ivector2& p) { IR_GetMousePosReal(RDEVICE.m_sdlWnd, p); }
void IInputReceiver::IR_GetMousePosIndependent(Fvector2& f)
{
    Ivector2 p;
    IR_GetMousePosReal(p);
    f.set(2.f * float(p.x) / float(RDEVICE.dwWidth) - 1.f, 2.f * float(p.y) / float(RDEVICE.dwHeight) - 1.f);
}
void IInputReceiver::IR_GetMousePosIndependentCrop(Fvector2& f)
{
    IR_GetMousePosIndependent(f);
    if (f.x < -1.f)
        f.x = -1.f;
    if (f.x > 1.f)
        f.x = 1.f;
    if (f.y < -1.f)
        f.y = -1.f;
    if (f.y > 1.f)
        f.y = 1.f;
}
