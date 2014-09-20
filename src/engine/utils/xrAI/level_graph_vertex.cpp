////////////////////////////////////////////////////////////////////////////
//	Module 		: level_graph_vertex.cpp
//	Created 	: 02.10.2001
//  Modified 	: 11.11.2003
//	Author		: Oles Shihkovtsov, Dmitriy Iassenev
//	Description : Level graph vertex functions
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "level_graph.h"
#include "game_level_cross_table.h"

#ifndef AI_COMPILER
#	include "ai_space.h"
#endif

float CLevelGraph::distance(const Fvector &position, const CLevelGraph::CVertex *vertex) const
{
	SContour				_contour;
	contour					(_contour,vertex);

	// calculate minimal distance
	float					best,dist;
	best					= distance(position,_contour.v1,_contour.v2);
	dist					= distance(position,_contour.v2,_contour.v3);
	if (dist < best)
		best				= dist;

	dist					= distance(position,_contour.v3,_contour.v4);
	if (dist < best)
		best				= dist;

	dist					= distance(position,_contour.v4,_contour.v1);
	if (dist < best)
		best				= dist;

	return					(best);
}

void CLevelGraph::choose_point(const Fvector &start_point, const Fvector &finish_point, const SContour &_contour, int node_id, Fvector &temp_point, int &saved_index) const
{
	SContour					tNextContour;
	SSegment					tNextSegment;
	Fvector						tCheckPoint1 = start_point, tCheckPoint2 = start_point, tIntersectPoint;
	contour						(tNextContour,node_id);
	intersect					(tNextSegment,tNextContour,_contour);
	u32							dwIntersect = intersect(start_point.x,start_point.z,finish_point.x,finish_point.z,tNextSegment.v1.x,tNextSegment.v1.z,tNextSegment.v2.x,tNextSegment.v2.z,&tIntersectPoint.x,&tIntersectPoint.z);
	if (!dwIntersect)
		return;
	for (int i=0; i<4; ++i) {
		switch (i) {
			case 0 : {
				tCheckPoint1	= tNextContour.v1;
				tCheckPoint2	= tNextContour.v2;
				break;
					 }
			case 1 : {
				tCheckPoint1	= tNextContour.v2;
				tCheckPoint2	= tNextContour.v3;
				break;
					 }
			case 2 : {
				tCheckPoint1	= tNextContour.v3;
				tCheckPoint2	= tNextContour.v4;
				break;
					 }
			case 3 : {
				tCheckPoint1	= tNextContour.v4;
				tCheckPoint2	= tNextContour.v1;
				break;
					 }
			default : NODEFAULT;
		}
		dwIntersect				= intersect(start_point.x,start_point.z,finish_point.x,finish_point.z,tCheckPoint1.x,tCheckPoint1.z,tCheckPoint2.x,tCheckPoint2.z,&tIntersectPoint.x,&tIntersectPoint.z);
		if (dwIntersect == eLineIntersectionIntersect) {
			if (finish_point.distance_to_xz(tIntersectPoint) < finish_point.distance_to_xz(temp_point) + EPS_L) {
				temp_point = tIntersectPoint;
				saved_index = node_id;
			}
		}
		else
			if (dwIntersect == eLineIntersectionEqual) {
				if (start_point.distance_to_xz(tCheckPoint1) > start_point.distance_to_xz(temp_point))
					if (start_point.distance_to_xz(tCheckPoint1) > start_point.distance_to_xz(tCheckPoint2)) {
						temp_point = tCheckPoint1;
						saved_index = node_id;
					}
					else {
						temp_point = tCheckPoint2;
						saved_index = node_id;
					}
				else
					if (start_point.distance_to_xz(tCheckPoint2) > start_point.distance_to_xz(temp_point)) {
						temp_point = tCheckPoint2;
						saved_index = node_id;
					}

			}
	}
}

