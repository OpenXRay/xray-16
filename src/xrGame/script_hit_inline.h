////////////////////////////////////////////////////////////////////////////
//	Module 		: script_hit_inline.h
//	Created 	: 06.02.2004
//  Modified 	: 24.06.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script hit class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptHit::CScriptHit			()
{
	m_fPower			= 100;
	m_tDirection.set	(1,0,0);
	m_caBoneName		= "";
	m_tpDraftsman		= 0;
	m_fImpulse			= 100;
	m_tHitType			= ALife::eHitTypeWound;
}

IC	CScriptHit::CScriptHit			(const CScriptHit *tpLuaHit)
{
	*this				= *tpLuaHit;
}

IC	void CScriptHit::set_bone_name	(LPCSTR bone_name)
{
	m_caBoneName		= bone_name;
}
