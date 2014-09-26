#include "stdafx.h"
#include "xrmu_model.h"

#include "serialize.h"
#include "mu_model_face.h"
#include "vector_clear.h"
#include "../../xrcore/xrPool.h"

xrMU_Model::xrMU_Model(): 
m_lod_ID(u16(-1)),read_faces(0),read_vertices(0),write_faces(0), write_vertices(0)
{}
xrMU_Model::~xrMU_Model()
{
	reading_close();
	writting_close();
	clear_mesh	();
}


poolSS<_vertex,8*1024>	&mu_vertices_pool();
poolSS<_face,8*1024>	&mu_faces_pool();

static struct destruct_vertex_not_uregister
{
	static void destruct (_vertex * &v)
	{
		mu_vertices_pool().destroy( v );
	}
} _destruct_vertex;

static struct destruct_face_not_uregister
{
	static void destruct (_face * &f)
	{
		mu_faces_pool().destroy( f );
	}
} _destruct_face;

void xrMU_Model::clear_mesh			()
{
	vec_clear( m_vertices, _destruct_vertex ); 
	vec_clear( m_faces, _destruct_face );
}


void	xrMU_Model::read_color( INetReader	&r )
{
	r_pod_vector( r, color );
}
void	xrMU_Model::write_color( IWriter	&w ) const 
{
	w_pod_vector( w ,color );
}

void	xrMU_Model::read_subdivs( INetReader	&r )
{
	r_pod_vector( r, m_subdivs );
}

void	xrMU_Model::write_subdivs( IWriter	&w ) const 
{
	w_pod_vector( w ,m_subdivs );
}

void xrMU_Model::read( INetReader	&r )
{
	reading_open();
	r.r_stringZ( m_name );
	m_lod_ID = r.r_u16();
	VERIFY( read_vertices );
	read_vertices->read( r );
	VERIFY( read_faces );
	read_faces->read( r );
	r_pod_vector( r, m_subdivs );
	read_adjacents( r );

	read_face_verts( r );
}
void xrMU_Model::write( IWriter	&w ) const 
{
	writting_open();
	w.w_stringZ( m_name );
	w.w_u16( m_lod_ID );
	VERIFY( write_vertices );
	write_vertices->write( w );
	VERIFY( write_faces );
	write_faces->write( w );
	w_pod_vector( w, m_subdivs );
	write_adjacents( w );
	
	write_face_verts( w );

}

u32	xrMU_Model::find( const _vertex *v ) const
{
 	v_vertices::const_iterator i = std::find( m_vertices.begin(), m_vertices.end(), v );
	if( i== m_vertices.end() )
		return u32(-1);
	return u32(i - m_vertices.begin());
}

u32	xrMU_Model::find( const _face *f ) const
{
	 v_faces::const_iterator i = std::find( m_faces.begin(), m_faces.end(), f ) ;
	 if(i== m_faces.end())
		return u32(-1);
	 return u32(i - m_faces.begin());
}

xrMU_Model * xrMU_Model::read_create()
{
	return xr_new<xrMU_Model>();
}








void		xrMU_Model::			reading_open		()
{
	VERIFY( !read_faces );
	read_faces		= xr_new<tread_faces>(&m_faces);
	VERIFY( !read_vertices );
	read_vertices	= xr_new<tread_vertices>(&m_vertices);
}
void		xrMU_Model::			reading_close		()
{
	xr_delete( read_faces );
	xr_delete( read_vertices );

}
void		xrMU_Model::			writting_open		()const
{
	VERIFY( !write_faces );
	write_faces			= xr_new<twrite_faces>(&m_faces);
	VERIFY( !write_vertices );
	write_vertices		= xr_new<twrite_vertices>(&m_vertices);
}
void		xrMU_Model::			writting_close		()const
{
	xr_delete(write_faces);
	xr_delete(write_vertices);
}


void		xrMU_Model::read	( INetReader	&r, _vertex* &v )const
{
	VERIFY( read_vertices );
	read_vertices->read( r, v );
}
void		xrMU_Model::read	( INetReader	&r, _face*	&v )const
{
	VERIFY( read_faces );
	read_faces->read( r, v );
}


void		xrMU_Model::write	( IWriter	&w, u32 id, const _vertex* v )const
{
	VERIFY( v==m_vertices[id] );
	VERIFY( write_vertices );
	VERIFY( write_vertices->get_id( v, m_vertices ) == id );
	write_vertices->write( w, v );
}
void		xrMU_Model::write	( IWriter	&w, u32 id, const _face* v )const
{
	VERIFY( v==m_faces[id] );
	VERIFY( write_faces );
	VERIFY( write_faces->get_id( v, m_faces ) == id );
	write_faces->write( w, v );
}

void xrMU_Model::read_adjacents( INetReader	&r, xrMU_Model::tread_faces &read_faces, _vertex &v )
{
	read_faces.read_ref( r, v.m_adjacents );
}

void xrMU_Model::write_adjacents( IWriter	&w, xrMU_Model::twrite_faces &write_faces, const _vertex &v )
{
	write_faces.write_ref( w, v.m_adjacents );
}

void		xrMU_Model::		read_adjacents		( INetReader	&r )
{
	R_ASSERT( read_faces );
	v_vertices_it i = m_vertices.begin(), e = m_vertices.end();
	for(;e!=i;++i)
			read_adjacents( r, *read_faces, *(*i) );
}
void		xrMU_Model::		write_adjacents		( IWriter	&w ) const
{
	R_ASSERT( write_faces );
	v_vertices_cit i = m_vertices.begin(), e = m_vertices.end();
	for(;e!=i;++i)
			write_adjacents( w, *write_faces, *(*i) );
}

void	xrMU_Model::read_face_verts		( INetReader	&r )
{
	R_ASSERT( read_vertices );
	v_faces_it i = m_faces.begin(), e = m_faces.end();
	for(;e!=i;++i)
			read_face_verts( r, *read_vertices, *(*i) );
}

void	xrMU_Model::write_face_verts	( IWriter	&w ) const
{
	R_ASSERT( write_vertices );
	v_faces_cit i = m_faces.begin(), e = m_faces.end();
	for(;e!=i;++i)
			write_face_verts( w, *write_vertices, *(*i) );
}

void	xrMU_Model::read_face_verts		( INetReader	&r, xrMU_Model::tread_vertices &read_verts, _face &v )
{
	//read_verts
	read_verts.read( r, v.v[0] );
	read_verts.read( r, v.v[1] );
	read_verts.read( r, v.v[2] );
}
void	xrMU_Model::write_face_verts	( IWriter	&w, xrMU_Model::twrite_vertices &write_verts, const _face &v )
{
	write_verts.write( w, v.v[0] );
	write_verts.write( w, v.v[1] );
	write_verts.write( w, v.v[2] );
}