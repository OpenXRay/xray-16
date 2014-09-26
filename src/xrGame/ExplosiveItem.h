//////////////////////////////////////////////////////////////////////
// ExplosiveItem.h: класс для вещи которая взрывается под 
//					действием различных хитов (канистры,
//					балоны с газом и т.д.)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Explosive.h"
#include "inventory_item_object.h"
#include "DelayedActionFuse.h"
class CExplosiveItem: 
			public CInventoryItemObject	,
			public CDelayedActionFuse	,
			public CExplosive
{
private:
	typedef CInventoryItemObject inherited;

public:
	CExplosiveItem(void);
	virtual ~CExplosiveItem(void);

	virtual void				Load					(LPCSTR section)							;
	virtual BOOL				net_Spawn				(CSE_Abstract* DC)							{return CInventoryItemObject::net_Spawn(DC);}
	virtual void				net_Destroy				()											;
	virtual void				net_Export				(NET_Packet& P)								{CInventoryItemObject::net_Export(P);}
	virtual void				net_Import				(NET_Packet& P)								{CInventoryItemObject::net_Import(P);}
	virtual void				net_Relcase				(CObject* O )								;
	virtual CGameObject			*cast_game_object		()											{return this;}
	virtual CExplosive*			cast_explosive			()											{return this;}
	virtual IDamageSource*		cast_IDamageSource		()											{return CExplosive::cast_IDamageSource();}
	virtual void				GetRayExplosionSourcePos(Fvector &pos)								;
	virtual void				ActivateExplosionBox	(const Fvector &size,Fvector &in_out_pos)	;
	virtual void				OnEvent					(NET_Packet& P, u16 type)					;
	virtual	void				Hit						(SHit* pHDS)								;
	virtual void				shedule_Update			(u32 dt)									;
	virtual bool				shedule_Needed			();

	virtual void				UpdateCL				()											;
	virtual void				renderable_Render		()											; 
	virtual void				ChangeCondition			(float fDeltaCondition)						{CInventoryItem::ChangeCondition(fDeltaCondition);};
	virtual void				StartTimerEffects		()											;

};