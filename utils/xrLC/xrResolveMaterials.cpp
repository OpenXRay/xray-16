#include "stdafx.h"
#include "build.h"

vec2Face	g_XSplit;
vecFace		L_Adj_Space;

void __fastcall _ADJ_Split(Face *F) {
	if (F->bSplitted) return;
	F->bSplitted = TRUE;
	L_Adj_Space.push_back(F);

	// now iterate on all our neigbours
	for (int i=0; i<3; i++) 
	{
		for (vecFaceIt it=F->v[i]->adjacent.begin(); it!=F->v[i]->adjacent.end(); it++)
			if (F->RenderEqualTo(*it)) _ADJ_Split(*it);
	}
};

void Detach(vecFace* S)
{
	static map_v2v verts;

	// Collect vertices
	for (vecFaceIt F=S->begin(); F!=S->end(); F++)
	{
		for (int i=0; i<3; i++) {
			Vertex*		V=(*F)->v[i];
			Vertex*		VC;
			map_v2v_it	W=verts.find(V);	// iterator
			
			if (W==verts.end()) 
			{	// where is no such-vertex
				VC = V->CreateCopy_NOADJ();	// make copy
				verts.insert(mk_pair(V, VC));
			} else {
				// such vertex(key) already exists - update its adjacency
				VC = W->second;
			}
			VC->prep_add		(*F);
			V->prep_remove		(*F);
			(*F)->v[i]=VC;
		}
	}
	// vertices are already registered in container
	// so we doesn't need "vers" for this time
	verts.clear	();
}

