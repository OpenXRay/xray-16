#include "StdAfx.h"
#include "controller_direction.h"
#include "controller.h"
#include "game_object_space.h"

const float _pmt_head_bone_limit = PI_DIV_6;
const float _pmt_torso_bone_limit = PI_DIV_3;
const float _pmt_rotation_speed = PI_MUL_3;
const float _pmt_min_speed = deg(10);

void CControllerDirection::reinit()
{
    inherited::reinit();
    m_controller = smart_cast<CController*>(m_object);

    assign_bones();

    m_head_orient = m_man->path_builder().body_orientation();
    m_head_look_point.set(0.f, 0.f, 0.f);
}

void CControllerDirection::bone_callback(CBoneInstance* B)
{
    CControllerDirection* this_class = static_cast<CControllerDirection*>(B->callback_param());
    this_class->m_bones.Update(B, time());
}

void CControllerDirection::assign_bones()
{
    // Установка callback на кости
    IKinematics* kinematics = smart_cast<IKinematics*>(m_controller->Visual());

    m_bone_spine = &kinematics->LL_GetBoneInstance(kinematics->LL_BoneID("bip01_spine"));
    m_bone_head = &kinematics->LL_GetBoneInstance(kinematics->LL_BoneID("bip01_head"));

    if (!m_controller->PPhysicsShell())
    { //нельзя ставить колбеки, если создан физ шел - у него стоят свои колбеки!!!
        m_bone_spine->set_callback(bctCustom, bone_callback, this);
        m_bone_head->set_callback(bctCustom, bone_callback, this);
    }

    // Bones settings
    m_bones.Reset();
    m_bones.AddBone(m_bone_spine, AXIS_X);
    m_bones.AddBone(m_bone_spine, AXIS_Y);
    m_bones.AddBone(m_bone_head, AXIS_X);
    m_bones.AddBone(m_bone_head, AXIS_Y);
}

void CControllerDirection::update_head_orientation()
{
    m_head_orient.current.yaw = 0.f;
    m_head_orient.current.pitch = 0.f;
    m_head_orient.current.roll = 0.f;

    bonesAxis& x_spine = m_bones.GetBoneParams(m_bone_spine, AXIS_X);
    bonesAxis& x_head = m_bones.GetBoneParams(m_bone_head, AXIS_X);

    float yaw = x_spine.cur_yaw + x_head.cur_yaw;

    // установить параметры вращения по yaw
    m_head_orient.current.yaw = m_man->direction().get_heading_current() + yaw;
}

void CControllerDirection::update_schedule()
{
    inherited::update_schedule();

    update_head_orientation();
}

void CControllerDirection::head_look_point(const Fvector& look_point)
{
    m_head_look_point = look_point;

    float dir_yaw, dir_pitch;
    Fvector().sub(look_point, get_head_position(m_controller)).getHP(dir_yaw, dir_pitch);
    dir_yaw = angle_normalize(-dir_yaw);

    float bone_angle_head;
    float bone_angle_torso;

    // установить параметры вращения по heading
    float cur_yaw = m_man->direction().get_heading_current();
    float dy = _abs(angle_normalize_signed(dir_yaw - cur_yaw)); // дельта, на которую нужно поворачиваться

    bone_angle_head = _pmt_head_bone_limit / (_pmt_head_bone_limit + _pmt_torso_bone_limit) * dy;
    bone_angle_torso = _pmt_torso_bone_limit / (_pmt_head_bone_limit + _pmt_torso_bone_limit) * dy;

    clamp(bone_angle_head, 0.f, _pmt_head_bone_limit);
    clamp(bone_angle_torso, 0.f, _pmt_torso_bone_limit);

    if (!from_right(dir_yaw, cur_yaw))
    {
        bone_angle_head *= -1.f;
        bone_angle_torso *= -1.f;
    }

    // setup speed
    float bone_speed;
    bonesAxis& x_spine = m_bones.GetBoneParams(m_bone_spine, AXIS_X);
    bonesAxis& x_head = m_bones.GetBoneParams(m_bone_head, AXIS_X);

    float target_dy = _abs(bone_angle_head + bone_angle_torso);
    if (fis_zero(target_dy))
        bone_speed = _pmt_min_speed;
    else
        bone_speed = _pmt_min_speed +
            _pmt_rotation_speed * (_abs((x_spine.cur_yaw + x_head.cur_yaw) - (bone_angle_head + bone_angle_torso)) /
                                      (2 * (_pmt_head_bone_limit + _pmt_torso_bone_limit)));
    // set motion
    m_bones.SetMotion(m_bone_spine, AXIS_X, bone_angle_torso, bone_speed, 1000);
    m_bones.SetMotion(m_bone_head, AXIS_X, bone_angle_head, bone_speed, 1000);

    //// установить параметры вращения по pitch (более упрощеная схема, без расчета скорости вращения)
    // bone_angle_head		= _pmt_head_bone_limit	/ (_pmt_head_bone_limit + _pmt_torso_bone_limit) * dir_pitch;
    // bone_angle_torso	= _pmt_torso_bone_limit / (_pmt_head_bone_limit + _pmt_torso_bone_limit) * dir_pitch;

    // clamp				(bone_angle_head,	-_pmt_head_bone_limit,	_pmt_head_bone_limit);
    // clamp				(bone_angle_torso,	-_pmt_torso_bone_limit, _pmt_torso_bone_limit);
    //
    // m_bones.SetMotion(m_bone_spine, AXIS_Y, bone_angle_torso,	_pmt_rotation_speed, 1000);
    // m_bones.SetMotion(m_bone_head,	AXIS_Y,	bone_angle_head,	_pmt_rotation_speed, 1000);
}