float CLevelGraph::check_position_in_direction(u32 start_vertex_id, const Fvector &start_position, const Fvector &finish_position, const float max_distance) const
{
	SContour				_contour;
	const_iterator			I,E;
	int						saved_index, iPrevIndex = -1, iNextNode;
	Fvector					start_point = start_position, temp_point = start_position, finish_point = finish_position;
	float					fCurDistance = 0.f, fDistance = start_position.distance_to_xz(finish_position);
	u32						dwCurNode = start_vertex_id;

	while (!inside(vertex(dwCurNode),finish_position) && (fCurDistance < (fDistance + EPS_L))) {
		begin				(dwCurNode,I,E);
		saved_index			= -1;
		contour				(_contour,dwCurNode);
		for ( ; I != E; ++I) {
			iNextNode = value(dwCurNode,I);
			if (valid_vertex_id(iNextNode) && (iPrevIndex != iNextNode))
				choose_point(start_point,finish_point,_contour, iNextNode,temp_point,saved_index);
		}

		if (saved_index > -1) {
			fCurDistance	= start_point.distance_to_xz(temp_point);
			iPrevIndex		= dwCurNode;
			dwCurNode		= saved_index;
		}
		else
			return			(max_distance);
	}

	if (inside(vertex(dwCurNode),finish_position) && (_abs(vertex_plane_y(*vertex(dwCurNode),finish_position.x,finish_position.z) - finish_position.y) < .5f))
		return				(start_point.distance_to_xz(finish_position));
	else
		return				(max_distance);
}

float CLevelGraph::mark_nodes_in_direction(u32 start_vertex_id, const Fvector &start_position, const Fvector &tDirection, float fDistance, xr_vector<u32> &tpaStack, xr_vector<bool> *tpaMarks) const
{
	Fvector					finish_point, direction = tDirection;
	direction.normalize		();
	finish_point.mul		(direction,fDistance);
	finish_point.add		(start_position);
	return					(mark_nodes_in_direction(start_vertex_id,start_position,finish_point,tpaStack,tpaMarks));
}

float CLevelGraph::mark_nodes_in_direction(u32 start_vertex_id, const Fvector &start_position, u32 finish_vertex_id, xr_vector<u32> &tpaStack, xr_vector<bool> *tpaMarks) const
{
	return					(mark_nodes_in_direction(start_vertex_id,start_position,vertex_position(finish_vertex_id),tpaStack,tpaMarks));
}

float CLevelGraph::mark_nodes_in_direction(u32 start_vertex_id, const Fvector &start_point, const Fvector &finish_point, xr_vector<u32> &tpaStack, xr_vector<bool> *tpaMarks) const
{
	SContour				_contour;
	const_iterator			I,E;
	int						saved_index, iPrevIndex = -1, iNextNode;
	Fvector					temp_point = start_point;
	float					fDistance = start_point.distance_to(finish_point), fCurDistance = 0.f;
	u32						dwCurNode = start_vertex_id;

	while (!inside(vertex(dwCurNode),finish_point) && (fCurDistance < (fDistance + EPS_L))) {
		begin				(dwCurNode,I,E);
		saved_index			= -1;
		contour				(_contour,dwCurNode);
		for ( ; I != E; ++I) {
			iNextNode = value(dwCurNode,I);
			if (valid_vertex_id(iNextNode) && (iPrevIndex != iNextNode))
				choose_point(start_point,finish_point,_contour, iNextNode,temp_point,saved_index);
		}

		if (saved_index > -1) {
			fCurDistance	= start_point.distance_to_xz(temp_point);
			iPrevIndex		= dwCurNode;
			dwCurNode		= saved_index;
		}
		else
			return(fCurDistance);

		if (tpaMarks)
			(*tpaMarks)[dwCurNode]	= true;
		tpaStack.push_back	(dwCurNode);
	}

	return					(fCurDistance);
}

