#include "stdafx.h"
#include "build.h"
#include "OGF_Face.h"

SWIContainer			g_SWI,x_SWI;
VBContainer				g_VB,x_VB;
IBContainer				g_IB,x_IB;

bool CBuild::IsOGFContainersEmpty()
{
	return	g_SWI.is_empty() &&
			x_SWI.is_empty() &&
			g_VB.is_empty()  &&
			x_VB.is_empty()  &&
			g_IB.is_empty()  &&
			x_IB.is_empty() ;
}

static u32						g_batch_count;
static u32						g_batch_verts;
static u32						g_batch_faces;

static u32						g_batch_50;
static u32						g_batch_100;
static u32						g_batch_500;
static u32						g_batch_1000;
static u32						g_batch_5000;

u16		RegisterShader		(LPCSTR T) 
{
	for (u32 it=0; it<pBuild->g_Shaders.size(); it++)
		if (0==stricmp(T,pBuild->g_Shaders[it]))	return it;
	pBuild->g_Shaders.push_back		(xr_strdup(T));
	return pBuild->g_Shaders.size	()-1;
}

void	geom_batch_average	(u32 verts, u32 faces)
{
	//clMsg			("* batch[%d], %d verts, %d faces",g_batch_count,verts,faces);
	g_batch_count	++;
	g_batch_verts	+=	verts;
	g_batch_faces	+=	faces;

	if (faces<=50)				g_batch_50	++;
	else if (faces<=100)		g_batch_100	++;
	else if (faces<=500)		g_batch_500	++;
	else if (faces<=1000)		g_batch_1000++;
	else if (faces<=5000)		g_batch_5000++;
}

static bool	remap_order		(u32 id0, u32 id1)
{
	OGF*	o0			= (OGF*)g_tree[id0];
	OGF*	o1			= (OGF*)g_tree[id1];
	return	xr_strcmp(*o0->textures.front().name,*o1->textures.front().name)<0;
}

static void	SaveGEOMs		(LPCSTR fn, VBContainer& vb, IBContainer& ib, SWIContainer& swi)
{
	Status				("Geometry '%s'...",	fn);
	// geometry
	string_path					lfn		;
	IWriter*					file	;
	file						= FS.w_open		(strconcat(sizeof(lfn),lfn,pBuild->path,fn));
	hdrLEVEL H;	H.XRLC_version	= XRCL_PRODUCTION_VERSION;
	file->w_chunk				(fsL_HEADER,&H,sizeof(H));

	// verts
	file->open_chunk	(fsL_VB);
	vb.Save				(*file);
	file->close_chunk	();

	// indices
	file->open_chunk	(fsL_IB);
	ib.Save				(*file);
	file->close_chunk	();

	// swis
	file->open_chunk	(fsL_SWIS);
	swi.Save			(*file);
	file->close_chunk	();
}

void CBuild::SaveTREE	(IWriter &fs)
{
	CMemoryWriter		MFS;

	Status				("Geometry buffers...");
	xr_vector<u32>		remap;
	remap.reserve		(g_tree.size());
	for (u32 rid=0; rid<g_tree.size(); rid++)	{
		OGF*	o		= dynamic_cast<OGF*>	(g_tree[rid]);
		if		(o)		remap.push_back(rid);
	}
	std::stable_sort	(remap.begin(),remap.end(),remap_order);
	clMsg				("remap-size: %d / %d",remap.size(),g_tree.size());
	for (u32 sid=0; sid<remap.size(); sid++)	{
		u32				id	= remap[sid];
		//clMsg			("%3d: subdiv: %d",sid,id);
		g_tree[id]->PreSave	(id);
	}

	Status				("Visuals...");
	fs.open_chunk		(fsL_VISUALS);
	for (xr_vector<OGF_Base*>::iterator it = g_tree.begin(); it!=g_tree.end(); it++)	{
		u32			idx = u32(it-g_tree.begin());
		MFS.open_chunk	(idx);
		(*it)->Save		(MFS);
		MFS.close_chunk	();
		Progress		(float(idx)/float(g_tree.size()));
	}
	fs.w				(MFS.pointer(),MFS.size());
	fs.close_chunk		();
	clMsg				("Average: %d verts/%d faces, 50(%2.1f), 100(%2.1f), 500(%2.1f), 1000(%2.1f), 5000(%2.1f)",
		g_batch_verts/g_batch_count,
		g_batch_faces/g_batch_count,
		100.f * float(g_batch_50)/float(g_batch_count),
		100.f * float(g_batch_100)/float(g_batch_count),
		100.f * float(g_batch_500)/float(g_batch_count),
		100.f * float(g_batch_1000)/float(g_batch_count),
		100.f * float(g_batch_5000)/float(g_batch_count)
		);
	mem_Compact			();

	SaveGEOMs			("level.geom",	g_VB,g_IB,g_SWI);	// Normal
	SaveGEOMs			("level.geomx",	x_VB,x_IB,x_SWI);	// Fast-Path

	Status				("Shader table...");
	fs.open_chunk		(fsL_SHADERS);
	fs.w_u32			(g_Shaders.size());
	for (xr_vector<LPCSTR>::iterator T=g_Shaders.begin(); T!=g_Shaders.end(); T++)
		fs.w_stringZ	(*T);
	fs.close_chunk		();
	//mem_Compact			();
}
