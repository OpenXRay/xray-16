////////////////////////////////////////////////////////////////////////////////
//	Module		:	artefact_activation.h
//	Created		:	19.12.2007
//	Modified	:	19.12.2007
//	Autor		:	Alexander Maniluk
//	Description	:	artefact activation class
////////////////////////////////////////////////////////////////////////////////
#ifndef ARTEFACT_ACTIVATION
#define ARTEFACT_ACTIVATION

#include "Artefact.h"

class SArtefactActivation
{
public:
	enum	EActivationStates	
	{
		eNone=0,
		eStarting,
		eFlying,
		eBeforeSpawn,
		eSpawnZone,
		eMax
	};
	
	struct SStateDef{
		float		m_time;
		shared_str	m_snd;
		Fcolor		m_light_color;
		float		m_light_range;
		shared_str	m_particle;
		shared_str	m_animation;
		
					SStateDef	():m_time(0.0f){};
		void		Load		(LPCSTR section, LPCSTR name);
	};

	SArtefactActivation			(CArtefact* af, u32 owner_id);
	~SArtefactActivation		();
	CArtefact*					m_af;
	svector<SStateDef,eMax>		m_activation_states;
	EActivationStates			m_cur_activation_state;
	float						m_cur_state_time;

	ref_light					m_light;
	ref_sound					m_snd;
	
	u32							m_owner_id;

	virtual		void			UpdateActivation				();
	virtual		void			Load							();
	virtual		void			Start							();
	virtual		void			Stop							();
	virtual		void			ChangeEffects					();
	virtual		void			UpdateEffects					();
	virtual		void			SpawnAnomaly					();
	virtual		void			PhDataUpdate					(float step);
				bool			IsInProgress					();
private:
	bool						m_in_process;
}; // class SArtefactActivation

#endif // ARTEFACT_ACTIVATION