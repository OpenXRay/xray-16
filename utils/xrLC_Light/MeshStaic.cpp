#include "stdafx.h"

#include "xrLC_GlobalData.h"

#include "xrface.h"
#include "vector_clear.h"

typedef poolSS<Vertex,8*1024>	poolVertices;
typedef poolSS<Face,8*1024>		poolFaces;
static poolVertices	_VertexPool;
static poolFaces	_FacePool;

Face* xrLC_GlobalData	::create_face	()		
{
	return _FacePool.create();
}
void xrLC_GlobalData	::destroy_face	(Face* &f)
{
	_FacePool.destroy( f );
}

Vertex* xrLC_GlobalData	::create_vertex	()		
{
	return _VertexPool.create();
}
void xrLC_GlobalData	::destroy_vertex	(Vertex* &f)
{
	_VertexPool.destroy( f );
}




static struct destruct_vertex_not_uregister
{
	static void destruct (Vertex * &v)
	{
		::destroy_vertex( v, false );
	}
} _destruct_vertex_not_uregister;
static struct destruct_face_not_uregister
{
	static void destruct (Face * &f)
	{
		::destroy_face( f, false );
	}
} _destruct_face_not_uregister;
void xrLC_GlobalData	::gl_mesh_clear	()
{
	vec_clear( _g_vertices, _destruct_vertex_not_uregister ); 
	vec_clear( _g_faces, _destruct_face_not_uregister );
	
	_VertexPool.clear();
	_FacePool.clear();
}


void xrLC_GlobalData	::vertices_isolate_and_pool_reload()
{


	const u32 inital_verts_count = _g_vertices.size();
		  u32 not_empty_verts = 0;
	//for(u32 i = 0; i < inital_verts_count; ++i )
	// if(!_g_vertices[i]->m_adjacents.empty())
	//	++not_empty_verts;
/////////////////////////////////////////////////////////
	
	string_path			path_name;
	FS.update_path		( path_name, "$app_root$", "ccc__temp__vertices"  );
	{
		IWriter * file		= FS.w_open( path_name );
		R_ASSERT( file );
		for(u32 i = 0; i < inital_verts_count; ++i )
		{
			Vertex	&v = *_g_vertices[i];
			if( v.m_adjacents.empty() )
			{
				::destroy_vertex( _g_vertices[i], false );
				continue;
			}
//isolate_pool_clear_read	
//isolate_pool_clear_write

			v.isolate_pool_clear_write( *file );
			::destroy_vertex( _g_vertices[i], false );
			++not_empty_verts;
		}
		FS.w_close(file);
	}
/////////////////////////////////////////////////////////
	_g_vertices.clear_not_free();
	clLog( "mem usage before clear pool: %u", Memory.mem_usage() );


	_VertexPool.clear();
	
/////////////////////////////////////////////////////////
	{
		b_vert_not_register = true;
		_g_vertices.resize( not_empty_verts, 0 );

		Memory.mem_compact();
		clLog( "mem usage after clear pool: %u", Memory.mem_usage() );

		INetReaderFile r_verts( path_name );
		for(u32 i = 0; i < not_empty_verts; ++i )
		{
			Vertex* &v = _g_vertices[i];
			v = _VertexPool.create();
			v->isolate_pool_clear_read( r_verts );
		}
		b_vert_not_register = false;
	}
}


void	xrLC_GlobalData::clear_mesh		()
{
	
	//R_ASSERT(g_XSplit.empty());
	clLog( "mem usage before clear mesh: %u", Memory.mem_usage() );
	//g_vertices().clear();
	//g_faces().clear();
	//_VertexPool.clear();
	//_FacePool.clear();
	gl_mesh_clear	();
	Memory.mem_compact();
	clLog( "mem usage after clear mesh: %u", Memory.mem_usage() );
}









