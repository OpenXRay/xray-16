////////////////////////////////////////////////////////////////////////////
//	Module 		: script_particle_action_inline.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script particle action class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CScriptParticleAction::CScriptParticleAction(
    LPCSTR caPartcileToRun, LPCSTR caBoneName, const CParticleParams& tParticleParams, bool bAutoRemove)
{
    SetBone(caBoneName);
    SetPosition(tParticleParams.m_tParticlePosition);
    SetAngles(tParticleParams.m_tParticleAngles);
    SetVelocity(tParticleParams.m_tParticleVelocity);
    SetParticle(caPartcileToRun, bAutoRemove);
}

IC CScriptParticleAction::CScriptParticleAction(
    LPCSTR caPartcileToRun, const CParticleParams& tParticleParams, bool bAutoRemove)
{
    SetParticle(caPartcileToRun, bAutoRemove);
    SetPosition(tParticleParams.m_tParticlePosition);
    SetAngles(tParticleParams.m_tParticleAngles);
    SetVelocity(tParticleParams.m_tParticleVelocity);
}

IC void CScriptParticleAction::SetPosition(const Fvector& tPosition)
{
    m_tParticlePosition = tPosition;
    m_tGoalType = eGoalTypeParticlePosition;
    m_bStartedToPlay = false;
    m_bCompleted = false;
}

IC void CScriptParticleAction::SetBone(LPCSTR caBoneName)
{
    m_caBoneName = caBoneName;
    m_bStartedToPlay = false;
    m_bCompleted = false;
}

IC void CScriptParticleAction::SetAngles(const Fvector& tAngleOffset)
{
    m_tParticleAngles = tAngleOffset;
    m_bStartedToPlay = false;
    m_bCompleted = false;
}

IC void CScriptParticleAction::SetVelocity(const Fvector& tVelocity)
{
    m_tParticleVelocity = tVelocity;
    m_bStartedToPlay = false;
    m_bCompleted = false;
}

IC void CScriptParticleAction::initialize() {}
