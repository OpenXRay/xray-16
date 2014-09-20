////////////////////////////////////////////////////////////////////////////
//	Module 		: level_graph_vertex_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 11.11.2003
//	Author		: Oles Shihkovtsov, Dmitriy Iassenev
//	Description : Level graph vertex inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC float CLevelGraph::distance(const Fvector &position, const Fvector &point0, const Fvector &point1) const
{
	Fvector				c, V;
	c.sub				(position,	point0);
	V.sub				(point1,	point0); 

	float				d = V.magnitude();

	V.div				(d); 
	float				t = V.dotproduct(c);

	if (t <= 0.0f)
		return			position.distance_to_sqr(point0);
	if (t >= d)
		return			position.distance_to_sqr(point1);

	Fvector				R;
	R.mad				(point0,V,t);
	return				(position.distance_to_sqr(R));
}

IC void CLevelGraph::project_point(const Fplane &plane, Fvector &point) const
{
	point.y				-= plane.classify(point)/plane.n.y;
}

IC float CLevelGraph::distance(u32 vertex_id0, u32 vertex_id1) const
{
	return				(distance(vertex(vertex_id0),vertex(vertex_id1)));
}

IC float CLevelGraph::distance(const CLevelGraph::CVertex *_node, u32 vertex_id1) const
{
	return				(distance(_node,vertex(vertex_id1)));
}

IC float CLevelGraph::distance(u32 vertex_id0, const CLevelGraph::CVertex *_node) const
{
	return				(distance(vertex(vertex_id0),_node));
}

IC float CLevelGraph::distance(const CLevelGraph::CVertex *node0, const CLevelGraph::CVertex *node1) const
{
	return				(vertex_position(node0).distance_to(vertex_position(node1)));
}

IC float CLevelGraph::distance(const Fvector &position, const u32 vertex_id) const
{
	return				(distance(position,vertex(vertex_id)));
}

IC float CLevelGraph::distance(const u32 vertex_id, const Fvector &position) const
{
	return				(distance(position,vertex_id));
}

IC float CLevelGraph::distance(const u32 vertex_id, const CPosition &position) const
{
	return				(distance(vertex_position(position),vertex_id));
}

IC float CLevelGraph::distance(const CPosition &position, const u32 vertex_id) const
{
	return				(distance(vertex_position(position),vertex_id));
}

IC CLevelGraph::ELineIntersections  CLevelGraph::intersect( 
					   float x1, float y1,		/* First line segment */
					   float x2, float y2,
					   float x3, float y3,		/* Second line segment */
					   float x4, float y4,
					   float *x, float *y      /* Output value: point of intersection */
		) const
{
	float a1, a2, b1, b2, c1, c2;	/* Coefficients of line eqns. */
	float r1, r2, r3, r4;			/* 'Sign' values */
	float denom, num;				/* Intermediate values */

	/* Compute a1, b1, c1, where line joining points 1 and 2
	* is "a1 x  +  b1 y  +  c1  =  0".
	*/

	a1 = y2 - y1;
	b1 = x1 - x2;
	c1 = x2 * y1 - x1 * y2;

	/* Compute r3 and r4.
	*/

	r3 = a1 * x3 + b1 * y3 + c1;
	r4 = a1 * x4 + b1 * y4 + c1;

	/* Check signs of r3 and r4.  If both point 3 and point 4 lie on
	* same side of line 1, the line segments do not intersect.
	*/

	if ((r3*r4 > EPS) && !fis_zero(r3,EPS_L) && !fis_zero(r4,EPS_L))     return (eLineIntersectionNone);

	/* Compute a2, b2, c2 */

	a2 = y4 - y3;
	b2 = x3 - x4;
	c2 = x4 * y3 - x3 * y4;

	/* Compute r1 and r2 */

	r1 = a2 * x1 + b2 * y1 + c2;
	r2 = a2 * x2 + b2 * y2 + c2;

	/* Check signs of r1 and r2.  If both point 1 and point 2 lie
	* on same side of second line segment, the line segments do
	* not intersect.
	*/

	if ((r1*r2 > EPS) && !fis_zero(r1,EPS_L) && !fis_zero(r2,EPS_L))     return (eLineIntersectionNone);

	// Check for equality
	if (fis_zero(r1*r2) && fis_zero(r3*r4)) return eLineIntersectionEqual;

	/* Line segments intersect: compute intersection point. 
	*/

	denom = a1 * b2 - a2 * b1;
	if (fis_zero(denom)) return (eLineIntersectionCollinear);

	num = b1 * c2 - b2 * c1;
	*x = num / denom;

	num = a2 * c1 - a1 * c2;
	*y = num / denom;

	return (eLineIntersectionIntersect);
} /* lines_intersect */

