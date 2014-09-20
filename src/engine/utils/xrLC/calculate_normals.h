#ifndef __CALCULATE_NORMALS_H__
#define __CALCULATE_NORMALS_H__

#include "itterate_adjacents_static.h"
#include "../../common/itterate_adjacents.h"

template	<typename typeVertex>
class calculate_normals
{

	typedef	typeVertex											type_vertex;
	typedef	calculate_normals<typeVertex>						type_self;
	typedef	typename typeVertex::type_face						type_face;
	//these typedefs to hide global typedefs!!!
	typedef xr_vector<type_vertex*>								vecVertex;
	typedef typename vecVertex::iterator						vecVertexIt;
	typedef xr_vector<type_face*>								vecFace;
	typedef typename vecFace::iterator							vecFaceIt;

	typedef vecFace												vecAdj;
	typedef typename vecAdj::iterator							vecAdjIt;
private:	

typedef  itterate_adjacents< itterate_adjacents_params_static<type_vertex> > itterate_adjacents_type;

public:
static void	calc_normals( vecVertex &vertices, vecFace &faces )
{
	
	u32		Vcount	= vertices.size();
	float	p_total = 0;
	float	p_cost  = 1.f/(Vcount);

	// Clear temporary flag
	Status			("Processing...");
	float sm_cos	= _cos(deg2rad(g_params().m_sm_angle));

	for (vecFaceIt it = faces.begin(); it!=faces.end(); it++)
	{
		(*it)->flags.bSplitted	= true;
		(*it)->CalcNormal		();
	}

	// remark:
	//	we use Face's bSplitted value to indicate that face is processed
	//  so bSplitted means bUsed
	for (u32 I=0; I<Vcount; I++)
	{
		type_vertex* pTestVertex = vertices[I];
		for (vecAdjIt AFit = pTestVertex->m_adjacents.begin(); AFit!=pTestVertex->m_adjacents.end(); ++AFit)
		{
			type_face*	F					= *AFit;
			F->flags.bSplitted			= false;
		}
		std::sort( pTestVertex->m_adjacents.begin(), pTestVertex->m_adjacents.end() );

		while ( pTestVertex->m_adjacents.size() )	
		{
			vecFace new_adj;
			itterate_adjacents_type::recurse_tri_params p( pTestVertex, new_adj, sm_cos );
			itterate_adjacents_type::RecurseTri( 0, p );// pTestVertex, new_adj, sm_cos );
			VERIFY( !new_adj.empty() );

			type_vertex*	pNewVertex			= pTestVertex->CreateCopy_NOADJ( vertices );

			for (u32 a=0; a<new_adj.size(); ++a)
			{
				type_face* test		= new_adj[a];
				test->VReplace	( pTestVertex, pNewVertex );
			}

			pNewVertex->normalFromAdj	();
		}
		Progress( p_total+=p_cost );
	}
	Progress		( 1.f );

	// Destroy unused vertices

	isolate_vertices<type_vertex>( FALSE, vertices );

	// Recalculate normals
	for ( vecVertexIt it=vertices.begin(); it!=vertices.end(); it++ )
		(*it)->normalFromAdj	();

	clMsg	("%d vertices was duplicated 'cause of SM groups",vertices.size()-Vcount);

	// Clear temporary flag
	for ( vecFaceIt it = faces.begin(); it!=faces.end(); it++ )
		(*it)->flags.bSplitted = false;
}
};
#endif //__CALCULATE_NORMALS_H__