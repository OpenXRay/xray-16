#ifndef	_REMOVE_ISOLATED_VERTS_H_
#define _REMOVE_ISOLATED_VERTS_H_


template<typename type_vertex, typename type_face>
static void	add_face(	const	type_face					&F,
								xr_vector<type_vertex>		&new_vertices,
								xr_vector<type_face>		&new_faces,
						const	xr_vector<type_vertex>		&vertices,
								xr_vector< u32 >			&remap
					)
{
	type_face new_face;
	for (u32 v=0; v<3; v++)
	{
		u32				old_id	= face_vertex( F, v );
		u32				new_id	= remap[old_id];
		// Register new if not found
		if ( new_id == u32(-1) )
		{
			new_vertices	.push_back( vertices[old_id] );
			new_id			=new_vertices.size()-1;
			remap[old_id]	=new_id;
		}
		face_vertex( new_face, v ) = new_id;
	}
	new_faces	.push_back		( new_face );
}

template<typename type_vertex, typename type_face>
static void	t_remove_isolated_verts( xr_vector<type_vertex> &new_vertices, xr_vector<type_face> &new_faces, const xr_vector<type_vertex> &vertices, const xr_vector<type_face> &faces )
{
	new_vertices.clear();
	new_faces.clear();
	xr_vector< u32 >	remap;
	remap.resize ( vertices.size(), u32(-1) );
	for (u32 f=0; f<faces.size(); f++)
		add_face( faces[f], new_vertices, new_faces, vertices, remap );
	remap.clear	();
}

//static vecOGF_V			old_vertices;
//static vecOGF_F			old_faces;
template<typename type_vertex, typename type_face>
void	t_remove_isolated_verts( xr_vector<type_vertex> &vertices, xr_vector<type_face> &faces )
{
	xr_vector<type_vertex>			old_vertices;
	xr_vector<type_face>			old_faces;
	old_vertices		.clear();
	old_faces			.clear();

	old_vertices		= vertices;
	old_faces			= faces;

	t_remove_isolated_verts( vertices, faces, old_vertices, old_faces );

	old_vertices		.clear();
	old_faces			.clear();
}

#endif