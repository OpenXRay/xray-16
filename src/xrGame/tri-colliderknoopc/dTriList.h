//#include "stdafx.h"
#include "ode_include.h"


/* Class ID */

extern int dTriListClass;



/* Single precision, no padding vector3 used for storage */

struct dcVector3{

	float x, y, z;

};



/* Per triangle callback */

typedef int dTriCallback(dGeomID TriList, dGeomID RefObject, int TriangleIndex);

void dGeomTriListSetCallback(dGeomID g, dTriCallback* Callback);

dTriCallback* dGeomTriListGetCallback(dGeomID g);



/* Per object callback */

typedef void dTriArrayCallback(dGeomID TriList, dGeomID RefObject, const int* TriIndices, int TriCount);

void dGeomTriListSetArrayCallback(dGeomID g, dTriArrayCallback* ArrayCallback);

dTriArrayCallback* dGeomTriListGetArrayCallback(dGeomID g);



/* Construction */

dxGeom* dCreateTriList(dSpaceID space, dTriCallback* Callback, dTriArrayCallback* ArrayCallback);



/* Setting data */

void dGeomTriListBuild(dGeomID g, const dcVector3* Vertices, int VertexCount, const int* Indices, int IndexCount);



/* Getting data */

void dGeomTriListGetTriangle(dGeomID g, int Index, dVector3* v0, dVector3* v1, dVector3* v2);