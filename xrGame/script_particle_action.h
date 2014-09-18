////////////////////////////////////////////////////////////////////////////
//	Module 		: script_particle_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script particle action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "script_export_space.h"
#include "particle_params.h"

class CParticlesObject;

class CScriptParticleAction : public CScriptAbstractAction {
public:
	enum EGoalType {
		eGoalTypeParticleAttached = u32(0),
		eGoalTypeParticlePosition,
		eGoalTypeDummy = u32(-1),
	};

public:
	shared_str			m_caParticleToRun;
	shared_str			m_caBoneName;
	EGoalType			m_tGoalType;
	CParticlesObject	*m_tpParticleSystem;
	bool				m_bStartedToPlay;
	Fvector				m_tParticlePosition;
	Fvector				m_tParticleAngles;
	Fvector				m_tParticleVelocity;
	bool				m_bAutoRemove;

public:
	IC					CScriptParticleAction	();
	IC					CScriptParticleAction	(LPCSTR caPartcileToRun, LPCSTR caBoneName, const CParticleParams &tParticleParams = CParticleParams(), bool bAutoRemove = false);
	IC					CScriptParticleAction	(LPCSTR caPartcileToRun, const CParticleParams &tParticleParams = CParticleParams(), bool bAutoRemove = false);
	virtual				~CScriptParticleAction	();
			void		SetParticle				(LPCSTR caParticleToRun, bool bAutoRemove);
	IC		void		SetPosition				(const Fvector &tPosition);
	IC		void		SetBone					(LPCSTR caBoneName);
	IC		void		SetAngles				(const Fvector &tAngleOffset);
	IC		void		SetVelocity				(const Fvector &tVelocity);
	IC		void		initialize				();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptParticleAction)
#undef script_type_list
#define script_type_list save_type_list(CScriptParticleAction)

#include "script_particle_action_inline.h"