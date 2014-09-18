/************************************************************************

MxPropSlim

Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

$Id: MxPropSlim.cxx,v 1.9 2000/11/20 20:36:38 garland Exp $

************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxPropSlim.h"
#include "MxGeom3D.h"

typedef MxQuadric Quadric;

MxPropSlim::MxPropSlim(MxStdModel *m0)
: MxStdSlim(m0),
__quadrics(m0->vert_count()),
edge_links(m0->vert_count())
{
	consider_color();
	consider_texture();
	consider_normals();

	D = compute_dimension(m);

	will_decouple_quadrics = false;
	contraction_callback = NULL;
}

void MxPropSlim::consider_color(bool will)
{
	use_color = will && (m->color_binding() == MX_PERVERTEX);
	D = compute_dimension(m);
}

void MxPropSlim::consider_texture(bool will)
{
	use_texture = will && (m->texcoord_binding() == MX_PERVERTEX);
	D = compute_dimension(m);
}

void MxPropSlim::consider_normals(bool will)
{
	use_normals = will && (m->normal_binding() == MX_PERVERTEX);
	D = compute_dimension(m);
}

unsigned int MxPropSlim::compute_dimension(MxStdModel *m)
{
	unsigned int d = 3;

	if( use_color )  d += 3;
	if( use_texture )  d += 2;
	if( use_normals )  d += 3;

	return d;
}

void MxPropSlim::pack_to_vector(MxVertexID id, MxVector& v)
{
	VERIFY( v.dim() == D );
	VERIFY( id < m->vert_count() );

	v[0] = m->vertex(id)[0];
	v[1] = m->vertex(id)[1];
	v[2] = m->vertex(id)[2];

	unsigned int i = 3;
	if( use_color )
	{
		v[i++] = m->color(id).R();
		v[i++] = m->color(id).G();
		v[i++] = m->color(id).B();
	}
	if( use_texture )
	{
		v[i++] = m->texcoord(id)[0];
		v[i++] = m->texcoord(id)[1];
	}
	if( use_normals )
	{
		v[i++] = m->normal(id)[0];
		v[i++] = m->normal(id)[1];
		v[i++] = m->normal(id)[2];
	}
}

void MxPropSlim::pack_prop_to_vector(MxVertexID id, MxVector& v, unsigned int target)
{
	if( target == 0 )
	{
		v[0] = m->vertex(id)[0];
		v[1] = m->vertex(id)[1];
		v[2] = m->vertex(id)[2];
		return;
	}

	unsigned int i = 3;
	target--;

	if( use_color )
	{
		if( target == 0 )
		{
			v[i]   = m->color(id).R();
			v[i+1] = m->color(id).G();
			v[i+2] = m->color(id).B();
			return;
		}
		i += 3;
		target--;
	}
	if( use_texture )
	{
		if( target == 0 )
		{
			v[i]   = m->texcoord(id)[0];
			v[i+1] = m->texcoord(id)[1];
			return;
		}
		i += 2;
		target--;
	}
	if( use_normals )
	{
		if( target == 0 )
		{
			v[i]   = m->normal(id)[0];
			v[i+1] = m->normal(id)[1];
			v[i+2] = m->normal(id)[2];
			return;
		}
	}
}

static inline void CLAMP(double& v, double lo, double hi)
{
	if( v<lo ) v = lo;
	if( v>hi ) v = hi;
}

void MxPropSlim::unpack_from_vector(MxVertexID id, MxVector& v)
{
	VERIFY( v.dim() == D );
	VERIFY( id < m->vert_count() );

	m->vertex(id)[0] = (float)v[0];
	m->vertex(id)[1] = (float)v[1];
	m->vertex(id)[2] = (float)v[2];

	unsigned int i = 3;
	if( use_color )
	{
		CLAMP(v[i], 0, 1);
		CLAMP(v[i+1], 0, 1);
		CLAMP(v[i+2], 0, 1);
		m->color(id).set((float)v[i], (float)v[i+1], (float)v[i+2]);
		i += 3;
	}
	if( use_texture )
	{
		m->texcoord(id)[0] = (float)v[i++];
		m->texcoord(id)[1] = (float)v[i++];
	}
	if( use_normals )
	{
		float n[3];  n[0]=(float)v[i++];  n[1]=(float)v[i++];  n[2]=(float)v[i++];
		mxv_unitize(n, 3);
		m->normal(id).set(n[0], n[1], n[2]);
	}
}

void MxPropSlim::unpack_prop_from_vector(MxVertexID id,MxVector& v,unsigned int target)
{
	if( target == 0 )
	{
		m->vertex(id)[0] = (float)v[0];
		m->vertex(id)[1] = (float)v[1];
		m->vertex(id)[2] = (float)v[2];
		return;
	}

	unsigned int i=3;
	target--;

	if( use_color )
	{
		if( target == 0 )
		{
			m->color(id).set((float)v[i], (float)v[i+1], (float)v[i+2]);
			return;
		}
		i+=3;
		target--;
	}
	if( use_texture )
	{
		if( target == 0 )
		{
			m->texcoord(id)[0] = (float)v[i];
			m->texcoord(id)[1] = (float)v[i+1];
			return;
		}
		i += 2;
		target--;
	}
	if( use_normals )
	{
		if( target == 0 )
		{
			float n[3];  n[0]=(float)v[i];  n[1]=(float)v[i+1];  n[2]=(float)v[i+2];
			mxv_unitize(n, 3);
			m->normal(id).set(n[0], n[1], n[2]);
			return;
		}
	}
}


unsigned int MxPropSlim::prop_count()
{
	unsigned int i = 1;

	if( use_color ) i++;
	if( use_texture) i++;
	if( use_normals ) i++;

	return i;
}

void MxPropSlim::compute_face_quadric(MxFaceID i, MxQuadric& Q)
{
	MxFace& f = m->face(i);

	MxVector v1(dim());
	MxVector v2(dim());
	MxVector v3(dim());

	if( will_decouple_quadrics )
	{
		Q.clear();

		for(unsigned int p=0; p<prop_count(); p++)
		{
			v1=0.0;  v2=0.0;  v3=0.0;

			pack_prop_to_vector(f[0], v1, p);
			pack_prop_to_vector(f[1], v2, p);
			pack_prop_to_vector(f[2], v3, p);

			// !!BUG: Will count area multiple times (once per property)
			MxQuadric Q_p(v1, v2, v3, m->compute_face_area(i));

			// !!BUG: Need to only extract the relevant block of the matrix.
			//        Adding the whole thing gives us extraneous stuff.
			Q += Q_p;
		}
	}
	else
	{
		pack_to_vector(f[0], v1);
		pack_to_vector(f[1], v2);
		pack_to_vector(f[2], v3);

		Q = MxQuadric(v1, v2, v3, m->compute_face_area(i));
	}
}

void MxPropSlim::collect_quadrics()
{
	for(unsigned int j=0; j<quadric_count(); j++)
		__quadrics[j] = xr_new<MxQuadric>(dim());

	for(MxFaceID i=0; i<m->face_count(); i++)
	{
		MxFace& f = m->face(i);

		MxQuadric Q			(dim());
		compute_face_quadric(i, Q);

		switch( weighting_policy )
		{
		case MX_WEIGHT_AREA:
		case MX_WEIGHT_AREA_AVG:
			Q *= Q.area();
			// no break: fallthrough
		default:
			quadric(f[0]) += Q;
			quadric(f[1]) += Q;
			quadric(f[2]) += Q;
			break;
		}
	}
}

void MxPropSlim::initialize()
{
	collect_quadrics();

	if( boundary_weight > 0.0 )
		constrain_boundaries();

	is_initialized = true;
}

unsigned int MxPropSlim::check_local_validity(unsigned int v, const float* vnew)
{
	const MxFaceList& N = m->neighbors(v);
	unsigned int nfailed = 0;
	unsigned int i;

	for(i=0; i<(unsigned int)N.length(); i++){
		if( m->face_mark(N[i]) == 1 ){
			MxFace&		 f = m->face(N[i]);
			unsigned int k = f.find_vertex(v);
			unsigned int x = f[(k+1)%3];
			unsigned int y = f[(k+2)%3];

			float d_yx[3], d_vx[3], d_vnew[3], f_n[3], n[3];
			mxv_sub(d_yx, m->vertex(y), m->vertex(x), 3);   // d_yx = y-x
			mxv_sub(d_vx, m->vertex(v), m->vertex(x), 3);   // d_vx = v-x
			mxv_sub(d_vnew, vnew, m->vertex(x), 3);         // d_vnew = vnew-x

			mxv_cross3(f_n, d_yx, d_vx);
			mxv_cross3(n, f_n, d_yx);     // n = ((y-x)^(v-x))^(y-x)
			mxv_unitize(n, 3);

			// assert( mxv_dot(d_vx, n, 3) > -FEQ_EPS );
			if(mxv_dot(d_vnew,n,3)<local_validity_threshold*mxv_dot(d_vx,n,3))
				nfailed++;
		}
	}

	return nfailed;
}

double MxPropSlim::check_local_compactness(unsigned int v, const float *vnew)
{
	const MxFaceList& N = m->neighbors(v);
	double c_min		= 1.0;

	for(unsigned int i=0; i<(unsigned int)N.length(); i++)
		if( m->face_mark(N[i]) == 1 ){
			const MxFace& f = m->face(N[i]);
			Vec3 f_after[3];
			for(unsigned int j=0; j<3; j++)
				f_after[j] = (f[j]==v)?Vec3(vnew):Vec3(m->vertex(f[j]));

			double c=triangle_compactness(f_after[0], f_after[1], f_after[2]);

			if( c < c_min ) c_min = c;
		}

		return c_min;
}

void MxPropSlim::apply_mesh_penalties(edge_info *info)
{
	unsigned int i;

	const MxFaceList& N1 = m->neighbors(info->v1);
	const MxFaceList& N2 = m->neighbors(info->v2);

	// Set up the face marks as the check_xxx() functions expect.
	//
	for(i=0; i<(unsigned int)N2.length(); i++) m->face_mark(N2[i], 0);
	for(i=0; i<(unsigned int)N1.length(); i++) m->face_mark(N1[i], 1);
	for(i=0; i<(unsigned int)N2.length(); i++) m->face_mark(N2[i], m->face_mark(N2[i])+1);

	double base_error	= info->heap_key();
	double bias			= 0.0;

	// Check for excess over degree bounds.
	//
	unsigned int max_degree = _max(N1.length(), N2.length());
	if( max_degree > vertex_degree_limit )
		bias += (max_degree-vertex_degree_limit) * meshing_penalty * 0.001f;

	// Local validity checks
	//
	float vnew[3];
	vnew[0] = (float)info->target[0];
	vnew[1] = (float)info->target[1];
	vnew[2] = (float)info->target[2];

	unsigned int nfailed = 0;
	nfailed += check_local_validity(info->v1, vnew);
	nfailed += check_local_validity(info->v2, vnew);
	if( nfailed )
		bias += nfailed*meshing_penalty;

	float _scale = 1.f;
	if( compactness_ratio > 0.0 ){
		double c1_min=check_local_compactness(info->v1, vnew);
		double c2_min=check_local_compactness(info->v2, vnew);
		double c_min = _min(c1_min, c2_min);

		if( c_min < compactness_ratio ) 
			_scale += float((compactness_ratio-c_min)/compactness_ratio);
	}

	info->heap_key(float((base_error-EDGE_BASE_ERROR)*_scale - bias));
}

void MxPropSlim::compute_target_placement(edge_info *info)
{
	MxVertexID i=info->v1, j=info->v2;

	const MxQuadric &Qi=quadric(i), &Qj=quadric(j);
	MxQuadric Q=Qi;  Q+=Qj;

	double e_min;

	if( placement_policy==MX_PLACE_OPTIMAL && Q.optimize(info->target)){
		e_min = Q(info->target);
	}else{
		// Fall back only on endpoints
		MxVector vi(dim());
		MxVector vj(dim());
		MxVector best(dim());

		pack_to_vector(i, vi);
		pack_to_vector(j, vj);

		double ei=Q(vi), ej=Q(vj);

		if( ei<ej )	{ e_min = ei; best = vi; }
		else		{ e_min = ej; best = vj; swap(info->v1,info->v2);}

		if( placement_policy>=MX_PLACE_ENDORMID ){
			MxVector mid(dim());
			mid			= vi;
			mid			+=vj;
			mid			/=2.f;
			double e_mid= Q(mid);

			if( e_mid < e_min ) { e_min = e_mid; best = mid; }
		}
		info->target	= best;
	}

	if( weighting_policy == MX_WEIGHT_AREA_AVG )
		e_min /= Q.area();

	info->heap_key(float(-e_min));
}

bool MxPropSlim::decimate(unsigned int target, float max_error, void* cb_params)
{
	MxPairContraction conx;

	max_error								+= EDGE_BASE_ERROR;
	while( valid_faces > target )
	{
		MxHeapable *top						= heap.top();
		if( !top )							{ return false; }
		if( -top->heap_key()>max_error )	{ return true;	}

		edge_info *info						= (edge_info *)heap.extract();
		MxVertexID v1						= info->v1;
		MxVertexID v2						= info->v2;

		if( m->vertex_is_valid(v1) && m->vertex_is_valid(v2) ){
			m->compute_contraction			(v1, v2, &conx);

			conx.dv1[0] = float(info->target[0] - m->vertex(v1)[0]);
			conx.dv1[1] = float(info->target[1] - m->vertex(v1)[1]);
			conx.dv1[2] = float(info->target[2] - m->vertex(v1)[2]);
			conx.dv2[0] = float(info->target[0] - m->vertex(v2)[0]);
			conx.dv2[1] = float(info->target[1] - m->vertex(v2)[1]);
			conx.dv2[2] = float(info->target[2] - m->vertex(v2)[2]);

			if( contraction_callback )
				(*contraction_callback)(conx, -(info->heap_key()+EDGE_BASE_ERROR), cb_params);
			
			apply_contraction(conx, info);
		}

		xr_delete(info);
	}

	return true;
}



////////////////////////////////////////////////////////////////////////
//
// This is *very* close to the code in MxEdgeQSlim

void MxPropSlim::create_edge(MxVertexID i, MxVertexID j)
{
	edge_info *info = xr_new<edge_info>(dim());

	edge_links(i).add(info);
	edge_links(j).add(info);

	info->v1 = i;
	info->v2 = j;

	compute_edge_info(info);
}

void MxPropSlim::discontinuity_constraint(MxVertexID i, MxVertexID j, MxFaceID f)
{
	Vec3 org(m->vertex(i)), dest(m->vertex(j));
	Vec3 e = dest - org;

	Vec3 v1(m->vertex(m->face(f)(0)));
	Vec3 v2(m->vertex(m->face(f)(1)));
	Vec3 v3(m->vertex(m->face(f)(2)));
	Vec3 n = triangle_normal(v1,v2,v3);

	Vec3 n2 = e ^ n;
	unitize(n2);

	MxQuadric3 Q3(n2, -(n2*org));
	Q3 *= boundary_weight;

	if( weighting_policy == MX_WEIGHT_AREA ||
		weighting_policy == MX_WEIGHT_AREA_AVG )
	{
		Q3.set_area(norm2(e));
		Q3 *= Q3.area();
	}

	MxQuadric Q(Q3, dim());

	quadric(i) += Q;
	quadric(j) += Q;
}

void MxPropSlim::discontinuity_constraint(MxVertexID i, MxVertexID j,
										  const MxFaceList& faces)
{
	for(unsigned int f=0; f<(unsigned int)faces.length(); f++)
		discontinuity_constraint(i,j,faces[f]);
}

void MxPropSlim::constraint_manual(MxVertexID v0, MxVertexID v1, MxFaceID f)
{
	discontinuity_constraint(v0, v1, f);
}

void MxPropSlim::apply_contraction(const MxPairContraction& conx,
								   edge_info *info)
{
	valid_verts--;
	valid_faces -= conx.dead_faces.length();
	quadric(conx.v1) += quadric(conx.v2);

	update_pre_contract(conx);

	m->apply_contraction(conx);

	unpack_from_vector(conx.v1, info->target);

	// Must update edge_info here so that the meshing penalties
	// will be computed with respect to the new mesh rather than the old
	for(unsigned int i=0; i<(unsigned int)edge_links(conx.v1).length(); i++)
		compute_edge_info(edge_links(conx.v1)[i]);
}



////////////////////////////////////////////////////////////////////////
//
// These were copied *unmodified* from MxEdgeQSlim
// (with some unsupported features commented out).
//

void MxPropSlim::collect_edges()
{
	MxVertexList star;

	for(MxVertexID i=0; i<m->vert_count(); i++)
	{
		star.reset();
		m->collect_vertex_star(i, star);

		for(unsigned int j=0; j<(unsigned int)star.length(); j++)
			if( i < star(j) )  // Only add particular edge once
				create_edge(i, star(j));
	}
}

void MxPropSlim::constrain_boundaries()
{
	MxVertexList star;
	MxFaceList faces;

	for(MxVertexID i=0; i<m->vert_count(); i++)
	{
		star.reset();
		m->collect_vertex_star(i, star);

		for(unsigned int j=0; j<(unsigned int)star.length(); j++){
			if( i < star(j) )
			{
				faces.reset();
				m->collect_edge_neighbors(i, star(j), faces);
				if( faces.length() == 1 )
					discontinuity_constraint(i, star(j), faces);
			}
		}
	}
}

void MxPropSlim::compute_edge_info(edge_info *info)
{
	compute_target_placement(info);

	//     if( will_normalize_error )
	//     {
	//         double e_max = Q_max(info->vnew);
	//         if( weight_by_area )
	//             e_max /= Q_max.area();

	//         info->heap_key(info->heap_key() / e_max);
	//     }

	finalize_edge_update(info);
}

void MxPropSlim::finalize_edge_update(edge_info *info)
{
	if( meshing_penalty > 1.0 )
		apply_mesh_penalties(info);

	if( info->is_in_heap() )
		heap.update(info);
	else
		heap.insert(info);
}

void MxPropSlim::update_pre_contract(const MxPairContraction& conx)
{
	MxVertexID v1=conx.v1, v2=conx.v2;
	unsigned int i, j;

	star.reset();
	m->collect_vertex_star(v1, star);

	for(i=0; i<(unsigned int)edge_links(v2).length(); i++)
	{
		edge_info *e = edge_links(v2)(i);
		MxVertexID u = (e->v1==v2)?e->v2:e->v1;
		VERIFY( e->v1==v2 || e->v2==v2 );
		VERIFY( u!=v2 );

		if( u==v1 || varray_find(star, u) )
		{
			// This is a useless link --- kill it
			bool found = varray_find(edge_links(u), e, &j);
			VERIFY( found );
			edge_links(u).remove(j);
			heap.remove(e);
			if( u!=v1 ) xr_delete(e); // (v1,v2) will be deleted later
		}
		else
		{
			// Relink this to v1
			e->v1 = v1;
			e->v2 = u;
			edge_links(v1).add(e);
		}
	}

	edge_links(v2).reset();
}