float CLevelGraph::farthest_vertex_in_direction(u32 start_vertex_id, const Fvector &start_point, const Fvector &finish_point, u32 &finish_vertex_id, xr_vector<bool> *tpaMarks, bool check_accessability) const
{
	SContour				_contour;
	const_iterator			I,E;
	int						saved_index, iPrevIndex = -1, iNextNode;
	Fvector					temp_point = start_point;
	float					fDistance = start_point.distance_to(finish_point), fCurDistance = 0.f;
	u32						dwCurNode = start_vertex_id;

	while (!inside(vertex(dwCurNode),finish_point) && (fCurDistance < (fDistance + EPS_L))) {
		begin				(dwCurNode,I,E);
		saved_index			= -1;
		contour				(_contour,dwCurNode);
		for ( ; I != E; ++I) {
			iNextNode		= value(dwCurNode,I);
			if (valid_vertex_id(iNextNode) && (iPrevIndex != iNextNode))
				choose_point(start_point,finish_point,_contour, iNextNode,temp_point,saved_index);
		}

		if (saved_index > -1) {
			if (check_accessability && !is_accessible(saved_index))
				return		(fCurDistance);

			fCurDistance	= start_point.distance_to_xz(temp_point);
			iPrevIndex		= dwCurNode;
			dwCurNode		= saved_index;
		}
		else
			return			(fCurDistance);

		if (tpaMarks)
			(*tpaMarks)[dwCurNode]	= true;
		finish_vertex_id	= dwCurNode;
	}
	return					(fCurDistance);
}

bool CLevelGraph::create_straight_path(u32 start_vertex_id, const Fvector &start_point, const Fvector &finish_point, xr_vector<Fvector> &tpaOutputPoints, xr_vector<u32> &tpaOutputNodes, bool bAddFirstPoint, bool bClearPath) const
{
	return					(create_straight_path(start_vertex_id,v2d(start_point),v2d(finish_point),tpaOutputPoints,tpaOutputNodes,bAddFirstPoint,bClearPath));
}

u32	 CLevelGraph::check_position_in_direction_slow	(u32 start_vertex_id, const Fvector2 &start_position, const Fvector2 &finish_position) const
{
	if (!valid_vertex_position(v3d(finish_position)))
		return				(u32(-1));

	u32						cur_vertex_id = start_vertex_id, prev_vertex_id = u32(-1);
	Fbox2					box;
	Fvector2				identity, start, dest, dir;

	identity.x = identity.y	= header().cell_size()*.5f;
	start					= start_position;
	dest					= finish_position;
	dir.sub					(dest,start);
	u32						dest_xz = vertex_position(v3d(dest)).xz();
	Fvector2				temp;
	unpack_xz				(vertex(start_vertex_id),temp.x,temp.y);

	float					cur_sqr = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
	for (;;) {
		const_iterator		I,E;
		begin				(cur_vertex_id,I,E);
		bool				found = false;
		for ( ; I != E; ++I) {
			u32				next_vertex_id = value(cur_vertex_id,I);
			if ((next_vertex_id == prev_vertex_id) || !valid_vertex_id(next_vertex_id))
				continue;
			CVertex			*v = vertex(next_vertex_id);
			unpack_xz		(v,temp.x,temp.y);
			box.min			= box.max = temp;
			box.grow		(identity);
			if (box.pick_exact(start,dir)) {

				if (dest_xz == v->position().xz()) {
					return	(is_accessible(next_vertex_id) ? next_vertex_id : u32(-1));
				}
				Fvector2		temp;
				temp.add		(box.min,box.max);
				temp.mul		(.5f);
				float			dist = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
				if (dist > cur_sqr)
					continue;
				
				if (!is_accessible(next_vertex_id))
					return		(u32(-1));

				cur_sqr			= dist;
				found			= true;
				prev_vertex_id	= cur_vertex_id;
				cur_vertex_id	= next_vertex_id;
				break;
			}
		}
		if (!found) {
			return			(u32(-1));
		}
	}
}

