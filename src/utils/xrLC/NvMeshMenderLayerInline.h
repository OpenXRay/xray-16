
#include "xrFace.h"

IC D3DXVECTOR3& cv_vector ( D3DXVECTOR3	&l, const Fvector& r  )
{
	l.x = r.x;
	l.y = r.y;
	l.z = r.z;
	return l;
}

IC Fvector&  cv_vector (  Fvector& l, const D3DXVECTOR3	&r  )
{
	l.x = r.x;
	l.y = r.y;
	l.z = r.z;
	return l;
}

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