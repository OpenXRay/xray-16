#include "stdafx.h"
//#include "cl_collector.h"
#include "build.h"
#include "../xrLC_Light/xrMU_Model.h"
#include "../xrLC_Light/xrMU_Model_Reference.h"

#include "../xrLC_Light/xrLC_GlobalData.h"
#include "../xrLC_Light/xrface.h"

#include "../../xrcore/fs.h"
#include "../../xrcdb/xrcdb.h"

int GetVertexIndex(Vertex *F)
{
	vecVertexIt it = std::lower_bound(lc_global_data()->g_vertices().begin(),lc_global_data()->g_vertices().end(),F);
	
	R_ASSERT	(it!=lc_global_data()->g_vertices().end());

	return int(it-lc_global_data()->g_vertices().begin());
}

int getCFormVID(vecVertex& V,Vertex *F)
{
	vecVertexIt it = std::lower_bound(V.begin(),V.end(),F);
	return int(it-V.begin());
}
int bCriticalErrCnt = 0;
/*

int getTriByEdge(Vertex *V1, Vertex *V2, Face* parent, vecFace &ids)
{
	Face*	found	= 0;
	int		f_count = 0;

	for (vecFaceIt I=V1->m_adjacents.begin(); I!=V1->m_adjacents.end(); ++I)
	{
		Face* test = *I;
		if (test == parent) continue;
		if (test->VContains(V2)) 
		{
			++f_count;
			found = test;
		}
	}
	if (f_count>1) 
	{
		bCriticalErrCnt	++;
		pBuild->err_multiedge.w_fvector3(V1->P);
		pBuild->err_multiedge.w_fvector3(V2->P);
	}
	if (found) {
		vecFaceIt F = std::lower_bound(ids.begin(),ids.end(),found);
		if (found == *F) return int(F-ids.begin());
		else return -1;
	} else {
		return -1;
	}
}
*/
void TestEdge			(Vertex *V1, Vertex *V2, Face* parent)
{
	Face*	found	= 0;
	int		f_count = 0;

	for (vecFaceIt I=V1->m_adjacents.begin(); I!=V1->m_adjacents.end(); ++I)	
	{
		Face* test = *I;
		if (test == parent) continue;
		if (test->VContains(V2)) 
		{
			++f_count;
			found = test;
		}
	}
	if (f_count>1) 
	{
		++bCriticalErrCnt;
		pBuild->err_multiedge().w_fvector3(V1->P);
		pBuild->err_multiedge().w_fvector3(V2->P);
	}
}
extern void SimplifyCFORM		(CDB::CollectorPacked& CL);
void CBuild::BuildCForm	()
{
	// Collecting data
	Phase		("CFORM: creating...");
	vecFace*	cfFaces		= xr_new<vecFace>	();
	vecVertex*	cfVertices	= xr_new<vecVertex>	();
	{
		xr_vector<bool>			cfVertexMarks;
		cfVertexMarks.assign	(lc_global_data()->g_vertices().size(),false);

		Status("Sorting...");
		std::sort(lc_global_data()->g_vertices().begin(),lc_global_data()->g_vertices().end());

		Status("Collecting faces...");
		cfFaces->reserve	(lc_global_data()->g_faces().size());
		for (vecFaceIt I=lc_global_data()->g_faces().begin(); I!=lc_global_data()->g_faces().end(); ++I)
		{
			Face* F = *I;
			if (F->Shader().flags.bCollision) 
			{
				cfFaces->push_back(F);
				int index = GetVertexIndex(F->v[0]);
				cfVertexMarks[index] = true;

				index = GetVertexIndex(F->v[1]);
				cfVertexMarks[index] = true;

				index = GetVertexIndex(F->v[2]);
				cfVertexMarks[index] = true;
			}
		}

		Status("Collecting vertices...");
		cfVertices->reserve	(lc_global_data()->g_vertices().size());
		std::sort(cfFaces->begin(),cfFaces->end());
		for (u32 V=0; V<lc_global_data()->g_vertices().size(); V++)
			if (cfVertexMarks[V]) cfVertices->push_back(lc_global_data()->g_vertices()[V]);
	}

	float	p_total = 0;
	float	p_cost  = 1.f/(cfVertices->size());
	
	Fbox BB; BB.invalidate();
	for (vecVertexIt it = cfVertices->begin(); it!=cfVertices->end(); it++)
		BB.modify((*it)->P );

	// CForm
	Phase	("CFORM: collision model...");
	Status	("Items to process: %d", cfFaces->size());
	p_total = 0;
	p_cost  = 1.f/(cfFaces->size());

	// Collect faces
	CDB::CollectorPacked CL	(BB,cfVertices->size(),cfFaces->size());
	for (vecFaceIt F = cfFaces->begin(); F!=cfFaces->end(); F++)
	{
		Face*	T = *F;

		TestEdge	(T->v[0],T->v[1],T);
		TestEdge	(T->v[1],T->v[2],T);
		TestEdge	(T->v[2],T->v[0],T);

		CL.add_face	(
			T->v[0]->P, T->v[1]->P, T->v[2]->P,
			T->dwMaterialGame, materials()[T->dwMaterial].sector, T->sm_group
			);
		Progress(p_total+=p_cost);		// progress
	}
	if (bCriticalErrCnt) {
		err_save	();
		clMsg		("MultipleEdges: %d faces",bCriticalErrCnt);
	}
	xr_delete		(cfFaces);
	xr_delete		(cfVertices);

	// Models
	Status			("Models...");
	for (u32 ref=0; ref<mu_refs().size(); ref++)
		mu_refs()[ref]->export_cform_game(CL);

	// Simplification
	if (g_params().m_quality!=ebqDraft)
		SimplifyCFORM	(CL);

	// bb?
	BB.invalidate	();
	for (size_t it = 0; it<CL.getVS(); it++)
		BB.modify( CL.getV()[it] );

	// Saving
	string_path		fn;
	IWriter*		MFS	= FS.w_open	(strconcat(sizeof(fn),fn,pBuild->path,"level.cform"));
	Status			("Saving...");

	// Header
	hdrCFORM hdr;
	hdr.version		= CFORM_CURRENT_VERSION;
	hdr.vertcount	= (u32)CL.getVS();
	hdr.facecount	= (u32)CL.getTS();
	hdr.aabb		= BB;
	MFS->w			(&hdr,sizeof(hdr));

	// Data
	MFS->w			(CL.getV(),(u32)CL.getVS()*sizeof(Fvector));
	MFS->w			(CL.getT(),(u32)CL.getTS()*sizeof(CDB::TRI));

	// Clear pDeflector (it is stored in the same memory space with dwMaterialGame)
	for (vecFaceIt I=lc_global_data()->g_faces().begin(); I!=lc_global_data()->g_faces().end(); I++)
	{
		Face* F			= *I;
		F->pDeflector	= NULL;
	}

	FS.w_close		(MFS);
}

void CBuild::BuildPortals(IWriter& fs)
{
	fs.w_chunk		(fsL_PORTALS,&*portals.begin(),portals.size()*sizeof(b_portal));
}
