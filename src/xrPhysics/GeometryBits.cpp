#include "StdAfx.h"
#include "GeometryBits.h"
#include "PHWorld.h"
#include "ExtendedGeom.h"
#include "xrPhysicsCore/IPhysicsCore.h"

void CPHGeometryBits::init_geom(CPhysicsGeom& g)
{

}

void CPHGeometryBits::init_geom(CPHMesh& g)
{

}

void CPHGeometryBits::set_ignore_static(CPhysicsGeom& g)
{
    if (g.get_body() != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetBodyIgnoreStatic(g.get_body());
    }
}