bool CLevelGraph::check_vertex_in_direction_slow	(u32 start_vertex_id, const Fvector2 &start_position, u32 finish_vertex_id) const
{
	Fvector					finish_position = vertex_position(finish_vertex_id);
	u32						cur_vertex_id = start_vertex_id, prev_vertex_id = u32(-1);
	Fbox2					box;
	Fvector2				identity, start, dest, dir;

	identity.x = identity.y	= header().cell_size()*.5f;
	start					= start_position;
	dest.set				(finish_position.x,finish_position.z);
	dir.sub					(dest,start);
	Fvector2				temp;
	unpack_xz				(vertex(start_vertex_id),temp.x,temp.y);

	float					cur_sqr = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
	for (;;) {
		const_iterator		I,E;
		begin				(cur_vertex_id,I,E);
		bool				found = false;
		for ( ; I != E; ++I) {
			u32				next_vertex_id = value(cur_vertex_id,I);
			if ((next_vertex_id == prev_vertex_id) || !valid_vertex_id(next_vertex_id))
				continue;
			unpack_xz		(vertex(next_vertex_id),temp.x,temp.y);
			box.min			= box.max = temp;
			box.grow		(identity);
			if (box.pick_exact(start,dir)) {
				if (next_vertex_id == finish_vertex_id) {
					return		(is_accessible(next_vertex_id));
				}
				Fvector2		temp;
				temp.add		(box.min,box.max);
				temp.mul		(.5f);
				float			dist = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
				if (dist > cur_sqr)
					continue;

				if (!is_accessible(next_vertex_id))
					return		(false);

				cur_sqr			= dist;
				found			= true;
				prev_vertex_id	= cur_vertex_id;
				cur_vertex_id	= next_vertex_id;
				break;
			}
		}
		if (!found) {
			return			(false);
		}
	}
}

IC  Fvector v3d(const Fvector2 &vector2d)
{
	return			(Fvector().set(vector2d.x,0.f,vector2d.y));
}

IC  Fvector2 v2d(const Fvector &vector3d)
{
	return			(Fvector2().set(vector3d.x,vector3d.z));
}

bool CLevelGraph::create_straight_path(u32 start_vertex_id, const Fvector2 &start_point, const Fvector2 &finish_point, xr_vector<Fvector> &tpaOutputPoints, xr_vector<u32> &tpaOutputNodes, bool bAddFirstPoint, bool bClearPath) const
{
	if (!valid_vertex_position(v3d(finish_point)))
		return				(false);

	u32						cur_vertex_id = start_vertex_id, prev_vertex_id = start_vertex_id;
	Fbox2					box;
	Fvector2				identity, start, dest, dir;

	identity.x = identity.y	= header().cell_size()*.5f;
	start					= start_point;
	dest					= finish_point;
	dir.sub					(dest,start);
	u32						dest_xz = vertex_position(v3d(dest)).xz();
	Fvector2				temp;
	Fvector					pos3d;
	unpack_xz				(vertex(start_vertex_id),temp.x,temp.y);

	if (bClearPath) {
		tpaOutputPoints.clear	();
		tpaOutputNodes.clear	();
	}
	if (bAddFirstPoint) {
		pos3d				= v3d(start_point);
		pos3d.y				= vertex_plane_y(start_vertex_id,start_point.x,start_point.y);
		tpaOutputPoints.push_back(pos3d);
		tpaOutputNodes.push_back(start_vertex_id);
	}

	float					cur_sqr = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
	for (;;) {
		const_iterator		I,E;
		begin				(cur_vertex_id,I,E);
		bool				found = false;
		for ( ; I != E; ++I) {
			u32				next_vertex_id = value(cur_vertex_id,I);
			if ((next_vertex_id == prev_vertex_id) || !valid_vertex_id(next_vertex_id))
				continue;
			CVertex			*v = vertex(next_vertex_id);
			unpack_xz		(v,temp.x,temp.y);
			box.min			= box.max = temp;
			box.grow		(identity);
			if (box.pick_exact(start,dir)) {
				Fvector2		temp;
				temp.add		(box.min,box.max);
				temp.mul		(.5f);
				float			dist = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
				if (dist > cur_sqr)
					continue;

				Fvector2		next1, next2;
#ifdef DEBUG
				next1			= next2 = Fvector2().set(0.f,0.f);
#endif
				Fvector			tIntersectPoint;

				switch (I) {
					case 0 : {
						next1		= box.max;
						next2.set	(box.max.x,box.min.y);
						break;
					}
					case 1 : {
						next1		= box.min;
						next2.set	(box.max.x,box.min.y);
						break;
					}
					case 2 : {
						next1		= box.min;
						next2.set	(box.min.x,box.max.y);
						break;
					}
					case 3 : {
						next1		= box.max;
						next2.set	(box.min.x,box.max.y);
						break;
					}
					default : NODEFAULT;
				}
				VERIFY			(_valid(next1));
				VERIFY			(_valid(next2));
				u32				dwIntersect = intersect(start_point.x,start_point.y,finish_point.x,finish_point.y,next1.x,next1.y,next2.x,next2.y,&tIntersectPoint.x,&tIntersectPoint.z);
				if (!dwIntersect)
					continue;
				tIntersectPoint.y = vertex_plane_y(vertex(cur_vertex_id),tIntersectPoint.x,tIntersectPoint.z);

				tpaOutputPoints.push_back(tIntersectPoint);
				tpaOutputNodes.push_back(cur_vertex_id);

				if (dest_xz == v->position().xz())
					return		(true);
				
				cur_sqr			= dist;
				found			= true;
				prev_vertex_id	= cur_vertex_id;
				cur_vertex_id	= next_vertex_id;
				break;
			}
		}
		if (!found)
			return				(false);
	}
}

