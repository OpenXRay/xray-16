////////////////////////////////////////////////////////////////////////////
//	Module 		: script_hit.h
//	Created 	: 06.02.2004
//  Modified 	: 24.06.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script hit class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"

class CScriptGameObject;

class CScriptHit {
public:
	float				m_fPower; 
	Fvector				m_tDirection;
	shared_str				m_caBoneName;
	CScriptGameObject		*m_tpDraftsman;
	float				m_fImpulse;
	int					m_tHitType;

public:
	IC					CScriptHit		();
	IC					CScriptHit		(const CScriptHit *tpLuaHit);
	virtual				~CScriptHit		();
	IC		void		set_bone_name	(LPCSTR bone_name);
};

#include "script_hit_inline.h"
