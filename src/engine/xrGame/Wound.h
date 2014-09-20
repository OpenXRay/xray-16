// Wound.h: класс описания раны
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "hit_immunity.h"

class NET_Packet;

class CWound
{
public:
	CWound				(u16 bone_num);
	virtual ~CWound		(void);

	//serialization
	virtual void save	(NET_Packet &output_packet);
	virtual void load	(IReader &input_packet);

	float	TotalSize	();
	float	TypeSize	(ALife::EHitType hit_type);
	float	BloodSize	();

	void	AddHit		(float hit_power, ALife::EHitType hit_type);
	
	//заживление раны
	void	Incarnation	(float percent, float min_wound_size);
	u16		GetBoneNum	() {return m_iBoneNum;}
	void 	SetBoneNum	(u16 bone_num) {m_iBoneNum = bone_num;}

	u16		GetParticleBoneNum	() {return m_iParticleBoneNum;}
	void	SetParticleBoneNum	(u16 bone_num) {m_iParticleBoneNum = bone_num;}

	const shared_str& GetParticleName	()						{return m_sParticleName;}
	void	SetParticleName	(shared_str particle_name) {m_sParticleName = particle_name;}

	void  SetDestroy(bool destroy) {m_bToBeDestroy = destroy;}
	bool  GetDestroy()			   {return m_bToBeDestroy;}

	//время обновления (для капель крови)
	float m_fDropTime;

protected:
	//косточка на которой появилась рана
	u16 m_iBoneNum;

	//косточка, если на ране отыгрывается партикл
	u16 m_iParticleBoneNum;
	//имя этого партикла
	shared_str m_sParticleName;

	//список составляющих раны 
	HitImmunity::HitTypeSVec	m_Wounds;

	bool						m_bToBeDestroy;
};