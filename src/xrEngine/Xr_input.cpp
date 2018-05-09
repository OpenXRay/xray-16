#include "stdafx.h"
#pragma hdrstop

#include "xr_input.h"
#include "IInputReceiver.h"
#include "Include/editor/ide.hpp"
#include "GameFont.h"
#include "PerformanceAlert.hpp"

#ifndef _EDITOR
#include "xr_input_xinput.h"
#endif
CInput* pInput = NULL;
IInputReceiver dummyController;

ENGINE_API float psMouseSens = 1.f;
ENGINE_API float psMouseSensScale = 1.f;
ENGINE_API Flags32 psMouseInvert = {FALSE};

float stop_vibration_time = flt_max;

#define MOUSEBUFFERSIZE 64
#define KEYBOARDBUFFERSIZE 64
#define _KEYDOWN(name, key) (name[key] & 0x80)

static bool g_exclusive = true;
static void on_error_dialog(bool before)
{
    if (!pInput || !g_exclusive || Device.editor())
        return;

    if (before)
    {
        pInput->unacquire();
        return;
    }

    pInput->acquire(true);
}

CInput::CInput(BOOL bExclusive, int deviceForInit)
{
    g_exclusive = !!bExclusive;

    Log("Starting INPUT device...");

    pDI = NULL;
    pMouse = NULL;
    pKeyboard = NULL;

    //=====================Mouse
    mouse_property.mouse_dt = 25;

    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(KBState, sizeof(KBState));
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));

    //===================== Dummy pack
    iCapture(&dummyController);

    if (!pDI)
        CHK_DX(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pDI, NULL));

    //. u32 kb_input_flags = ((bExclusive)?DISCL_EXCLUSIVE:DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND;
    u32 kb_input_flags = ((bExclusive) ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND;

    //. u32 mouse_input_flags = ((bExclusive)?DISCL_EXCLUSIVE:DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND | DISCL_NOWINKEY,
    u32 mouse_input_flags = ((bExclusive) ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND | DISCL_NOWINKEY;

    // KEYBOARD
    if (deviceForInit & keyboard_device_key)
        CHK_DX(CreateInputDevice(&pKeyboard, GUID_SysKeyboard, &c_dfDIKeyboard, kb_input_flags, KEYBOARDBUFFERSIZE));

    // MOUSE
    if (deviceForInit & mouse_device_key)
        CHK_DX(CreateInputDevice(&pMouse, GUID_SysMouse, &c_dfDIMouse2, mouse_input_flags, MOUSEBUFFERSIZE));

    xrDebug::SetDialogHandler(on_error_dialog);

#ifdef ENGINE_BUILD
    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH);
#endif
}

CInput::~CInput(void)
{
#ifdef ENGINE_BUILD
    Device.seqFrame.Remove(this);
    Device.seqAppDeactivate.Remove(this);
    Device.seqAppActivate.Remove(this);
#endif
    //_______________________

    // Unacquire and release the device's interfaces
    if (pMouse)
    {
        pMouse->Unacquire();
        _RELEASE(pMouse);
    }

    if (pKeyboard)
    {
        pKeyboard->Unacquire();
        _RELEASE(pKeyboard);
    }

    _SHOW_REF("Input: ", pDI);
    _RELEASE(pDI);
}

//-----------------------------------------------------------------------------
// Name: CreateInputDevice()
// Desc: Create a DirectInput device.
//-----------------------------------------------------------------------------
HRESULT CInput::CreateInputDevice(
    LPDIRECTINPUTDEVICE8* device, GUID guidDevice, const DIDATAFORMAT* pdidDataFormat, u32 dwFlags, u32 buf_size)
{
    // Obtain an interface to the input device
    //. CHK_DX( pDI->CreateDeviceEx( guidDevice, IID_IDirectInputDevice8, (void**)device, NULL ) );
    CHK_DX(pDI->CreateDevice(guidDevice, /*IID_IDirectInputDevice8,*/ device, NULL));

    // Set the device data format. Note: a data format specifies which
    // controls on a device we are interested in, and how they should be
    // reported.
    CHK_DX((*device)->SetDataFormat(pdidDataFormat));

// Set the cooperativity level to let DirectInput know how this device
// should interact with the system and with other DirectInput applications.
    if (!Device.editor())
    {
        HRESULT _hr = (*device)->SetCooperativeLevel(RDEVICE.m_hWnd, dwFlags);
        if (FAILED(_hr) && (_hr == E_NOTIMPL))
            Msg("! INPUT: Can't set coop level. Emulation???");
        else
            R_CHK(_hr);
    }

    // setup the buffer size for the keyboard data
    DIPROPDWORD dipdw;
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = buf_size;

    CHK_DX((*device)->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph));

    return S_OK;
}

