#ifndef _ITTERATE_ADJACENTS_DYNAMIC_H_
#define _ITTERATE_ADJACENTS_DYNAMIC_H_

#include "../../../common/face_smoth_flags.h"
template<typename  typeVertex>
struct itterate_adjacents_params_dynamic
{
	typedef	typeVertex											type_vertex;
	typedef	typename typeVertex::type_face						type_face;
	//typedef xr_vector<type_face*>								vecFace;
	
private:
	 

	 Fvector				&normal;
	 const u32				*m_SmoothGroups;
	 const Fvector			*m_FaceNormals;
	 const IntVec			&a_lst;
	 const type_face		*m_Faces;
		   u32				m_FaceCount;
		   U8Vec			m_procesed;
public:
	itterate_adjacents_params_dynamic(
	
			 Fvector				&_normal,
			 const u32				*_SmoothGroups,
			 const Fvector			*_FaceNormals,
			 const IntVec			&_a_lst,
			 const type_face		*_Faces,
				   u32				_FaceCount
			):
	
				normal				(_normal),
				m_SmoothGroups		(_SmoothGroups),
				m_FaceNormals		(_FaceNormals),
				a_lst				(_a_lst),
				m_Faces				(_Faces),		
				m_FaceCount			( _FaceCount )

	{ 
		normal.set(0,0,0);
		m_procesed.resize( a_lst.size(), 0 ); 
	}
	IC const u32 current_adjacents_size( ) const
	{
		return u32( a_lst.size() );
	}

private:
	IC static	bool has_same_edge( const type_face* F1, const type_face* F2, u16 &F1_edge_index, u16 &F2_edge_index )
	{
		F1_edge_index = u16(-1);
		F2_edge_index = u16(-1);

		for (int e=0; e<3; e++)
		{
			type_vertex v1_a, v1_b;
			F1->EdgeVerts(e,v1_a,v1_b);	
			if ( v1_a.gt(v1_b) ) 
					swap(v1_a,v1_b);

			for (int r=0; r<3; ++r)
			{
				type_vertex v2_a, v2_b;
				F2->EdgeVerts(r,v2_a,v2_b);	
				if ( v2_a.gt(v2_b) ) 
					swap(v2_a,v2_b);

				if ((v1_a.eq(v2_a))&&(v1_b.eq(v2_b)))
				{
					F1_edge_index = e;
					F2_edge_index = r;
					return true;
				}
			}
		}
		return false;
	}
	IC u32  face_idx( u32 adj_idx ) const
	{
		VERIFY( adj_idx < a_lst.size() );
		return a_lst[adj_idx];
	}
	IC const type_face* current_adjacents_face( u32 adj_idx ) const
	{
		
		VERIFY( m_Faces );
		u32 index = face_idx( adj_idx );
		VERIFY( index < m_FaceCount );
		return &m_Faces[ index ];
	}

	IC  bool do_connect_faces( u32 start_face_idx, u32 test_face_idx, u16 start_common_edge_idx, u16 test_common_edge_idx )
	{
		VERIFY( start_face_idx < m_FaceCount );
		VERIFY( test_face_idx < m_FaceCount );
		return do_connect_faces_by_faces_edge_flags( m_SmoothGroups[start_face_idx], m_SmoothGroups[test_face_idx], start_common_edge_idx, test_common_edge_idx ); 
	}

	IC bool is_processed( u32 adj_idx ) const
	{
		VERIFY( adj_idx < a_lst.size() );
		return !!m_procesed[adj_idx];
	}

	IC void set_processed( u32 adj_idx )
	{
		VERIFY( adj_idx < a_lst.size() );
		m_procesed[adj_idx] = u8(1);
	}

	IC void add_adjacents(  u32 test_face_idx )
	{
		VERIFY( test_face_idx < m_FaceCount );
		
		normal.add( m_FaceNormals[ test_face_idx ] );
	}
public:

	IC bool add_adjacents( u32 start_face_adj_idx, u32 test_face_adj_idx )
	{
		if( is_processed( test_face_adj_idx ) )
				return false;
		
		const	type_face *start_face	= current_adjacents_face( start_face_adj_idx );
		const	type_face *test_face	= current_adjacents_face( test_face_adj_idx );

		u32		test_face_idx = face_idx( test_face_adj_idx );
		u32		start_face_idx = face_idx( start_face_adj_idx );



		u16 StartFace_common_edge_index = u16(-1);
		u16 TestFace_common_edge_index = u16(-1);

		if ( has_same_edge( start_face, test_face, StartFace_common_edge_index, TestFace_common_edge_index ) )
		{
			if ( (start_face_idx==test_face_idx) || do_connect_faces( start_face_idx, test_face_idx, StartFace_common_edge_index, TestFace_common_edge_index ) )
			{
				set_processed( test_face_adj_idx );
				add_adjacents( test_face_idx );
				return true;
			}
		}
		return false;
	}
};

#endif