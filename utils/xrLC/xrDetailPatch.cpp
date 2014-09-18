#include "stdafx.h"
#include "build.h"

void LightPatch(Fcolor &C, Fvector &P, float scale)
{
	C.set(0,0,0,0);
	b_light* L = pBuild->lights_lmaps.begin();
	for (;L!=pBuild->lights_lmaps.end(); L++)
	{
		if (L->type==D3DLIGHT_DIRECTIONAL) {
			// Cos
			Fvector		Ldir;
			Ldir.invert	(L->direction);

			// Raypick
			XRC.RayPick(0,&RCAST_Model,P,Ldir,1000.f);
			if (XRC.GetRayContactCount()) continue;

			// Light
			C.r += scale * L->diffuse.r; 
			C.g += scale * L->diffuse.g;
			C.b += scale * L->diffuse.b;
		} else {
			// Distance
			float sqD	= P.distance_to_sqr(L->position);
			if (sqD > L->range*L->range) continue;
			
			// Dir
			Fvector		Ldir;
			Ldir.sub	(L->position,P);
			Ldir.normalize_safe();
			
			// Raypick
			float R		= sqrtf(sqD);
			XRC.RayPick(0,&RCAST_Model,P,Ldir,R);
			if (XRC.GetRayContactCount()) continue;
			
			// Light
			float A		= 1 / (L->attenuation0 + L->attenuation1*R + L->attenuation2*sqD);
			
			C.r += scale * A * L->diffuse.r;
			C.g += scale * A * L->diffuse.g;
			C.b += scale * A * L->diffuse.b;
		}
	}
}

extern xr_vector<DetailPatch>	g_pathes;

float g_merge_limit = 6.f;

Fvector	sort_key;
IC bool sort_pred(const DetailPatch& P1, const DetailPatch& P2)
{
	float d1 = sort_key.distance_to_sqr(P1.P);
	float d2 = sort_key.distance_to_sqr(P2.P);
	return d1<d2;
}

void CBuild::LightPatches()
{
	Status("Raytracing...");
	for (u32 i=0; i<g_pathes.size(); i++)
	{
		Fcolor C,Lumel;
		LightPatch(C,g_pathes[i].P,.7f);

		g_pathes[i].color	= Ñ.get();

		if (0 == i%32)		Progress(float(i)/float(g_pathes.size()));
	}

	Status("Subdividing...");
	xr_vector<DetailPatch> one_subd;
	while (!g_pathes.empty())
	{
		// Prepare
		one_subd.push_back(g_pathes.front());
		g_pathes.erase(g_pathes.begin());
		sort_key.set(one_subd.front().P);
		std::sort(g_pathes.begin(),g_pathes.end(),sort_pred);

		// Search patches with identical materials until size limit
		Fbox bb,bbtest; 
		Fvector	size;

		bb.invalidate(); 
		bb.modify(one_subd.front().P);
		for (i=0; i<g_pathes.size(); i++)
		{
			if (g_pathes[i].dwMaterial != one_subd.front().dwMaterial) continue;

			bbtest.set		(bb);
			bbtest.modify	(g_pathes[i].P);
			bbtest.getsize	(size);
			if (size.x>g_merge_limit || size.y>g_merge_limit || size.z>g_merge_limit) continue;

			// all tests OK :)
			bb.set(bbtest);
			one_subd.push_back(g_pathes[i]);
			g_pathes.erase(g_pathes.begin()+i);
			i--;
		}

		// Convert to OGF
		OGF_Patch* pOGF = xr_new<OGF_Patch> (one_subd);
		pOGF->treeID	= g_tree.size();
		pOGF->Sector	= materials[one_subd[0].dwMaterial].sector;
		pOGF->CalcBounds();
		g_tree.push_back(pOGF);
		one_subd.clear();
	}
}
