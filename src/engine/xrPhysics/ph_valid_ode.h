#pragma once

//#include "ode_include.h"
#include "../3rd party/ode/include/ode/common.h"
#include "../3rd party/ode/include/ode/mass.h"
#include "../3rd party/ode/include/ode/objects.h"
IC BOOL dV_valid			(const dReal * v)
{
	return _valid(v[0])&&_valid(v[1])&&_valid(v[2]);
}

IC BOOL dM_valid			(const dReal* m)
{
	return  _valid(m[0])&&_valid(m[1])&&_valid(m[2])&& 
		_valid(m[4])&&_valid(m[5])&&_valid(m[6])&&
		_valid(m[8])&&_valid(m[9])&&_valid(m[10]);
}

IC BOOL dV4_valid			(const dReal* v4)
{
	return _valid(v4[0])&&_valid(v4[1])&&
		_valid(v4[2])&&_valid(v4[3]);
}
IC BOOL dQ_valid			(const dReal* q)
{
	return dV4_valid(q);
}
IC BOOL dMass_valide(const dMass* m)
{
	return	_valid(m->mass)&&
		dV_valid(m->c)&&
		dM_valid(m->I);
}
IC BOOL dBodyStateValide(const dBodyID body)
{
	return dM_valid(dBodyGetRotation(body)) &&
		dV_valid(dBodyGetPosition(body))&&
		dV_valid(dBodyGetLinearVel(body))&&
		dV_valid(dBodyGetAngularVel(body))&&
		dV_valid(dBodyGetTorque(body))&&
		dV_valid(dBodyGetForce(body))
		;
}