IC CLevelGraph::ELineIntersections  CLevelGraph::intersect_no_check( 
					   float x1, float y1,		/* First line segment */
					   float x2, float y2,
					   float x3, float y3,		/* Second line segment */
					   float x4, float y4,
					   float *x, float *y      /* Output value: point of intersection */
		) const
{
	float		a1, a2, b1, b2, c1, c2;	/* Coefficients of line eqns. */
	float		r1, r2, r3, r4;			/* 'Sign' values */
	float		denom, num;				/* Intermediate values */

	/* Compute a1, b1, c1, where line joining points 1 and 2
	* is "a1 x  +  b1 y  +  c1  =  0".
	*/

	a1			= y2 - y1;
	b1			= x1 - x2;
	c1			= x2 * y1 - x1 * y2;

	/* Compute r3 and r4.
	*/

	r3			= a1 * x3 + b1 * y3 + c1;
	r4			= a1 * x4 + b1 * y4 + c1;

	/* Check signs of r3 and r4.  If both point 3 and point 4 lie on
	* same side of line 1, the line segments do not intersect.
	*/

	/* Compute a2, b2, c2 */

	a2			= y4 - y3;
	b2			= x3 - x4;
	c2			= x4 * y3 - x3 * y4;

	/* Compute r1 and r2 */

	r1			= a2 * x1 + b2 * y1 + c2;
	r2			= a2 * x2 + b2 * y2 + c2;

	/* Check signs of r1 and r2.  If both point 1 and point 2 lie
	* on same side of second line segment, the line segments do
	* not intersect.
	*/

	// Check for equality
	if (fis_zero(r1*r2) && fis_zero(r3*r4)) {
		*x		= x4;
		*y		= y4;
		return	(eLineIntersectionEqual);
	}

	/* Line segments intersect: compute intersection point. 
	*/

	denom = a1 * b2 - a2 * b1;
	if (fis_zero(denom)) {
		*x		= x4;
		*y		= y4;
		return	(eLineIntersectionEqual);
	}

	num			= b1 * c2 - b2 * c1;
	*x			= num / denom;

	num			= a2 * c1 - a1 * c2;
	*y			= num / denom;

	return		(eLineIntersectionIntersect);
} /* lines_intersect */

IC bool CLevelGraph::similar(const Fvector &tPoint0, const Fvector &tPoint1) const
{
	return((_abs(tPoint0.x - tPoint1.x) < EPS_L) && (_abs(tPoint0.z - tPoint1.z) < EPS_L));
}

IC bool CLevelGraph::inside(const Fvector &tPoint, const CLevelGraph::SContour &tContour) const
{
	return((tContour.v1.x - EPS_L <= tPoint.x) && (tContour.v1.z - EPS_L <= tPoint.z) && (tContour.v3.x + EPS_L >= tPoint.x) && (tContour.v3.z + EPS_L >= tPoint.z));
}

