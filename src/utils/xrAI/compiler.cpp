#include "stdafx.h"
#include "compiler.h"

CDB::MODEL Level;
Nodes g_nodes;
SAIParams g_params;

void mem_Optimize()
{
    Memory.mem_compact();
    Msg("* Memory usage: %d M", Memory.mem_usage() / (1024 * 1024));
}

void xrCompiler(LPCSTR name, bool draft_mode, bool pure_covers, LPCSTR out_name)
{
    Logger.Phase("Loading level...");
    xrLoad(name, draft_mode);
    mem_Optimize();
    if (!draft_mode)
    {
        Logger.Phase("Calculating coverage...");
        xrCover(pure_covers);
        mem_Optimize();
    }
    Logger.Phase("Saving nodes...");
    xrSaveNodes(name, out_name);
    mem_Optimize();
}
