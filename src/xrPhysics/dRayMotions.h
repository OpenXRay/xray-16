#ifndef dRayMotions_h
#define dRayMotions_h

struct dxRayMotions;
extern int dRayMotionsClassUser;


dxGeom *dCreateRayMotions (dSpaceID space);
void dGeomRayMotionsSet (dGeomID g,const dReal* p,const dReal* d, dReal l);
void dGeomRayMotionSetGeom(dGeomID rm,dGeomID g);

#endif //dRayMotions_h