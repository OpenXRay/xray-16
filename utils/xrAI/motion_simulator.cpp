#include "stdafx.h"
#include "cl_intersect.h"
#include "motion_simulator.h"
#include "compiler.h"

struct cl_tri 
{
	Fvector	e10; float e10s;
	Fvector	e21; float e21s;
	Fvector	e02; float e02s;
	Fvector p[3];
	Fvector N;
	float	d;
};
struct SCollisionData
{
	// data about player movement
	Fvector				vVelocity;
	Fvector				vSourcePoint;
		
	// for error handling  
	Fvector				vLastSafePosition;
	BOOL				bStuck; 
		
	// data for collision response 
	bool				bFoundCollision;
	float				fNearestDistance;					// nearest distance to hit
	Fvector				vNearestIntersectionPoint;			// on sphere
	Fvector				vNearestPolygonIntersectionPoint;	// on polygon
		
	Fvector				vRadius;
};
DEF_VECTOR(vecTris,cl_tri);

static int		psCollideActDepth		= 8;
static int		psCollideActStuckDepth	= 16;
static float	cl_epsilon				= EPS_L;
static vecTris	clContactedT;

// ----------------------------------------------------------------------
// Name  : classifyPoint()
// Input : point - point we wish to classify 
//         pO - Origin of plane
//         pN - Normal to plane 
// Notes : 
// Return: One of 3 classification codes
// -----------------------------------------------------------------------  
IC float classifyPoint(const Fvector& point, const Fvector& planeO, const Fvector& planeN)
{
	Fvector dir;
	dir.sub	(point,planeO);
	return dir.dotproduct(planeN);
}
// ----------------------------------------------------------------------
// Name  : intersectRayPlane()
// Input : rOrigin - origin of ray in world space
//         rVector - xr_vector describing direction of ray in world space
//         pOrigin - Origin of plane 
//         pNormal - Normal to plane
// Notes : Normalized directional vectors expected
// Return: distance to plane in world units, -1 if no intersection.
// -----------------------------------------------------------------------  
IC float intersectRayPlane(	const Fvector& rayOrigin,	const Fvector& rayDirection, 
							const Fvector& planeOrigin,	const Fvector& planeNormal)
{
	float numer = classifyPoint(rayOrigin,planeOrigin,planeNormal);
	float denom = planeNormal.dotproduct(rayDirection);
	
	if (denom == 0)  // normal is orthogonal to xr_vector, cant intersect
		return (-1.0f);
	
	return -(numer / denom);	
}
// ----------------------------------------------------------------------
// Name  : closestPointOnLine()
// Input : a - first end of line segment
//         b - second end of line segment
//         p - point we wish to find closest point on line from 
// Notes : Helper function for closestPointOnTriangle()
// Return: closest point on line segment
// -----------------------------------------------------------------------  

IC void closestPointOnLine(Fvector& res, const Fvector& a, const Fvector& b, const Fvector& p) 
{
	
	// Determine t (the length of the xr_vector from ‘a’ to ‘p’)
	Fvector c; c.sub(p,a);
	Fvector V; V.sub(b,a); 
	
	float d = V.magnitude();
	
	V.div(d);  
	float t = V.dotproduct(c);
	
	// Check to see if ‘t’ is beyond the extents of the line segment
	if (t <= 0.0f)	{ res.set(a); return; }
	if (t >= d)		{ res.set(b); return; }
	
	// Return the point between ‘a’ and ‘b’
	// set length of V to t. V is normalized so this is easy
	res.mad		(a,V,t);
}
IC void closestPointOnEdge(Fvector& res,						// result
						   const Fvector& a, const Fvector& b,	// points
						   const Fvector& ED, float elen,		// edge direction (b-a) and length
						   const Fvector& P)					// query point
{
	// Determine t (the length of the xr_vector from ‘a’ to ‘p’)
	Fvector c; c.sub(P,a);
	float t = ED.dotproduct(c);
	
	// Check to see if ‘t’ is beyond the extents of the line segment
	if (t <= 0.0f)	{ res.set(a); return; }
	if (t >= elen)	{ res.set(b); return; }
	
	// Return the point between ‘a’ and ‘b’
	res.mad(a,ED,t);
}

