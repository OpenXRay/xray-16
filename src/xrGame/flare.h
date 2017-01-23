#pragma once

class CLAItem;
class CParticlesObject;

#include "hud_item_object.h"

class CFlare :public CHudItemObject
{
private:
	typedef			CHudItemObject	inherited;
	enum FlareStates{eFlareHidden,eFlareShowing,eFlareIdle,eFlareHiding,eFlareDropping};

	CLAItem*					light_lanim;
	ref_light					light_render;
	CParticlesObject*			m_pFlareParticles;
	float						m_work_time_sec;
	void						SwitchOn						();
	void						SwitchOff						();
	void						FirePoint						(Fvector&);
	void						ParticlesMatrix					(Fmatrix&);
public:
	virtual void				UpdateCL						();
	virtual void				Load							(LPCSTR section);
	virtual BOOL				net_Spawn						(CSE_Abstract* DC);
	virtual void				net_Destroy						();

	virtual void				OnStateSwitch					(u32 S);
	virtual void				OnAnimationEnd					(u32 state);

	virtual	void				UpdateXForm						();

	void						ActivateFlare					();
	void						DropFlare						();
	bool						IsFlareActive					();
};