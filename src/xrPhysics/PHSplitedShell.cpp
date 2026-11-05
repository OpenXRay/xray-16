#include "StdAfx.h"
#include "PhysicsShell.h"
#include "PHObject.h"
#include "PHWorld.h"
#include "PHInterpolation.h"
#include "PHShell.h"
#include "PHJoint.h"
#include "PHElement.h"
#include "PHSplitedShell.h"
#include "Physics.h"

void CPHSplitedShell::Collide()
{
}

void CPHSplitedShell::get_spatial_params()
{
    CPHShell::get_spatial_params();

    if (spatial.sphere.R > m_max_AABBradius)
        spatial.sphere.R = m_max_AABBradius;
}

void CPHSplitedShell::DisableObject()
{
    CPHObject::deactivate();
}
