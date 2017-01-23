//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "particle_core.h"

using namespace PAPI;

// To offset [0 .. 1] vectors to [-.5 .. .5]
static pVector vHalf(0.5, 0.5, 0.5);

ICF pVector RandVec()
{
	return pVector(drand48(), drand48(), drand48());
}

// Return a random number with a normal distribution.
float PAPI::NRand(float sigma)
{
#define ONE_OVER_SIGMA_EXP (1.0f / 0.7975f)
	
	if(sigma == 0) return 0;
	
	float y;
	do
	{
		y = -logf(drand48());
	}
	while(drand48() > expf(-_sqr(y - 1.0f)*0.5f));
	
	if(rand() & 0x1)
		return y * sigma * ONE_OVER_SIGMA_EXP;
	else
		return -y * sigma * ONE_OVER_SIGMA_EXP;
}
////////////////////////////////////////////////////////////////////////////////
// Stuff for the pDomain.
pDomain::pDomain(PDomainEnum dtype, float a0, float a1,
				 float a2, float a3, float a4, float a5,
				 float a6, float a7, float a8)
{
	type = dtype;
	switch(type)
	{
	case PDPoint:
		p1 = pVector(a0, a1, a2);
		break;
	case PDLine:
		{
			p1 = pVector(a0, a1, a2);
			pVector tmp(a3, a4, a5);
			// p2 is vector3 from p1 to other endpoint.
			p2 = tmp - p1;
		}
		break;
	case PDBox:
		// p1 is the min corner. p2 is the max corner.
		if(a0 < a3)
		{
			p1.x = a0; p2.x = a3;
		}
		else
		{
			p1.x = a3; p2.x = a0;
		}
		if(a1 < a4)
		{
			p1.y = a1; p2.y = a4;
		}
		else
		{
			p1.y = a4; p2.y = a1;
		}
		if(a2 < a5)
		{
			p1.z = a2; p2.z = a5;
		}
		else
		{
			p1.z = a5; p2.z = a2;
		}
		break;
	case PDTriangle:
		{
			p1 = pVector(a0, a1, a2);
			pVector tp2 = pVector(a3, a4, a5);
			pVector tp3 = pVector(a6, a7, a8);
			
			u = tp2 - p1;
			v = tp3 - p1;
			
			// The rest of this is needed for bouncing.
			radius1Sqr = u.length();
			pVector tu = u / radius1Sqr;
			radius2Sqr = v.length();
			pVector tv = v / radius2Sqr;
			
			p2 = tu ^ tv; // This is the non-unit normal.
			p2.normalize_safe(); // Must normalize it.
			
			// radius1 stores the d of the plane eqn.
			radius1 = -(p1 * p2);
		}
		break;
	case PDRectangle:
		{
			p1 = pVector(a0, a1, a2);
			u = pVector(a3, a4, a5);
			v = pVector(a6, a7, a8);
			
			// The rest of this is needed for bouncing.
			radius1Sqr = u.length();
			pVector tu = u / radius1Sqr;
			radius2Sqr = v.length();
			pVector tv = v / radius2Sqr;
			
			p2 = tu ^ tv; // This is the non-unit normal.
			p2.normalize_safe(); // Must normalize it.
			
			// radius1 stores the d of the plane eqn.
			radius1 = -(p1 * p2);
		}
		break;
	case PDPlane:
		{
			p1 = pVector(a0, a1, a2);
			p2 = pVector(a3, a4, a5);
			p2.normalize_safe(); // Must normalize it.
			
			// radius1 stores the d of the plane eqn.
			radius1 = -(p1 * p2);
		}
		break;
	case PDSphere:
		p1 = pVector(a0, a1, a2);
		if(a3 > a4)
		{
			radius1 = a3; radius2 = a4;
		}
		else
		{
			radius1 = a4; radius2 = a3;
		}
		radius1Sqr = radius1 * radius1;
		radius2Sqr = radius2 * radius2;
		break;
	case PDCone:
	case PDCylinder:
		{
			// p2 is a vector3 from p1 to the other end of cylinder.
			// p1 is apex of cone.
			
			p1 = pVector(a0, a1, a2);
			pVector tmp(a3, a4, a5);
			p2 = tmp - p1;
			
			if(a6 > a7)
			{
				radius1 = a6; radius2 = a7;
			}
			else
			{
				radius1 = a7; radius2 = a6;
			}
			radius1Sqr = _sqr(radius1);
			
			// Given an arbitrary nonzero vector3 n, make two orthonormal
			// vectors u and v forming a frame [u,v,n.normalize()].
			pVector n = p2;
			float p2l2 = n.length2(); // Optimize this.
			n.normalize_safe();
			
			// radius2Sqr stores 1 / (p2.p2)
			// XXX Used to have an actual if.
			radius2Sqr = p2l2 ? 1.0f / p2l2 : 0.0f;
			
			// Find a vector3 orthogonal to n.
			pVector basis(1.0f, 0.0f, 0.0f);
			if(_abs(basis * n) > 0.999)
				basis = pVector(0.0f, 1.0f, 0.0f);
			
			// Project away N component, normalize and cross to get
			// second orthonormal vector3.
			u = basis - n * (basis * n);
			u.normalize_safe();
			v = n ^ u;
		}
		break;
	case PDBlob:
		{
			p1 = pVector(a0, a1, a2);
			radius1 = a3;
			float tmp = 1.f/radius1;
			radius2Sqr = -0.5f*_sqr(tmp);
			radius2 = ONEOVERSQRT2PI * tmp;
		}
		break;
	case PDDisc:
		{
			p1 = pVector(a0, a1, a2); // Center point
			p2 = pVector(a3, a4, a5); // Normal (not used in Within and Generate)
			p2.normalize_safe();
			
			if(a6 > a7)
			{
				radius1 = a6; radius2 = a7;
			}
			else
			{
				radius1 = a7; radius2 = a6;
			}
			
			// Find a vector3 orthogonal to n.
			pVector basis(1.0f, 0.0f, 0.0f);
			if(_abs(basis * p2) > 0.999)
				basis = pVector(0.0f, 1.0f, 0.0f);
			
			// Project away N component, normalize and cross to get
			// second orthonormal vector3.
			u = basis - p2 * (basis * p2);
			u.normalize_safe();
			v = p2 ^ u;
			radius1Sqr = -(p1 * p2); // D of the plane eqn.
		}
		break;
	}
}