// ----------------------------------------------------------------------
// Name  : closestPointOnTriangle()
//         p - point we wish to find closest point on triangle from 
// Return: closest point on line triangle edge
// -----------------------------------------------------------------------  

IC void closestPointOnTriangle(Fvector& result, const Fvector* V, const Fvector& p) {
	Fvector Rab; closestPointOnLine(Rab, V[0], V[1], p); float dAB = p.distance_to_sqr(Rab);
	Fvector Rbc; closestPointOnLine(Rbc, V[1], V[2], p); float dBC = p.distance_to_sqr(Rbc);
	Fvector Rca; closestPointOnLine(Rca, V[2], V[0], p); float dCA = p.distance_to_sqr(Rca);
	
	float min = dAB;
	result.set(Rab);
	if (dBC < min) {
		min = dBC;
		result.set(Rbc);
    }
	if (dCA < min)
		result.set(Rca);
}
IC void closestPointOnTriangle(Fvector& result, const cl_tri& T, const Fvector& P) {
	Fvector Rab; closestPointOnEdge(Rab, T.p[0], T.p[1], T.e10, T.e10s, P); float dAB = P.distance_to_sqr(Rab);
	Fvector Rbc; closestPointOnEdge(Rbc, T.p[1], T.p[2], T.e21, T.e21s, P); float dBC = P.distance_to_sqr(Rbc);
	Fvector Rca; closestPointOnEdge(Rca, T.p[2], T.p[0], T.e02, T.e02s, P); float dCA = P.distance_to_sqr(Rca);
	
	float min;
	if (dBC < dAB)	{ min = dBC; result.set(Rbc); } 
	else			{ min = dAB; result.set(Rab); }
	if (dCA < min)	result.set(Rca);
}

// ----------------------------------------------------------------------
// Name  : intersectRaySphere()
// Input : rO - origin of ray in world space
//         rV - xr_vector describing direction of ray in world space
//         sO - Origin of sphere 
//         sR - radius of sphere
// Notes : Normalized directional vectors expected
// Return: distance to sphere in world units, -1 if no intersection.
// -----------------------------------------------------------------------  

