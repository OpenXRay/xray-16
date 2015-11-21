
#ifndef dCylinder_h
#define dCylinder_h



#include "ODE/include/ode/common.h"

struct dxCylinder;
extern int dCylinderClassUser;


dxGeom *dCreateCylinder (dSpaceID space, dReal r, dReal lz);
void dGeomCylinderSetParams (dGeomID g, dReal radius, dReal length);

void dGeomCylinderGetParams (dGeomID g, dReal *radius, dReal *length);
#endif //dCylinder_h