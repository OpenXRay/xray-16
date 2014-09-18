#pragma once

#include "../../ActorEffector.h"

//////////////////////////////////////////////////////////////////////////
// CMonsterEffector
//////////////////////////////////////////////////////////////////////////
class CMonsterEffector : public CEffectorPP {
	typedef CEffectorPP inherited;	
	
	SPPInfo state;			// current state
	float	m_total;		// total PP time
	float	m_attack;		// attack time in percents	[0..1]
	float	m_release;		// release time in percents	[0..1]
	
	float	m_spec_factor;

public:
					CMonsterEffector		(const SPPInfo &ppi, float life_time, float attack_time = 0.0f, float release_time = 0.0f, float spec_factor = 1.f);
	virtual	BOOL	Process					(SPPInfo& pp);
};

//////////////////////////////////////////////////////////////////////////
// CMonsterEffectorHit
//////////////////////////////////////////////////////////////////////////
class CMonsterEffectorHit : public CEffectorCam {

	float total;
	float max_amp;
	float period_number;
	float power;

	Fvector offset;
public:
					CMonsterEffectorHit		(float time, float amp, float periods, float power);
	virtual BOOL	ProcessCam				(SCamEffectorInfo& info);
};


