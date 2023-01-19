#include "stdafx.h"
#pragma hdrstop

#include "IInputReceiver.h"

#include "xr_input.h"

void IInputReceiver::IR_Capture()
{
    VERIFY(pInput);
    pInput->iCapture(this);
}

void IInputReceiver::IR_Release()
{
    VERIFY(pInput);
    pInput->iRelease(this);
}

void IInputReceiver::IR_OnActivate() {}

void IInputReceiver::IR_OnDeactivate()
{
    int i;
    for (i = 0; i < CInput::COUNT_KB_BUTTONS; i++)
        if (IR_GetKeyState(i))
            IR_OnKeyboardRelease(i);

    for (i = MOUSE_INVALID + 1; i < MOUSE_MAX; i++)
        if (IR_GetKeyState(i))
            IR_OnMouseRelease(i);

    for (i = XR_CONTROLLER_BUTTON_INVALID + 1; i < XR_CONTROLLER_BUTTON_MAX; i++)
        if (IR_GetKeyState(i))
            IR_OnControllerRelease(i, 0.0f, 0.0f);

    for (i = XR_CONTROLLER_AXIS_INVALID + 1; i < XR_CONTROLLER_AXIS_MAX; i++)
        if (IR_GetKeyState(i))
            IR_OnControllerRelease(i, 0.0f, 0.0f);
}

bool IInputReceiver::IR_GetKeyState(int dik) const
{
    VERIFY(pInput);
    return pInput->iGetAsyncKeyState(dik);
}
