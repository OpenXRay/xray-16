#pragma once

class CBulletManager;

#include "xrUICore/ui_defs.h"

class CTracer
{
    friend CBulletManager;

protected:
    ui_shader sh_Tracer;
    xr_vector<u32> m_aColors;
    float m_circle_size_k;

public:
    CTracer();
    void Render(const Fvector& pos, const Fvector& center, const Fvector& dir, float length, float width, u8 colorID,
        float speed, bool bActor);
};