//-----------------------------------------------------------------------

void CInput::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** INPUT:    %2.2fms", pInput->GetStats().FrameTime.result);
}

void CInput::SetAllAcquire(BOOL bAcquire)
{
    if (pMouse)
        bAcquire ? pMouse->Acquire() : pMouse->Unacquire();
    if (pKeyboard)
        bAcquire ? pKeyboard->Acquire() : pKeyboard->Unacquire();
}

void CInput::SetMouseAcquire(BOOL bAcquire)
{
    if (pMouse)
        bAcquire ? pMouse->Acquire() : pMouse->Unacquire();
}
void CInput::SetKBDAcquire(BOOL bAcquire)
{
    if (pKeyboard)
        bAcquire ? pKeyboard->Acquire() : pKeyboard->Unacquire();
}
//-----------------------------------------------------------------------
BOOL b_altF4 = FALSE;
void CInput::KeyUpdate()
{
    if (b_altF4)
        return;

    HRESULT hr;
    DWORD dwElements = KEYBOARDBUFFERSIZE;
    DIDEVICEOBJECTDATA od[KEYBOARDBUFFERSIZE];
    DWORD key = 0;

    VERIFY(pKeyboard);

    hr = pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od[0], &dwElements, 0);
    if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
    {
        hr = pKeyboard->Acquire();
        if (hr != S_OK)
            return;

        hr = pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od[0], &dwElements, 0);
        if (hr != S_OK)
            return;
    }

    bool b_dik_pause_was_pressed = false;
    for (u32 idx = 0; idx < dwElements; idx++)
    {
        if (od[idx].dwOfs == DIK_PAUSE)
        {
            if (od[idx].dwData & 0x80)
                b_dik_pause_was_pressed = true;

            if (b_dik_pause_was_pressed && !(od[idx].dwData & 0x80))
            {
                od[idx].uAppData = 666;
                continue; // skip one-frame pause key on-off switch
            }
        }
        KBState[od[idx].dwOfs] = od[idx].dwData & 0x80;
    }

#ifndef _EDITOR
    bool b_alt_tab = false;

    if (!b_altF4 && KBState[DIK_F4] && (KBState[DIK_RMENU] || KBState[DIK_LMENU]))
    {
        b_altF4 = TRUE;
        Engine.Event.Defer("KERNEL:disconnect");
        Engine.Event.Defer("KERNEL:quit");
    }

#endif
    if (b_altF4)
        return;

#ifndef _EDITOR
    if (Device.dwPrecacheFrame == 0)
#endif
    {
        for (u32 i = 0; i < dwElements; i++)
        {
            if (od[i].uAppData == 666) // ignored action
                continue;

            key = od[i].dwOfs;
            if (od[i].dwData & 0x80)
                cbStack.back()->IR_OnKeyboardPress(key);
            else
            {
                cbStack.back()->IR_OnKeyboardRelease(key);
#ifndef _EDITOR
                if (key == DIK_TAB && (iGetAsyncKeyState(DIK_RMENU) || iGetAsyncKeyState(DIK_LMENU)))
                    b_alt_tab = true;
#endif
            }
        }

        for (u32 i = 0; i < COUNT_KB_BUTTONS; i++)
            if (KBState[i])
                cbStack.back()->IR_OnKeyboardHold(i);
    }

