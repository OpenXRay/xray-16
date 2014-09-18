#include "stdafx.h"

#include "build.h"
#include "../xrLC_Light/xrLC_GlobalData.h"
#include "../xrLC_Light/xrface.h"

const int	 HDIM_X = 56;
const int	 HDIM_Y = 24;
const int	 HDIM_Z = 56;


//extern volatile u32	dwInvalidFaces;

IC bool				FaceEqual(Face& F1, Face& F2)
{
	// Test for 6 variations
	if ((F1.v[0]==F2.v[0]) && (F1.v[1]==F2.v[1]) && (F1.v[2]==F2.v[2])) return true;
	if ((F1.v[0]==F2.v[0]) && (F1.v[2]==F2.v[1]) && (F1.v[1]==F2.v[2])) return true;
	if ((F1.v[2]==F2.v[0]) && (F1.v[0]==F2.v[1]) && (F1.v[1]==F2.v[2])) return true;
	if ((F1.v[2]==F2.v[0]) && (F1.v[1]==F2.v[1]) && (F1.v[0]==F2.v[2])) return true;
	if ((F1.v[1]==F2.v[0]) && (F1.v[0]==F2.v[1]) && (F1.v[2]==F2.v[2])) return true;
	if ((F1.v[1]==F2.v[0]) && (F1.v[2]==F2.v[1]) && (F1.v[0]==F2.v[2])) return true;
	return false;
}