IC void CLevelGraph::intersect(SSegment &tSegment, const SContour &tContour0, const SContour &tContour1) const
{
	bool bFound = false;

	if (inside(tContour0.v1,tContour1)) {
		tSegment.v1 = tContour0.v1;
		bFound = true;
	}

	if (inside(tContour0.v2,tContour1)) {
		if (!bFound) {
			tSegment.v1 = tContour0.v2;
			bFound = true;
		}
		else {
			tSegment.v2 = tContour0.v2;
			return;
		}
	}
	if (inside(tContour0.v3,tContour1)) {
		if (!bFound) {
			tSegment.v1 = tContour0.v3;
			bFound = true;
		}
		else {
			tSegment.v2 = tContour0.v3;
			return;
		}
	}
	if (inside(tContour0.v4,tContour1)) {
		if (!bFound) {
			tSegment.v1 = tContour0.v4;
			bFound = true;
		}
		else {
			tSegment.v2 = tContour0.v4;
			return;
		}
	}
	if (bFound) {
		if (inside(tContour1.v1,tContour0) && (!(similar(tSegment.v1,tContour1.v1)))) {
			tSegment.v2 = tContour1.v1;
			return;
		}
		if (inside(tContour1.v2,tContour0) && (!(similar(tSegment.v1,tContour1.v2)))) {
			tSegment.v2 = tContour1.v2;
			return;
		}
		if (inside(tContour1.v3,tContour0) && (!(similar(tSegment.v1,tContour1.v3)))) {
			tSegment.v2 = tContour1.v3;
			return;
		}
		if (inside(tContour1.v4,tContour0) && (!(similar(tSegment.v1,tContour1.v4)))) {
			tSegment.v2 = tContour1.v4;
			return;
		}
	}
	else {
		if (inside(tContour1.v1,tContour0)) {
			tSegment.v1 = tContour1.v1;
			bFound = true;
		}
		if (inside(tContour1.v2,tContour0)) {
			if (!bFound) {
				tSegment.v1 = tContour1.v2;
				bFound = true;
			}
			else {
				tSegment.v2 = tContour1.v2;
				return;
			}
		}
		if (inside(tContour1.v3,tContour0)) {
			if (!bFound) {
				tSegment.v1 = tContour1.v3;
				bFound = true;
			}
			else {
				tSegment.v2 = tContour1.v3;
				return;
			}
		}
		if (inside(tContour1.v4,tContour0)) {
			if (!bFound) {
				tSegment.v1 = tContour1.v4;
				bFound = true;
			}
			else {
				tSegment.v2 = tContour1.v4;
				return;
			}
		}
	}

	if (bFound) {
		tSegment.v2 = tSegment.v1;
		Log("! AI_PathNodes: segment has null length");
	}
	else
		Log("! AI_PathNodes: Can't find intersection segment");
}

IC float CLevelGraph::nearest(Fvector& Dest, const Fvector& P, const Fvector& A, const Fvector& B) const
{
	// Determine t (the length of the xr_vector from ‘a’ to ‘p’)
	Fvector c; c.sub(P,A);
	Fvector V; V.sub(B,A); 
	
	float d = V.magnitude();
	
	V.div	(d); 
	float t = V.dotproduct(c);
	
	// Check to see if ‘t’ is beyond the extents of the line segment
	if (t <= 0.0f)	{ Dest.set(A); return P.distance_to_sqr(Dest); }
	if (t >= d)		{ Dest.set(B); return P.distance_to_sqr(Dest); }
	
	// Return the point between ‘a’ and ‘b’
	// set length of V to t. V is normalized so this is easy
	Dest.mad(A,V,t);
	return P.distance_to_sqr(Dest);
}

IC void CLevelGraph::contour(CLevelGraph::SContour &_contour, u32 vertex_id) const
{
	contour					(_contour,vertex(vertex_id));
}

