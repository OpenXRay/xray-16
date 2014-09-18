#pragma once
#include "ShootingObject.h"
#include "HudSound.h"

class CPhysicsShellHolder;

class CCarWeapon :public CShootingObject
{
protected:
	typedef CShootingObject		inheritedShooting;

	void					SetBoneCallbacks	();
	void					ResetBoneCallbacks	();
	virtual	void			FireStart			();
	virtual	void			FireEnd				();
	virtual	void			UpdateFire			();
	virtual	void			OnShot				();
	void					UpdateBarrelDir		();
	virtual const Fvector&	get_CurrentFirePoint();
	virtual const Fmatrix&	get_ParticlesXFORM	();
	
	CPhysicsShellHolder*	m_object;
	bool					m_bActive;
	bool					m_bAutoFire;
	float					m_weapon_h;
	virtual bool			IsHudModeNow		(){return false;};

public:
	enum{
			eWpnDesiredDir		=1,
			eWpnDesiredPos,
			eWpnActivate,
			eWpnFire,
			eWpnAutoFire,
			eWpnToDefaultDir,
	};	
							CCarWeapon			(CPhysicsShellHolder* obj);
				virtual		~CCarWeapon			();
	static void _BCL		BoneCallbackX		(CBoneInstance *B);
	static void	_BCL		BoneCallbackY		(CBoneInstance *B);
				void		Load				(LPCSTR section);
				void		UpdateCL			();
			void			Action				(u16 id, u32 flags);
			void			SetParam			(int id, Fvector2 val);
			void			SetParam			(int id, Fvector val);
			bool			AllowFire			();
			float			FireDirDiff			();
			IC bool			IsActive			() {return m_bActive;}
			float			_height				() const	{return m_weapon_h;};
			const Fvector&	ViewCameraPos		();
			const Fvector&	ViewCameraDir		();
			const Fvector&	ViewCameraNorm		();

			void			Render_internal		();
private:
	u16						m_rotate_x_bone, m_rotate_y_bone, m_fire_bone, m_camera_bone;
	float					m_tgt_x_rot, m_tgt_y_rot, m_cur_x_rot, m_cur_y_rot, m_bind_x_rot, m_bind_y_rot;
	Fvector					m_bind_x, m_bind_y;
	Fvector					m_fire_dir,m_fire_pos, m_fire_norm;

	Fmatrix					m_i_bind_x_xform, m_i_bind_y_xform, m_fire_bone_xform;
	Fvector2				m_lim_x_rot, m_lim_y_rot; //in bone space
	float					m_min_gun_speed, m_max_gun_speed;
	CCartridge*				m_Ammo;
	float					m_barrel_speed;
	Fvector					m_destEnemyDir;
	bool					m_allow_fire;
	HUD_SOUND_ITEM			m_sndShot;
};