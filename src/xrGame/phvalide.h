#pragma	once

#include "ode_include.h"

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
bool valid_pos( const Fvector &P );

#ifdef DEBUG

std::string dbg_valide_pos_string( const Fvector &pos,const Fbox &bounds, const CObject *obj, LPCSTR msg );
std::string dbg_valide_pos_string( const Fvector &pos, const CObject *obj, LPCSTR msg );

#define	VERIFY_BOUNDARIES2(pos,bounds,obj,msg) VERIFY2(  valid_pos( pos, bounds ), dbg_valide_pos_string( pos, bounds, obj, msg ) )
#define	VERIFY_BOUNDARIES(pos,bounds,obj)	VERIFY_BOUNDARIES2(pos,bounds,obj,"	")

#else
#define	VERIFY_BOUNDARIES(pos,bounds,obj)
#define	VERIFY_BOUNDARIES2(pos,bounds,obj,msg)
#endif