#pragma once

#include "StdAfx.h"

class CGameObject;

namespace XRay
{

struct SHologramPattern
{
    enum EPatternType
    {
        Fractal = 0,
        Wave = 1,
        Spiral = 2,
        Geometric = 3,
        Organic = 4,
    };

    float frequency = 8.0f;
    float intensity = 0.75f;
    float duration = 3.0f;
    float animation_speed = 1.0f;
    EPatternType pattern_type = Fractal;
    Fvector color_warm = Fvector().set(0.85f, 0.35f, 0.95f);
    Fvector color_cool = Fvector().set(0.25f, 0.65f, 1.0f);
};

class CHologramProjector
{
public:
    CHologramProjector();
    ~CHologramProjector();

    void ActivateDesignTerminal();
    void DeactivateDesignTerminal();
    void ProjectHologram(const SHologramPattern& pattern);
    void CancelProjection();

    bool IsProjecting() const;
    const SHologramPattern& CurrentPattern() const;
    void SetTarget(CGameObject* target);
    CGameObject* GetTarget() const;

private:
    CGameObject* m_target = nullptr;
    SHologramPattern m_currentPattern;
    bool m_bDesignTerminalActive = false;
    bool m_bProjecting = false;
    float m_projectionTime = 0.0f;
    float m_projectionDuration = 0.0f;
};

} // namespace XRay
