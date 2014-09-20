#pragma once
#include "../xrEngine/effectorPP.h"
#include "../xrEngine/cameramanager.h"

//////////////////////////////////////////////////////////////////////////

class CPPEffectorCustom : public CEffectorPP {
	typedef CEffectorPP inherited;
public:
					CPPEffectorCustom	(const SPPInfo &ppi, bool one_instance = false, bool destroy_from_engine = true);
	EEffectorPPType	get_type			(){return m_type;}

protected:
	virtual	BOOL	Process				(SPPInfo& pp);

	// update factor; if return FALSE - destroy
	virtual BOOL	update				(){return TRUE;}

private:
	SPPInfo			m_state;
	EEffectorPPType	m_type;
protected:
	float			m_factor;
};

//////////////////////////////////////////////////////////////////////////

template<class _Effector>
class CPPEffectorCustomController {
public:
					CPPEffectorCustomController	();
IC	virtual void	load						(LPCSTR section);
IC	virtual bool	active						() {return (m_effector != 0);}

protected:
	_Effector		*m_effector;
	SPPInfo			m_state;
};


template<class _Effector>
CPPEffectorCustomController<_Effector>::CPPEffectorCustomController()
{
	m_effector = 0;
}

template<class _Effector>
void CPPEffectorCustomController<_Effector>::load(LPCSTR section)
{
	m_state.duality.h			= pSettings->r_float(section,"duality_h");
	m_state.duality.v			= pSettings->r_float(section,"duality_v");
	m_state.gray				= pSettings->r_float(section,"gray");
	m_state.blur				= pSettings->r_float(section,"blur");
	m_state.noise.intensity		= pSettings->r_float(section,"noise_intensity");
	m_state.noise.grain			= pSettings->r_float(section,"noise_grain");
	m_state.noise.fps			= pSettings->r_float(section,"noise_fps");
	VERIFY(!fis_zero(m_state.noise.fps));

	sscanf(pSettings->r_string(section,"color_base"),	"%f,%f,%f", &m_state.color_base.r, &m_state.color_base.g, &m_state.color_base.b);
	sscanf(pSettings->r_string(section,"color_gray"),	"%f,%f,%f", &m_state.color_gray.r, &m_state.color_gray.g, &m_state.color_gray.b);
	sscanf(pSettings->r_string(section,"color_add"),	"%f,%f,%f", &m_state.color_add.r,	&m_state.color_add.g,	 &m_state.color_add.b);
}

//////////////////////////////////////////////////////////////////////////


class CPPEffectorController;

class CPPEffectorControlled : public CPPEffectorCustom {
	typedef CPPEffectorCustom inherited;

	CPPEffectorController	*m_controller;
public:
					CPPEffectorControlled	(CPPEffectorController *controller, const SPPInfo &ppi, bool one_instance = false, bool destroy_from_engine = true);
	virtual BOOL	update					();
	IC		void	set_factor				(float value){m_factor = value;}
};

//////////////////////////////////////////////////////////////////////////


class CPPEffectorController : public CPPEffectorCustomController<CPPEffectorControlled>{
public:
					CPPEffectorController	();
	virtual 		~CPPEffectorController	();

	virtual void	frame_update			(); 

	virtual bool	check_completion		() = 0;
	virtual bool	check_start_conditions	() = 0;
	virtual void	update_factor			() = 0;
	
	// factory method
	virtual CPPEffectorControlled *create_effector	() = 0;

protected:
			void	activate				();
			void	deactivate				();
};

