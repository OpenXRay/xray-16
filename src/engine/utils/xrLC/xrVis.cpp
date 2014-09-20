#include "stdafx.h"
#include "build.h"
#include "math.h"
#include "ffileops.h"

// from xrPVS
typedef xr_vector<u16>	vecW;
typedef vecW::iterator	vecW_IT;

extern xr_vector<vecW>		g_pvs;
extern u32			g_pvs_X,g_pvs_Y,g_pvs_Z;

struct V_Header {
	u32	nX,nY,nZ;
	float	relevance;
	Fvector	min;
};

u32 PlaceData(xr_vector<vecW> &C, vecW &P)
{
	if (P.size()>1) {
		std::sort	(P.begin(),P.end());
		vecW::iterator I = std::unique	(P.begin(),P.end());
		P.erase(I,P.end());
	}

	// Search placeholder
	u32 sz	= P.size();
	u32 pos	= 0;
	for (xr_vector<vecW>::iterator it=C.begin(); it!=C.end(); it++)
	{
		u32 S = it->size();
		if (S!=sz) { pos+=S+1; continue; }
		if (0!=memcmp(it->begin(),P.begin(),S*sizeof(u16))) { pos+=S+1; continue; }

		// Ok-Ob :)
		goto exit;
	}

	// If we get here - need to register _new set of data
	C.push_back(P);

exit:
	P.clear();
	return pos*sizeof(u16);
}

void SaveDATA(IWriter &fs, xr_vector<vecW> &C)
{
	for (xr_vector<vecW>::iterator it=C.begin(); it!=C.end(); it++)
	{
		fs.Wword(it->size());
		fs.write(it->begin(),it->size()*sizeof(u16));
	}
}

int*	g_pvs_map_vm = 0;
void CalculateRelSet(Fvector &pos, vecW &rel_set)
{
	// calculate volume in local level space
	Fbox	Volume;
	Volume.set(pos,pos);
	float	Vsize = (g_params.m_relevance+g_params.m_sample_step)/2.f; 
	Volume.min.sub(Vsize);
	Volume.max.add(Vsize);
	Volume.min.sub(g_TREE_ROOT->bbox.min);
	Volume.max.sub(g_TREE_ROOT->bbox.min);

	// scale it to sample grid space
	Volume.min.div(g_params.m_sample_step);
	Volume.max.div(g_params.m_sample_step);

	// calc volume in grid-space
	int minX,minY,minZ;
	int maxX,maxY,maxZ;

	minX = iROUND(floorf(Volume.min.x));	clamp(minX, 0, int(g_pvs_X)-1);
	minY = iROUND(floorf(Volume.min.y));	clamp(minY, 0, int(g_pvs_Y)-1);
	minZ = iROUND(floorf(Volume.min.z));	clamp(minZ, 0, int(g_pvs_Z)-1);

	maxX = iROUND(ceilf(Volume.max.x));		clamp(maxX, 0, int(g_pvs_X)-1);
	maxY = iROUND(ceilf(Volume.max.y));		clamp(maxY, 0, int(g_pvs_Y)-1);
	maxZ = iROUND(ceilf(Volume.max.z));		clamp(maxZ, 0, int(g_pvs_Z)-1);

	/*
	clMsg("- Selected BB: [%d,%d,%d]-[%d,%d,%d]",
		minX,minY,minZ,
		maxX,maxY,maxZ
		);
	*/

	// merge data
	for (int z=minZ; z<=maxZ; z++) {
		for (int x=minX; x<=maxX; x++) {
			for (int y=minY; y<=maxY; y++)
			{
				u32	cell	= z*g_pvs_X*g_pvs_Y + x*g_pvs_Y + y;
//				clMsg("* Sample #%d",cell);
				int		ptr		= g_pvs_map_vm[cell];
				if (ptr>=0)
				{
					rel_set.insert(rel_set.end(),g_pvs[ptr].begin(),g_pvs[ptr].end());
				}
			}
		}
	}
	if (rel_set.size()>1)
	{
		std::sort(rel_set.begin(),rel_set.end());
		vecW_IT I = std::unique(rel_set.begin(),rel_set.end());
		rel_set.erase(I,rel_set.end());
	}
}

