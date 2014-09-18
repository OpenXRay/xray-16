#include "stdafx.h"
/*
#include "xr_input_xinput.h"
#include <xinput.h>


DXUT_GAMEPAD  g_GamePads[DXUT_MAX_CONTROLLERS];

typedef DWORD (WINAPI* LPXINPUTGETSTATE)(DWORD dwUserIndex, XINPUT_STATE* pState );
typedef DWORD (WINAPI* LPXINPUTSETSTATE)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration );
typedef DWORD (WINAPI* LPXINPUTGETCAPABILITIES)( DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities );
typedef void  (WINAPI* LPXINPUTENABLE)(BOOL bEnable);

HRESULT DXUTGetGamepadState( DWORD dwPort, DXUT_GAMEPAD* pGamePad, bool bThumbstickDeadZone, bool bSnapThumbstickToCardinals )
{
    if( dwPort >= DXUT_MAX_CONTROLLERS || pGamePad == NULL )
        return E_FAIL;

    static LPXINPUTGETSTATE s_pXInputGetState = NULL;
    static LPXINPUTGETCAPABILITIES s_pXInputGetCapabilities = NULL;
    if( NULL == s_pXInputGetState || NULL == s_pXInputGetCapabilities )
    {
        HINSTANCE hInst = LoadLibrary( XINPUT_DLL );
        if( hInst ) 
        {
            s_pXInputGetState = (LPXINPUTGETSTATE)GetProcAddress( hInst, "XInputGetState" );
            s_pXInputGetCapabilities = (LPXINPUTGETCAPABILITIES)GetProcAddress( hInst, "XInputGetCapabilities" );
        }
    }
    if( s_pXInputGetState == NULL )
        return E_FAIL;

    XINPUT_STATE InputState;
    DWORD dwResult = s_pXInputGetState( dwPort, &InputState );

    // Track insertion and removals
    BOOL bWasConnected = pGamePad->bConnected;
    pGamePad->bConnected = (dwResult == ERROR_SUCCESS);
    pGamePad->bRemoved  = (  bWasConnected && !pGamePad->bConnected );
    pGamePad->bInserted = ( !bWasConnected &&  pGamePad->bConnected );

    // Don't update rest of the state if not connected
    if( !pGamePad->bConnected )
        return S_OK;

    // Store the capabilities of the device
    if( pGamePad->bInserted )
    {
        ZeroMemory( pGamePad, sizeof(DXUT_GAMEPAD) );
        pGamePad->bConnected = true;
        pGamePad->bInserted  = true;
        if( s_pXInputGetCapabilities )
            s_pXInputGetCapabilities( dwPort, XINPUT_DEVTYPE_GAMEPAD, &pGamePad->caps );
    }

    // Copy gamepad to local structure (assumes that XINPUT_GAMEPAD at the front in CONTROLER_STATE)
    memcpy( pGamePad, &InputState.Gamepad, sizeof(XINPUT_GAMEPAD) );

    if( bSnapThumbstickToCardinals )
    {
        // Apply deadzone to each axis independantly to slightly snap to up/down/left/right
        if( pGamePad->sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pGamePad->sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
            pGamePad->sThumbLX = 0;
        if( pGamePad->sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pGamePad->sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) 
            pGamePad->sThumbLY = 0;
        if( pGamePad->sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pGamePad->sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE )
            pGamePad->sThumbRX = 0;
        if( pGamePad->sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pGamePad->sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) 
            pGamePad->sThumbRY = 0;
    }
    else if( bThumbstickDeadZone )
    {
        // Apply deadzone if centered
        if( (pGamePad->sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pGamePad->sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) && 
            (pGamePad->sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pGamePad->sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) ) 
        {   
            pGamePad->sThumbLX = 0;
            pGamePad->sThumbLY = 0;
        }
        if( (pGamePad->sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pGamePad->sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) && 
            (pGamePad->sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pGamePad->sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) ) 
        {
            pGamePad->sThumbRX = 0;
            pGamePad->sThumbRY = 0;
        }
    }

    // Convert [-1,+1] range
    pGamePad->fThumbLX = pGamePad->sThumbLX / 32767.0f;
    pGamePad->fThumbLY = pGamePad->sThumbLY / 32767.0f;
    pGamePad->fThumbRX = pGamePad->sThumbRX / 32767.0f;
    pGamePad->fThumbRY = pGamePad->sThumbRY / 32767.0f;

    // Get the boolean buttons that have been pressed since the last call. 
    // Each button is represented by one bit.
    pGamePad->wPressedButtons = ( pGamePad->wLastButtons ^ pGamePad->wButtons ) & pGamePad->wButtons;
    pGamePad->wLastButtons    = pGamePad->wButtons;

    // Figure out if the left trigger has been pressed or released
    bool bPressed = ( pGamePad->bLeftTrigger > DXUT_GAMEPAD_TRIGGER_THRESHOLD );
    pGamePad->bPressedLeftTrigger = ( bPressed ) ? !pGamePad->bLastLeftTrigger : false;
    pGamePad->bLastLeftTrigger = bPressed;

    // Figure out if the right trigger has been pressed or released
    bPressed = ( pGamePad->bRightTrigger > DXUT_GAMEPAD_TRIGGER_THRESHOLD );
    pGamePad->bPressedRightTrigger = ( bPressed ) ? !pGamePad->bLastRightTrigger : false;
    pGamePad->bLastRightTrigger = bPressed;

    return S_OK;
}

void set_vibration (u16 s1, u16 s2)
{
    static LPXINPUTSETSTATE s_pXInputSetState = NULL;
    if( NULL == s_pXInputSetState )
    {
        HINSTANCE hInst = LoadLibrary( XINPUT_DLL );
        if( hInst ) 
            s_pXInputSetState = (LPXINPUTSETSTATE)GetProcAddress( hInst, "XInputSetState" );
    }


	XINPUT_VIBRATION			vibration;
    vibration.wLeftMotorSpeed	= s1;
    vibration.wRightMotorSpeed	= s2;
    s_pXInputSetState			( 0, &vibration );
}
*/