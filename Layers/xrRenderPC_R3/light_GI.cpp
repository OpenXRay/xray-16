#include "StdAfx.h"
#include "../xrRender/light.h"

IC bool		pred_LI			(const light_indirect& A, const light_indirect& B)
{
	return A.E > B.E;
}

void	light::gi_generate	()
{
	indirect.clear		();
	indirect_photons	= ps_r2_ls_flags.test(R2FLAG_GI)?ps_r2_GI_photons:0;

	CRandom				random;
	random.seed			(0x12071980);

	xrXRC&		xrc		= RImplementation.Sectors_xrc;
	CDB::MODEL*	model	= g_pGameLevel->ObjectSpace.GetStaticModel	();
	CDB::TRI*	tris	= g_pGameLevel->ObjectSpace.GetStaticTris	();
	Fvector*	verts	= g_pGameLevel->ObjectSpace.GetStaticVerts	();
	xrc.ray_options		(CDB::OPT_CULL|CDB::OPT_ONLYNEAREST);

	for (int it=0; it<int(indirect_photons*8); it++)	{
		Fvector	dir,idir;
		switch	(flags.type)		{
		case IRender_Light::POINT		:	dir.random_dir(random);					break;
		case IRender_Light::SPOT		:	dir.random_dir(direction,cone,random);	break;
		case IRender_Light::OMNIPART	:	dir.random_dir(direction,cone,random);	break;
		}
		dir.normalize		();
		xrc.ray_query		(model,position,dir,range);
		if (!xrc.r_count()) continue;
		CDB::RESULT *R		= RImplementation.Sectors_xrc.r_begin	();
		CDB::TRI&	T		= tris[R->id];
		Fvector		Tv[3]	= { verts[T.verts[0]],verts[T.verts[1]],verts[T.verts[2]] };
		Fvector		TN;		TN.mknormal		(Tv[0],Tv[1],Tv[2]);
		float		dot		= TN.dotproduct	(idir.invert(dir));

		light_indirect		LI;
		LI.P.mad			(position,dir,R->range);
		LI.D.reflect		(dir,TN);
		LI.E				= dot * (1-R->range/range);
		if (LI.E < ps_r2_GI_clip)				continue;
		LI.S				= spatial.sector;	//. BUG

		indirect.push_back	(LI);
	}

	// sort & clip
	std::sort				(indirect.begin(),indirect.end(), pred_LI);
	if (indirect.size()>indirect_photons)
		indirect.erase(indirect.begin()+indirect_photons,indirect.end());

	// normalize
	if (indirect.size())	{
		float	target_E		=	ps_r2_GI_refl;
		float	total_E			=	0;
		for (u32 it=0; it<indirect.size(); it++)
			total_E				+=	indirect[it].E;
		float	scale_E			=	target_E/total_E;
		for (u32 it=0; it<indirect.size(); it++)
			indirect[it].E		*=	scale_E;
	}
}
