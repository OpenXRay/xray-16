#pragma once

//#include "gameobject.h"

#include "script_export_space.h"

class CInventory;
class CGameObject;
class CCameraBase;
class CActor;

class CHolderCustom
{
private:
	CGameObject*			m_owner;
	CActor*					m_ownerActor;
protected:
	CGameObject*			Owner				(){return m_owner;}
	CActor*					OwnerActor			(){return m_ownerActor;}
public:
							CHolderCustom		()				{m_owner=NULL;m_ownerActor=NULL;}
	virtual					~CHolderCustom		()				{;}
	virtual	void			UpdateEx			(float fov){}; //called by owner
	virtual CHolderCustom	*cast_holder_custom	()				{return this;}
			bool			Engaged				()				{return m_owner!=NULL;}
	virtual void			OnMouseMove			(int x, int y)	= 0;
	virtual void			OnKeyboardPress		(int dik)		= 0;
	virtual void			OnKeyboardRelease	(int dik)		= 0;
	virtual void			OnKeyboardHold		(int dik)		= 0;
	// Inventory for the car
	virtual CInventory*		GetInventory		()				= 0;

	virtual void			cam_Update			(float dt, float fov=90.0f)		= 0;

	virtual bool			Use					(const Fvector& pos,const Fvector& dir,const Fvector& foot_pos)=0;
	virtual bool			attach_Actor		(CGameObject* actor);
	virtual void			detach_Actor		();
	virtual bool			allowWeapon			()	const		= 0;
	virtual bool			HUDView				() const		= 0;
	virtual Fvector			ExitPosition		()				= 0;
	virtual Fvector			ExitVelocity		()				{return Fvector().set(0,0,0);}
	virtual CCameraBase*	Camera				()				= 0;
	virtual void			Action				(u16 id, u32 flags)				{};
	virtual void			SetParam			(int id, Fvector2 val)			{};
	virtual void			SetParam			(int id, Fvector val)			{};
	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CHolderCustom)
#undef script_type_list
#define script_type_list save_type_list(CHolderCustom)