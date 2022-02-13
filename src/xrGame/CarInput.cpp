#include "StdAfx.h"
#pragma hdrstop
#ifdef DEBUG

#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "Actor.h"
#include "CameraLook.h"
#include "CameraFirstEye.h"
#include "script_entity_action.h"
#include "xrEngine/xr_level_controller.h"
#include "Include/xrRender/Kinematics.h"
#include "Level.h"
#include "CarWeapon.h"

void CCar::OnAxisMove(float x, float y, float scale, bool invert)
{
    CCameraBase* C = active_camera;
    if (!fis_zero(x))
    {
        const float d = x * scale;
        C->Move((d < 0) ? kLEFT : kRIGHT, _abs(d));
    }
    if (!fis_zero(y))
    {
        const float d = (invert ? -1.f : 1.f) * y * scale * 3.f / 4.f;
        C->Move((d > 0) ? kUP : kDOWN, _abs(d));
    }
}

void CCar::OnMouseMove(int dx, int dy)
{
    if (Remote())
        return;

    const float scale = (active_camera->f_fov / g_fov) * psMouseSens * psMouseSensScale / 50.f;
    OnAxisMove(float(dx), float(dy), scale, psMouseInvert.test(1));
}

bool CCar::bfAssignMovement(CScriptEntityAction* tpEntityAction)
{
    if (tpEntityAction->m_tMovementAction.m_bCompleted)
        return (false);

    u32 l_tInput = tpEntityAction->m_tMovementAction.m_tInputKeys;

    vfProcessInputKey(kFWD, !!(l_tInput & CScriptMovementAction::eInputKeyForward));
    vfProcessInputKey(kBACK, !!(l_tInput & CScriptMovementAction::eInputKeyBack));
    vfProcessInputKey(kL_STRAFE, !!(l_tInput & CScriptMovementAction::eInputKeyLeft));
    vfProcessInputKey(kR_STRAFE, !!(l_tInput & CScriptMovementAction::eInputKeyRight));
    vfProcessInputKey(kACCEL, !!(l_tInput & CScriptMovementAction::eInputKeyShiftUp));
    vfProcessInputKey(kCROUCH, !!(l_tInput & CScriptMovementAction::eInputKeyShiftDown));
    vfProcessInputKey(kJUMP, !!(l_tInput & CScriptMovementAction::eInputKeyBreaks));
    if (!!(l_tInput & CScriptMovementAction::eInputKeyEngineOn))
        StartEngine();
    if (!!(l_tInput & CScriptMovementAction::eInputKeyEngineOff))
        StopEngine();

    // if (_abs(tpEntityAction->m_tMovementAction.m_fSpeed) > EPS_L)
    // m_current_rpm = _abs(tpEntityAction->m_tMovementAction.m_fSpeed*m_current_gear_ratio);

    return (true);
}

bool CCar::bfAssignObject(CScriptEntityAction* tpEntityAction)
{
    CScriptObjectAction& l_tObjectAction = tpEntityAction->m_tObjectAction;
    if (l_tObjectAction.m_bCompleted || !xr_strlen(l_tObjectAction.m_caBoneName))
        return ((l_tObjectAction.m_bCompleted = true) == false);

    s16 l_sBoneID = smart_cast<IKinematics*>(Visual())->LL_BoneID(l_tObjectAction.m_caBoneName);
    if (is_Door(l_sBoneID))
    {
        switch (l_tObjectAction.m_tGoalType)
        {
        case MonsterSpace::eObjectActionActivate:
        {
            if (!DoorOpen(l_sBoneID))
                return ((l_tObjectAction.m_bCompleted = true) == false);
            break;
        }
        case MonsterSpace::eObjectActionDeactivate:
        {
            if (!DoorClose(l_sBoneID))
                return ((l_tObjectAction.m_bCompleted = true) == false);
            break;
        }
        case MonsterSpace::eObjectActionUse:
        {
            if (!DoorSwitch(l_sBoneID))
                return ((l_tObjectAction.m_bCompleted = true) == false);
            break;
        }
        default: return ((l_tObjectAction.m_bCompleted = true) == false);
        }
        return (false);
    }
    SCarLight* light = NULL;
    if (m_lights.findLight(l_sBoneID, light))
    {
        switch (l_tObjectAction.m_tGoalType)
        {
        case MonsterSpace::eObjectActionActivate:
        {
            light->TurnOn();
            return ((l_tObjectAction.m_bCompleted = true) == false);
        }
        case MonsterSpace::eObjectActionDeactivate:
        {
            light->TurnOff();
            return ((l_tObjectAction.m_bCompleted = true) == false);
        }
        case MonsterSpace::eObjectActionUse:
        {
            light->Switch();
            return ((l_tObjectAction.m_bCompleted = true) == false);
        }
        default: return ((l_tObjectAction.m_bCompleted = true) == false);
        }
    }

    return (false);
}