IC void CLevelGraph::contour(CLevelGraph::SContour &_contour, const CLevelGraph::CVertex *vertex) const
{
	Fvector					vertex_position = this->vertex_position	(vertex->p);

	// decompress plane
	Fplane					plane;	
	pvDecompress			(plane.n,vertex->plane());
	plane.d					= - plane.n.dotproduct(vertex_position);

	// create vertices
	float					st = header().cell_size()/2;

	_contour.v1.set			(vertex_position.x - st, vertex_position.y, vertex_position.z-st);	
	_contour.v2.set			(vertex_position.x + st, vertex_position.y, vertex_position.z-st);	
	_contour.v3.set			(vertex_position.x + st, vertex_position.y, vertex_position.z+st);	
	_contour.v4.set			(vertex_position.x - st, vertex_position.y, vertex_position.z+st);	

	project_point			(plane,_contour.v1);	// minX,minZ
	project_point			(plane,_contour.v2);	// maxX,minZ
	project_point			(plane,_contour.v3);	// maxX,maxZ
	project_point			(plane,_contour.v4);	// minX,maxZ
}

IC void CLevelGraph::nearest(Fvector &destination, const Fvector &position, const CLevelGraph::SContour &contour) const
{
	// calculate minimal distance
	Fvector		T;
	float		best,dist;
	
	best		= nearest(destination,position,contour.v1,contour.v2);
	
	dist		= nearest(T,position,contour.v2,contour.v3);
	if (dist<best) {
		best	= dist;
		destination.set(T);
	}
	
	dist		= nearest(T,position,contour.v3,contour.v4);
	if (dist<best) {
		best	= dist;
		destination.set(T);
	}
	
	dist		= nearest(T,position,contour.v4,contour.v1);
	if (dist<best) {
		best	= dist;
		destination.set(T);
	}
}

const float corner_r = 0.05f;

IC bool CLevelGraph::intersect(Fvector& dst, const Fvector& v1, const Fvector& v2, const Fvector& v3, const Fvector& v4) const
{
	// corner check (v4 - end, v1-v2 - segm)
	if (v4.similar(v1,corner_r)) {
		dst.set	(v1);
		return	(true);
	}
	
	if (v4.similar(v2,corner_r)){
		dst.set	(v2);
		return	(true);
	}
	
	if (v3.similar(v1,corner_r)){
		dst.set	(v1);
		return	(true);
	}
	
	if (v3.similar(v2,corner_r)){
		dst.set	(v2);
		return	(true);
	}

	// projected intersection
	Fvector		T;
	if (eLineIntersectionIntersect != 
		intersect(v1.x,v1.z,v2.x,v2.z,v3.x,v3.z,v4.x,v4.z,&T.x,&T.z))
		return (false);
	
	// unproject into 3D
	float		lx	= (v2.x-v1.x);
	float		lz	= (v2.z-v1.z);
	float		bary1 = (T.x-v1.x)/lx;
	float		bary2 = (T.z-v1.z)/lz;

	if (fis_zero(lx,EPS_L))
		bary1	= bary2;

	if (fis_zero(lz,EPS_L))
		bary2	= bary1;

	float		bary = (bary1+bary2)/2;
	dst.x		= T.x;
	dst.y		= v1.y + bary*(v2.y-v1.y);
	dst.z		= T.z;
	
	return		(true);
}

IC	float CLevelGraph::square(float a1, float b1, float fAlpha) const
{
	float a = 2*(b1 - a1)/PI, b = a1;	
	return(fAlpha*fAlpha*fAlpha*a*a/6 + fAlpha*fAlpha*a*b/2 + fAlpha*b*b/2);
}

#define NORMALIZE_NODE_COVER_HIGH(a,b) (float(a->high_cover(b))/15.f)
#define NORMALIZE_NODE_COVER_LOW(a,b)  (float(a->low_cover(b))/15.f)

