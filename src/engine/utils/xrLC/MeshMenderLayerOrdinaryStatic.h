#ifndef	_MESH_MENDER_LAYER_ORDINARY_STATIC_H_
#define	_MESH_MENDER_LAYER_ORDINARY_STATIC_H_

#include	"../../common/NvMender2003/nvMeshMender.h"
#include	"../../common/NvMender2003/mender_input_output.h"

#include	"../xrlc_light/xrFace.h"

IC void	set_vertex( MeshMender::Vertex &out_vertex, const Vertex& in_veretex, const Fvector2 Ftc )
{
			cv_vector( out_vertex.pos, in_veretex.P );
			cv_vector( out_vertex.normal, in_veretex.N );
			out_vertex.s		= Ftc.x;
			out_vertex.t		= Ftc.y;
			//out_vertex.tangent;
			//out_vertex.binormal;
}


IC void	set_face( Face &out_face, const MeshMender::Vertex in_vertices[3] )
{
	for( u16 v = 0; v< 3; ++v )
	{
		out_face.tc.front().uv[v]	.set( in_vertices[v].s, in_vertices[v].t );					
		Fvector tangent; Fvector binormal;
		out_face.basis_tangent[v].set( cv_vector( tangent , in_vertices[v].tangent ) );								
		out_face.basis_binormal[v].set( cv_vector( binormal, in_vertices[v].binormal ) ); 		
	}
}

#endif
