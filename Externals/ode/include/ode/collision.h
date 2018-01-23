/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 2.1 of the License, or (at  *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE.TXT.                                               *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *                                                                       *
 * This library is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files    *
 * LICENSE.TXT and LICENSE-BSD.TXT for more details.                     *
 *                                                                       *
 *************************************************************************/

#ifndef _ODE_COLLISION_H_
#define _ODE_COLLISION_H_

#include <ode/common.h>
#include <ode/collision_space.h>
#include <ode/contact.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup collide Collision Detection
 *
 * ODE has two main components: a dynamics simulation engine and a collision 
 * detection engine. The collision engine is given information about the 
 * shape of each body. At each time step it figures out which bodies touch 
 * each other and passes the resulting contact point information to the user. 
 * The user in turn creates contact joints between bodies.
 *
 * Using ODE's collision detection is optional - an alternative collision 
 * detection system can be used as long as it can supply the right kinds of 
 * contact information. 
 */


/* ************************************************************************ */
/* general functions */

/**
 * @brief Destroy a geom, removing it from any space.
 *
 * Destroy a geom, removing it from any space it is in first. This one 
 * function destroys a geom of any type, but to create a geom you must call 
 * a creation function for that type.
 *
 * When a space is destroyed, if its cleanup mode is 1 (the default) then all 
 * the geoms in that space are automatically destroyed as well. 
 *
 * @param geom the geom to be destroyed.
 * @ingroup collide
 */
void dGeomDestroy (dGeomID geom);


/**
 * @brief Set the user-defined data pointer stored in the geom.
 *
 * @param geom the geom to hold the data
 * @param data the data pointer to be stored
 * @ingroup collide
 */
void dGeomSetData (dGeomID geom, void* data);


/**
 * @brief Get the user-defined data pointer stored in the geom.
 *
 * @param geom the geom containing the data
 * @ingroup collide
 */
void *dGeomGetData (dGeomID geom);


/**
 * @brief Set the body associated with a placeable geom. 
 *
 * Setting a body on a geom automatically combines the position vector and 
 * rotation matrix of the body and geom, so that setting the position or 
 * orientation of one will set the value for both objects. Setting a body 
 * ID of zero gives the geom its own position and rotation, independent 
 * from any body. If the geom was previously connected to a body then its 
 * new independent position/rotation is set to the current position/rotation 
 * of the body.
 *
 * Calling these functions on a non-placeable geom results in a runtime 
 * error in the debug build of ODE. 
 *
 * @param geom the geom to connect
 * @param body the body to attach to the geom
 * @ingroup collide
 */
void dGeomSetBody (dGeomID geom, dBodyID body);


/**
 * @brief Get the body associated with a placeable geom. 
 * @param geom the geom to query.
 * @sa dGeomSetBody
 * @ingroup collide
 */
dBodyID dGeomGetBody (dGeomID geom);


/**
 * @brief Set the position vector of a placeable geom.
 *
 * If the geom is attached to a body, the body's position will also be changed.
 * Calling this function on a non-placeable geom results in a runtime error in 
 * the debug build of ODE. 
 *
 * @param geom the geom to set.
 * @param x the new X coordinate.
 * @param y the new Y coordinate.
 * @param z the new Z coordinate.
 * @sa dBodySetPosition
 * @ingroup collide
 */
void dGeomSetPosition (dGeomID geom, dReal x, dReal y, dReal z);


/**
 * @brief Set the rotation matrix of a placeable geom.
 *
 * If the geom is attached to a body, the body's rotation will also be changed.
 * Calling this function on a non-placeable geom results in a runtime error in 
 * the debug build of ODE. 
 *
 * @param geom the geom to set.
 * @param R the new rotation matrix.
 * @sa dBodySetRotation
 * @ingroup collide
 */
void dGeomSetRotation (dGeomID geom, const dMatrix3 R);


/**
 * @brief Set the rotation of a placeable geom.
 *
 * If the geom is attached to a body, the body's rotation will also be changed.
 *
 * Calling this function on a non-placeable geom results in a runtime error in 
 * the debug build of ODE. 
 *
 * @param geom the geom to set.
 * @param Q the new rotation.
 * @sa dBodySetQuaternion
 * @ingroup collide
 */
void dGeomSetQuaternion (dGeomID geom, const dQuaternion Q);


/**
 * @brief Get the position vector of a placeable geom.
 *
 * If the geom is attached to a body, the body's position will be returned.
 *
 * Calling this function on a non-placeable geom results in a runtime error in 
 * the debug build of ODE. 
 *
 * @param geom the geom to query.
 * @returns A pointer to the geom's position vector.
 * @remarks The returned value is a pointer to the geom's internal
 *          data structure. It is valid until any changes are made
 *          to the geom.
 * @sa dBodyGetPosition
 * @ingroup collide
 */
const dReal * dGeomGetPosition (dGeomID geom);


/**
 * @brief Get the rotation matrix of a placeable geom.
 *
 * If the geom is attached to a body, the body's rotation will be returned.
 *
 * Calling this function on a non-placeable geom results in a runtime error in 
 * the debug build of ODE. 
 *
 * @param geom the geom to query.
 * @returns A pointer to the geom's rotation matrix.
 * @remarks The returned value is a pointer to the geom's internal
 *          data structure. It is valid until any changes are made
 *          to the geom.
 * @sa dBodyGetRotation
 * @ingroup collide
 */
const dReal * dGeomGetRotation (dGeomID geom);


/**
 * @brief Get the rotation quaternion of a placeable geom.
 *
 * If the geom is attached to a body, the body's quaternion will be returned.
 *
 * Calling this function on a non-placeable geom results in a runtime error in 
 * the debug build of ODE. 
 *
 * @param geom the geom to query.
 * @param result a copy of the rotation quaternion.
 * @sa dBodyGetQuaternion
 * @ingroup collide
 */
void dGeomGetQuaternion (dGeomID geom, dQuaternion result);


/**
 * @brief Return the axis-aligned bounding box.
 *
 * Return in aabb an axis aligned bounding box that surrounds the given geom. 
 * The aabb array has elements (minx, maxx, miny, maxy, minz, maxz). If the 
 * geom is a space, a bounding box that surrounds all contained geoms is 
 * returned.
 *
 * This function may return a pre-computed cached bounding box, if it can 
 * determine that the geom has not moved since the last time the bounding 
 * box was computed.
 *
 * @param geom the geom to query
 * @param aabb the returned bounding box
 * @ingroup collide
 */
void dGeomGetAABB (dGeomID geom, dReal aabb[6]);


/**
 * @brief Determing if a geom is a space.
 * @param geom the geom to query
 * @returns Non-zero if the geom is a space, zero otherwise.
 * @ingroup collide
 */
int dGeomIsSpace (dGeomID geom);


/**
 * @brief Query for the space containing a particular geom.
 * @param geom the geom to query
 * @returns The space that contains the geom, or NULL if the geom is
 *          not contained by a space.
 * @ingroup collide
 */
dSpaceID dGeomGetSpace (dGeomID);


/**
 * @brief Given a geom, this returns its class.
 *
 * The ODE classes are:
 *  @li dSphereClass
 *  @li dBoxClass
 *  @li dCylinderClass
 *  @li dPlaneClass
 *  @li dRayClass
 *  @li dConvexClass
 *  @li dGeomTransformClass
 *  @li dTriMeshClass
 *  @li dSimpleSpaceClass
 *  @li dHashSpaceClass
 *  @li dQuadTreeSpaceClass
 *  @li dFirstUserClass
 *  @li dLastUserClass
 *
 * User-defined class will return their own number.
 *
 * @param geom the geom to query
 * @returns The geom class ID.
 * @ingroup collide
 */
int dGeomGetClass (dGeomID geom);


/**
 * @brief Set the "category" bitfield for the given geom. 
 *
 * The category bitfield is used by spaces to govern which geoms will 
 * interact with each other. The bitfield is guaranteed to be at least 
 * 32 bits wide. The default category values for newly created geoms 
 * have all bits set.
 *
 * @param geom the geom to set
 * @param bits the new bitfield value
 * @ingroup collide
 */
void dGeomSetCategoryBits (dGeomID geom, unsigned long bits);


/**
 * @brief Set the "collide" bitfield for the given geom. 
 *
 * The collide bitfield is used by spaces to govern which geoms will 
 * interact with each other. The bitfield is guaranteed to be at least 
 * 32 bits wide. The default category values for newly created geoms 
 * have all bits set.
 *
 * @param geom the geom to set
 * @param bits the new bitfield value
 * @ingroup collide
 */
void dGeomSetCollideBits (dGeomID geom, unsigned long bits);


/**
 * @brief Get the "category" bitfield for the given geom. 
 *
 * @param geom the geom to set
 * @param bits the new bitfield value
 * @sa dGeomSetCategoryBits
 * @ingroup collide
 */
unsigned long dGeomGetCategoryBits (dGeomID);


/**
 * @brief Get the "collide" bitfield for the given geom. 
 *
 * @param geom the geom to set
 * @param bits the new bitfield value
 * @sa dGeomSetCollideBits
 * @ingroup collide
 */
unsigned long dGeomGetCollideBits (dGeomID);


/**
 * @brief Enable a geom. 
 *
 * Disabled geoms are completely ignored by dSpaceCollide and dSpaceCollide2,
 * although they can still be members of a space. New geoms are created in 
 * the enabled state. 
 *
 * @param geom   the geom to enable
 * @sa dGeomDisable
 * @sa dGeomIsEnabled
 * @ingroup collide
 */
void dGeomEnable (dGeomID geom);


/**
 * @brief Disable a geom. 
 *
 * Disabled geoms are completely ignored by dSpaceCollide and dSpaceCollide2,
 * although they can still be members of a space. New geoms are created in 
 * the enabled state. 
 *
 * @param geom   the geom to disable
 * @sa dGeomDisable
 * @sa dGeomIsEnabled
 * @ingroup collide
 */
void dGeomDisable (dGeomID geom);


/**
 * @brief Check to see if a geom is enabled.
 *
 * Disabled geoms are completely ignored by dSpaceCollide and dSpaceCollide2,
 * although they can still be members of a space. New geoms are created in 
 * the enabled state. 
 *
 * @param geom   the geom to query
 * @returns Non-zero if the geom is enabled, zero otherwise.
 * @sa dGeomDisable
 * @sa dGeomIsEnabled
 * @ingroup collide
 */
int dGeomIsEnabled (dGeomID geom);



/* ************************************************************************ */
/* collision detection */

int dCollide (dGeomID o1, dGeomID o2, int flags, dContactGeom *contact,
	      int skip);
void dSpaceCollide (dSpaceID space, void *data, dNearCallback *callback);
void dSpaceCollide2 (dGeomID o1, dGeomID o2, void *data,
		     dNearCallback *callback);

/* ************************************************************************ */
/* standard classes */

/* the maximum number of user classes that are supported */
enum {
  dMaxUserClasses = 4
};

/* class numbers - each geometry object needs a unique number */
enum {
  dSphereClass = 0,
  dBoxClass,
  dCCylinderClass,
  dCylinderClass,
  dPlaneClass,
  dRayClass,
  dGeomTransformClass,
  dTriMeshClass,

  dFirstSpaceClass,
  dSimpleSpaceClass = dFirstSpaceClass,
  dHashSpaceClass,
  dQuadTreeSpaceClass,
  dLastSpaceClass = dQuadTreeSpaceClass,

  dFirstUserClass,
  dLastUserClass = dFirstUserClass + dMaxUserClasses - 1,
  dGeomNumClasses
};


dGeomID dCreateSphere (dSpaceID space, dReal radius);
void dGeomSphereSetRadius (dGeomID sphere, dReal radius);
dReal dGeomSphereGetRadius (dGeomID sphere);
dReal dGeomSpherePointDepth (dGeomID sphere, dReal x, dReal y, dReal z);

dGeomID dCreateBox (dSpaceID space, dReal lx, dReal ly, dReal lz);
void dGeomBoxSetLengths (dGeomID box, dReal lx, dReal ly, dReal lz);
void dGeomBoxGetLengths (dGeomID box, dVector3 result);
dReal dGeomBoxPointDepth (dGeomID box, dReal x, dReal y, dReal z);

dGeomID dCreatePlane (dSpaceID space, dReal a, dReal b, dReal c, dReal d);
void dGeomPlaneSetParams (dGeomID plane, dReal a, dReal b, dReal c, dReal d);
void dGeomPlaneGetParams (dGeomID plane, dVector4 result);
dReal dGeomPlanePointDepth (dGeomID plane, dReal x, dReal y, dReal z);

dGeomID dCreateCCylinder (dSpaceID space, dReal radius, dReal length);
void dGeomCCylinderSetParams (dGeomID ccylinder, dReal radius, dReal length);
void dGeomCCylinderGetParams (dGeomID ccylinder, dReal *radius, dReal *length);
dReal dGeomCCylinderPointDepth (dGeomID ccylinder, dReal x, dReal y, dReal z);

dGeomID dCreateRay (dSpaceID space, dReal length);
void dGeomRaySetLength (dGeomID ray, dReal length);
dReal dGeomRayGetLength (dGeomID ray);
void dGeomRaySet (dGeomID ray, dReal px, dReal py, dReal pz,
		  dReal dx, dReal dy, dReal dz);
void dGeomRayGet (dGeomID ray, dVector3 start, dVector3 dir);

/*
 * Set/get ray flags that influence ray collision detection.
 * These flags are currently only noticed by the trimesh collider, because
 * they can make a major differences there.
 */
void dGeomRaySetParams (dGeomID g, int FirstContact, int BackfaceCull);
void dGeomRayGetParams (dGeomID g, int *FirstContact, int *BackfaceCull);
void dGeomRaySetClosestHit (dGeomID g, int closestHit);
int dGeomRayGetClosestHit (dGeomID g);

#include "collision_trimesh.h"

dGeomID dCreateGeomTransform (dSpaceID space);
void dGeomTransformSetGeom (dGeomID g, dGeomID obj);
dGeomID dGeomTransformGetGeom (dGeomID g);
void dGeomTransformSetCleanup (dGeomID g, int mode);
int dGeomTransformGetCleanup (dGeomID g);
void dGeomTransformSetInfo (dGeomID g, int mode);
int dGeomTransformGetInfo (dGeomID g);

/* ************************************************************************ */
/* utility functions */

void dClosestLineSegmentPoints (const dVector3 a1, const dVector3 a2,
				const dVector3 b1, const dVector3 b2,
				dVector3 cp1, dVector3 cp2);

int dBoxTouchesBox (const dVector3 _p1, const dMatrix3 R1,
		    const dVector3 side1, const dVector3 _p2,
		    const dMatrix3 R2, const dVector3 side2);

void dInfiniteAABB (dGeomID geom, dReal aabb[6]);
void dCloseODE(void);

/* ************************************************************************ */
/* custom classes */

typedef void dGetAABBFn (dGeomID, dReal aabb[6]);
typedef int dColliderFn (dGeomID o1, dGeomID o2,
			 int flags, dContactGeom *contact, int skip);
typedef dColliderFn * dGetColliderFnFn (int num);
typedef void dGeomDtorFn (dGeomID o);
typedef int dAABBTestFn (dGeomID o1, dGeomID o2, dReal aabb[6]);

typedef struct dGeomClass {
  int bytes;
  dGetColliderFnFn *collider;
  dGetAABBFn *aabb;
  dAABBTestFn *aabb_test;
  dGeomDtorFn *dtor;
} dGeomClass;

int dCreateGeomClass (const dGeomClass *classptr);
void * dGeomGetClassData (dGeomID);
dGeomID dCreateGeom (int classnum);

/* ************************************************************************ */

#ifdef __cplusplus
}
#endif

#endif
