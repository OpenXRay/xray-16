#include "stdafx.h"
#include "build.h"

Vertex*	tessEdge(Face *F, int id1, int id2)
{
	// Get vertices
	Vertex *V1 = F->v[id1];
	Vertex *V2 = F->v[id2];

	Fplane	p1,p2;
	Fvector t1,t2;
	Fvector pr1,pr2;

	// project
	p1.build(V1->P,V1->N); p1.project(pr2,V2->P); // projected point 2 on plane 1
	p2.build(V2->P,V2->N); p2.project(pr1,V1->P); // projected point 1 on plane 2
	
	t1.sub(pr2,V1->P); t1.mul(.5f); t1.add(V1->P);
	t2.sub(pr1,V2->P); t2.mul(.5f); t2.add(V2->P);

	Vertex* V	= VertexPool.create();
	V->P.add	(t1,t2);
	V->P.mul	(.5f);
	V->N.set	(0,0,0);
	return	V;
}

void Face::TessMark(int E)
{
	if (TESS_MASK & TESS_EDGE(E)) return;
	TESS_MASK |= TESS_EDGE(E);

	// Now we have to mark faces that are adjacent to this edge
	Vertex* V1 = v[edge2idx[E][0]];
	Vertex* V2 = v[edge2idx[E][1]];

	// !!! Recursion takes place here !!!
	for (adjIt it=V1->adjacent.begin(); it!=V1->adjacent.end(); it++)
	{
		Face* F = (*it);
		if (F->VContains(V2)) {
			// Face shares edge E(V1..V2)
			F->TessMark(idx2edge[F->VIndex(V1)][F->VIndex(V2)]);
		}
	}

	// Check if 2 edges are marked - if so - mark third too
	if ((TESS_MASK & TESS_EDGE_0)&&(TESS_MASK & TESS_EDGE_1)&& !(TESS_MASK & TESS_EDGE_2)) TessMark(2);
	if ((TESS_MASK & TESS_EDGE_0)&&(TESS_MASK & TESS_EDGE_2)&& !(TESS_MASK & TESS_EDGE_1)) TessMark(1);
	if ((TESS_MASK & TESS_EDGE_1)&&(TESS_MASK & TESS_EDGE_2)&& !(TESS_MASK & TESS_EDGE_0)) TessMark(0);
}

void CBuild::Tesselate()
{
	u32	Fcount	= g_faces.size();
	u32	Tcount  = 0;

	Status("Calculating candidates...");
	for (int fi=0; fi<Fcount; fi++)
	{
		Face* F = g_faces[fi];
		for (int E=0; E<3; E++)
		{
			if (F->TESS_MASK & TESS_EDGE(E)) continue;
			float len = F->EdgeLen(E);
			if (len>g_params.m_maxedge) {
				// Need to tesselate this edge
				F->TessMark(E);
			}
		}

		Progress(float(fi)/float(Fcount));
	}

	Status("Tesselating...");
	for (fi=0; fi<Fcount; fi++)
	{
		Face* F = g_faces[fi];

		if ((F->TESS_MASK&TESS_EDGE_ALL)==TESS_EDGE_ALL) {
			// Tesselate face
			Tcount++;
			Vertex*	V1	= tessEdge( F, 0, 1 );
			Vertex*	V2	= tessEdge( F, 1, 2 );
			Vertex*	V3	= tessEdge( F, 2, 0 );

			Face*	F0		= FacePool.create();	
			F0->dwRMode		= F->dwRMode; 
			F0->dwMaterial	= F->dwMaterial;

			Face*	F1		= FacePool.create();
			F1->dwRMode		= F->dwRMode;
			F1->dwMaterial	= F->dwMaterial;

			Face*	F2		= FacePool.create();
			F2->dwRMode		= F->dwRMode;
			F2->dwMaterial	= F->dwMaterial;

			Face*	F3		= FacePool.create();
			F3->dwRMode		= F->dwRMode;
			F3->dwMaterial	= F->dwMaterial;

			F0->SetVertices(V1,V2,V3);			
			F1->SetVertices(F->v[0], V1, V3);
			F2->SetVertices(F->v[1], V2, V1);
			F3->SetVertices(F->v[2], V3, V2);

			xr_vector<_TCF>::iterator it=F->tc.begin();
			for (;it!=F->tc.end();it++)
			{
				Fvector2 p1,p2,p3;	// for V1,V2,V3

				p1.averageA(it->uv[0], it->uv[1]); // edge 0-1
				p2.averageA(it->uv[1], it->uv[2]); // edge 1-2
				p3.averageA(it->uv[2], it->uv[0]); // edge 2-0

				F0->AddChannel(p1,p2,p3);
				F1->AddChannel(it->uv[0], p1, p3);
				F2->AddChannel(it->uv[1], p2, p1);
				F3->AddChannel(it->uv[2], p3, p2);
			}

			FacePool.destroy(F);
			fi--;
		} else {
			if (F->TESS_MASK & TESS_EDGE_0) {
			}
		}
		Progress(float(fi)/float(Fcount));
	}
	clMsg("Faces tesselated: %d",Tcount);
	clMsg("Was %d, now %d",Fcount,g_faces.size());
}
