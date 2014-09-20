/************************************************************************

  MxBlockModel

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxBlockModel.cxx,v 1.27 2000/11/20 20:36:38 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxBlockModel.h"
#include "MxVector.h"

////////////////////////////////////////////////////////////////////////
//
// Basic allocation routines
//

MxBlockModel *MxBlockModel::clone(MxBlockModel *m)
{
    if( !m ) m = xr_new<MxBlockModel>(vert_count(), face_count());

    unsigned int i;

    for(i=0; i<vert_count(); i++)
	m->add_vertex(vertex(i));
    for(i=0; i<face_count(); i++)
	m->add_face(face(i)[0], face(i)[1], face(i)[2]);

    m->normal_binding((u8)normal_binding());
    if( normal_binding() != MX_UNBOUND )
    {
	m->normals->room_for(normal_count());
	m->normals->bitcopy(*normals);
    }

    m->color_binding((u8)color_binding());
    if( color_binding() != MX_UNBOUND )
    {
	m->colors->room_for(color_count());
	m->colors->bitcopy(*colors);
    }

    m->texcoord_binding((u8)texcoord_binding());
    if( texcoord_binding() != MX_UNBOUND )
    {
	m->tcoords->room_for(texcoord_count());
	m->tcoords->bitcopy(*tcoords);
    }

    return m;
}

MxFaceID MxBlockModel::alloc_face(MxVertexID v1, MxVertexID v2, MxVertexID v3)
{
    faces.add(MxFace(v1,v2,v3));
    return faces.last_id();
}

MxVertexID MxBlockModel::alloc_vertex(float x, float y, float z)
{
    vertices.add(MxVertex(x,y,z));
    return vertices.last_id();
}

MxVertexID MxBlockModel::add_vertex(float x, float y, float z)
{
    MxVertexID id = alloc_vertex(x,y,z);
    init_vertex(id);
    return id;
}

void MxBlockModel::remove_vertex(MxVertexID v)
{
    VERIFY( v < (unsigned int)vertices.length() );

    free_vertex(v);
    vertices.remove(v);
    if( normal_binding() == MX_PERVERTEX ) normals->remove(v);
    if( color_binding() == MX_PERVERTEX ) colors->remove(v);
    if( texcoord_binding() == MX_PERVERTEX ) tcoords->remove(v);
}

void MxBlockModel::remove_face(MxFaceID f)
{
    VERIFY( f < (unsigned int)faces.length() );

    free_face(f);
    faces.remove(f);
    if( normal_binding() == MX_PERFACE ) normals->remove(f);
    if( color_binding() == MX_PERFACE ) colors->remove(f);
    if( texcoord_binding() == MX_PERFACE ) tcoords->remove(f);
}

MxFaceID MxBlockModel::add_face(unsigned int v1,
				unsigned int v2,
				unsigned int v3,
				bool will_link)
{
    MxFaceID id = alloc_face(v1, v2, v3);
    if( will_link )  init_face(id);
    return id;
}

unsigned int MxBlockModel::add_color(float r, float g, float b)
{
    VERIFY( colors );
    MxColor c(r, g, b);
    colors->add(c);
    return colors->last_id();
}

unsigned int MxBlockModel::add_normal(float x, float y, float z)
{
    MxNormal n(x, y, z);
    normals->add(n);
    return normals->last_id();
}

unsigned int MxBlockModel::add_texcoord(float s, float t)
{
    tcoords->add(MxTexCoord(s,t));
    return tcoords->last_id();
}

unsigned int MxBlockModel::add_texmap(const char *name)
{
    if( !name ) name = "tex";

    if( tex_name ) xr_free(tex_name);

    tex_name = xr_strdup(name);
    return 0;
}

////////////////////////////////////////////////////////////////////////
//
// Property binding
//

static const char *bindings[] = {
    "unbound",
    "face",
    "vertex",
    NULL
};

static
unsigned int binding_size(MxBlockModel& m, unsigned char i)
{
    switch( i )
    {
    case MX_UNBOUND: return 0;
    case MX_PERVERTEX: return _max(1, m.vert_count());
    case MX_PERFACE: return _max(1, m.face_count());
    default: return 0;
    }
}

const char *MxBlockModel::binding_name(int b)
{
    if( b > MX_MAX_BINDING )
	return NULL;
    else
	return bindings[b];
}
    
int MxBlockModel::parse_binding(const char *name)
{
    for(int i=0; i<=MX_MAX_BINDING; i++)
	if( streq(bindings[i], name) )  return i;

    return MX_UNBOUND;
}

void MxBlockModel::color_binding(unsigned char b)
{
    int size = binding_size(*this, b);

    if( b==MX_UNBOUND )
    {
	if( colors ) { xr_delete(colors); }
	binding_mask &= (~MX_COLOR_MASK);
    }
    else
    {
	if( colors )
	    colors->reset();
	else
	    colors = xr_new<MxDynBlock<MxColor> >(size);
	binding_mask |= MX_COLOR_MASK;
    }

    cbinding=b;
}

void MxBlockModel::normal_binding(unsigned char b)
{
    int size = binding_size(*this, b);

    if( b==MX_UNBOUND )
    {
	if( normals ) { xr_delete(normals); }
	binding_mask &= (~MX_NORMAL_MASK);
    }
    else
    {
	if( normals )
	    normals->reset();
	else
	    normals = xr_new<MxDynBlock<MxNormal> >(size);
	binding_mask |= MX_NORMAL_MASK;
    }

    nbinding=b;
}

void MxBlockModel::texcoord_binding(unsigned char b)
{
    if( b!=MX_UNBOUND && b!=MX_PERVERTEX )
		FATAL	("Illegal texture coordinate binding.");

    int size = binding_size(*this, b);
    if( tcoords )  tcoords->reset();
    else tcoords = xr_new<MxDynBlock<MxTexCoord> >(size);

    tbinding = b;
}

////////////////////////////////////////////////////////////////////////
//
// Utility methods for computing characteristics of faces.
//

void MxBlockModel::compute_face_normal(MxFaceID f, float *n, bool will_unitize)
{
    float *v1 = vertex(face(f)[0]);
    float *v2 = vertex(face(f)[1]);
    float *v3 = vertex(face(f)[2]);

    float a[3], b[3];

    mxv_sub(a, v2, v1, 3);
    mxv_sub(b, v3, v1, 3);
    mxv_cross3(n, a, b);
    if( will_unitize )
	mxv_unitize(n, 3);
}

void MxBlockModel::compute_face_normal(MxFaceID f, double *n,bool will_unitize)
{
    float *v1 = vertex(face(f)[0]);
    float *v2 = vertex(face(f)[1]);
    float *v3 = vertex(face(f)[2]);

    double a[3], b[3];
    for(int i=0; i<3; i++) { a[i] = v2[i]-v1[i];  b[i] = v3[i]-v1[i]; }

    mxv_cross3(n, a, b);
    if( will_unitize )
	mxv_unitize(n, 3);
}

void MxBlockModel::compute_face_plane(MxFaceID f, float *p, bool will_unitize)
{
    compute_face_normal(f, p, will_unitize);
    p[3] = -mxv_dot(p, corner(f, 0), 3);
}

double MxBlockModel::compute_face_area(MxFaceID f)
{
    double n[3];

    compute_face_normal(f, n, false);
    return 0.5 * mxv_norm(n, 3);
}

double MxBlockModel::compute_face_perimeter(MxFaceID fid, bool *flags)
{
    double perim = 0.0;
    const MxFace& f = face(fid);

    for(unsigned int i=0; i<3; i++)
    {
	if( !flags || flags[i] )
	{
	    float *vi = vertex(f[i]),  *vj = vertex(f[(i+1)%3]), e[3];
	    perim += mxv_norm(mxv_sub(e, vi, vj, 3), 3);
	}
    }

    return perim;
}

double MxBlockModel::compute_corner_angle(MxFaceID f, unsigned int i)
{
    unsigned int i_prev = (i==0)?2:i-1;
    unsigned int i_next = (i==2)?0:i+1;

    float e_prev[3], e_next[3];
    mxv_unitize(mxv_sub(e_prev, corner(f, (short)i_prev), corner(f, (short)i), 3), 3);
    mxv_unitize(mxv_sub(e_next, corner(f, (short)i_next), corner(f, (short)i), 3), 3);

    return acos(mxv_dot(e_prev, e_next, 3));
}
