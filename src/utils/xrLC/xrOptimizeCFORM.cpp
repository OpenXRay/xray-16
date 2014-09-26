#include "stdafx.h"
#include "build.h"
//#include "cl_collector.h"
#include "cform_build.h"

//main
static	int	bCriticalErrCnt = 0;
void	TestEdge			(Vertex *V1, Vertex *V2, Face* parent)
{
	Face*	found	= 0;
	int		f_count = 0;

	for (vecFaceIt I=V1->adjacent.begin(); I!=V1->adjacent.end(); I++)	{
		Face* test = *I;
		if (test == parent) continue;
		if (test->VContains(V2)) {
			f_count++;
			found = test;
		}
	}
	if (f_count>1) {
		bCriticalErrCnt	++;
		pBuild->err_multiedge.w_fvector3(V1->P);
		pBuild->err_multiedge.w_fvector3(V2->P);
	}
}

void	CBuild::BuildCForm	()
{
	Phase			("CFORM: construction...");
	//Status		("Building base mesh : vertices...");

	_mesh			mesh;						// a mesh object
	_decimater		decimater(mesh);			// a decimater object, connected to a mesh
	_HModQuadric	hModQuadric;				// use a quadric module
	decimater.add	(hModQuadric);				// register module at the decimater
	decimater.module(hModQuadric).set_max_err	(0.0001,false);	// error-limit 0.0001

	// Initializing mesh
	Status			("Building base mesh : vertices[%d]...",g_vertices.size());
	for (vecVertexIt _v=g_vertices.begin(); _v!=g_vertices.end(); _v++)	(*_v)->handle	= _mesh::InvalidVertexHandle.idx();

	Status			("Building base mesh : base faces[%d]...",g_faces.size());
	std::vector <_mesh::VertexHandle>	fhandles;
	xr_vector	<cform_FailFace>		failedfaces;
	cform_mergeprops					fmergeprops;

	for (vecFaceIt I=g_faces.begin(); I!=g_faces.end(); I++)
	{
		Progress	(float(I-g_faces.begin())/float(g_faces.size()));
		Face* F		= *I;
		if (F->Shader().flags.bCollision) 
		{
			// test correctness
			TestEdge	(F->v[0],F->v[1],F);
			TestEdge	(F->v[1],F->v[2],F);
			TestEdge	(F->v[2],F->v[0],F);

			// add vertices
			fhandles.clear	();
			for (u32 v=0; v<3; v++)
			{
				_mesh::VertexHandle	h	= _mesh::VertexHandle(F->v[v]->handle);
				if (_mesh::InvalidVertexHandle == h)	{ 
					Fvector& p			= F->v[v]->P; 
					h					= mesh.add_vertex	(_mesh::Point(p.x,p.y,p.z));
					F->v[v]->handle		= h.idx();
				}
				fhandles.push_back	(h);
			}

			// add face
			fmergeprops.material		= F->dwMaterialGame;
			fmergeprops.sector			= materials[F->dwMaterial].sector;
			_mesh::FaceHandle	hface	= mesh.add_face		(fhandles);
			if (hface == _mesh::InvalidFaceHandle)	{
				failedfaces.push_back		(cform_FailFace());
				failedfaces.back().P[0]		= F->v[0]->P;
				failedfaces.back().P[1]		= F->v[1]->P;
				failedfaces.back().P[2]		= F->v[2]->P;
				failedfaces.back().props	= fmergeprops.props;
			}
			else				mesh.face(hface).set_props	(fmergeprops.props);
		}
	}
	if (bCriticalErrCnt) {
		err_save	();
		clMsg		("MultipleEdges: %d faces",bCriticalErrCnt);
	}

	Status			("Building base mesh : models[%d]...",mu_refs.size());
	mesh.garbage_collection		();
	for (u32 ref=0; ref<mu_refs.size(); ref++)
		mu_refs[ref]->export_cform_game(mesh,failedfaces);

	Status			("Building base mesh : normals...");
	mesh.garbage_collection		();
	mesh.request_vertex_normals	();
	mesh.update_vertex_normals	();

	// Decimate
	Status			("Reconstructing mesh-topology...");
	clMsg			("%d faces failed topology check",	failedfaces.size());
	clMsg			("%f%% geometry/artist quality",	100.f * (1-float(failedfaces.size())/float(mesh.n_faces())));

	decimater.initialize	();      // let the decimater initialize the mesh and the modules

	int		nf_before		= int	(mesh.n_faces());
	int		nv_before		= int	(mesh.n_vertices());
	int		nc				= decimater.decimate	(nv_before);	// do decimation, as large, as possible
							mesh.garbage_collection	();
	int		nf_after		= int	(mesh.n_faces());
	int		nv_after		= int	(mesh.n_vertices());
	clMsg					("vertices: was[%d], now[%d] => %f %% left",nv_before,nv_after, 100.f*float(nv_after)/float(nv_before) );
	clMsg					("   faces: was[%d], now[%d] => %f %% left",nf_before,nf_after, 100.f*float(nf_after)/float(nf_before) );

	// Decimate
	Status			("Refactoring CFORM...");
	Fbox BB;	BB.invalidate();
	_mesh::VertexIter	vit	=mesh.vertices_begin(),vend=mesh.vertices_end();
	for (; vit!=vend; ++vit)
		BB.modify( reinterpret_cast<Fvector&>(mesh.point(vit)) );

	CDB::CollectorPacked	CL(BB,mesh.n_vertices(),mesh.n_faces());
	_mesh::FaceIter			fit=mesh.faces_begin(),fend=mesh.faces_end();
	for (; fit!=fend; ++fit){
		// get vertex-handles
		fhandles.clear	();
		for (_mesh::CFVIter fv_it=mesh.cfv_iter(fit); fv_it; ++fv_it)
			fhandles.push_back	(fv_it.handle());
		CL.add_face_D	(
			reinterpret_cast<Fvector&>(mesh.point	(fhandles[0])),
			reinterpret_cast<Fvector&>(mesh.point	(fhandles[1])),
			reinterpret_cast<Fvector&>(mesh.point	(fhandles[2])),
			fit->props	()
			);
	}
	Status			("Restoring fail-faces...");
	for (u32 it=0; it<failedfaces.size(); it++)		{
		cform_FailFace&	F	= failedfaces[it];
		CL.add_face_D		( F.P[0], F.P[1], F.P[2], F.props );
	}

	// Saving
	Status			("Saving...");
			nf_after		= int	(CL.getTS());
			nv_after		= int	(CL.getVS());
	clMsg					("vertices: was[%d], now[%d] => %f %% left",nv_before,nv_after, 100.f*float(nv_after)/float(nv_before) );
	clMsg					("   faces: was[%d], now[%d] => %f %% left",nf_before,nf_after, 100.f*float(nf_after)/float(nf_before) );
	string512		fn;
	IWriter*		MFS	= FS.w_open	(strconcat(fn,pBuild->path,"level.cform"));
	
	// Header
	hdrCFORM		hdr;
	hdr.version		= CFORM_CURRENT_VERSION;
	hdr.vertcount	= (u32)CL.getVS();
	hdr.facecount	= (u32)CL.getTS();
	hdr.aabb		= BB;
	MFS->w			(&hdr,sizeof(hdr));
	// Data
	MFS->w			(CL.getV(),(u32)CL.getVS()*sizeof(Fvector));
	MFS->w			(CL.getT(),(u32)CL.getTS()*sizeof(CDB::TRI));
	// Clear pDeflector (it is stored in the same memory space with dwMaterialGame)
	for (vecFaceIt I=g_faces.begin(); I!=g_faces.end(); I++)	{
		Face* F			= *I;
		F->pDeflector	= NULL;
	}
	FS.w_close		(MFS);
}

void CBuild::BuildPortals(IWriter& fs)
{
	fs.w_chunk		(fsL_PORTALS,&*portals.begin(),portals.size()*sizeof(b_portal));
}
