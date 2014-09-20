//////////////////////////////////////////////////////////////////////
// SleepEffector.cpp:	эффектор, который запускаетс€ во врем€ сна
//						актера                           
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "SleepEffector.h"


//////////////////////////////////////////////////////////////////////////
// CMonsterEffector
//////////////////////////////////////////////////////////////////////////
CSleepEffectorPP::CSleepEffectorPP(const SPPInfo &ppi, float life_time, float attack_time, float release_time) 
: CEffectorPP(EEffectorPPType(SLEEP_EFFECTOR_TYPE_ID), life_time)
{
	state		= ppi;
	m_total		= life_time;

	m_attack	= ((fis_zero(attack_time)) ? 0.5f : attack_time);
	m_release	= ((fis_zero(release_time)) ? 0.5f : release_time);

	VERIFY(!fsimilar(m_release, 1.0f));
	VERIFY(!fis_zero(m_attack));

	m_eSleepState = BEGIN_SLEEP;
}

BOOL CSleepEffectorPP::Process(SPPInfo& pp)
{
	inherited::Process(pp);

	// amount of time passed in percents
	float time_past_perc = (m_total - fLifeTime) / m_total;
    float factor;


	if (time_past_perc < m_attack) 
	{
		factor = time_past_perc / m_attack;
		m_eSleepState = BEGIN_SLEEP;
	}
	else if (BEGIN_SLEEP == m_eSleepState && 
				(time_past_perc >= m_attack) && 
				(time_past_perc <= m_release)) 
	{
		factor = 1.0f;
		m_eSleepState = BEFORE_SLEEPING;
	}
	else if (SLEEPING == m_eSleepState)
	{
		//не измен€ть значение fLifeTime пока спим
		fLifeTime = m_attack*m_total;
		factor = 1.0f;
	}
	else if (AWAKING == m_eSleepState)//просыпаемс€
		factor = (1.0f - time_past_perc) / (1.0f - m_release);

	clamp(factor,0.01f, 1.0f);

	if(m_eSleepState==SLEEPING) return TRUE;
		
	SPPInfo	def;

	pp.duality.h		= def.duality.h			+ (state.duality.h			- def.duality.h)		* factor; 			
	pp.duality.v		= def.duality.v			+ (state.duality.v			- def.duality.v)		* factor;
	pp.gray				= def.gray				+ (state.gray				- def.gray)				* factor;
	pp.blur				= def.blur				+ (state.blur				- def.blur)				* factor;
	pp.noise.intensity	= def.noise.intensity	+ (state.noise.intensity	- def.noise.intensity)	* factor;
	pp.noise.grain		= def.noise.grain		+ (state.noise.grain		- def.noise.grain)		* factor;
	pp.noise.fps		= def.noise.fps			+ (state.noise.fps			- def.noise.fps)		* factor;	
	VERIFY(!fis_zero(pp.noise.fps));


	pp.color_base.set	(
		def.color_base.r	+ (state.color_base.r - def.color_base.r) * factor, 
		def.color_base.g	+ (state.color_base.g - def.color_base.g) * factor, 
		def.color_base.b	+ (state.color_base.b - def.color_base.b) * factor
		);

	pp.color_gray.set	(
		def.color_gray.r	+ (state.color_gray.r - def.color_gray.r) * factor, 
		def.color_gray.g	+ (state.color_gray.g - def.color_gray.g) * factor, 
		def.color_gray.b	+ (state.color_gray.b - def.color_gray.b) * factor
		);

	pp.color_add.set	(
		def.color_add.r	+ (state.color_add.r - def.color_add.r) * factor, 
		def.color_add.g	+ (state.color_add.g - def.color_add.g) * factor, 
		def.color_add.b	+ (state.color_add.b - def.color_add.b) * factor
		);

	return TRUE;
}