void CCar::vfProcessInputKey(int iCommand, bool bPressed)
{
    if (bPressed)
        OnKeyboardPress(iCommand);
    else
        OnKeyboardRelease(iCommand);
}

void CCar::OnKeyboardPress(int cmd)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kCAM_1: OnCameraChange(ectFirst); break;
    case kCAM_2: OnCameraChange(ectChase); break;
    case kCAM_3: OnCameraChange(ectFree); break;
    case kACCEL: TransmissionUp(); break;
    case kCROUCH: TransmissionDown(); break;
    case kFWD: PressForward(); break;
    case kBACK: PressBack(); break;
    case kR_STRAFE:
        PressRight();
        if (OwnerActor())
            OwnerActor()->steer_Vehicle(1);
        break;
    case kL_STRAFE:
        PressLeft();
        if (OwnerActor())
            OwnerActor()->steer_Vehicle(-1);
        break;
    case kJUMP: PressBreaks(); break;
    case kENGINE: [[fallthrough]];
    case kDETECTOR: SwitchEngine(); break;
    case kTORCH: m_lights.SwitchHeadLights(); break;
    case kUSE: break;
    };
}

void CCar::OnKeyboardRelease(int cmd)
{
    if (Remote())
        return;
    switch (cmd)
    {
    case kACCEL: break;
    case kFWD: ReleaseForward(); break;
    case kBACK: ReleaseBack(); break;
    case kL_STRAFE:
        ReleaseLeft();
        if (OwnerActor())
            OwnerActor()->steer_Vehicle(0);
        break;
    case kR_STRAFE:
        ReleaseRight();
        if (OwnerActor())
            OwnerActor()->steer_Vehicle(0);
        break;
    case kJUMP: ReleaseBreaks(); break;
    };
}

void CCar::OnKeyboardHold(int cmd)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kCAM_ZOOM_IN:
    case kCAM_ZOOM_OUT:
    case kUP:
    case kDOWN:
    case kLEFT:
    case kRIGHT:
        active_camera->Move(cmd);
        break;
        /*
            case kFWD:
                if (ectFree==active_camera->tag)	active_camera->Move(kUP);
                else								m_vCamDeltaHP.y += active_camera->rot_speed.y*Device.fTimeDelta;
                break;
            case kBACK:
                if (ectFree==active_camera->tag)	active_camera->Move(kDOWN);
                else								m_vCamDeltaHP.y -= active_camera->rot_speed.y*Device.fTimeDelta;
                break;
            case kL_STRAFE:
                if (ectFree==active_camera->tag)	active_camera->Move(kLEFT);
                else								m_vCamDeltaHP.x -= active_camera->rot_speed.x*Device.fTimeDelta;
                break;
            case kR_STRAFE:
                if (ectFree==active_camera->tag)	active_camera->Move(kRIGHT);
                else								m_vCamDeltaHP.x += active_camera->rot_speed.x*Device.fTimeDelta;
                break;
        */
    }
    //	clamp(m_vCamDeltaHP.x, -PI_DIV_2,	PI_DIV_2);
    //	clamp(m_vCamDeltaHP.y, active_camera->lim_pitch.x,	active_camera->lim_pitch.y);
}

void CCar::OnControllerPress(int cmd, float x, float y)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kLOOK_AROUND:
    {
        const float scale = (active_camera->f_fov / g_fov) * psControllerStickSens * psControllerStickSensScale / 50.f;
        OnAxisMove(x, y, scale, psControllerInvertY.test(1));
        break;
    }

    case kMOVE_AROUND:
    {
        if (!fis_zero(x))
        {
            if (x > 35.f)
                OnKeyboardPress(kR_STRAFE);
            else if (x < -35.f)
                OnKeyboardPress(kL_STRAFE);
        }
        if (!fis_zero(y))
        {
            if (y > 35.f)
                OnKeyboardPress(kBACK);
            else if (y < -35.f)
                OnKeyboardPress(kFWD);
        }
        break;
    }

    default:
        OnKeyboardPress(cmd);
        break;
    };
}