#ifndef _EDITOR
    if (b_alt_tab)
        SendMessage(Device.m_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#endif
    /*
    #ifndef _EDITOR
    //update xinput if exist
    for( DWORD iUserIndex=0; iUserIndex<DXUT_MAX_CONTROLLERS; iUserIndex++ )
    {
    DXUTGetGamepadState( iUserIndex, &g_GamePads[iUserIndex], true, false );

    if( !g_GamePads[iUserIndex].bConnected )
    continue; // unplugged?

    bool new_b, old_b;
    new_b = !!(g_GamePads[iUserIndex].wPressedButtons & XINPUT_GAMEPAD_A);
    old_b = !!(g_GamePads[iUserIndex].wLastButtons & XINPUT_GAMEPAD_A);

    if(new_b != old_b)
    {
    if(old_b)
    cbStack.back()->IR_OnMousePress(0);
    else
    cbStack.back()->IR_OnMouseRelease(0);
    }
    int dx,dy;
    dx = iFloor(g_GamePads[iUserIndex].fThumbRX*6);
    dy = iFloor(g_GamePads[iUserIndex].fThumbRY*6);
    if(dx || dy)
    cbStack.back()->IR_OnMouseMove ( dx, dy );
    }

    if(Device.fTimeGlobal > stop_vibration_time)
    {
    stop_vibration_time = flt_max;
    set_vibration (0, 0);
    }
    //xinput
    #endif
    */
}
bool CInput::get_dik_name(int dik, LPSTR dest_str, int dest_sz)
{
    DIPROPSTRING keyname;
    keyname.diph.dwSize = sizeof(DIPROPSTRING);
    keyname.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    keyname.diph.dwObj = static_cast<DWORD>(dik);
    keyname.diph.dwHow = DIPH_BYOFFSET;
    HRESULT hr = pKeyboard->GetProperty(DIPROP_KEYNAME, &keyname.diph);
    if (FAILED(hr))
        return false;

    const wchar_t* wct = keyname.wsz;
    if (0 == wcslen(wct))
        return false;

    int cnt = WideCharToMultiByte(CP_ACP, 0, keyname.wsz, -1, dest_str, dest_sz, NULL, NULL);
    if (cnt == -1)
    {
        Msg("! cant convert dik_name for dik[%d], prop=[%S]", dik, keyname.wsz);
        return false;
    }
    return (cnt != -1);
}

#define MOUSE_1 (0xED + 100)
#define MOUSE_8 (0xED + 107)

BOOL CInput::iGetAsyncKeyState(int dik)
{
    if (dik < COUNT_KB_BUTTONS)
        return !!KBState[dik];
    else if (dik >= MOUSE_1 && dik <= MOUSE_8)
    {
        int mk = dik - MOUSE_1;
        return iGetAsyncBtnState(mk);
    }
    else
        return FALSE; // unknown key ???
}

BOOL CInput::iGetAsyncBtnState(int btn) { return !!mouseState[btn]; }
void CInput::ClipCursor(bool clip)
{
    if (clip)
    {
        ::ClipCursor(&Device.m_rcWindowClient);
        while (ShowCursor(FALSE) >= 0) {}
    }
    else
    {
        ::ClipCursor(nullptr);
        while (ShowCursor(TRUE) <= 0) {}
    }
}

void CInput::MouseUpdate()
{
    HRESULT hr;
    DWORD dwElements = MOUSEBUFFERSIZE;
    DIDEVICEOBJECTDATA od[MOUSEBUFFERSIZE];

    VERIFY(pMouse);

    hr = pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od[0], &dwElements, 0);
    if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
    {
        hr = pMouse->Acquire();
        if (hr != S_OK)
            return;
        hr = pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od[0], &dwElements, 0);
        if (hr != S_OK)
            return;
    };

#ifndef _EDITOR
    if (Device.dwPrecacheFrame)
        return;
