////////////////////////////////////////////////////////////////////////////
//	Module 		: particle_params.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Particle parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once

struct CParticleParams final
{
    Fvector m_tParticlePosition;
    Fvector m_tParticleAngles;
    Fvector m_tParticleVelocity;

    CParticleParams(const Fvector& tPositionOffset = {}, const Fvector& tAnglesOffset = {}, const Fvector& tVelocity = {})
        : m_tParticlePosition(tPositionOffset), m_tParticleAngles(tAnglesOffset), m_tParticleVelocity(tVelocity) {}

    void initialize() {}
};