void CCar::OnControllerRelease(int cmd, float x, float y)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kLOOK_AROUND:
        break;

    case kMOVE_AROUND:
        OnKeyboardRelease(kFWD);
        OnKeyboardRelease(kBACK);
        OnKeyboardRelease(kL_STRAFE);
        OnKeyboardRelease(kR_STRAFE);
        break;

    default:
        OnKeyboardPress(cmd);
        break;
    };
}

void CCar::OnControllerHold(int cmd, float x, float y)
{
    if (Remote())
        return;

    switch (cmd)
    {
    case kLOOK_AROUND:
    {
        const float scale = (active_camera->f_fov / g_fov) * psControllerStickSens * psControllerStickSensScale / 50.f;
        OnAxisMove(x, y, scale, psControllerInvertY.test(1));
        break;
    }

    case kMOVE_AROUND:
    {
        if (!fis_zero(x))
        {
            if (x > 35.f && !rsp) // right
            {
                OnKeyboardRelease(kL_STRAFE);
                OnKeyboardPress(kR_STRAFE);
            }
            else if (x < -35.f && !lsp) // left
            {
                OnKeyboardRelease(kR_STRAFE);
                OnKeyboardPress(kL_STRAFE);
            }
            else
            {
                if (lsp)
                    OnKeyboardRelease(kL_STRAFE);
                if (rsp)
                    OnKeyboardRelease(kR_STRAFE);
            }
        }
        if (!fis_zero(y))
        {
            if (y > 35.f && !bkp) // backward
            {
                OnKeyboardRelease(kFWD);
                OnKeyboardPress(kBACK);
            }
            else if (y < -35.f && !fwp) // forward
            {
                OnKeyboardRelease(kBACK);
                OnKeyboardPress(kFWD);
            }
            else
            {
                if (fwp)
                    OnKeyboardRelease(kFWD);
                if (bkp)
                    OnKeyboardRelease(kBACK);
            }
        }
        break;
    }

    default:
        OnKeyboardPress(cmd);
        break;
    }
}

void CCar::OnControllerAttitudeChange(Fvector change)
{
    const float scale = (active_camera->f_fov / g_fov) * psControllerSensorSens / 50.f;
    OnAxisMove(change.x, change.y, scale, psControllerInvertY.test(1));
}

void CCar::Action(u16 id, u32 flags)
{
    if (m_car_weapon)
        m_car_weapon->Action(id, flags);
}
void CCar::SetParam(int id, Fvector2 val)
{
    if (m_car_weapon)
        m_car_weapon->SetParam(id, val);
}
void CCar::SetParam(int id, Fvector val)
{
    if (m_car_weapon)
        m_car_weapon->SetParam(id, val);
}
bool CCar::WpnCanHit()
{
    if (m_car_weapon)
        return m_car_weapon->AllowFire();
    return false;
}

float CCar::FireDirDiff()
{
    if (m_car_weapon)
        return m_car_weapon->FireDirDiff();
    return 0.0f;
}
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "car_memory.h"
#include "visual_memory_manager.h"

bool CCar::isObjectVisible(CScriptGameObject* O_)
{
    if (m_memory)
    {
        return m_memory->visual().visible_now(&O_->object());
    }
    else
    {
        if (!O_)
        {
            Msg("Attempt to call CCar::isObjectVisible method wihth passed NULL parameter");
            return false;
        }
        IGameObject* O = &O_->object();
        Fvector dir_to_object;
        Fvector to_point;
        O->Center(to_point);

        Fvector from_point;
        Center(from_point);

        if (HasWeapon())
        {
            from_point.y = XFORM().c.y + m_car_weapon->_height();
        }

        dir_to_object.sub(to_point, from_point).normalize_safe();
        float ray_length = from_point.distance_to(to_point);

        BOOL res = Level().ObjectSpace.RayTest(from_point, dir_to_object, ray_length, collide::rqtStatic, NULL, NULL);
        return (0 == res);
    }
}

bool CCar::HasWeapon() { return (m_car_weapon != NULL); }
Fvector CCar::CurrentVel()
{
    Fvector lin_vel;
    m_pPhysicsShell->get_LinearVel(lin_vel);

    return lin_vel;
}
