/////////////////////////////////////////////////////
// Аномальная зона: "мясорубка"
// При попадании живого объекта в зону происходит 
// электрический разряд
// Зона восстанавливает заряд через определенное время
// (через m_dwPeriod заряжается с 0 до m_fMaxPower)
//
/////////////////////////////////////////////////////
#pragma once

#include "gravizone.h"
#include "telewhirlwind.h"
#include "PhysicsShellHolder.h"
#include "script_export_space.h"
#include "PHDestroyable.h"

class CMincer :	
	public CBaseGraviZone,
	public CPHDestroyableNotificator
{
private:
	typedef CBaseGraviZone inherited;
	CTeleWhirlwind m_telekinetics;
	shared_str	   m_torn_particles;
	ref_sound	   m_tearing_sound;
	float		   m_fActorBlowoutRadiusPercent;

public:
	virtual CTelekinesis &Telekinesis			() {return m_telekinetics;}

public:
					CMincer						();
	virtual			~CMincer					();
//	virtual void	SwitchZoneState				(EZoneState new_state);
	virtual void	OnStateSwitch				(EZoneState new_state);
	virtual	BOOL	feel_touch_contact			(CObject* O);
	virtual void	feel_touch_new				(CObject* O);
	virtual void	Load						(LPCSTR section);
	virtual bool	BlowoutState				();
	virtual void	AffectPullDead				(CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist){}
	virtual void	AffectPullAlife				(CEntityAlive* EA,const Fvector& throw_in_dir,float dist);
	virtual void	AffectThrow					(SZoneObjectInfo* O, CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist);
	virtual void	ThrowInCenter				(Fvector& C);
	virtual BOOL	net_Spawn					(CSE_Abstract* DC);
	virtual void	net_Destroy					();
	virtual void	Center						(Fvector& C) const;
	virtual	void	NotificateDestroy			(CPHDestroyableNotificate *dn);
	virtual float	BlowoutRadiusPercent		(CPhysicsShellHolder* GO);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CMincer)
#undef script_type_list
#define script_type_list save_type_list(CMincer)