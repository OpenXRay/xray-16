#include "stdafx.h"
#include "build.h"
#include "../xrLC_Light/xrLC_GlobalData.h"
#include "../xrLC_Light/xrface.h"

extern void		Detach		(vecFace* S);

struct _counter
{
	u16	dwMaterial;
	u32	dwCount;
};

void	CBuild::xrPhase_ResolveMaterials()
{
	// Count number of materials
	Status		("Calculating materials/subdivs...");
	xr_vector<_counter>	counts;
	{
		counts.reserve		(256);
		for (vecFaceIt F_it=lc_global_data()->g_faces().begin(); F_it!=lc_global_data()->g_faces().end(); F_it++)
		{
			Face*	F			= *F_it;
			BOOL	bCreate		= TRUE;
			for (u32 I=0; I<counts.size(); I++)
			{
				if (F->dwMaterial == counts[I].dwMaterial)
				{
					counts[I].dwCount	+= 1;
					bCreate				= FALSE;
					break;
				}
			}
			if (bCreate)	{
				_counter	C;
				C.dwMaterial	= F->dwMaterial;
				C.dwCount		= 1;
				counts.push_back(C);
			}
			Progress(float(F_it-lc_global_data()->g_faces().begin())/float(lc_global_data()->g_faces().size()));
		}
	}
	
	Status				("Perfroming subdivisions...");
	{
		g_XSplit.reserve(64*1024);
		g_XSplit.resize	(counts.size());
		for (u32 I=0; I<counts.size(); I++) 
		{
			g_XSplit[I] = xr_new<vecFace> ();
			g_XSplit[I]->reserve	(counts[I].dwCount);
		}
		
		for (vecFaceIt F_it=lc_global_data()->g_faces().begin(); F_it!=lc_global_data()->g_faces().end(); F_it++)
		{
			Face*	F							= *F_it;
			if (!F->Shader().flags.bRendering)	continue;

			for (u32 I=0; I<counts.size(); I++)
			{
				if (F->dwMaterial == counts[I].dwMaterial)
				{
					g_XSplit[I]->push_back	(F);
				}
			}
			Progress(float(F_it-lc_global_data()->g_faces().begin())/float(lc_global_data()->g_faces().size()));
		}
	}

	Status				("Removing empty subdivs...");
	{
		for (int SP = 0; SP<int(g_XSplit.size()); SP++) 
			if (g_XSplit[SP]->empty())	xr_delete(g_XSplit[SP]);
		g_XSplit.erase(std::remove(g_XSplit.begin(),g_XSplit.end(),(vecFace*) NULL),g_XSplit.end());
	}
	
	Status				("Detaching subdivs...");
	{
		for (u32 it=0; it<g_XSplit.size(); it++)
		{
			Detach(g_XSplit[it]);
		}
	}
	clMsg				("%d subdivisions.",g_XSplit.size());
}
