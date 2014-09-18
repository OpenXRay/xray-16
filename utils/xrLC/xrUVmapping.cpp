#include "stdafx.h"
#include "build.h"



IC	void GetVIdxByEdge(int edge, int &e1, int &e2, int &v)
{
	e1 = edge2idx3[edge][0];
	e2 = edge2idx3[edge][1];
	v  = edge2idx3[edge][2];
}

void Face::OA_Unwarp()
{
	if (pDeflector)					return;
	if (!Deflector->OA_Place(this))	return;

	// now iterate on all our neigbours
	for (int i=0; i<3; i++) 
		for (vecFaceIt it=v[i]->adjacent.begin(); it!=v[i]->adjacent.end(); it++)
			(*it)->OA_Unwarp();
}

extern void Detach				(vecFace* S);

void CBuild::BuildUVmap()
{
	// Take a copy of g_XSplit;
	g_XMerge = g_XSplit;
	
	// Main loop
	Status("Processing...");
	float p_cost	= 1.f / float(g_XSplit.size());
	float p_total	= 0.f;
	for (int SP = 0; SP<int(g_XSplit.size()); SP++) 
	{
		Progress(p_total+=p_cost);
		
		// Detect vertex-lighting and avoid this subdivision
		R_ASSERT(!g_XSplit[SP].empty());
		Face*		Fvl = g_XSplit[SP][0];
		if (Fvl->Shader().flags.bLIGHT_Vertex) 	continue;	// do-not touch (skip)
		if (Fvl->hasImplicitLighting())			continue;

		//   find first poly that doesn't has mapping and start recursion
		while (TRUE) {
			// Select maximal sized poly
			Face *	msF		= NULL;
			float	msA		= 0;
			for (vecFaceIt it = g_XSplit[SP].begin(); it!=g_XSplit[SP].end(); it++)
			{
				if ( (*it)->pDeflector == NULL ) {
					float a = (*it)->CalcArea();
					if (a>msA) {
						msF = (*it);
						msA = a;
					}
				}
			}
			if (msF) {
				g_deflectors.push_back(xr_new<CDeflector>());
				
				// Start recursion from this face
				Deflector->OA_SetNormal(msF->N);
				msF->OA_Unwarp();
				
				// break the cycle to startup again
				Deflector->OA_Export();
				
				// Detach affected faces
				static vecFace faces_affected;
				for (int i=0; i<int(g_XSplit[SP].size()); i++) {
					Face *F = g_XSplit[SP][i];
					if ( F->pDeflector==Deflector ) {
						faces_affected.push_back(F);
						g_XSplit[SP].erase(g_XSplit[SP].begin()+i);
						i--;
					}
				}
				
				// detaching itself
				Detach(&faces_affected);
				
				g_XSplit.push_back(faces_affected);
				faces_affected.clear();
			} else {
				if (g_XSplit[SP].empty()) 
				{
					g_XSplit.erase(g_XSplit.begin()+SP);
					SP--;
				}
				// Cancel infine loop (while)
				break;
			}
		}
	}
	clMsg("%d subdivisions...",g_XSplit.size());
}
