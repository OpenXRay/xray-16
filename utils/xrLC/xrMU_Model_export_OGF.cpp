#include "stdafx.h"


#include "../xrLC_Light/xrMU_Model.h"
#include "../xrLC_Light/xrMU_Model_Reference.h"

#include "build.h"
#include "OGF_Face.h"

#define	TRY(a) try { a; } catch (...) { clMsg("* E: %s", #a); }

void export_ogf( xrMU_Reference& mu_reference )
{
	xr_vector<u32>		generated_ids;
	xrMU_Model *model = mu_reference.model;
	// Export nodes
	{
		for (xrMU_Model::v_subdivs_it it=model->m_subdivs.begin(); it!=model->m_subdivs.end(); it++)
		{
			OGF_Reference*	pOGF	= xr_new<OGF_Reference> ();
			b_material*		M		= &(pBuild->materials()[it->material]);	// and it's material
			R_ASSERT		(M);

			// Common data
			pOGF->Sector			= mu_reference.sector;
			pOGF->material			= it->material;

			// Collect textures
			OGF_Texture				T;
			TRY(T.name			= pBuild->textures()[M->surfidx].name);
			TRY(T.pBuildSurface	= &(pBuild->textures()[M->surfidx]));
			TRY(pOGF->textures.push_back(T));

			// Special
			pOGF->model				= it->ogf;
			pOGF->vb_id				= it->vb_id;
			pOGF->vb_start			= it->vb_start;
			pOGF->ib_id				= it->ib_id;
			pOGF->ib_start			= it->ib_start;
			pOGF->xform.set			(mu_reference.xform);
			pOGF->c_scale			= mu_reference.c_scale;
			pOGF->c_bias			= mu_reference.c_bias;
			pOGF->sw_id				= it->sw_id;

			pOGF->CalcBounds		();
			generated_ids.push_back	((u32)g_tree.size());
			g_tree.push_back		(pOGF);
		}
	}

	// Now, let's fuck with LODs
	if (u16(-1) == model->m_lod_ID)	return;
	{
		// Create Node and fill it with information
		b_lod&		LOD		= pBuild->lods	[model->m_lod_ID];
		OGF_LOD*	pNode	= xr_new<OGF_LOD> (1,mu_reference.sector);
		pNode->lod_Material	= LOD.dwMaterial;
		for (int lf=0; lf<8; lf++)
		{
			b_lod_face&		F = LOD.faces[lf];
			OGF_LOD::_face& D = pNode->lod_faces[lf];
			for (int lv=0; lv<4; lv++)
			{
				mu_reference.xform.transform_tiny(D.v[lv].v,F.v[lv]);
				D.v[lv].t			= F.t[lv];
				D.v[lv].c_rgb_hemi	= 0xffffffff;
				D.v[lv].c_sun		= 0xff;
			}
		}

		// Add all 'OGFs' with such LOD-id
		for (u32 o=0; o<generated_ids.size(); o++)
			pNode->AddChield(generated_ids[o]);

		// Register node
		R_ASSERT						(pNode->chields.size());
		pNode->CalcBounds				();
		g_tree.push_back				(pNode);

		// Calculate colors
		const float sm_range		= 5.f;
		for (int lf=0; lf<8; lf++)
		{
			OGF_LOD::_face& F = pNode->lod_faces[lf];
			for (int lv=0; lv<4; lv++)
			{
				Fvector	ptPos	= F.v[lv].v;

				base_color_c	_C;
				float 			_N	= 0;

				for (u32 v_it=0; v_it<model->m_vertices.size(); v_it++)
				{
					// get base
					Fvector			baseP;	mu_reference.xform.transform_tiny	(baseP,model->m_vertices[v_it]->P);
					base_color_c	baseC;	mu_reference.color[v_it]._get(baseC);

					base_color_c	vC;
					float			oD	= ptPos.distance_to	(baseP);
					float			oA  = 1/(1+100*oD*oD);
					vC = 			(baseC);
					vC.mul			(oA);
					_C.add			(vC);
					_N				+= oA;
				}

				float	s			= 1/(_N+EPS);
				_C.mul				(s);
				F.v[lv].c_rgb_hemi	= color_rgba(u8_clr(_C.rgb.x),u8_clr(_C.rgb.y),u8_clr(_C.rgb.z),u8_clr(_C.hemi));
				F.v[lv].c_sun		= u8_clr	(_C.sun);
			}
		}
	}
}