// Determines if pos is inside the domain
BOOL pDomain::Within(const pVector &pos) const
{
	switch (type)
	{
	case PDBox:
		return !((pos.x < p1.x) || (pos.x > p2.x) ||
			(pos.y < p1.y) || (pos.y > p2.y) ||
			(pos.z < p1.z) || (pos.z > p2.z));
	case PDPlane:
		// Distance from plane = n * p + d
		// Inside is the positive half-space.
		return pos * p2 >= -radius1;
	case PDSphere:
		{
			pVector rvec(pos - p1);
			float rSqr = rvec.length2();
			return rSqr <= radius1Sqr && rSqr >= radius2Sqr;
		}
	case PDCylinder:
	case PDCone:
		{
			// This is painful and slow. Might be better to do quick
			// accept/reject tests.
			// Let p2 = vector3 from base to tip of the cylinder
			// x = vector3 from base to test point
			// x . p2
			// dist = ------ = projected distance of x along the axis
			// p2. p2 ranging from 0 (base) to 1 (tip)
			//
			// rad = x - dist * p2 = projected vector3 of x along the base
			// p1 is the apex of the cone.
			
			pVector x(pos - p1);
			
			// Check axial distance
			// radius2Sqr stores 1 / (p2.p2)
			float dist = (p2 * x) * radius2Sqr;
			if(dist < 0.0f || dist > 1.0f)
				return FALSE;
			
			// Check radial distance; scale radius along axis for cones
			pVector xrad = x - p2 * dist; // Radial component of x
			float rSqr = xrad.length2();
			
			if(type == PDCone)
				return (rSqr <= _sqr(dist * radius1) &&
				rSqr >= _sqr(dist * radius2));
			else
				return (rSqr <= radius1Sqr && rSqr >= _sqr(radius2));
		}
	case PDBlob:
		{
			pVector x(pos - p1);
			// return exp(-0.5 * xSq * Sqr(oneOverSigma)) * ONEOVERSQRT2PI * oneOverSigma;
			float Gx = expf(x.length2() * radius2Sqr) * radius2;
			return (drand48() < Gx);
		}
	case PDPoint:
	case PDLine:
	case PDRectangle:
	case PDTriangle:
	case PDDisc:
	default:
		return FALSE; // XXX Is there something better?
	}
}