void CBuild::BuildRelevance(IWriter &fs)
{
	static Fvector size;
	static u32	nx,ny,nz;

	Status("Preparing...");
	R_ASSERT(g_TREE_ROOT);
	g_TREE_ROOT->bbox.getsize(size);
	clMsg("Level dimensions: [%3.1f,%3.1f,%3.1f]",size.x,size.y,size.z);
	nx = iROUND(ceilf(size.x/g_params.m_relevance));
	ny = iROUND(ceilf(size.y/g_params.m_relevance));
	nz = iROUND(ceilf(size.z/g_params.m_relevance));
	clMsg("Ceiling dimensions: [%3d,%3d,%3d]",nx, ny, nz);

	fs.open_chunk(fsL_VISIBILITY);
 
	fs.open_chunk(fsV_HEADER);
	V_Header		H;
	H.nX			= nx;
	H.nY			= ny;
	H.nZ			= nz;
	H.relevance		= g_params.m_relevance;
	H.min.set		(g_TREE_ROOT->bbox.min);
	fs.write		(&H,sizeof(H));
	fs.close_chunk	();

	// Build Visibility
	static xr_vector< u32 >		slots;
	static xr_vector< vecW >		vis_nodes;
	static xr_vector< vecW >		vis_lights;
	static xr_vector< vecW >		vis_glows;
	static xr_vector< vecW >		vis_occluders;
	static vecW					rel_set;
	static vecW					unroll;

	CVirtualFileStream*			pvs_map_stream=0;
	if (g_params.m_bTestOcclusion)
	{
		pvs_map_stream  = xr_new<CVirtualFileStream> ("pvs.temp");
		g_pvs_map_vm	= (int *)pvs_map_stream->Pointer();
	}
 
	static u32				dwSlot = 0;
	for (int z=0; z<nz; z++) {
		for (int x=0; x<nx; x++) {
			for (int y=0; y<ny; y++)
			{
				Status("Volume #%d...",dwSlot);
				static Fvector pos;
				pos.set(x,y,z);
				pos.add(.5f);
				pos.mul(g_params.m_relevance);
				pos.add(g_TREE_ROOT->bbox.min);

				// ******* Nodes relevance
				if (g_params.m_bTestOcclusion)
				{
					CalculateRelSet(pos,unroll);
					if (unroll.empty() || g_TREE_ROOT->VisCapture(unroll,rel_set))
						rel_set.push_back(g_tree.size()-1);
					unroll.clear();
					slots.push_back(PlaceData(vis_nodes,rel_set));
				} else {
					// Unroll hierrarhy
					VERIFY(g_TREE_ROOT);
					g_TREE_ROOT->VisUnroll(pos,unroll);
					if (unroll.size()>1)
						std::sort(unroll.begin(),unroll.end());
					// Capture results
					if (g_TREE_ROOT->VisCapture(unroll,rel_set))
						rel_set.push_back(g_tree.size()-1);
					unroll.clear();
					// Register in container
					slots.push_back(PlaceData(vis_nodes,rel_set));
				}

				// Lights relevance
				for (int i=0; i<lights.size(); i++)
				{
					if ((pos.distance_to(lights[i].position) - lights[i].range) < g_params.m_viewdist) {
						rel_set.push_back(i);
					}
				}
				slots.push_back(PlaceData(vis_lights,rel_set));

				// Glows relevance
				for (i=0; i<glows.size(); i++)
				{
					if ((pos.distance_to(glows[i].P) - glows[i].size) < g_params.m_viewdist) {
						rel_set.push_back(i);
					}
				}
				slots.push_back(PlaceData(vis_glows,rel_set));

				// Occluders relevance
				for (i=0; i<occluders.size(); i++)
				{
					Fvector P; float R=-1; float T;
					P.add(occluders[i].V2,occluders[i].V4);
					P.mul(.5f);
					T = P.distance_to(occluders[i].V1); if (T>R) R=T;
					T = P.distance_to(occluders[i].V2); if (T>R) R=T;
					T = P.distance_to(occluders[i].V4); if (T>R) R=T;
					if ((pos.distance_to(P) - R) < g_params.m_viewdist*g_params.m_occluder_rel_scale) {
						rel_set.push_back(i);
					}
				}
				slots.push_back(PlaceData(vis_occluders,rel_set));

				dwSlot++;
				Progress(float(dwSlot)/float(nz*nx*ny));
			}
		}
	}


	xr_delete		(pvs_map_stream);

	fs.open_chunk	(fsV_NODES);
	SaveDATA		(fs,vis_nodes);
	fs.close_chunk	();

	fs.open_chunk(fsV_LIGHTS);
	SaveDATA(fs,vis_lights);
	fs.close_chunk();

	fs.open_chunk(fsV_GLOWS);
	SaveDATA(fs,vis_glows);
	fs.close_chunk();

	fs.open_chunk(fsV_OCCLUDERS);
	SaveDATA(fs,vis_occluders);
	fs.close_chunk();

	fs.open_chunk(fsV_MAP);
	fs.write(slots.begin(),slots.size()*sizeof(u32));
	fs.close_chunk();

	fs.close_chunk();
}