float CLevelGraph::cover_in_direction(float fAngle, float b1, float b0, float b3, float b2) const
{
	float fResult;
	
	if (fAngle < PI_DIV_2)
		;
	else
		if (fAngle < PI) {
			fAngle	-= PI_DIV_2;
			b0		= b1;
			b1		= b2;
		}
		else
			if (fAngle < 3*PI_DIV_2) {
				fAngle -= PI;
				b0		= b2;
				b1		= b3;
			}
			else {
				fAngle -= 3*PI_DIV_2;
				b1		= b0;
				b0		= b3;
			}
	fResult = (b1 - b0)*fAngle/PI_DIV_2 + b0;
	return(fResult);
}

bool CLevelGraph::neighbour_in_direction	(const Fvector &direction, u32 start_vertex_id) const
{
	u32						cur_vertex_id = start_vertex_id, prev_vertex_id = u32(-1);
	Fbox2					box;
	Fvector2				identity, start, dest, dir;

	identity.x = identity.y	= header().cell_size()*.5f;
	start					= v2d(vertex_position(start_vertex_id));
	dir						= v2d(direction);
	dir.normalize_safe		();
	dest					= dir;
	dest.mul				(header().cell_size()*4.f);
	dest.add				(start);
	Fvector2				temp;
	unpack_xz				(vertex(start_vertex_id),temp.x,temp.y);

	float					cur_sqr = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
	const_iterator			I,E;
	begin					(cur_vertex_id,I,E);
	for ( ; I != E; ++I) {
		u32					next_vertex_id = value(cur_vertex_id,I);
		if ((next_vertex_id == prev_vertex_id) || !is_accessible(next_vertex_id))
			continue;
		unpack_xz			(vertex(next_vertex_id),temp.x,temp.y);
		box.min				= box.max = temp;
		box.grow			(identity);
		if (box.pick_exact(start,dir)) {
			Fvector2		temp;
			temp.add		(box.min,box.max);
			temp.mul		(.5f);
			float			dist = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
			if (dist > cur_sqr)
				continue;
			return			(true);
		}
	}
	return					(false);
}
