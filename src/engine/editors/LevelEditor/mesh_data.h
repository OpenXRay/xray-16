#ifndef	_MESH_DATA_H_
#define	_MESH_DATA_H_
namespace CDB{ struct TRI; }
struct	mesh_build_data
{
	int		   l_vert_cnt , l_vert_it ;
	int		   l_face_cnt , l_face_it ;
	Fvector*   l_verts ;
	CDB::TRI*  l_faces ;

    mesh_build_data():l_verts(0),l_faces(0),l_vert_cnt(0),l_vert_it(0),l_face_cnt(0),l_face_it(0){ }
    ~mesh_build_data(){ R_ASSERT(!l_verts); R_ASSERT(!l_faces); }
};

#endif