void CBuild::ResolveMaterials()
{
	float	p_total = 0;
	float	p_cost  = .33f/(g_faces.size());

	Status("Subdividing by adjacement and materials...");
	g_XSplit.reserve	(4096);
	p_cost = .33f / g_faces.size();
	for (vecFaceIt F=g_faces.begin(); F!=g_faces.end(); F++)
	{
		if ( (*F)->bSplitted) continue;

		Progress(float(F-g_faces.begin())/float(g_faces.size()));

		// record splitting
		_ADJ_Split(*F);
		g_XSplit.push_back	(L_Adj_Space);
		L_Adj_Space.clear	();
	}
	clMsg	("%d subdivisions.",g_XSplit.size());

	Status	("Partial adjacement reduction...");
	Fbox	bb;	bb.invalidate();
	Fvector size;

	xr_vector<Fbox> bbox;
	xr_vector<int>	 id;

	float merge_lim = c_SS_maxsize*g_params.m_SS_merge_coeff;
	for (int X=0; X<int(g_XSplit.size()); X++)
	{
		// calc bounding box
		bb.invalidate();
		R_ASSERT(g_XSplit[X].size());
		for (vecFaceIt F=g_XSplit[X].begin(); F!=g_XSplit[X].end(); F++) 
		{
			Face *XF = *F;
			bb.modify(XF->v[0]->P);
			bb.modify(XF->v[1]->P);
			bb.modify(XF->v[2]->P);
		}
		bb.getsize		(size);
		bbox.push_back	(bb);
		if (size.y>merge_lim || size.x>merge_lim || size.z>merge_lim) continue;

		id.push_back	(X);
	}
	R_ASSERT(id.size()<=bbox.size());

	while (!id.empty())
	{
		int	  ID	= id	[0];
		R_ASSERT(ID<(int)bbox.size());
		R_ASSERT(ID<(int)g_XSplit.size());

		Fbox &BB	= bbox	[ID];
		R_ASSERT(!g_XSplit[ID].empty());
		Face* fBase = g_XSplit[ID][0];

		for (X=1; X<(int)id.size(); X++)
		{
			R_ASSERT(X>=1);

			int		guid	= id[X];
			R_ASSERT(guid<(int)g_XSplit.size());
			R_ASSERT(guid<(int)bbox.size());

			Face*	fTest	= g_XSplit[guid][0];
			if (!fBase->RenderEqualTo(fTest)) continue;

			bb.set		(BB);
			bb.merge	(bbox[guid]);
			bb.getsize	(size);
			if (size.x>merge_lim || size.y>merge_lim || size.z>merge_lim)	continue;
//			if (fTest->Shader().R.bStrictB2F)								continue;

			// ok, merge face lists
			BB.set	(bb);
			g_XSplit[ID].insert(
				g_XSplit[ID].end(),
				g_XSplit[guid].begin(),g_XSplit[guid].end()
				);
			g_XSplit[guid].clear();
			id.erase(id.begin()+X);
			X--;
		}

		// split #ID processed - erase it from list
		R_ASSERT(!id.empty());
		R_ASSERT(ID==id[0]);
		id.erase(id.begin());
	}

	Status("Removing empty subdivisions...");
	g_XMerge = g_XSplit;
	g_XSplit.clear	();
	g_XSplit.reserve(g_XMerge.size());
	for (X=0; X<int(g_XMerge.size()); X++)
	{
		if (!g_XMerge[X].empty	())	g_XSplit.push_back(g_XMerge[X]);
		g_XMerge[X].clear();
	}
	g_XMerge.clear	();

	Status("Subdividing in space...");
	vecFace s1, s2;
	Fbox	b1, b2;
	for (X=0; X<int(g_XSplit.size()); X++)
	{
		if (g_XSplit[X].empty()) {
			g_XSplit.erase(g_XSplit.begin()+X);
			X--;
			continue;
		}
//		clMsg("curent: %d, total: %d",X,g_XSplit.size());

		// skip if subdivision is too small already
		if (int(g_XSplit[X].size())<(c_SS_LowVertLimit*2))	continue;

		// calc bounding box
		Fbox	bb;
		Fvector size;

		bb.invalidate();
		for (vecFaceIt F=g_XSplit[X].begin(); F!=g_XSplit[X].end(); F++) 
		{
			Face *XF = *F;
			bb.modify(XF->v[0]->P);
			bb.modify(XF->v[1]->P);
			bb.modify(XF->v[2]->P);
		}

		// analyze bb size
		size.sub(bb.max,bb.min);
		if  (	(size.y>g_params.c_SS_maxsize) 
			||	(size.x>g_params.c_SS_maxsize)
			||	(size.z>g_params.c_SS_maxsize)
			||	(int(g_XSplit[X].size()) > c_SS_HighVertLimit)
			) 
		{
			// perform split
			b1.set(bb);	b2.set(bb);

			// select longest BBox edge
			if (size.x>=size.y && size.x>=size.z) {
				b1.max.x -= size.x/2;
				b2.min.x += size.x/2;
			} else 
			if (size.y>=size.x && size.y>=size.z) {
				b1.max.y -= size.y/2;
				b2.min.y += size.y/2;
			} else
			if (size.z>=size.x && size.z>=size.y) {
				b1.max.z -= size.z/2;
				b2.min.z += size.z/2;
			}

			// Process all faces and rearrange them
			for (vecFaceIt F=g_XSplit[X].begin(); F!=g_XSplit[X].end(); F++) 
			{
				Face *XF = *F;
				Fvector C;
				XF->CalcCenter(C);
				if (b1.contains(C))	{ s1.push_back(XF); }
				else				{ s2.push_back(XF); }
			}

			if ((int(s1.size())<c_SS_LowVertLimit) || (int(s2.size())<c_SS_LowVertLimit))
			{
				// splitting failed
			} else {
				// Delete old and push two new
				g_XSplit[X].clear	();
				g_XSplit.erase		(g_XSplit.begin()+X); X--;
				g_XSplit.push_back	(s1);
				g_XSplit.push_back	(s2);
			}
			s1.clear	();
			s2.clear	();
		}

		Progress			(.33f+.33f*float(X)/float(g_XSplit.size()));
	}
	clMsg("%d subdivisions.",g_XSplit.size());

	Status("Breaking down connectivity...");
	for (splitIt S=g_XSplit.begin(); S!=g_XSplit.end(); S++)
	{
		Detach		(S);
		Progress	(.66f+.33f*float(S-g_XSplit.begin())/float(g_XSplit.size()));
	}
}
