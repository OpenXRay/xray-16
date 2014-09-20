#include "stdafx.h"
#include "../xrLC_Light/xrMU_Model.h"
#include "OGF_Face.h"

const u32	max_tile	= 16;
const s32	quant		= 32768/max_tile;

s16 QC	(float v)
{
	int t		=	iFloor(v*float(quant)); clamp(t,-32768,32767);
	return	s16	(t);
}

D3DVERTEXELEMENT9	decl[] = // 12+4+4+4+8=32
{
	{0, 0,  D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_POSITION,	0 },
	{0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_NORMAL,	0 },
	{0, 16, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_TANGENT,	0 },
	{0, 20, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_BINORMAL,	0 },
	{0, 24, D3DDECLTYPE_SHORT4,		D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_TEXCOORD,	0 },
	D3DDECL_END()
};

void	export_geometry		( xrMU_Model &	mu_model )
{
	// Declarator
	VDeclarator			D;
	D.set				(decl);

	// RT-check, BOX, low-point, frac-size
	Fbox			BB; 
	BB.invalidate	();
	for (xrMU_Model::v_vertices_it vit=mu_model.m_vertices.begin(); vit!=mu_model.m_vertices.end(); vit++)
		BB.modify	((*vit)->P);

	Fvector			frac_low;
	float			frac_Ysize;
	BB.getcenter	(frac_low);		frac_low.y	= BB.min.y;
	frac_Ysize		= BB.max.y - BB.min.y;

	// Begin building
	for (xrMU_Model::v_subdivs_it it=mu_model.m_subdivs.begin(); it!=mu_model.m_subdivs.end(); it++)
	{
		// Vertices
		{
			g_VB.Begin		(D);

			vecOGF_V&	verts	= it->ogf->data.vertices;
			for (u32 v_it=0; v_it<verts.size(); v_it++)
			{
				OGF_Vertex&		oV	= verts[v_it];

				// Position
				g_VB.Add	(&oV.P,3*sizeof(float));

				// Normal
				{
					base_color_c	oV_c;	oV.Color._get(oV_c);
					Fvector N	= oV.N;
					N.add		(1.f);
					N.mul		(.5f*255.f);
					s32 nx		= iFloor(N.x);				clamp(nx,0,255);
					s32 ny		= iFloor(N.y);				clamp(ny,0,255);
					s32 nz		= iFloor(N.z);				clamp(nz,0,255);
					s32 cc		= iFloor(oV_c.hemi*255.f);	clamp(cc,0,255);
					u32	uN		= color_rgba(nx,ny,nz,cc);
					g_VB.Add	(&uN,4);
				}

				// Tangent
				{
					u32	uT		= color_rgba(oV.T.x,oV.T.y,oV.T.z,0);
					g_VB.Add	(&uT,4);
				}

				// Binormal
				{
					u32	uB		= color_rgba(oV.B.x,oV.B.y,oV.B.z,0);
					g_VB.Add	(&uB,4);
				}

				// TC
				s16	tu,tv,frac,dummy;
				tu			= QC(oV.UV.begin()->x);
				tv			= QC(oV.UV.begin()->y);
				g_VB.Add	(&tu,2);
				g_VB.Add	(&tv,2);

				// frac
				float	f1	= (oV.P.y - frac_low.y)		/frac_Ysize;
				float	f2	= oV.P.distance_to(frac_low)/frac_Ysize;
				frac		= QC((f1+f2)/2.f);
				dummy		= 0;
				g_VB.Add	(&frac,	2);
				g_VB.Add	(&dummy,2);
			}

			g_VB.End		(&it->vb_id,&it->vb_start);
		}

		// Indices
		g_IB.Register	(LPWORD(&*it->ogf->data.faces.begin()),LPWORD(&*it->ogf->data.faces.end()),&it->ib_id,&it->ib_start);

		// SW
		if (it->ogf->progressive_test())
			g_SWI.Register	(&it->sw_id,&it->ogf->data.m_SWI);
	}
}
