////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_object.cpp
//	Created 	: 24.03.2003
//  Modified 	: 27.12.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item object implementation
////////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch_script.h"
#include "inventory_item_object.h"


CInventoryItemObject::CInventoryItemObject	()
{
}

CInventoryItemObject::~CInventoryItemObject	()
{
}

DLL_Pure *CInventoryItemObject::_construct	()
{
	CInventoryItem::_construct	();
	CPhysicItem::_construct		();
	return						(this);
}

void CInventoryItemObject::Load				(LPCSTR section) 
{
	CPhysicItem::Load			(section);
	CInventoryItem::Load		(section);
}
/* remove
LPCSTR CInventoryItemObject::Name			()
{
	return						(CInventoryItem::Name());
}

LPCSTR CInventoryItemObject::NameShort		()
{
	return						(CInventoryItem::NameShort());
}
*/
/*
LPCSTR CInventoryItemObject::NameComplex	()
{
	return						(CInventoryItem::NameComplex());
}
*/

void				CInventoryItemObject::Hit					(SHit* pHDS)
{
	CPhysicItem::Hit(pHDS);
	CInventoryItem::Hit(pHDS);
}

void CInventoryItemObject::OnH_B_Independent(bool just_before_destroy)
{
	CInventoryItem::OnH_B_Independent	(just_before_destroy);
	CPhysicItem::OnH_B_Independent		(just_before_destroy);
}

void CInventoryItemObject::OnH_A_Independent()
{
	CInventoryItem::OnH_A_Independent	();
	CPhysicItem::OnH_A_Independent		();
}


void CInventoryItemObject::OnH_B_Chield		()
{
	CPhysicItem::OnH_B_Chield			();
	CInventoryItem::OnH_B_Chield		();
}

void CInventoryItemObject::OnH_A_Chield		()
{
	CPhysicItem::OnH_A_Chield			();
	CInventoryItem::OnH_A_Chield		();
}

void CInventoryItemObject::UpdateCL			()
{
	CPhysicItem::UpdateCL				();
	CInventoryItem::UpdateCL			();
}

void CInventoryItemObject::OnEvent			(NET_Packet& P, u16 type)
{
	CPhysicItem::OnEvent				(P, type);
	CInventoryItem::OnEvent				(P, type);
}

BOOL CInventoryItemObject::net_Spawn		(CSE_Abstract* DC)
{
	BOOL								res = CPhysicItem::net_Spawn(DC);
	CInventoryItem::net_Spawn			(DC);
	return								(res);
}

void CInventoryItemObject::net_Destroy		()
{
	CInventoryItem::net_Destroy			();
	CPhysicItem::net_Destroy			();
}

void CInventoryItemObject::net_Import		(NET_Packet& P) 
{	
	CInventoryItem::net_Import			(P);
}

void CInventoryItemObject::net_Export		(NET_Packet& P) 
{	
	CInventoryItem::net_Export			(P);
}

void CInventoryItemObject::save				(NET_Packet &packet)
{
	CPhysicItem::save					(packet);
	CInventoryItem::save				(packet);
}

void CInventoryItemObject::load				(IReader &packet)
{
	CPhysicItem::load					(packet);
	CInventoryItem::load				(packet);
}

void CInventoryItemObject::renderable_Render()
{
	CPhysicItem::renderable_Render		();
	CInventoryItem::renderable_Render	();
}

void CInventoryItemObject::reload			(LPCSTR section)
{
	CPhysicItem::reload					(section);
	CInventoryItem::reload				(section);
}

void CInventoryItemObject::reinit		()
{
	CInventoryItem::reinit				();
	CPhysicItem::reinit					();
}

void CInventoryItemObject::activate_physic_shell	()
{
	CInventoryItem::activate_physic_shell	();
}

void CInventoryItemObject::on_activate_physic_shell	()
{
	CPhysicItem::activate_physic_shell	();
}

void CInventoryItemObject::make_Interpolation	()
{
	CInventoryItem::make_Interpolation	();
}

void CInventoryItemObject::PH_B_CrPr		()
{
	CInventoryItem::PH_B_CrPr			();
}	

void CInventoryItemObject::PH_I_CrPr		()
{
	CInventoryItem::PH_I_CrPr			();
} 

#ifdef DEBUG
void CInventoryItemObject::PH_Ch_CrPr		()
{
	CInventoryItem::PH_Ch_CrPr			();
}
#endif

void CInventoryItemObject::PH_A_CrPr		()
{
	CInventoryItem::PH_A_CrPr			();
}

#ifdef DEBUG
void CInventoryItemObject::OnRender			()
{
	CInventoryItem::OnRender			();
}
#endif

void CInventoryItemObject::modify_holder_params	(float &range, float &fov) const
{
	CInventoryItem::modify_holder_params		(range,fov);
}

u32	 CInventoryItemObject::ef_weapon_type		() const
{
	return								(0);
}

bool CInventoryItemObject::NeedToDestroyObject	() const
{
	return CInventoryItem::NeedToDestroyObject();
}

bool CInventoryItemObject::Useful				() const
{
	return			(CInventoryItem::Useful());
}
