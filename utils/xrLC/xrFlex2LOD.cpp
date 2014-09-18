#include "stdafx.h"

void	CBuild::Flex2LOD()
{
	int complete = g_tree.size();
	for (int it=0; it<int(lods.size()); it++)
	{
		Progress(float(it)/float(lods.size()));

		// Create Node and fill it with information
		b_lod&		LOD		= lods[it];
		OGF_LOD*	pNode	= xr_new<OGF_LOD> (1,pBuild->materials[LOD.dwMaterial].sector);
		pNode->lod_Material	= LOD.dwMaterial;
		for (int lf=0; lf<8; lf++)
		{
			b_lod_face&		F = LOD.faces[lf];
			OGF_LOD::_face& D = pNode->lod_faces[lf];
			for (int lv=0; lv<4; lv++)
			{
				D.v[lv].v	= F.v[lv];
				D.v[lv].t	= F.t[lv];
				D.v[lv].c	= 0xffffffff;
			}
		}

		// Search all 'OGFs' with such LOD-id
		for (int o=0; o<complete; o++)
		{
			OGF*	P		= (OGF*)g_tree[o];
			int		lod_id	= pBuild->materials[P->material].lod_id;
			if (it!=lod_id)	continue;
			pNode->AddChield(o);
		}

		// Register node
		R_ASSERT						(pNode->chields.size());
		pNode->CalcBounds				();
		g_tree.push_back				(pNode);

		// Calculate colors
		const float sm_range	= 5.f;
		for (int lf=0; lf<8; lf++)
		{
			OGF_LOD::_face& F = pNode->lod_faces[lf];
			for (int lv=0; lv<4; lv++)
			{
				Fvector	ptPos	= F.v[lv].v;
				u32&	ptColor	= F.v[lv].c;

				Fcolor	_C;		_C.set(0,0,0,0);
				float 	_N		= 0;

				for (u32 c_it=0; c_it<pNode->chields.size(); c_it++)
				{
					OGF*		ogf		= (OGF*)g_tree[pNode->chields[c_it]];
					vecOGF_V&	verts	= ogf->vertices;
					for (u32 v_it=0; v_it<verts.size(); v_it++)
					{
						Fcolor			vC; 
						OGF_Vertex&		oV	= verts[v_it];
						float			oD	= ptPos.distance_to(oV.P);
						float			oA  = 1/(1+100*oD*oD);
						vC.set			(oV.Color); 
						vC.mul_rgb		(oA);
						_C.r			+= vC.r;
						_C.g			+= vC.g;
						_C.b			+= vC.b;
						_N				+= oA;
					}
				}

				_C.mul_rgb		(1/(_N+EPS));
				ptColor			= _C.get();
			}
		}
	}
}
