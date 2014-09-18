#pragma once
/*
#include <xinput.h>

struct DXUT_GAMEPAD;

#define DXUT_GAMEPAD_TRIGGER_THRESHOLD      30
#define DXUT_MAX_CONTROLLERS				4  // XInput handles up to 4 controllers 

HRESULT DXUTGetGamepadState(	DWORD dwPort, 
								DXUT_GAMEPAD* pGamePad, 
								bool bThumbstickDeadZone, 
								bool bSnapThumbstickToCardinals );
void set_vibration			(u16 s1, u16 s2);

struct DXUT_GAMEPAD
{
    // From XINPUT_GAMEPAD
    WORD    wButtons;
    BYTE    bLeftTrigger;
    BYTE    bRightTrigger;
    SHORT   sThumbLX;
    SHORT   sThumbLY;
    SHORT   sThumbRX;
    SHORT   sThumbRY;

    // Device properties
    XINPUT_CAPABILITIES caps;
    bool    bConnected; // If the controller is currently connected
    bool    bInserted;  // If the controller was inserted this frame
    bool    bRemoved;   // If the controller was removed this frame

    // Thumb stick values converted to range [-1,+1]
    float   fThumbRX;
    float   fThumbRY;
    float   fThumbLX;
    float   fThumbLY;

    // Records which buttons were pressed this frame.
    // These are only set on the first frame that the button is pressed
    WORD    wPressedButtons;
    bool    bPressedLeftTrigger;
    bool    bPressedRightTrigger;

    // Last state of the buttons
    WORD    wLastButtons;
    bool    bLastLeftTrigger;
    bool    bLastRightTrigger;
};

extern DXUT_GAMEPAD          g_GamePads[DXUT_MAX_CONTROLLERS];
*/