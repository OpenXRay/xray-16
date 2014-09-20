#ifndef _MESH_MENDER_LAYER_OGF_H_
#define _MESH_MENDER_LAYER_OGF_H_

#include	"../../common/NvMender2003/nvMeshMender.h"
#include	"../../common/NvMender2003/mender_input_output.h"
#include	"../../common/NvMender2003/remove_isolated_verts.h"

#include	"OGF_Face.h"

IC void	set_vertex( MeshMender::Vertex &out_vertex, const OGF_Vertex& in_vertex )
{
			cv_vector( out_vertex.pos,		in_vertex.P );
			cv_vector( out_vertex.normal,	in_vertex.N );
			out_vertex.s		= in_vertex.UV[0].x;
			out_vertex.t		= in_vertex.UV[0].y;
			//out_vertex.tangent;
			//out_vertex.binormal;
}

IC void	set_vertex( OGF_Vertex& out_vertex,  const OGF_Vertex& in_old_vertex, const MeshMender::Vertex &in_vertex )
{
			out_vertex = in_old_vertex;
			cv_vector( out_vertex.P, in_vertex.pos );
			cv_vector( out_vertex.N, in_vertex.normal );

			out_vertex.UV[0].x	= in_vertex.s;
			out_vertex.UV[0].y	= in_vertex.t;
			Fvector tangent; Fvector binormal;
			out_vertex.T.set( cv_vector( tangent, in_vertex.tangent ) );
			out_vertex.B.set( cv_vector( binormal, in_vertex.binormal ) );
}

/////////////////////////////////////////////////////////////////////////////////////
IC u16	&face_vertex( OGF_Face &F, u32 vertex_index )
{
	VERIFY( vertex_index < 3 );
	return F.v[vertex_index];
}

IC const u16 &face_vertex( const OGF_Face &F, u32 vertex_index )
{
	VERIFY( vertex_index < 3 );
	return F.v[vertex_index];
}

/////////////////////////////////////////////////////////////////////////////////////




#endif