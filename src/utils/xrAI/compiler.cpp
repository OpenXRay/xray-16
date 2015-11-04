#include "stdafx.h"
#include "compiler.h"
#include "xrCDB/Intersect.hpp"

CDB::MODEL			Level;
CDB::COLLIDER		XRC;
Nodes				g_nodes;
xr_vector<SCover>	g_covers_palette;
Lights				g_lights;
SAIParams			g_params;
Fbox				LevelBB;

void	mem_Optimize	()
{
	Memory.mem_compact	();
	Msg("* Memory usage: %d M",Memory.mem_usage()/(1024*1024));
}

void xrCompiler	(LPCSTR name, bool draft_mode, bool pure_covers, LPCSTR out_name)
{
    Logger.Phase("Loading level...");
	xrLoad		(name,draft_mode);
	mem_Optimize();	
	if (!draft_mode)
    {
        Logger.Phase("Calculating coverage...");
		xrCover		(pure_covers);
		mem_Optimize();
	}
    Logger.Phase("Saving nodes...");
	xrSaveNodes	(name,out_name);
	mem_Optimize();
}
