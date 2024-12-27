#include "StdAfx.h"
#include "WeaponStatMgun.h"
#include "xrEngine/xr_level_controller.h"
#include "xrEngine/IInputReceiver.h"

void CWeaponStatMgun::OnAxisMove(float x, float y, float scaleX, float scaleY, bool invertX, bool invertY)
{
    if (fis_zero(x) && fis_zero(y))
        return;

    float h, p;
    m_destEnemyDir.getHP(h, p);
    h -= (invertX ? -1.f : 1.f) * x * scaleX;
    p -= (invertY ? -1.f : 1.f) * y * scaleY * 3.f / 4.f;
    SetDesiredDir(h, p);
}


void CWeaponStatMgun::OnMouseMove(int dx, int dy)
{
    if (Remote())
        return;

    const float scale = psMouseSens * psMouseSensScale / 50.f;
    OnAxisMove(float(dx), float(dy), scale, scale, false, psMouseInvert.test(1));
}

void CWeaponStatMgun::OnKeyboardPress(int dik)
{
    if (Remote())
        return;

    switch (dik)
    {
    case kWPN_FIRE: FireStart(); break;
    };
}

void CWeaponStatMgun::OnKeyboardRelease(int dik)
{
    if (Remote())
        return;
    switch (dik)
    {
    case kWPN_FIRE: FireEnd(); break;
    };
}

void CWeaponStatMgun::OnKeyboardHold(int dik)
{

}

void CWeaponStatMgun::OnControllerPress(int cmd, float x, float y)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kLOOK_AROUND:
    {
        const float scaleX = psControllerStickSensX * psControllerStickSensScale / 50.f;
        const float scaleY = psControllerStickSensY * psControllerStickSensScale / 50.f;
        OnAxisMove(x, y, scaleX, scaleY, psControllerFlags.test(ControllerInvertX), psControllerFlags.test(ControllerInvertY));
        break;
    }

    default:
        OnKeyboardPress(cmd);
        break;
    };
}

void CWeaponStatMgun::OnControllerRelease(int cmd, float x, float y)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kLOOK_AROUND:
        break;

    default:
        OnKeyboardRelease(cmd);
        break;
    };
}

void CWeaponStatMgun::OnControllerHold(int cmd, float x, float y)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kLOOK_AROUND:
        const float scaleX = psControllerStickSensX * psControllerStickSensScale / 50.f;
        const float scaleY = psControllerStickSensY * psControllerStickSensScale / 50.f;
        OnAxisMove(x, y, scaleX, scaleY, psControllerFlags.test(ControllerInvertX), psControllerFlags.test(ControllerInvertY));
        break;
    }; // switch (cmd)
}

void CWeaponStatMgun::OnControllerAttitudeChange(Fvector change)
{
    const float scale = psControllerSensorSens / 50.f;
    OnAxisMove(change.x, change.y, scale, scale, psControllerFlags.test(ControllerInvertX), psControllerFlags.test(ControllerInvertY));
}