#endif
    BOOL mouse_prev[COUNT_MOUSE_BUTTONS];

    mouse_prev[0] = mouseState[0];
    mouse_prev[1] = mouseState[1];
    mouse_prev[2] = mouseState[2];
    mouse_prev[3] = mouseState[3];
    mouse_prev[4] = mouseState[4];
    mouse_prev[5] = mouseState[5];
    mouse_prev[6] = mouseState[6];
    mouse_prev[7] = mouseState[7];

    offs[0] = offs[1] = offs[2] = 0;
    for (u32 i = 0; i < dwElements; i++)
    {
        switch (od[i].dwOfs)
        {
        case DIMOFS_X:
            offs[0] += od[i].dwData;
            timeStamp[0] = od[i].dwTimeStamp;
            break;
        case DIMOFS_Y:
            offs[1] += od[i].dwData;
            timeStamp[1] = od[i].dwTimeStamp;
            break;
        case DIMOFS_Z:
            offs[2] += od[i].dwData;
            timeStamp[2] = od[i].dwTimeStamp;
            break;
        case DIMOFS_BUTTON0:
            if (od[i].dwData & 0x80)
            {
                mouseState[0] = TRUE;
                cbStack.back()->IR_OnMousePress(0);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[0] = FALSE;
                cbStack.back()->IR_OnMouseRelease(0);
            }
            break;
        case DIMOFS_BUTTON1:
            if (od[i].dwData & 0x80)
            {
                mouseState[1] = TRUE;
                cbStack.back()->IR_OnMousePress(1);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[1] = FALSE;
                cbStack.back()->IR_OnMouseRelease(1);
            }
            break;
        case DIMOFS_BUTTON2:
            if (od[i].dwData & 0x80)
            {
                mouseState[2] = TRUE;
                cbStack.back()->IR_OnMousePress(2);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[2] = FALSE;
                cbStack.back()->IR_OnMouseRelease(2);
            }
            break;
        case DIMOFS_BUTTON3:
            if (od[i].dwData & 0x80)
            {
                mouseState[3] = TRUE;
                cbStack.back()->IR_OnKeyboardPress(0xED + 103);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[3] = FALSE;
                cbStack.back()->IR_OnKeyboardRelease(0xED + 103);
            }
            break;
        case DIMOFS_BUTTON4:
            if (od[i].dwData & 0x80)
            {
                mouseState[4] = TRUE;
                cbStack.back()->IR_OnKeyboardPress(0xED + 104);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[4] = FALSE;
                cbStack.back()->IR_OnKeyboardRelease(0xED + 104);
            }
            break;
        case DIMOFS_BUTTON5:
            if (od[i].dwData & 0x80)
            {
                mouseState[5] = TRUE;
                cbStack.back()->IR_OnKeyboardPress(0xED + 105);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[5] = FALSE;
                cbStack.back()->IR_OnKeyboardRelease(0xED + 105);
            }
            break;
        case DIMOFS_BUTTON6:
            if (od[i].dwData & 0x80)
            {
                mouseState[6] = TRUE;
                cbStack.back()->IR_OnKeyboardPress(0xED + 106);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[6] = FALSE;
                cbStack.back()->IR_OnKeyboardRelease(0xED + 106);
            }
            break;
        case DIMOFS_BUTTON7:
            if (od[i].dwData & 0x80)
            {
                mouseState[7] = TRUE;
                cbStack.back()->IR_OnKeyboardPress(0xED + 107);
            }
            if (!(od[i].dwData & 0x80))
            {
                mouseState[7] = FALSE;
                cbStack.back()->IR_OnKeyboardRelease(0xED + 107);
            }
            break;
        }
    }

    // Giperion: double check mouse buttons state
    DIMOUSESTATE2 MouseState;
    hr = pMouse->GetDeviceState(sizeof(MouseState), &MouseState);

    auto RecheckMouseButtonFunc = [&](int i)
    {
        if (MouseState.rgbButtons[i] & 0x80 && mouseState[i] == FALSE)
        {
            mouseState[i] = TRUE;
            cbStack.back()->IR_OnMousePress(i);
        }
        else if (!(MouseState.rgbButtons[i] & 0x80) && mouseState[i] == TRUE)
        {
            mouseState[i] = FALSE;
            cbStack.back()->IR_OnMouseRelease(i);
        }
    };

    if (hr == S_OK)
    {
        RecheckMouseButtonFunc(0);
        RecheckMouseButtonFunc(1);
        RecheckMouseButtonFunc(2);
    }
    //-Giperion

    auto isButtonOnHold = [&](int i)
    {
        if (mouseState[i] && mouse_prev[i])
            cbStack.back()->IR_OnMouseHold(i);
    };

    isButtonOnHold(0);
    isButtonOnHold(1);
    isButtonOnHold(2);

    if (dwElements)
    {
        if (offs[0] || offs[1])
            cbStack.back()->IR_OnMouseMove(offs[0], offs[1]);
        if (offs[2])
            cbStack.back()->IR_OnMouseWheel(offs[2]);
    }
    else
    {
        if (timeStamp[1] && ((dwCurTime - timeStamp[1]) >= mouse_property.mouse_dt))
            cbStack.back()->IR_OnMouseStop(DIMOFS_Y, timeStamp[1] = 0);
        if (timeStamp[0] && ((dwCurTime - timeStamp[0]) >= mouse_property.mouse_dt))
            cbStack.back()->IR_OnMouseStop(DIMOFS_X, timeStamp[0] = 0);
    }
}

