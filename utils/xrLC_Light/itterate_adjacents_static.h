#ifndef _ITTERATE_ADJACENTS_STATIC_H_
#define _ITTERATE_ADJACENTS_STATIC_H_
#include "../../common/face_smoth_flags.h"





template<typename  typeVertex>
struct itterate_adjacents_params_static
{
	typedef	typeVertex											type_vertex;
	typedef	typename typeVertex::type_face						type_face;
	typedef xr_vector<type_face*>								vecFace;

private:
	 
	 const	type_vertex*	pTestVertex;
			vecFace&		new_adj_vec;
	 const	float			sm_cos;
public:
	itterate_adjacents_params_static(
			const	type_vertex*	_pTestVertex,
			vecFace&				_new_adj_vec,
			float					_sm_cos
			):
		pTestVertex	(_pTestVertex)		,
		new_adj_vec	(_new_adj_vec)		,
		sm_cos		(_sm_cos)
	{}

private:
	IC static	bool has_same_edge(const type_face* F1, const type_face* F2, u16 &F1_edge_index, u16 &F2_edge_index )
	{
		F1_edge_index = u16(-1);
		F2_edge_index = u16(-1);

		for (int e=0; e<3; e++)
		{
			type_vertex *v1_a, *v1_b;
			F1->EdgeVerts(e,&v1_a,&v1_b);	
			if (v1_a>v1_b) 
				swap(v1_a,v1_b);

			for (int r=0; r<3; ++r)
			{
				type_vertex *v2_a, *v2_b;
				F2->EdgeVerts(r,&v2_a,&v2_b);	
				if (v2_a>v2_b) 
					swap(v2_a,v2_b);

				if ((v1_a==v2_a)&&(v1_b==v2_b))
				{
					F1_edge_index = e;
					F2_edge_index = r;
					return true;
				}
			}
		}
		return false;
	}


IC static bool do_connect_faces( const type_face &start, const type_face &test, u16 start_common_edge_idx, u16 test_common_edge_idx, float sm_cos )
	{
		if( g_using_smooth_groups )
		{
			if( g_smooth_groups_by_faces )	
				return ( start.sm_group != u32(-1) && 
						start.sm_group == test.sm_group );
			else
				return do_connect_faces_by_faces_edge_flags( start.sm_group, test.sm_group, start_common_edge_idx, test_common_edge_idx ); 
		}
		else
		{
			float	cosa = start.N.dotproduct(test.N);
			return (  cosa>sm_cos ) ;
		}
	}

public:


	IC const u32 current_adjacents_size( ) const
	{
		VERIFY(pTestVertex);
		return u32( pTestVertex->m_adjacents.size() );
	}

	IC type_face* current_adjacents_face( u32 i ) const
	{
		VERIFY( pTestVertex );
		return pTestVertex->m_adjacents[i];
	}

	IC bool is_processed( const type_face &f ) const
	{
		 return f.flags.bSplitted;
	}

	IC bool add_adjacents( u32 start_face_idx, u32 test_face_idx )
	{
		const	type_face *start_face	= current_adjacents_face( start_face_idx );
				type_face *test_face	= current_adjacents_face( test_face_idx );
		if(is_processed(*test_face))
				return false;
		u16 StartFace_common_edge_index = u16(-1);
		u16 TestFace_common_edge_index = u16(-1);
		if ( has_same_edge( start_face, test_face, StartFace_common_edge_index, TestFace_common_edge_index ) )
		{
			if ( (start_face==test_face) || do_connect_faces( *start_face, *test_face, StartFace_common_edge_index, TestFace_common_edge_index, sm_cos ) )
			{
				new_adj_vec.push_back		( test_face ); 
				test_face->flags.bSplitted	= true;
				return true;
			}
		}
		return false;
	}

};




#endif