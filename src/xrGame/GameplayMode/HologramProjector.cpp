#include "StdAfx.h"
#include "HologramProjector.h"

namespace XRay
{

CHologramProjector::CHologramProjector()
{
    m_currentPattern.frequency = 8.0f;
    m_currentPattern.intensity = 0.75f;
    m_currentPattern.duration = 3.0f;
    m_currentPattern.animation_speed = 1.0f;
    m_currentPattern.pattern_type = SHologramPattern::Fractal;
}

CHologramProjector::~CHologramProjector()
{
}

void CHologramProjector::ActivateDesignTerminal()
{
    m_bDesignTerminalActive = true;
}

void CHologramProjector::DeactivateDesignTerminal()
{
    m_bDesignTerminalActive = false;
}

void CHologramProjector::ProjectHologram(const SHologramPattern& pattern)
{
    m_currentPattern = pattern;
    m_bProjecting = true;
    m_projectionTime = 0.0f;
    m_projectionDuration = pattern.duration;
}

void CHologramProjector::CancelProjection()
{
    m_bProjecting = false;
    m_projectionTime = 0.0f;
    m_projectionDuration = 0.0f;
}

bool CHologramProjector::IsProjecting() const
{
    return m_bProjecting;
}

const SHologramPattern& CHologramProjector::CurrentPattern() const
{
    return m_currentPattern;
}

void CHologramProjector::SetTarget(CGameObject* target)
{
    m_target = target;
}

CGameObject* CHologramProjector::GetTarget() const
{
    return m_target;
}

} // namespace XRay
