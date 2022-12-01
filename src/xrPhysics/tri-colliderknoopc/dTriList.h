#pragma once
#include <ode/common.h>

/* Class ID */

extern int dTriListClass;

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
