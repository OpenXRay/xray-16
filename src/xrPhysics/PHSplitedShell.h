#pragma once

#include "PHShell.h"
#include <cfloat> // Для FLT_MAX

class CPHSplitedShell : public CPHShell
{
    float m_max_AABBradius;
    virtual void SetMaxAABBRadius(float size) { m_max_AABBradius = size; }
protected:
    virtual void Collide();
    virtual void get_spatial_params();
    virtual void DisableObject();

public:
    CPHSplitedShell() { m_max_AABBradius = FLT_MAX; }
};
