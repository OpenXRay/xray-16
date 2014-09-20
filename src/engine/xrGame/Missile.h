#pragma once
#include "hud_item_object.h"
#include "HudSound.h"

struct dContact;
struct SGameMtl;
class CMissile : public CHudItemObject
{
	typedef CHudItemObject inherited;
public:
	enum EMissileStates{
		eThrowStart = eLastBaseState+1,
		eReady,
		eThrow,
		eThrowEnd,
	};
							CMissile					();
	virtual					~CMissile					();

	virtual BOOL			AlwaysTheCrow				()				{ return TRUE; }
	virtual void			render_item_ui					();
	virtual bool			render_item_ui_query					();

	virtual void			reinit						();
	virtual CMissile*		cast_missile				()				{return this;}

	virtual void 			Load						(LPCSTR section);
	virtual BOOL 			net_Spawn					(CSE_Abstract* DC);
	virtual void 			net_Destroy					();

	virtual void 			UpdateCL					();
	virtual void 			shedule_Update				(u32 dt);

	virtual void 			OnH_A_Chield				();
	virtual void 			OnH_B_Independent			(bool just_before_destroy);

	virtual void 			OnEvent						(NET_Packet& P, u16 type);

	virtual void 			OnAnimationEnd				(u32 state);
	virtual void			OnMotionMark				(u32 state, const motion_marks&);


	virtual void 			Throw();
	virtual void 			Destroy();

	virtual bool 			Action						(u16 cmd, u32 flags);

	virtual void 			State						(u32 state);
	virtual void 			OnStateSwitch				(u32 S);
	virtual bool			GetBriefInfo				(II_BriefInfo& info);

protected:
	virtual void			UpdateFireDependencies_internal	();
	virtual void			UpdateXForm						();
	void					UpdatePosition					(const Fmatrix& trans);
	void					spawn_fake_missile				();

	virtual void			OnActiveItem		();
	virtual void			OnHiddenItem		();

	//для сети
	virtual void			net_Relcase			(CObject* O );
protected:

	//время нахождения в текущем состоянии
	u32						m_dwStateTime;
	bool					m_throw;
	
	//время уничтожения
	u32						m_dwDestroyTime;
	u32						m_dwDestroyTimeMax;

	Fvector					m_throw_direction;
	Fmatrix					m_throw_matrix;

	CMissile				*m_fake_missile;

	//параметры броска
	
	float m_fMinForce, m_fConstForce, m_fMaxForce, m_fForceGrowSpeed;
//private:
	bool					m_constpower;
	float					m_fThrowForce;
protected:
	//относительная точка и направление вылета гранаты
	Fvector					m_vThrowPoint;
	Fvector					m_vThrowDir;

protected:
			void			setup_throw_params		();
public:
	Fvector const&			throw_point_offset		() const {return m_vThrowPoint;}
	virtual void			activate_physic_shell	();
	virtual void			setup_physic_shell		();
	virtual void			create_physic_shell		();
	IC		void			set_destroy_time		(u32 delta_destroy_time) {m_dwDestroyTime = delta_destroy_time + Device.dwTimeGlobal;}
	virtual void			PH_A_CrPr				();

protected:
	u32						m_ef_weapon_type;

public:
	virtual u32				ef_weapon_type			() const;
	IC		u32				destroy_time			() const { return m_dwDestroyTime; }
	IC		int				time_from_begin_throw	() const { return (Device.dwTimeGlobal + m_dwDestroyTimeMax - m_dwDestroyTime); }
	static	void			ExitContactCallback		(bool& do_colide,bool bo1,dContact& c,SGameMtl * /*material_1*/,SGameMtl * /*material_2*/);
};