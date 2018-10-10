#pragma once

#include "PHShell.h"

class CPHSplitedShell : public CPHShell
{
    float m_max_AABBradius;
    virtual void SetMaxAABBRadius(float size) { m_max_AABBradius = size; }
protected:
    virtual void Collide();
    virtual void get_spatial_params();
    virtual void DisableObject();

private:
public:
    CPHSplitedShell() { m_max_AABBradius = dInfinity; }
};
