#include "stdafx.h"
//#include "build.h"
#include "mu_model_face.h"
#include "calculate_normals.h"
#include "xrMU_Model.h"

#include "../../xrcore/xrPool.h"

//#include "build.h"

poolSS<_vertex,8*1024>	&mu_vertices_pool();

void destroy_vertex( _vertex* &v, bool unregister )
{
	mu_vertices_pool().destroy(v);
	v = NULL;
}

void calc_normals( xrMU_Model &model )
{
	calculate_normals<_vertex>::calc_normals( model.m_vertices, model.m_faces );
}


/*
void xrMU_Model::calc_normals()
{
	u32		Vcount	= (u32)m_vertices.size();
	float	p_total = 0;
	float	p_cost  = 1.f/(Vcount);

	// Clear temporary flag
//.	float	sm_cos	= _cos(deg2rad(g_params.m_sm_angle));
	float	sm_cos	= _cos(deg2rad(89.f));

	for (v_faces_it it = m_faces.begin(); it!=m_faces.end(); it++)
	{
		(*it)->flags.bSplitted	= FALSE;
		(*it)->CalcNormal		();
	}

	// remark:
	//	we use Face's bSplitted value to indicate that face is processed
	//  so bSplitted means bUsed
	for (u32 I=0; I<Vcount; I++)
	{
		_vertex* V	= m_vertices[I];

		for (v_faces_it AFit = V->m_adjacents.begin(); AFit != V->m_adjacents.end(); AFit++)
		{
			_face*	F			= *AFit;
			F->flags.bSplitted	= FALSE;
		}
		std::sort	(V->m_adjacents.begin(), V->m_adjacents.end());

		for (u32 AF = 0; AF < V->m_adjacents.size(); AF++)
		{
			_face*	F				= V->m_adjacents[AF];
			if (F->flags.bSplitted)	continue;	// Face already used in calculation

			// Create new vertex (except adjacency)
			_vertex*	NV		= mu_vertices.create();
			NV->P				= V->P;

			// Calculate it's normal
			NV->N.set	(0,0,0);
			for (u32 NF = 0; NF < V->m_adjacents.size(); NF++)
			{
				_face*	Fn		= V->m_adjacents[NF];

				float	cosa	= F->N.dotproduct(Fn->N);
				if (cosa>sm_cos) 
				{
					NV->N.add		(Fn->N);
					if (!Fn->flags.bSplitted) {
						Fn->VReplace_not_remove	(V,NV);
						Fn->flags.bSplitted		= true;
					}
				}
			}

			if (NV->m_adjacents.empty()) mu_vertices.destroy(NV);
			else {
				NV->N.normalize_safe();
				m_vertices.push_back(NV);
			}
		}
	}

	// Destroy unused vertices
	for (u32 I=0; I<Vcount; I++) mu_vertices.destroy	(m_vertices[I]);
	m_vertices.erase(m_vertices.begin(),m_vertices.begin()+Vcount);

	// Destroy unused vertices
	{
		for (int I=0; I<int(m_vertices.size()); I++) 
			if (m_vertices[I]->m_adjacents.empty())
			{
				mu_vertices.destroy	(m_vertices[I]);
				m_vertices.erase	(m_vertices.begin()+I);
				I--;
			}
	}

	// Clear temporary flag
	for (v_faces_it it = m_faces.begin(); it!=m_faces.end(); it++)
		(*it)->flags.bSplitted = FALSE;

	clMsg("%5s %d vertices duplicated","-",m_vertices.size()-Vcount);
}
*/
