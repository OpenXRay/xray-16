////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_obstacle.cpp
//	Created 	: 02.04.2007
//  Modified 	: 06.04.2007
//	Author		: Dmitriy Iassenev
//	Description : ai obstacle class inline functions
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_obstacle.h"
#include "ai_space.h"
#include "level_graph.h"
#include "GameObject.h"
#include "../Include/xrRender/Kinematics.h"
#include <boost/crc.hpp>

#include "../xrEngine/bone.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

const bool use_additional_radius	= true;

static const Fvector		local_points[8] = {
	Fvector().set(-1.f,-1.f,-1.f),
	Fvector().set(-1.f,-1.f,+1.f),
	Fvector().set(-1.f,+1.f,+1.f),
	Fvector().set(-1.f,+1.f,-1.f),
	Fvector().set(+1.f,+1.f,+1.f),
	Fvector().set(+1.f,+1.f,-1.f),
	Fvector().set(+1.f,-1.f,+1.f),
	Fvector().set(+1.f,-1.f,-1.f)
};

extern MagicBox3 MagicMinBox (int iQuantity, const Fvector* akPoint);

struct merge_predicate {
public:
	typedef moving_objects::AREA	AREA;

public:
	ai_obstacle					*m_object;
	AREA						*m_area;
	const CLevelGraph			*m_level_graph;

	IC			merge_predicate	(ai_obstacle *object, AREA &area)
	{
		m_object				= object;
		m_area					= &area;
		m_level_graph			= &ai().level_graph();
	}

	IC	void	operator()		(const CLevelGraph::CVertex &vertex) const
	{
		u32						vertex_id = m_level_graph->vertex_id(&vertex);
		if (!m_object->inside	(vertex_id))
			return;

		m_area->push_back		(vertex_id);
	}
};

IC	Fvector construct_position	(u32 level_vertex_id, float x, float z)
{
	return						(Fvector().set(x,ai().level_graph().vertex_plane_y(level_vertex_id,x,z),z));
}

IC	bool ai_obstacle::inside	(const Fvector &position, const float &radius) const
{
	for (u32 i=0; i<PLANE_COUNT; ++i) {
		if (m_box.m_planes[i].classify(position) > radius)
			return				(false);
	}
	return						(true);
}

IC	bool ai_obstacle::inside	(const Fvector &position, const float &radius, const float &increment, const u32 step_count) const
{
	Fvector						temp = position;
	for (u32 i=0; i<step_count; ++i, temp.y+=increment) {
		if (inside(temp,radius))
			return				(true);
	}
	return						(false);
}

IC	bool ai_obstacle::inside	(const u32 &vertex_id) const
{
	const Fvector				&position = ai().level_graph().vertex_position(vertex_id);
	float						offset = ai().level_graph().header().cell_size()*.5f - EPS_L;
	return						(
		inside(construct_position(vertex_id,position.x + offset,position.z + offset),EPS_L,.3f,6) || 
		inside(construct_position(vertex_id,position.x + offset,position.z - offset),EPS_L,.3f,6) ||
		inside(construct_position(vertex_id,position.x - offset,position.z + offset),EPS_L,.3f,6) || 
		inside(construct_position(vertex_id,position.x - offset,position.z - offset),EPS_L,.3f,6) ||
		inside(Fvector().set(position.x,position.y,position.z),EPS_L,.3f,6)
	);
}

void ai_obstacle::compute_matrix(Fmatrix &result, const Fvector &additional)
{
	IKinematics					*kinematics = smart_cast<IKinematics*>(m_object->Visual());
	VERIFY						(kinematics);
	u16							bone_count = kinematics->LL_BoneCount();
	VERIFY						(bone_count);
	u16							visible_bone_count = kinematics->LL_VisibleBoneCount();
	if (!visible_bone_count) {
		result.scale			(0.f,0.f,0.f);
		return;
	}

	Fvector						last_half_size = Fvector().set(flt_max,flt_max,flt_max);
	Fmatrix						before_scale_matrix	= Fidentity;
	Fmatrix						xform = m_object->XFORM();
	Fvector						*points = (Fvector*)_alloca(visible_bone_count*8*sizeof(Fvector));
	Fvector						*I = points;
	for (u16 i=0; i<bone_count; ++i) {
		if (!kinematics->LL_GetBoneVisible(i))
			continue;
		
		const Fobb				&obb = kinematics->LL_GetData(i).obb;
		if (fis_zero(obb.m_halfsize.square_magnitude())) {
			VERIFY				(visible_bone_count > 1);
			--visible_bone_count;
			continue;
		}

		Fmatrix					Mbox;
		obb.xform_get			(Mbox);

		const Fmatrix			&Mbone = kinematics->LL_GetBoneInstance(i).mTransform;
		Fmatrix					X;
		result.mul_43			(xform,X.mul_43(Mbone,Mbox));

		before_scale_matrix		= result;
		last_half_size			= obb.m_halfsize;
		result.mulB_43			(Fmatrix().scale(obb.m_halfsize));

		for (u32 i=0; i<8; ++i, ++I)
			result.transform_tiny	(*I,local_points[i]);
	}

	VERIFY						(visible_bone_count);
	if (visible_bone_count == 1) {
		result.mul_43			(before_scale_matrix,Fmatrix().scale(last_half_size.add(additional)));
		return;
	}

	VERIFY						((I - points) == (visible_bone_count*8));
	MagicBox3					min_box = MagicMinBox(visible_bone_count*8,points);
	min_box.ComputeVertices		(points);
	
	result.identity				();

	result.c					= min_box.Center();

	result.i.sub(points[3],points[2]).normalize();
	result.j.sub(points[2],points[1]).normalize();
	result.k.sub(points[2],points[6]).normalize();

	Fvector						scale;
	scale.x						= points[3].distance_to(points[2])*.5f + additional.x;
	scale.y						= points[2].distance_to(points[1])*.5f + additional.y;
	scale.z						= points[2].distance_to(points[6])*.5f + additional.z;
	result.mulB_43				(Fmatrix().scale(scale));
}

