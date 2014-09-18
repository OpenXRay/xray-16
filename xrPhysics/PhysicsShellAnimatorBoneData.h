#pragma once

#include "PHShell.h"

class CPhysicsShellAnimatorBoneData//Содержит информацию об целевой матрице анимации
{
	friend class CPhysicsShellAnimator;	
	dJointID					m_anim_fixed_dJointID;
	CPHElement*					m_element;
};