//-------------------------------------------------------
void CInput::iCapture(IInputReceiver* p)
{
    VERIFY(p);
    if (pMouse)
        MouseUpdate();
    if (pKeyboard)
        KeyUpdate();

    // change focus
    if (!cbStack.empty())
        cbStack.back()->IR_OnDeactivate();
    cbStack.push_back(p);
    cbStack.back()->IR_OnActivate();

    // prepare for _new_ controller
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::iRelease(IInputReceiver* p)
{
    if (p == cbStack.back())
    {
        cbStack.back()->IR_OnDeactivate();
        cbStack.pop_back();
        IInputReceiver* ir = cbStack.back();
        ir->IR_OnActivate();
    }
    else
    {
        // we are not topmost receiver, so remove the nearest one
        u32 cnt = cbStack.size();
        for (; cnt > 0; --cnt)
            if (cbStack[cnt - 1] == p)
            {
                xr_vector<IInputReceiver*>::iterator it = cbStack.begin();
                std::advance(it, cnt - 1);
                cbStack.erase(it);
                break;
            }
    }
}

void CInput::OnAppActivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnActivate();

    SetAllAcquire(true);
    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(KBState, sizeof(KBState));
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnAppDeactivate(void)
{
    if (CurrentIR())
        CurrentIR()->IR_OnDeactivate();

    SetAllAcquire(false);
    ZeroMemory(mouseState, sizeof(mouseState));
    ZeroMemory(KBState, sizeof(KBState));
    ZeroMemory(timeStamp, sizeof(timeStamp));
    ZeroMemory(timeSave, sizeof(timeStamp));
    ZeroMemory(offs, sizeof(offs));
}

void CInput::OnFrame(void)
{
    stats.FrameStart();
    stats.FrameTime.Begin();
    dwCurTime = RDEVICE.TimerAsync_MMT();
    if (pKeyboard)
        KeyUpdate();
    if (pMouse)
        MouseUpdate();
    stats.FrameTime.End();
    stats.FrameEnd();
}

IInputReceiver* CInput::CurrentIR()
{
    if (cbStack.size())
        return cbStack.back();
    else
        return NULL;
}

void CInput::unacquire()
{
    pKeyboard->Unacquire();
    pMouse->Unacquire();
}

void CInput::acquire(const bool& exclusive)
{
    pKeyboard->SetCooperativeLevel(
        Device.editor() ? Device.editor()->main_handle() :
                          RDEVICE.m_hWnd,
        (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND);
    pKeyboard->Acquire();

    pMouse->SetCooperativeLevel(
        Device.editor() ? Device.editor()->main_handle() :
                          RDEVICE.m_hWnd,
        (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE) | DISCL_FOREGROUND | DISCL_NOWINKEY);
    pMouse->Acquire();
}

void CInput::exclusive_mode(const bool& exclusive)
{
    g_exclusive = exclusive;
    unacquire();
    acquire(exclusive);
}
bool CInput::get_exclusive_mode() { return g_exclusive; }
void CInput::feedback(u16 s1, u16 s2, float time)
{
    stop_vibration_time = RDEVICE.fTimeGlobal + time;
#ifndef _EDITOR
//. set_vibration (s1, s2);
#endif
}