void ai_obstacle::prepare_inside	(Fvector &min, Fvector &max)
{
	min							= Fvector().set(flt_max,flt_max,flt_max);
	max							= Fvector().set(flt_min,flt_min,flt_min);

	Fmatrix						matrix;
	float						half_cell_size = (use_additional_radius ? 1.f : 0.f)*ai().level_graph().header().cell_size()*.5f;
	Fvector						half_size = Fvector().set(half_cell_size,half_cell_size,half_cell_size);
	compute_matrix				(matrix,half_size);

	Fvector						points[8];
	for (int i=0; i<8; ++i) {
        matrix.transform_tiny	(points[i],local_points[i]);
		
		min.x					= _min(min.x,points[i].x);
		min.y					= _min(min.y,points[i].y);
		min.z					= _min(min.z,points[i].z);
		
		max.x					= _max(max.x,points[i].x);
		max.y					= _max(max.y,points[i].y);
		max.z					= _max(max.z,points[i].z);
	}

	m_box.m_planes[0].build		(points[0],points[3],points[5]);
	m_box.m_planes[1].build		(points[1],points[2],points[3]);
	m_box.m_planes[2].build		(points[6],points[5],points[4]);
	m_box.m_planes[3].build		(points[4],points[2],points[1]);
	m_box.m_planes[4].build		(points[3],points[2],points[4]);
	m_box.m_planes[5].build		(points[1],points[0],points[6]);
}

void ai_obstacle::correct_position	(Fvector &position)
{
	const Fbox					&box = ai().level_graph().header().box();
	position.x					= _max(position.x,box.min.x);
	position.y					= _max(position.y,box.min.y);
	position.z					= _max(position.z,box.min.z);
	position.x					= _min(position.x,box.max.x);
	position.y					= _min(position.y,box.max.y);
	position.z					= _min(position.z,box.max.z);
}

void ai_obstacle::compute_impl		()
{
//	VERIFY						(m_object->is_ai_obstacle());

	typedef CLevelGraph::CPosition				CPosition;
	typedef CLevelGraph::const_vertex_iterator	const_iterator;

	Fvector						min_position;
	Fvector						max_position;
	prepare_inside				(min_position,max_position);

	correct_position			(min_position);
	correct_position			(max_position);

	const CLevelGraph			&level_graph = ai().level_graph();
	const CPosition				&min_vertex_position = level_graph.vertex_position(min_position);
	const CPosition				&max_vertex_position = level_graph.vertex_position(max_position);

	u32							x_min, z_min;
	level_graph.unpack_xz		(min_vertex_position,x_min,z_min);

	u32							x_max, z_max;
	level_graph.unpack_xz		(max_vertex_position,x_max,z_max);

	u32							row_length = level_graph.row_length();
	const_iterator				B = level_graph.begin();
	const_iterator				E = level_graph.end();

	m_area.clear				();
	merge_predicate				predicate(this,m_area);
	for (u32 x=x_min; x<=x_max; ++x) {
		for (u32 z=z_min; z<=z_max; ++z) {
			u32					xz = x*row_length + z;
			const_iterator		I = std::lower_bound(B,E,xz);
			if ((I == E) || ((*I).position().xz() != xz))
				continue;

			predicate			(*I);

			for (++I; I != E; ++I) {
				if ((*I).position().xz() != xz)
					break;
				
				predicate		(*I);
			}
		}
	}

//	VERIFY						(m_area.empty());
	if (m_area.empty()) {
		m_crc					= 0;
		return;
	}

	boost::crc_32_type			temp;
	temp.process_block			(&*m_area.begin(),&*m_area.end());
	m_crc						= temp.checksum();
}

void ai_obstacle::on_move			()
{
	m_actual					= false;
}

float ai_obstacle::distance_to		(const Fvector &position) const
{
	return						(position.distance_to(m_object->Position()));
}
