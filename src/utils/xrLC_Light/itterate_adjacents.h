#ifndef _ITTERATE_ADJACENTS_H_
#define _ITTERATE_ADJACENTS_H_

template	<typename itterate_adjacents_params>
class itterate_adjacents
{

	typedef	typename itterate_adjacents_params::type_vertex		type_vertex;
	typedef	typename itterate_adjacents_params::type_face		type_face;
	
public:

typedef	itterate_adjacents_params recurse_tri_params;

static	void RecurseTri( u32	start_face_idx, recurse_tri_params &p  )
	{

		for ( u32 test_face_idx=0; test_face_idx<p.current_adjacents_size(); ++test_face_idx )
		{
			if( p.add_adjacents( start_face_idx, test_face_idx ) )
				RecurseTri		( test_face_idx, p );
		}
		
	}

};

#endif