IC float intersectRaySphere(const Fvector& rO, const Fvector& rV, const Fvector& sO, float sR) {
	Fvector Q;
	Q.sub(sO,rO);
	
	float c = Q.magnitude();
	float v = Q.dotproduct(rV);
	float d = sR*sR - (c*c - v*v);
	
	// If there was no intersection, return -1
	if (d < 0.0) return (-1.0f);
	
	// Return the distance to the [first] intersecting point
	return (v - _sqrt(d));
}
IC float intersectRayIdentitySphere(const Fvector& rO, const Fvector& rV) 
{
	Fvector Q;
	Q.invert(rO);
	
	float c = Q.magnitude();
	float v = Q.dotproduct(rV);
	float d = 1 - (c*c - v*v);
	
	// If there was no intersection, return -1
	if (d < 0.0) return (-1.0f);
	
	// Return the distance to the [first] intersecting point
	return (v - _sqrt(d));
}
// ----------------------------------------------------------------------
// Name  : CheckPointInSphere()
// Input : point - point we wish to check for inclusion
//         sO - Origin of sphere
//         sR - radius of sphere 
// Notes : 
// Return: TRUE if point is in sphere, FALSE if not.
// -----------------------------------------------------------------------  
IC bool CheckPointInSphere(const Fvector& point, const Fvector& sO, float sR) {
	return (sO.distance_to_sqr(point)< sR*sR);
}
IC bool CheckPointInIdentitySphere(const Fvector& point) { 
	return (point.square_magnitude() <= 1);
}
// -----------------------------------------------------------------------  
void msimulator_CheckCollision(SCollisionData& cl) 
{
	// from package
	Fvector			source;
	Fvector			velocity;
	source.set		(cl.vSourcePoint);
	velocity.set	(cl.vVelocity);
	
	// keep a copy of this as it's needed a few times
	Fvector			normalizedVelocity;
	normalizedVelocity.normalize_safe(cl.vVelocity);
	
	// intersection data
	Fvector			sIPoint;    // sphere intersection point
	Fvector			pIPoint;    // plane intersection point 	
	Fvector			polyIPoint; // polygon intersection point
	
	// how long is our velocity
	float			distanceToTravel = velocity.magnitude();	
	
	float			distToPlaneIntersection;
	float			distToEllipsoidIntersection;
	
	for (u32 i_t=0; i_t!=clContactedT.size(); i_t++){
		cl_tri& T=clContactedT[i_t];
		
		//ignore backfaces. What we cannot see we cannot collide with ;)
		if (T.N.dotproduct(normalizedVelocity) < 0.0f)
		{
			// calculate sphere intersection point (in future :)
			// OLES: 'cause our radius has unit length, this point lies exactly on sphere
			sIPoint.sub(source, T.N);
			
			// find the plane intersection point
			// classify point to determine if ellipsoid span the plane
			BOOL bInsideTri;
			if ((sIPoint.dotproduct(T.N)+T.d) < -EPS_S) 
			{ 
				// plane is embedded in ellipsoid / sphere
				// find plane intersection point by shooting a ray from the 
				// sphere intersection point along the planes normal.
				bInsideTri = CDB::TestRayTri2(sIPoint,T.N,T.p,distToPlaneIntersection);
				
				// calculate plane intersection point
				pIPoint.mad(sIPoint,T.N,distToPlaneIntersection);
			}
			else
			{ 
				// shoot ray along the velocity xr_vector
				bInsideTri = CDB::TestRayTri2(sIPoint,normalizedVelocity,T.p,distToPlaneIntersection);
				
				// calculate plane intersection point
				pIPoint.mad(sIPoint,normalizedVelocity,distToPlaneIntersection);
			}
			
			
			// find polygon intersection point. By default we assume its equal to the 
			// plane intersection point
			
			polyIPoint.set(pIPoint);
			distToEllipsoidIntersection = distToPlaneIntersection;
			
			if (!bInsideTri) 
			{ 
				// if not in triangle
				closestPointOnTriangle(polyIPoint, T, pIPoint);	
				
				Fvector _normalizedVelocity;
				_normalizedVelocity.invert(normalizedVelocity);
				distToEllipsoidIntersection = intersectRaySphere(polyIPoint, _normalizedVelocity, source, 1.0f);  
				
				if (distToEllipsoidIntersection >= 0){
					// calculate true sphere intersection point
					sIPoint.mad(polyIPoint, normalizedVelocity, -distToEllipsoidIntersection);
				}
			} 
			
			// Here we do the error checking to see if we got ourself stuck last frame
			if (CheckPointInSphere(polyIPoint, source, 1.0f)) 
			{
				cl.bStuck = TRUE;
			}
			
			// Ok, now we might update the collision data if we hit something
			if ((distToEllipsoidIntersection >= 0) && (distToEllipsoidIntersection <= distanceToTravel)) 
			{ 
				if ((cl.bFoundCollision == FALSE) || (distToEllipsoidIntersection < cl.fNearestDistance))  
				{
					// if we are hit we have a closest hit so far. We save the information
					cl.fNearestDistance = distToEllipsoidIntersection;
					cl.vNearestIntersectionPoint.set(sIPoint);
					cl.vNearestPolygonIntersectionPoint.set(polyIPoint);
					cl.bFoundCollision = TRUE;
				}
			} 
		} // if not backface
	} // for all faces	
}
//-----------------------------------------------------------------------------
void msimulator_ResolveStuck(SCollisionData& cl, Fvector& position)
{
	// intersection data
	Fvector			polyIPoint;		// polygon intersection point
	Fvector			stuckDir;
	int				stuckCount;
	
	float			dist;
	float			safe_R = 1.f + EPS_L*2;//psSqueezeVelocity*Device.fTimeDelta;
	
	for (int passes=0; passes<psCollideActStuckDepth; passes++)
	{
		// initialize
		stuckDir.set	(0,0,0);
		stuckCount		= 0;
		
		// for all faces
		for (u32 i_t=0; i_t!=clContactedT.size(); i_t++)
		{
			cl_tri& T=clContactedT[i_t];
			Fvector N_inv;
			N_inv.invert(T.N);
			
			// find plane intersection point by shooting a ray from the 
			// sphere intersection point along the planes normal.
			if (CDB::TestRayTri2(position,N_inv,T.p,dist)){
				// calculate plane intersection point
				polyIPoint.mad(position,N_inv,dist);
			}else{
				// calculate plane intersection point
				Fvector tmp;
				tmp.mad(position,N_inv,dist);
				closestPointOnTriangle(polyIPoint, T, tmp);	
			}
			if (CheckPointInSphere(polyIPoint, position, safe_R)) 
			{
				Fvector dir;
				dir.sub(position,polyIPoint);
				float len = dir.magnitude();
				dir.mul( (safe_R-len)/len );
				stuckDir.add(dir);
				stuckCount++;
			}
		}

		if (stuckCount){
			stuckDir.div(float(stuckCount));
			position.add(stuckDir);
			if (stuckDir.magnitude()<EPS) break;
		} else break;
	}
}