IC	float CLevelGraph::compute_square(float fAngle, float fAngleOfView, float b1, float b0, float b3, float b2) const
{
	fAngle				= angle_normalize(fAngle - 0*PI_DIV_2);

	if (fAngle < PI_DIV_2)
		;
	else
		if (fAngle < PI) {
			fAngle		-= PI_DIV_2;
			b3			= b0;
			b0			= b1;
			b1			= b2;
		}
		else
			if (fAngle < 3*PI_DIV_2) {
				fAngle -= PI;
				b0		= b2;
				float	bx = b1;
				b1		= b3;
				b3		= bx;
			}
			else {
				fAngle -= 3*PI_DIV_2;
				b1		= b0;
				b0		= b3;
				b3		= b2;
			}

	float				fSquare;
	
	if (fAngle + fAngleOfView >= PI_DIV_2) {
		fSquare			= square(b1,b2,fAngleOfView + fAngle - PI_DIV_2);
		if (fAngle - fAngleOfView < 0) {
			fSquare		+= square(b0,b1);
			fSquare		+= square(b0,b3,fAngleOfView - fAngle);
		}
		else
			fSquare		+= square(b1,b0,PI_DIV_2 - (fAngle - fAngleOfView));
	}
	else {
		fSquare			= square(b0,b1,fAngle + fAngleOfView);
		if (fAngle - fAngleOfView < 0)
			fSquare		+= square(b0,b3,fAngleOfView - fAngle);
		else
			fSquare		-= square(b0,b1,fAngle - fAngleOfView);
	}

	return				(fSquare);
}

IC	float CLevelGraph::compute_high_square(float fAngle, float fAngleOfView, const CLevelGraph::CVertex *vertex) const
{
	return(
		compute_square(
			fAngle, 
			fAngleOfView,
			NORMALIZE_NODE_COVER_HIGH(vertex,0),
			NORMALIZE_NODE_COVER_HIGH(vertex,1),
			NORMALIZE_NODE_COVER_HIGH(vertex,2),
			NORMALIZE_NODE_COVER_HIGH(vertex,3)
		)
	);
}

IC	float CLevelGraph::compute_low_square(float fAngle, float fAngleOfView, const CLevelGraph::CVertex *vertex) const
{
	return(
		compute_square(
			fAngle, 
			fAngleOfView,
			NORMALIZE_NODE_COVER_LOW(vertex,0),
			NORMALIZE_NODE_COVER_LOW(vertex,1),
			NORMALIZE_NODE_COVER_LOW(vertex,2),
			NORMALIZE_NODE_COVER_LOW(vertex,3)
		)
	);
}

IC	float CLevelGraph::compute_high_square(float fAngle, float fAngleOfView, u32 dwNodeID) const
{
	return(compute_high_square(fAngle, fAngleOfView,vertex(dwNodeID)));
}

IC	float CLevelGraph::compute_low_square(float fAngle, float fAngleOfView, u32 dwNodeID) const
{
	return(compute_low_square(fAngle, fAngleOfView,vertex(dwNodeID)));
}

IC	float CLevelGraph::vertex_high_cover(const CLevelGraph::CVertex *vertex) const
{
	float			_cover = 0.f;
	_cover			+= square(NORMALIZE_NODE_COVER_HIGH(vertex,0),NORMALIZE_NODE_COVER_HIGH(vertex,1));
	_cover			+= square(NORMALIZE_NODE_COVER_HIGH(vertex,1),NORMALIZE_NODE_COVER_HIGH(vertex,2));
	_cover			+= square(NORMALIZE_NODE_COVER_HIGH(vertex,2),NORMALIZE_NODE_COVER_HIGH(vertex,3));
	_cover			+= square(NORMALIZE_NODE_COVER_HIGH(vertex,3),NORMALIZE_NODE_COVER_HIGH(vertex,0));
	return			(_cover);
}

IC	float CLevelGraph::vertex_low_cover(const CLevelGraph::CVertex *vertex) const
{
	float			_cover = 0.f;
	_cover			+= square(NORMALIZE_NODE_COVER_LOW(vertex,0),NORMALIZE_NODE_COVER_LOW(vertex,1));
	_cover			+= square(NORMALIZE_NODE_COVER_LOW(vertex,1),NORMALIZE_NODE_COVER_LOW(vertex,2));
	_cover			+= square(NORMALIZE_NODE_COVER_LOW(vertex,2),NORMALIZE_NODE_COVER_LOW(vertex,3));
	_cover			+= square(NORMALIZE_NODE_COVER_LOW(vertex,3),NORMALIZE_NODE_COVER_LOW(vertex,0));
	return			(_cover);
}

