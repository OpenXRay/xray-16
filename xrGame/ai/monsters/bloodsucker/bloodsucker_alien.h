#pragma once

class CAI_Bloodsucker;
class CAlienEffector;
class CAlienEffectorPP;

class CBloodsuckerAlien {
	
	CAI_Bloodsucker				*m_object;
	
	bool						m_active;

	CAlienEffector				*m_effector;
	CAlienEffectorPP			*m_effector_pp;
	
	bool						m_crosshair_show;

public:
			CBloodsuckerAlien	();
			~CBloodsuckerAlien	();
	
	void	init_external		(CAI_Bloodsucker *obj);	
	void	reinit				();

	void	activate			();
	void	deactivate			();

	bool	active				() {return m_active;}

};