// Generate a random point uniformly distrbuted within the domain
void pDomain::Generate(pVector &pos) const
{
	switch (type)
	{
	case PDPoint:
		pos = p1;
		break;
	case PDLine:
		pos = p1 + p2 * drand48();
		break;
	case PDBox:
		// Scale and translate [0,1] random to fit box
		pos.x = p1.x + (p2.x - p1.x) * drand48();
		pos.y = p1.y + (p2.y - p1.y) * drand48();
		pos.z = p1.z + (p2.z - p1.z) * drand48();
		break;
	case PDTriangle:
		{
			float r1 = drand48();
			float r2 = drand48();
			if(r1 + r2 < 1.0f)
				pos = p1 + u * r1 + v * r2;
			else
				pos = p1 + u * (1.0f-r1) + v * (1.0f-r2);
		}
		break;
	case PDRectangle:
		pos = p1 + u * drand48() + v * drand48();
		break;
	case PDPlane: // How do I sensibly make a point on an infinite plane?
		pos = p1;
		break;
	case PDSphere:
		// Place on [-1..1] sphere
		pos = RandVec() - vHalf;
		pos.normalize_safe();
		
		// Scale unit sphere pos by [0..r] and translate
		// (should distribute as r^2 law)
		if(radius1 == radius2)
			pos = p1 + pos * radius1;
		else
			pos = p1 + pos * (radius2 + drand48() * (radius1 - radius2));
		break;
	case PDCylinder:
	case PDCone:
		{
			// For a cone, p2 is the apex of the cone.
			float dist = drand48(); // Distance between base and tip
			float theta = drand48() * 2.0f * float(M_PI); // Angle around axis
			// Distance from axis
			float r = radius2 + drand48() * (radius1 - radius2);
			
			float x = r * _cos(theta); // Weighting of each frame vector3
			float y = r * _sin(theta);
			
			// Scale radius along axis for cones
			if(type == PDCone)
			{
				x *= dist;
				y *= dist;
			}
			
			pos = p1 + p2 * dist + u * x + v * y;
		}
		break;
	case PDBlob:
		pos.x = p1.x + NRand(radius1);
		pos.y = p1.y + NRand(radius1);
		pos.z = p1.z + NRand(radius1);
		
		break;
	case PDDisc:
		{
			float theta = drand48() * 2.0f * float(M_PI); // Angle around normal
			// Distance from center
			float r = radius2 + drand48() * (radius1 - radius2);
			
			float x = r * _cos(theta); // Weighting of each frame vector3
			float y = r * _sin(theta);
			
			pos = p1 + u * x + v * y;
		}
		break;
	default:
		pos = pVector(0,0,0);
	}
}

void pDomain::transform(const pDomain& domain, const Fmatrix& m)
{
	switch (type)
	{
	case PDBox:{
		Fbox* bb_dest=(Fbox*)&p1;
		Fbox* bb_from=(Fbox*)&domain.p1;
		bb_dest->xform(*bb_from,m);
		}break;
	case PDPlane:
		m.transform_tiny(p1,domain.p1);
		m.transform_dir(p2,domain.p2);
		// radius1 stores the d of the plane eqn.
		radius1 = -(p1 * p2);
		break;
	case PDSphere:
		m.transform_tiny(p1,domain.p1);
		break;
	case PDCylinder:
	case PDCone:
		m.transform_tiny(p1,domain.p1);
		m.transform_dir(p2,domain.p2);
		m.transform_dir(u,domain.u);
		m.transform_dir(v,domain.v);
		break;
	case PDBlob:
		m.transform_tiny(p1,domain.p1);
		break;
	case PDPoint:
		m.transform_tiny(p1,domain.p1);
		break;
	case PDLine:
		m.transform_tiny(p1,domain.p1);
		m.transform_dir(p2,domain.p2);
		break;
	case PDRectangle:
		m.transform_tiny(p1,domain.p1);
		m.transform_dir(p2,domain.p2);
		m.transform_dir(u,domain.u);
		m.transform_dir(v,domain.v);
		break;
	case PDTriangle:
		m.transform_tiny(p1,domain.p1);
		m.transform_dir(p2,domain.p2);
		m.transform_dir(u,domain.u);
		m.transform_dir(v,domain.v);
		break;
	case PDDisc:
		m.transform_tiny(p1,domain.p1);
		m.transform_dir(p2,domain.p2);
		m.transform_dir(u,domain.u);
		m.transform_dir(v,domain.v);
		break;
	default:
    	NODEFAULT;
	}
}

void pDomain::transform_dir(const pDomain& domain, const Fmatrix& m)
{
	Fmatrix M=m;
	M.c.set(0,0,0);
	transform(domain,M);
}
 
