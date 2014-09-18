#pragma once

#include "PhysicsShell.h"
#include "PhysicsShellAnimatorBoneData.h"
class animation_movement_controller;
class CPhysicsShellAnimator
{
	friend class CPhysicsShellAnimatorBoneData;
	xr_vector<CPhysicsShellAnimatorBoneData>		m_bones_data;
	CPhysicsShell*									m_pPhysicsShell;
	Fmatrix											m_StartXFORM;

	void											CreateJoints( LPCSTR controled );
	void											CreateJoint( CPHElement *e );
public:
													CPhysicsShellAnimator		( CPhysicsShell* _pPhysicsShell, CInifile* ini, LPCSTR section );
													~CPhysicsShellAnimator		();
	void											OnFrame						();
};
	

