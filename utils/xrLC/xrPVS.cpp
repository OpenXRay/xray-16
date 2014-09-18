#include "stdafx.h"
#include "build.h"
#include "xrOcclusion.h"

#pragma comment(lib,"xrOcclusion.lib")
#pragma comment(lib,"winmm.lib")

typedef xr_vector<u16>	vecW;
typedef vecW::iterator	vecW_IT;

typedef xr_vector<BOOL>	vecB;
typedef vecB::iterator	vecB_IT;

typedef xr_multimap<u32,u32>					treeCompress;
typedef treeCompress::iterator						treeCompressIt;
typedef treeCompress::value_type					treeCompressType;
typedef std::pair<treeCompressIt,treeCompressIt>	treeCompressPair;

xr_vector<vecW>	g_pvs;

treeCompress	g_compress_tree;

vecW			g_selected;
vecB			g_result;

u32			g_pvs_X,g_pvs_Y,g_pvs_Z;

int	CompressSelected()
{
	if (g_selected.size()>1)
	{
		std::sort	(g_selected.begin(),g_selected.end());
		vecW_IT I = std::unique	(g_selected.begin(),g_selected.end());
		g_selected.erase(I,g_selected.end());
	}

	// Search placeholder
	u32 sz					= g_selected.size();
	u32 sz_bytes				= sz*sizeof(u16);
	treeCompressPair	Range	= g_compress_tree.equal_range(sz);

	for (treeCompressIt it=Range.first; it!=Range.second; it++)
	{
		treeCompressType	&V	= *it;
		u32	entry			= V.second;
		vecW	&entry_data		= g_pvs[entry];
		if (0!=memcmp(entry_data.begin(),g_selected.begin(),sz_bytes)) continue;

		// Ok-Ob :)
		return entry;
	}

	// If we get here - need to register _new set of data
	u32 entry = g_pvs.size();
	g_pvs.push_back(g_selected);
	g_compress_tree.insert(mk_pair(sz,entry));
	return entry;
}

void CBuild::BuildPVS()
{
	Fvector size;
	Fvector pos;
	Fvector	ground_dir;

	Status("Preparing...");

	g_TREE_ROOT->bbox.getsize(size);
	g_pvs_X = iROUND(ceilf(size.x/g_params.m_sample_step))+1;
	g_pvs_Y = iROUND(ceilf(size.y/g_params.m_sample_step))+1;
	g_pvs_Z = iROUND(ceilf(size.z/g_params.m_sample_step))+1;
	clMsg("Ceiling dimensions: [%3d,%3d,%3d]",g_pvs_X, g_pvs_Y, g_pvs_Z);

	// ground pick setup
	XRC.RayMode			(RAY_ONLYFIRST|RAY_CULL);
	ground_dir.set		(0,-1,0);

	// reserve memory
	CFS_File			pvs_map		("pvs.temp");
	u32				dwSlot		= 0;
	u32				dwSlotsTotal= g_pvs_X*g_pvs_Y*g_pvs_Z;
	u32	pvs_reserve	= dwSlotsTotal/1024 + 512;
	clMsg("PVS: %d M",	(pvs_reserve*sizeof(vecW))/(1024*1024));
	g_pvs.reserve		(pvs_reserve);

	// begin!
	Status("Processing...");
	u32				dwStartTime	= timeGetTime();
	for (int z=0; z<g_pvs_Z; z++) {
		for (int x=0; x<g_pvs_X; x++) {
			for (int y=0; y<g_pvs_Y; y++)
			{
				pos.set(x,y,z);
				pos.mul(g_params.m_sample_step);
				pos.add(g_TREE_ROOT->bbox.min);
				dwSlot++;
				
				// ground pick
				XRC.RayPick(precalc_identity,1.f,&RCAST_Model,pos,ground_dir,g_params.m_sample_break);
				if (XRC.GetRayContactCount()==0)
				{
					// don't calculate PVS for this point
					int tmp = -1;
					pvs_map.write(&tmp,4);
					continue;
				}

				// Sample PVS data
				g_TREE_ROOT->VisUnroll(pos,g_selected);
				if (!g_selected.empty()) {
					g_result.resize(g_selected.size());
					ZeroMemory(g_result.begin(),g_result.size()*sizeof(BOOL));
					ORM_Process(g_selected.size(),pos,g_selected.begin(),g_result.begin());
					
					// Exclude invisible
					for (int i=0; i<g_selected.size(); i++)
					{
						if (!g_result[i]) {
							if (g_tree[g_selected[i]]->isPatch) continue;
							g_selected.erase(g_selected.begin()+i);
							g_result.erase(g_result.begin()+i);
							i--;
						}
					}
				}
				
				// Compress and record PVS sample
				pvs_map.w_u32(CompressSelected());
				g_selected.clear();

				// Statistic
				if (dwSlot%64 == 63)
				{
					Progress(float(dwSlot)/float(dwSlotsTotal));
					u32 dwCurrentTime = timeGetTime();
					Status("Sample #%d\nSpeed %3.1f samples per second\nPVS entrys: %d",
						dwSlot,1000.f*float(dwSlot)/float(dwCurrentTime-dwStartTime),
						g_pvs.size()
						);

				}
			}
		}
	}
	ORM_Destroy();
	clMsg("* PVS entrys:  %d",	g_pvs.size());
	clMsg("* Aver. Speed: %3.1f",	1000.f*float(dwSlot)/float(timeGetTime()-dwStartTime));
}
