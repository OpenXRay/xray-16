#pragma once

#include "holder_custom.h"
#include "physicsshellholder.h"
class CCameraBase;

class CHolderEntityObject:	public CPhysicsShellHolder, 
						public CHolderCustom
{
private:
	typedef CPhysicsShellHolder inheritedPH;
	typedef CHolderCustom		inheritedHolder;

private:
	CCameraBase*			camera;
	// 
	static void		_BCL	BoneCallbackX		(CBoneInstance *B);
	static void		_BCL	BoneCallbackY		(CBoneInstance *B);
	void					SetBoneCallbacks	();
	void					ResetBoneCallbacks	();

//casts
public:
	virtual CHolderCustom	*cast_holder_custom	()				{return this;}

//general
public:
							CHolderEntityObject		();
	virtual					~CHolderEntityObject	();

	virtual void			Load				(LPCSTR section);

	virtual BOOL			net_Spawn			(CSE_Abstract* DC);
	virtual void			net_Destroy			();
	virtual void			net_Export			(NET_Packet& P);	// export to server
	virtual void			net_Import			(NET_Packet& P);	// import from server

	virtual void			UpdateCL			();

	virtual	void			Hit					(SHit* pHDS);

//shooting
private:
	u16						m_rotate_x_bone, m_rotate_y_bone, m_camera_bone;
	float					m_tgt_x_rot, m_tgt_y_rot, m_cur_x_rot, m_cur_y_rot, m_bind_x_rot, m_bind_y_rot;
	Fvector					m_bind_x, m_bind_y;

	Fmatrix					m_i_bind_x_xform, m_i_bind_y_xform, m_fire_bone_xform;
	Fvector2				m_lim_x_rot, m_lim_y_rot; //in bone space
	bool 					m_bAllowWeapon;
protected:
			void			SetDesiredDir		(float h, float p);
	virtual bool			IsHudModeNow		(){return false;};

//HolderCustom
public:
	virtual bool			Use					(const Fvector& pos,const Fvector& dir,const Fvector& foot_pos) {return !Owner();};
	virtual void			OnMouseMove			(int x, int y);
	virtual void			OnKeyboardPress		(int dik);
	virtual void			OnKeyboardRelease	(int dik);
	virtual void			OnKeyboardHold		(int dik);
	virtual CInventory*		GetInventory		(){return NULL;};
	virtual void			cam_Update			(float dt, float fov=90.0f);

	virtual void			renderable_Render	();

	virtual bool			attach_Actor		(CGameObject* actor);
	virtual void			detach_Actor		();
	virtual bool			allowWeapon			()	const				{return m_bAllowWeapon;};
	virtual bool			HUDView				()	const				{return true;};
	virtual Fvector			ExitPosition		()						{return Fvector().set(0.0f,0.0f,0.0f);};

	virtual CCameraBase*	Camera				()						{return camera;};

	virtual void			Action				(u16 id, u32 flags);
	virtual void			SetParam			(int id, Fvector2 val);
};