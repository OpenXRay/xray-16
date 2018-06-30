#pragma once

#include "dcylinder/dCylinder.h"
#include "PhysicsShell.h"
#include "PHObject.h"
#include "PHInterpolation.h"
#include "xrCore/_cylinder.h"
#include "BlockAllocator.h"
#include "PhysicsCommon.h"
#include "PHWorld.h"
#include "PHContactBodyEffector.h"
#include "phvalide.h"
//#define ODE_SLOW_SOLVER
///////////////////////////////////////////////////////////////////////////////

void BodyCutForce(dBodyID body, float l_limit, float w_limit);
void dBodyAngAccelFromTorqu(const dBodyID body, dReal* ang_accel, const dReal* torque);
//	float	E_NlS						(dBodyID body,const dReal* norm,float norm_sign)					;
float E_NLD(dBodyID b1, dBodyID b2, const dReal* norm);

//	float E_NL( dBodyID b1, dBodyID b2, const dReal* norm );

void ApplyGravityAccel(dBodyID body, const dReal* accel);
const dReal fix_ext_param = 10000.f;
const dReal fix_mass_param = 100000000.f;
void FixBody(dBodyID body);
void dMassSub(dMass* a, const dMass* b);
void SaveContacts(dGeomID o1, dGeomID o2, dJointGroupID jointGroup);
const dReal* dJointGetPositionContact(dJointID joint);

// const dReal world_spring=24000000.f;//2400000.f;//550000.f;///1000000.f;;
// const dReal world_damping=400000.f;//erp/cfm1.1363636e-006f,0.54545456f

extern class CBlockAllocator<dJointFeedback, 128> ContactFeedBacks;
extern CBlockAllocator<CPHContactBodyEffector, 128> ContactEffectors;
// void NearCallback(void* /*data*/, dGeomID o1, dGeomID o2);
void NearCallback(CPHObject* obj1, CPHObject* obj2, dGeomID o1, dGeomID o2);
void CollideStatic(dGeomID o2, CPHObject* obj2);

class CPHElement;
class CPHShell;
extern dJointGroupID ContactGroup;
extern Fbox phBoundaries;
