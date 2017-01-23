#ifndef	_MENDER_INPUT_OUTPUT_H_
#define	_MENDER_INPUT_OUTPUT_H_

#include	"convert.h"

template <typename type_face>
IC void	add_face( xr_vector< unsigned int > &theIndices, const type_face	&iF )
{
	theIndices.push_back	( face_vertex( iF, 0 ) );
	theIndices.push_back	( face_vertex( iF, 1 ) );
	theIndices.push_back	( face_vertex( iF, 2 ) );
}

template <typename type_face>
IC void set_face( type_face &iF, unsigned int v0, unsigned int v1, unsigned int v2 )
{
	face_vertex( iF, 0 ) 	= v0;
	face_vertex( iF, 1 )	= v1;
	face_vertex( iF, 2 )	= v2;
}


template<typename type_vertex, typename type_face>
static void fill_mender_input(	const	xr_vector<type_vertex>						&vertices,
								const	xr_vector<type_face>						&faces,
								xr_vector< MeshMender::Vertex >						&theVerts,
								xr_vector< unsigned int >							&theIndices
					)
{
	theVerts.clear();
	theIndices.clear();
	// fill inputs ( verts )
	const u32 vertices_number =  vertices.size();
	theVerts.resize( vertices_number );
	for ( u32 i = 0; i < vertices_number; ++i )
			set_vertex( theVerts[i], vertices[i] );
	// fill inputs ( indices )
	for ( xr_vector<type_face>::const_iterator face_it=faces.begin(); face_it!=faces.end(); face_it++ )
		add_face( theIndices, *face_it );

}

template<typename type_vertex, typename type_face>
static void retrive_data_from_mender_otput( xr_vector<type_vertex>					&vertices,//in-out
									 		xr_vector<type_face>					&faces, 
									 		const xr_vector< MeshMender::Vertex >	&theVerts,
									 		const xr_vector< unsigned int >			&theIndices,
									 		const xr_vector< unsigned int >			&mappingNewToOldVert
											)
{
	xr_vector<type_vertex>	old_vertices;
    {
      old_vertices.clear();
      old_vertices =  vertices;// save old vertices to retrive through mappingNewToOldVert data that missing in MeshMender::Vertex 
      // retriving data
      const u32 face_count = faces.size();
      for (u32 i = 0; i< face_count; ++i )
          set_face( faces[i], theIndices[ 3*i + 0 ],
                              theIndices[ 3*i + 1 ],
                              theIndices[ 3*i + 2 ]
          );
     }
    {
      const u32 vertex_count		= theVerts.size();
      vertices.clear	(); vertices.resize( vertex_count );
      for (u32 i=0; i < vertex_count; i++)
          set_vertex( vertices[i], old_vertices[ mappingNewToOldVert[i] ], theVerts[i] );
    }
	
}


#endif