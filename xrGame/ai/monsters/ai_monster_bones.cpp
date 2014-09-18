#include "stdafx.h"
#include "ai_monster_bones.h"
#include "../../../Include/xrRender/Kinematics.h"
#include "../../../xrEngine/bone.h"


//****************************************************************************************************
// class bonesBone
//****************************************************************************************************
void bonesBone::Set(CBoneInstance *b, u8 a, float ty, float cy, float r_s)
{
	bone			= b; 
	axis			= a;

	params.target_yaw	= ty; 
	params.cur_yaw	= cy; 
	params.r_speed	= r_s;
	params.dist_yaw	= _abs(ty-cy);
}


bool bonesBone::NeedTurn()
{
	if (!fsimilar(params.cur_yaw, params.target_yaw, EPS_L)) return true;
	return false;
}

void bonesBone::Turn(u32 dt)
{
	float PI_DIV_2m		= 8 * PI_DIV_6 / 3;		
	float PIm			= PI_DIV_2m * 2;

	float cur_speed = params.r_speed * _cos(PI_DIV_2m - PIm * _abs(params.target_yaw - params.cur_yaw) / params.dist_yaw);

	float dy;
	dy =  cur_speed * dt / 1000;  // учитываем милисек и радианную меры

	if (_abs(params.target_yaw - params.cur_yaw) < dy) params.cur_yaw = params.target_yaw;
	else params.cur_yaw += ((params.target_yaw > params.cur_yaw) ? dy : -dy);

}

void bonesBone::Apply()
{
	float x = 0.f, y = 0.f, z = 0.f;

	if ((axis & AXIS_X) == AXIS_X) x = params.cur_yaw;
	if ((axis & AXIS_Y) == AXIS_Y) y = params.cur_yaw;
	if ((axis & AXIS_Z) == AXIS_Z) z = params.cur_yaw;

	// создать матрицу вращения и умножить на mTransform боны
	Fmatrix M;
	M.setHPB (-y, -x, -z);
	bone->mTransform.mulB_43	(M);
}

//****************************************************************************************************
// class bonesManipulation
//****************************************************************************************************

void bonesManipulation::Reset()
{
	time_started		= 0;
	time_last_update	= 0;
	in_return_state		= false;
	bActive				= false;
	freeze_time			= 0;
	time_last_delta		= 1;
}

void bonesManipulation::AddBone (CBoneInstance *bone, u8 axis_used)
{
	bonesBone tempB;

	tempB.Set(bone, axis_used,0.f,0.f,1.f);

	m_Bones.push_back(tempB);
}

void bonesManipulation::SetMotion(CBoneInstance *bone, u8 axis, float target_yaw, float r_speed, u32 t)
{
	int index = -1;
	// найти бону bone в m_Bones
	for (u32 i=0; i<m_Bones.size(); ++i)  {
		if ((m_Bones[i].bone == bone) && (m_Bones[i].axis == axis)) {
			index = i;
			break;
		}
	}
	R_ASSERT(-1 != index);

	m_Bones[index].params.target_yaw	= target_yaw;
	m_Bones[index].params.r_speed		= r_speed;
	m_Bones[index].params.dist_yaw		= angle_difference(target_yaw,m_Bones[index].params.cur_yaw);
	if (t > freeze_time) freeze_time = t;

	bActive				= true;
	in_return_state		= false;
	time_started	= 0;
}



void bonesManipulation::Update(CBoneInstance *bone, u32 cur_time)
{
	// провести обработку всех костей
	bool bones_were_turned = false;

	// вычисление dt
	u32 dt;
	if (cur_time == time_last_update) {
		dt = time_last_delta;
	} else dt = cur_time - time_last_update;
	time_last_delta = dt;
	time_last_update = cur_time;

	for (u32 i=0; i<m_Bones.size(); ++i) {
		if (m_Bones[i].NeedTurn()){
			if (m_Bones[i].bone == bone) m_Bones[i].Turn(dt);				
			bones_were_turned = true;
		}
	}

	// если процесс возврата завершен
	if (!bones_were_turned && in_return_state) {
		Reset();
		return;
	}

	// если ничего не произошло - выход
	if (!bActive && !bones_were_turned) return;

	// если выполняется наращивание угла и ни одна кость не повернулась (достигли таргета...)
	if (!bones_were_turned && !in_return_state) {
		if ((0 == time_started) && (freeze_time > 0)) { // начинаем ждать
			time_started = cur_time;
		}

		if ((0 != time_started) && (time_started + freeze_time < cur_time)) { // время вышло?
			time_started	= 0;

			// делаем возврат
			in_return_state	= true;
			// установить у всех костей в m_Bone таргеты в 0
			for (u32 i = 0; i<m_Bones.size(); ++i) {
				m_Bones[i].params.target_yaw	= 0.f;
				m_Bones[i].params.dist_yaw		= _abs(m_Bones[i].params.target_yaw - m_Bones[i].params.cur_yaw);
			}
			bActive = false;
		} 
	}

	// Установить параметры из m_Bones
	for (u32 i = 0; i<m_Bones.size(); ++i) {
		if (m_Bones[i].bone == bone) m_Bones[i].Apply();
	}
}

bonesAxis &bonesManipulation::GetBoneParams(CBoneInstance *bone, u8 axis_used)
{
	// найти бону bone в m_Bones
	for (u32 i=0; i<m_Bones.size(); ++i)  {
		if ((m_Bones[i].bone == bone) && (m_Bones[i].axis == axis_used)) {
			return m_Bones[i].params;
		}
	}
	VERIFY(FALSE);
	return m_Bones[0].params;
}
