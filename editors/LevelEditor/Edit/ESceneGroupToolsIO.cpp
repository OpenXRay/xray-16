#include "stdafx.h"
#pragma hdrstop

#include "ESceneGroupTools.h"
#include "GroupObject.h"
#include "SceneObject.h"

// chunks
static const u16 GROUP_TOOLS_VERSION  	= 0x0000;
//----------------------------------------------------
enum{
    CHUNK_VERSION			= 0x1001ul,
};
//----------------------------------------------------

bool ESceneGroupTool::LoadStream(IReader& F)
{
	u16 version 	= 0;
    if(F.r_chunk(CHUNK_VERSION,&version))
        if( version!=GROUP_TOOLS_VERSION )
        {
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
        }

	if (!inherited::LoadStream(F)) return false;

    return true;
}
//----------------------------------------------------
bool ESceneGroupTool::LoadLTX(CInifile& ini)
{
	LPCSTR section = "main";
	u16 version 	= ini.r_u16(section, "version");

    if( version!=GROUP_TOOLS_VERSION )
    {
        ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
        return false;
    }
	if (!inherited::LoadLTX(ini)) return false;

    return true;
}

void ESceneGroupTool::SaveLTX(CInifile& ini, int id)
{
	LPCSTR section	= "main";
	ini.w_u16(section, "version", GROUP_TOOLS_VERSION);

	inherited::SaveLTX	(ini, id);
}

void ESceneGroupTool::SaveStream(IWriter& F)
{
	inherited::SaveStream(F);

	F.w_chunk		(CHUNK_VERSION,(u16*)&GROUP_TOOLS_VERSION,sizeof(GROUP_TOOLS_VERSION));
}
//----------------------------------------------------
 
bool ESceneGroupTool::LoadSelection(IReader& F)
{
	u16 version 	= 0;
    R_ASSERT(F.r_chunk(CHUNK_VERSION,&version));
    if( version!=GROUP_TOOLS_VERSION )
    {
        ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
        return false;
    }

	return inherited::LoadSelection(F);
}
//----------------------------------------------------

void ESceneGroupTool::SaveSelection(IWriter& F)
{
	F.w_chunk		(CHUNK_VERSION,(u16*)&GROUP_TOOLS_VERSION,sizeof(GROUP_TOOLS_VERSION));
    
	inherited::SaveSelection(F);
}

