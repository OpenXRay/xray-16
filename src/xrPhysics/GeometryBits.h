#pragma once

class CPHMesh;
class CPhysicsGeom;
class CPHGeometryBits
{
public:
    static void init_geom(CPhysicsGeom& g);
    static void init_geom(CPHMesh& g);
    static void set_ignore_static(CPhysicsGeom& g);
};
