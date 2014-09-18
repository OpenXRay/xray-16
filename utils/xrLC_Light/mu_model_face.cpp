#include "stdafx.h"

#include "mu_model_face.h"



#include "serialize.h"

#include "../../xrcore/xrPool.h"


poolSS<_vertex,8*1024>	&mu_vertices_pool();
poolSS<_face,8*1024>	&mu_faces_pool();

Tface<data_vertex>::Tface()
{}

Tvertex<data_vertex>::Tvertex()
{}

_vertex*	_vertex::CreateCopy_NOADJ(v_vertices& vertises_storage ) const
{
	//xrMU_Model::_vertex* V	= create_vertex(Fvector().set(0,0,0));
	_vertex*	V		= mu_vertices_pool().create();
	vertises_storage.push_back( V );
	V->P.set	( P );
	V->N.set	( N );
	V->C		= C;
	return		V;
}

template<>
Tface<data_vertex>::~Tface()
{}
template<>
Tvertex<data_vertex>::~Tvertex()
{}

void _face::Failure		()
{

}

		//Fvector2	tc	[3];
		//Fvector		N;
		//u32			sm_group;
void	_face::read_vertices		( INetReader	&r )
{}
void	_face::write_vertices		( IWriter	&w )const
{}

void _face::read( INetReader &r )
{
	base_Face::read( r );
	r.r_fvector2( tc[0] );
	r.r_fvector2( tc[1] );
	r.r_fvector2( tc[2] );
	r.r_fvector3( N );
	sm_group  = r.r_u32( );
	

}

void _face::write( IWriter &w ) const
{
	base_Face::write( w );
	w.w_fvector2( tc[0] );
	w.w_fvector2( tc[1] );
	w.w_fvector2( tc[2] );
	w.w_fvector3( N );
	w.w_u32( sm_group );
}




void _vertex::read( INetReader &r )
{
	base_Vertex::read( r );
}

void _vertex::write( IWriter &w ) const
{
	base_Vertex::write( w );
	
}


//////////////////////////////////////////////////////////////
void	_vertex::isolate_pool_clear_read		( INetReader	&r )
{
	R_ASSERT(false);
}
void	_vertex::isolate_pool_clear_write	( IWriter	&w )const
{
	R_ASSERT(false);
}
///////////////////////////////////////////////////////////////
void	_vertex::read_adjacents		( INetReader	&r )
{
}
void	_vertex::write_adjacents	( IWriter	&w )const
{
}

_vertex* _vertex::read_create()
{
	return mu_vertices_pool().create();
}





_face* _face::read_create()
{
	return mu_faces_pool().create();
}


poolSS<_vertex,8*1024>	mu_vertices;
poolSS<_face,8*1024>	mu_faces;

poolSS<_vertex,8*1024>	&mu_vertices_pool()
{
	return mu_vertices;
}
poolSS<_face,8*1024>	&mu_faces_pool()
{
	return mu_faces;
}

void mu_mesh_clear()
{
	mu_vertices.clear();
	mu_faces.clear();
}