void CBuild::PreOptimize()
{
	// We use overlapping hash table to avoid boundary conflicts
	vecVertex*			HASH	[HDIM_X+1][HDIM_Y+1][HDIM_Z+1];
	Fvector				VMmin,	VMscale, VMeps, scale;
	
	// Calculate offset,scale,epsilon
	Fbox				bb = scene_bb;
	VMscale.set			(bb.max.x-bb.min.x, bb.max.y-bb.min.y, bb.max.z-bb.min.z);
	VMmin.set			(bb.min);
	VMeps.set			(VMscale.x/HDIM_X/2,VMscale.y/HDIM_Y/2,VMscale.z/HDIM_Z/2);
	VMeps.x				= (VMeps.x<EPS_L)?VMeps.x:EPS_L;
	VMeps.y				= (VMeps.y<EPS_L)?VMeps.y:EPS_L;
	VMeps.z				= (VMeps.z<EPS_L)?VMeps.z:EPS_L;
	scale.set			(float(HDIM_X),float(HDIM_Y),float(HDIM_Z));
	scale.div			(VMscale);
	
	u32	Vcount		= lc_global_data()->g_vertices().size(),	Vremoved=0;
	u32	Fcount		= lc_global_data()->g_faces().size(),		Fremoved=0;
	
	// Pre-alloc memory
	int		_size	= (HDIM_X+1)*(HDIM_Y+1)*(HDIM_Z+1);
	int		_average= (Vcount/_size)/2;	if (_average<2)	_average = 2;
	{
		for (int ix=0; ix<HDIM_X+1; ix++)
			for (int iy=0; iy<HDIM_Y+1; iy++)
				for (int iz=0; iz<HDIM_Z+1; iz++)
				{
					HASH[ix][iy][iz] = xr_new<vecVertex> ();
					HASH[ix][iy][iz]->reserve	(_average);
				}
	}
	
	// 
	Status("Processing...");
	g_bUnregister		= false;
	for (int it = 0; it<(int)lc_global_data()->g_vertices().size(); it++)
	{
		if (0==(it%100000)) {
			Progress(_sqrt(float(it)/float(lc_global_data()->g_vertices().size())));
			Status	("Processing... (%d verts removed)",Vremoved);
		}

		if (it>=(int)lc_global_data()->g_vertices().size()) break;

		Vertex	*pTest	= lc_global_data()->g_vertices()[it];
		Fvector	&V		= pTest->P;

		// Hash
		u32 ix,iy,iz;
		ix = iFloor		((V.x-VMmin.x)*scale.x);
		iy = iFloor		((V.y-VMmin.y)*scale.y);
		iz = iFloor		((V.z-VMmin.z)*scale.z);
		R_ASSERT		(ix<=HDIM_X && iy<=HDIM_Y && iz<=HDIM_Z);
		vecVertex &H	= *(HASH[ix][iy][iz]);

		// Search similar vertices in hash table
		for (vecVertexIt T=H.begin(); T!=H.end(); T++)
		{
			Vertex *pBase = *T;
			if (pBase->similar(*pTest,g_params().m_weld_distance)) 
			{
				while(pTest->m_adjacents.size())	
					pTest->m_adjacents.front()->VReplace(pTest, pBase);

				lc_global_data()->destroy_vertex(lc_global_data()->g_vertices()[it]);
				Vremoved			+= 1;
				pTest				= NULL;
				break;
			}
		}
		
		// If we get here - there is no similar vertices - register in hash tables
		if (pTest) 
		{
			H.push_back	(pTest);

			u32 ixE,iyE,izE;
			ixE = iFloor((V.x+VMeps.x-VMmin.x)*scale.x);
			iyE = iFloor((V.y+VMeps.y-VMmin.y)*scale.y);
			izE = iFloor((V.z+VMeps.z-VMmin.z)*scale.z);
			R_ASSERT(ixE<=HDIM_X && iyE<=HDIM_Y && izE<=HDIM_Z);

			if (ixE!=ix)							HASH[ixE][iy][iz]->push_back		(pTest);
			if (iyE!=iy)							HASH[ix][iyE][iz]->push_back		(pTest);
			if (izE!=iz)							HASH[ix][iy][izE]->push_back		(pTest);
			if ((ixE!=ix)&&(iyE!=iy))				HASH[ixE][iyE][iz]->push_back		(pTest);
			if ((ixE!=ix)&&(izE!=iz))				HASH[ixE][iy][izE]->push_back		(pTest);
			if ((iyE!=iy)&&(izE!=iz))				HASH[ix][iyE][izE]->push_back		(pTest);
			if ((ixE!=ix)&&(iyE!=iy)&&(izE!=iz))	HASH[ixE][iyE][izE]->push_back		(pTest);
		}
	}
	
	Status("Removing degenerated/duplicated faces...");
	g_bUnregister	= false;
	for (u32 it=0; it<lc_global_data()->g_faces().size(); it++)
	{
		R_ASSERT		(it>=0 && it<(int)lc_global_data()->g_faces().size());
		Face* F			= lc_global_data()->g_faces()[it];
		if ( F->isDegenerated()) {
			lc_global_data()->destroy_face	(lc_global_data()->g_faces()[it]);
			Fremoved			++;
		} else {
			// Check validity
			F->Verify			( );
		}
		Progress	(float(it)/float(lc_global_data()->g_faces().size()));
	}
	if (InvalideFaces())	
	{
		err_save		();
		Debug.fatal		(DEBUG_INFO,"* FATAL: %d invalid faces. Compilation aborted",InvalideFaces());
	}

	Status				("Adjacency check...");
	g_bUnregister		= false;

	for (u32 it = 0; it<lc_global_data()->g_vertices().size(); ++it)
	{
		if (lc_global_data()->g_vertices()[it] && (lc_global_data()->g_vertices()[it]->m_adjacents.empty()))
		{
			lc_global_data()->destroy_vertex	(lc_global_data()->g_vertices()[it]);
			++Vremoved;
		}
	}
	
	Status				("Cleanup...");
	lc_global_data()->g_vertices().erase	(std::remove(lc_global_data()->g_vertices().begin(),lc_global_data()->g_vertices().end(),(Vertex*)0),lc_global_data()->g_vertices().end());
	lc_global_data()->g_faces().erase		(std::remove(lc_global_data()->g_faces().begin(),lc_global_data()->g_faces().end(),(Face*)0),lc_global_data()->g_faces().end());
	{
		for (int ix=0; ix<HDIM_X+1; ix++)
			for (int iy=0; iy<HDIM_Y+1; iy++)
				for (int iz=0; iz<HDIM_Z+1; iz++)
				{
					xr_delete(HASH[ix][iy][iz]);
				}
	}
	mem_Compact			();
	clMsg("%d vertices removed. (%d left)",Vcount-lc_global_data()->g_vertices().size(),lc_global_data()->g_vertices().size());
	clMsg("%d faces removed. (%d left)",   Fcount-lc_global_data()->g_faces().size(),   lc_global_data()->g_faces().size());
	
	// -------------------------------------------------------------
	/*
	int		err_count	=0 ;
	for (int _1=0; _1<g_faces.size(); _1++)
	{
		Progress(float(_1)/float(g_faces.size()));
		for (int _2=0; _2<g_faces.size(); _2++)
		{
			if (_1==_2)		continue;
			if (FaceEqual(*g_faces[_1],*g_faces[_2]))	{
				err_count	++;
			}
		}
	}
	clMsg		("! duplicate/same faces found:%d",err_count);
	*/
	// -------------------------------------------------------------
}


void CBuild::IsolateVertices	(BOOL bProgress)
{
	isolate_vertices<Vertex>( bProgress, lc_global_data()->g_vertices() );
}
