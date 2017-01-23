////////////////////////////////////////////////////////////////////////////
//	Module 		: physic_item.h
//	Created 	: 11.02.2004
//  Modified 	: 11.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Physic item
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "gameobject.h"
#include "PHShellCreator.h"
#include "PhysicsShellHolder.h"
class CPhysicItem : 
			public CPhysicsShellHolder,
			public CPHShellSimpleCreator
{
	typedef CPhysicsShellHolder inherited;
public:
	bool			m_ready_to_destroy;

public:
					CPhysicItem						();
	virtual			~CPhysicItem					();
			void	init							();
	virtual void	reinit							();
	virtual void	Load							(LPCSTR section);
	virtual void	reload							(LPCSTR section);
	virtual void	OnH_B_Independent				(bool just_before_destroy);
	virtual void	OnH_B_Chield					();
	virtual void	UpdateCL						();
	virtual BOOL	net_Spawn						(CSE_Abstract* DC);
	virtual void	net_Destroy						();
	virtual void	activate_physic_shell			();
	virtual void	setup_physic_shell				();
	virtual void	create_box_physic_shell			();
	virtual void	create_box2sphere_physic_shell	();
	virtual void	create_physic_shell				();
};

#include "physic_item_inline.h"