//-----------------------------------------------------------------------------
// Name: collideWithWorld()
// Desc: Recursive part of the collision response. This function is the
//       one who actually calls the collision check on the meshes
//-----------------------------------------------------------------------------
Fvector msimulator_CollideWithWorld(SCollisionData& cl, Fvector position, Fvector velocity, WORD cnt) 
{
	// 
	msimulator_ResolveStuck(cl,position);

	// do we need to worry ?
//	if (fsimilar(position.x,target.x,EPS_L)&&fsimilar(position.z,target.z,EPS_L))
	if (velocity.magnitude()<EPS_L)
	{
		cl.vVelocity.set(0,0,0);
		return position;
	}
	if (cnt>psCollideActDepth)				return cl.vLastSafePosition;
	
	Fvector ret_pos;
	Fvector destinationPoint;
	destinationPoint.add(position,velocity);
	
	// reset the collision package we send to the mesh 
	cl.vVelocity.set	(velocity);
	cl.vSourcePoint.set	(position);
	cl.bFoundCollision	= FALSE;
	cl.bStuck			= FALSE;
	cl.fNearestDistance	= -1;	
	
	// Check collision
	msimulator_CheckCollision	(cl);	
	
	// check return value here, and possibly call recursively 	
	if (cl.bFoundCollision == FALSE  && !cl.bStuck)
	{ 
		// if no collision move very close to the desired destination. 
		float l = velocity.magnitude();
		Fvector V;
		V.mul(velocity,(l-cl_epsilon)/l); 
		
		// return the final position
		ret_pos.add(position,V);
		
		// update the last safe position for future error recovery	
		cl.vLastSafePosition.set(ret_pos);
		return ret_pos;
	}else{ 
		// There was a collision
		// OK, first task is to move close to where we hit something :
		Fvector newSourcePoint;
		
		// If we are stuck, we just back up to last safe position
		if (cl.bStuck){
			return cl.vLastSafePosition;
		}

		// only update if we are not already very close
		if (cl.fNearestDistance >= cl_epsilon) {
			Fvector V;
			V.set(velocity);
			V.set_length(cl.fNearestDistance-cl_epsilon);
			newSourcePoint.add(cl.vSourcePoint,V);
		}else {
			newSourcePoint.set(cl.vSourcePoint);
		}
		
		// Now we must calculate the sliding plane
		Fvector slidePlaneOrigin; slidePlaneOrigin.set(cl.vNearestPolygonIntersectionPoint);
		Fvector slidePlaneNormal; slidePlaneNormal.sub(newSourcePoint,cl.vNearestPolygonIntersectionPoint);
		
		// We now project the destination point onto the sliding plane
		float l = intersectRayPlane(destinationPoint, slidePlaneNormal, slidePlaneOrigin, slidePlaneNormal); 

		// We can now calculate a _new destination point on the sliding plane
		Fvector newDestinationPoint;
		newDestinationPoint.mad(destinationPoint,slidePlaneNormal,l);
		
		// Generate the slide xr_vector, which will become our _new velocity xr_vector
		// for the next iteration
		Fvector newVelocityVector;
		newVelocityVector.sub(newDestinationPoint, cl.vNearestPolygonIntersectionPoint);
		
		// now we recursively call the function with the _new position and velocity 
		cl.vLastSafePosition.set(position);
		return msimulator_CollideWithWorld(cl, newSourcePoint, newVelocityVector,cnt+1); 
	}
}

