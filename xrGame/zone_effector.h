#pragma once

#include "../xrServerEntities/alife_space.h"
class CActor;
class CPostprocessAnimatorLerp;

class CZoneEffector {
	float						r_min_perc;
	float						r_max_perc;
	float						m_radius;
	float						m_factor;
	CPostprocessAnimatorLerp*	m_pp_effector;
	shared_str					m_pp_fname;
public:
			CZoneEffector		();
			~CZoneEffector		();

	CActor*						m_pActor;

	void	Load				(LPCSTR section);
	void	Update				(float dist, float radius, ALife::EHitType hit_type);
	void	Stop				();
	float xr_stdcall GetFactor	();

private:
	void	Activate			();


};