IC	float CLevelGraph::vertex_high_cover(const u32 vertex_id) const
{
	return			(vertex_high_cover(vertex(vertex_id)));
}

IC	float CLevelGraph::vertex_low_cover(const u32 vertex_id) const
{
	return			(vertex_low_cover(vertex(vertex_id)));
}

IC	float CLevelGraph::high_cover_in_direction(float angle, const CLevelGraph::CVertex *vertex) const
{
	return				(
		cover_in_direction(
			angle,
			NORMALIZE_NODE_COVER_HIGH(vertex,0),
			NORMALIZE_NODE_COVER_HIGH(vertex,1),
			NORMALIZE_NODE_COVER_HIGH(vertex,2),
			NORMALIZE_NODE_COVER_HIGH(vertex,3)
		)
	);
}

IC	float CLevelGraph::low_cover_in_direction(float angle, const CLevelGraph::CVertex *vertex) const
{
	return				(
		cover_in_direction(
			angle,
			NORMALIZE_NODE_COVER_LOW(vertex,0),
			NORMALIZE_NODE_COVER_LOW(vertex,1),
			NORMALIZE_NODE_COVER_LOW(vertex,2),
			NORMALIZE_NODE_COVER_LOW(vertex,3)
		)
	);
}

#undef NORMALIZE_NODE_COVER_HIGH
#undef NORMALIZE_NODE_COVER_LOW

IC	float CLevelGraph::high_cover_in_direction(float angle, u32 vertex_id) const
{
	return				(high_cover_in_direction(angle, vertex(vertex_id)));
}

IC	float CLevelGraph::low_cover_in_direction(float angle, u32 vertex_id) const
{
	return				(low_cover_in_direction(angle, vertex(vertex_id)));
}

IC	u32	 CLevelGraph::check_position_in_direction	(u32 start_vertex_id, const Fvector2 &start_position, const Fvector2 &finish_position) const
{
	if (inside(start_vertex_id,finish_position))
		return				(start_vertex_id);
	return					(check_position_in_direction_slow(start_vertex_id,start_position,finish_position));
}

IC	bool CLevelGraph::check_vertex_in_direction		(u32 start_vertex_id, const Fvector2 &start_position, u32 finish_vertex_id) const
{
	if (start_vertex_id == finish_vertex_id)
		return				(true);
	return					(check_vertex_in_direction_slow(start_vertex_id,start_position,finish_vertex_id));
}

IC	u32 CLevelGraph::check_position_in_direction(u32 start_vertex_id, const Fvector &start_position, const Fvector &finish_position) const
{
	return					(check_position_in_direction(start_vertex_id,Fvector2().set(start_position.x,start_position.z),Fvector2().set(finish_position.x,finish_position.z)));
}

IC	bool CLevelGraph::check_vertex_in_direction(u32 start_vertex_id, const Fvector &start_position, u32 finish_vertex_id) const
{
	return					(check_vertex_in_direction(start_vertex_id,Fvector2().set(start_position.x,start_position.z),finish_vertex_id));
}

template <class _predicate>
float CLevelGraph::vertex_cover_angle(u32 vertex_id, float inc_angle, _predicate compare_predicate) const
{
	float best_angle	= 0.f;
	float best_value	= compute_square(best_angle, PI_DIV_2, vertex_id);

	for (float angle = inc_angle; angle <= PI_MUL_2; angle += inc_angle) {
		float cover = compute_square(angle, PI_DIV_2, vertex_id);
		if (compare_predicate(cover,best_value)) {
			best_value = cover;
			best_angle = angle;
		}
	}

	return best_angle;
}