IC void create_bb(Fbox& B, Fvector& P, float r, float h)
{
	B.set(P.x-r, P.y, P.z-r, P.x+r, P.y+h, P.z+r);
}
void msimulator_Simulate( Fvector& result, Fvector& start, Fvector& end, float _radius, float _height)
{
	SCollisionData	cl_data;
	float			half_height	= _height/2;

	// Calc BB
	Fbox			b1,b2,bb;
	create_bb		(b1,start,	_radius,_height);
	create_bb		(b2,end,	_radius,_height);
	bb.merge		(b1,b2);
	bb.grow			(0.05f);

	// Collision query
	Fvector			bbC,bbD;
	bb.get_CD		(bbC,bbD);
	XRC.box_options	(0);
	XRC.box_query	(&Level,bbC,bbD);
	
	// XForm everything to ellipsoid space
	Fvector			xf;
	xf.set			(1/_radius,1/half_height,1/_radius);

	Fvector			Lposition;
	Lposition.set	(start);
	Lposition.y		+= half_height;
	Lposition.mul	(xf);

	Fvector			target;
	target.set		(end);
	target.y		+= half_height;
	target.mul		(xf);

	Fvector			Lvelocity;
	Lvelocity.sub	(end,start);
	Lvelocity.mul	(xf);

	cl_data.vLastSafePosition.set	(Lposition);

	// Get the data for the triangles in question and scale to ellipsoid space
	int tri_count			= XRC.r_count();
	clContactedT.resize		(tri_count);
	if (tri_count) {
		Fvector vel_dir;
		vel_dir.normalize_safe	(Lvelocity);
		for (int i_t=0; i_t<tri_count; i_t++){
			cl_tri& T			= clContactedT[i_t];
			CDB::RESULT&		rp = XRC.r_begin()[i_t];
//			CDB::TRI&	O		= 
				*(Level.get_tris()+rp.id);

			T.p[0].mul			(rp.verts[0],xf);
			T.p[1].mul			(rp.verts[1],xf);
			T.p[2].mul			(rp.verts[2],xf);
			T.N.mknormal		(T.p[0],T.p[1],T.p[2]);
			
			T.d = -T.N.dotproduct(T.p[0]);
			T.e10.sub(T.p[1],T.p[0]); T.e10s = T.e10.magnitude(); T.e10.div(T.e10s);
			T.e21.sub(T.p[2],T.p[1]); T.e21s = T.e21.magnitude(); T.e21.div(T.e21s);
			T.e02.sub(T.p[0],T.p[2]); T.e02s = T.e02.magnitude(); T.e02.div(T.e02s);
		}
	}

	// call the recursive collision response function	
	Fvector POS;

	for (int i=0; i<3; i++) {
		POS.set(msimulator_CollideWithWorld(cl_data, Lposition, Lvelocity, 0));
		if (fsimilar(POS.x,target.x)&&fsimilar(POS.z,target.z)) break;

		Lposition.set	(POS);
		Lvelocity.sub	(target,POS);
		Lvelocity.y		= 0;
	}

	result.div(POS,xf);
	result.y-=half_height;
}
