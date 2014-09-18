#include "stdafx.h"

#include "dTriCollideK.h"
#include "dxTriList.h"
#include "dcTriListCollider.h"
#include "../ExtendedGeom.h"
#include "dcTriListCollider.cpp"	// Allow inlining
#include "../gameobject.h"



int dTriListClass = -1;

dcTriListCollider* GetData(dxGeom* TriList){

	dxTriList* Data = (dxTriList*)dGeomGetClassData(TriList);

	return Data->Collider;

}



inline bool ValidateCollision(dxGeom* o1, dxGeom* o2){
	return dGeomGetUserData(o1)->b_static_colide;
	/*
	dxBody* b1 = dGeomGetBody(o1);

	dxBody* b2 = dGeomGetBody(o2);



	if (b1){

		if (!dBodyIsEnabled(b1)){

			b1 = 0;

		}

	}

	if (b2){

		if (!dBodyIsEnabled(b2)){

			b2 = 0;

		}

	}

	return b1 || b2;
	*/
	//return true;
}



int dCollideSTL(dxGeom* TriList, dxGeom* Sphere, int Flags, dContactGeom* Contact, int Stride) throw()
{

	if (ValidateCollision(Sphere, TriList)){

		return GetData(TriList)->CollideSphere(Sphere, Flags, Contact, Stride);

	}

	else return 0;

}



int dCollideBTL(dxGeom* TriList, dxGeom* Box, int Flags, dContactGeom* Contact, int Stride)throw()
{

	if (ValidateCollision(Box, TriList)){

		return GetData(TriList)->CollideBox(Box, Flags, Contact, Stride);

	}

	else return 0;

}

int dCollideCTL(dxGeom* TriList, dxGeom* Cyl, int Flags, dContactGeom* Contact, int Stride)throw()
{

	if (ValidateCollision(Cyl, TriList)){

		return GetData(TriList)->CollideCylinder(Cyl, Flags, Contact, Stride);

	}

	else return 0;

}



dColliderFn* dTriListColliderFn(int num)
{
	//	Log("in dTriListColliderFn ");
	//	Msg("num=%d",num);
	if (num ==dBoxClass){ 
		return 	(dColliderFn*)&dCollideBTL;
	}
	if (num ==dSphereClass) {
		return (dColliderFn*)&dCollideSTL;
	}

	if (num == dCylinderClassUser) return (dColliderFn*)&dCollideCTL;

	return 0;

}

int dAABBTestTL(dxGeom* TriList, dxGeom* Object, dReal AABB[6]) throw()
{

	return 1;
}

void dDestroyTriList(dGeomID g){

	xr_delete(((dxTriList*)dGeomGetClassData(g))->Collider);
}



/* External functions */

void dGeomTriListSetCallback(dGeomID g, dTriCallback* Callback){

	dxTriList* Data = (dxTriList*)dGeomGetClassData(g);

	Data->Callback = Callback;

}



dTriCallback* dGeomTriListGetCallback(dGeomID g){

	dxTriList* Data = (dxTriList*)dGeomGetClassData(g);

	return Data->Callback;

}



void dGeomTriListSetArrayCallback(dGeomID g, dTriArrayCallback* ArrayCallback){

	dxTriList* Data = (dxTriList*)dGeomGetClassData(g);

	Data->ArrayCallback = ArrayCallback;

}



dTriArrayCallback* dGeomTriListGetArrayCallback(dGeomID g){

	dxTriList* Data = (dxTriList*)dGeomGetClassData(g);

	return Data->ArrayCallback;

}



dxGeom* dCreateTriList(dSpaceID space, dTriCallback* Callback, dTriArrayCallback* ArrayCallback){

	if (dTriListClass == -1){

		dGeomClass c;

		c.bytes = sizeof(dxTriList);

		c.collider = &dTriListColliderFn;

		c.aabb = &dInfiniteAABB;

		c.aabb_test = &dAABBTestTL;

		//	c.aabb_test=NULL;
		c.dtor = &dDestroyTriList;



		dTriListClass = dCreateGeomClass(&c);

	}



	dxGeom* g = dCreateGeom(dTriListClass);

	if (space) dSpaceAdd(space, g);



	dxTriList* Data = (dxTriList*)dGeomGetClassData(g);

	Data->Callback = Callback;

	Data->ArrayCallback = ArrayCallback;

	Data->Collider = xr_new<dcTriListCollider>(g);



	return g;

}



