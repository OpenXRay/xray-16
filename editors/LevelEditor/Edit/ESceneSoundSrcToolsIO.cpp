#include "stdafx.h"
#pragma hdrstop

#include "ESceneSoundSrcTools.h"

// chunks
static const u16 SOUND_SRC_TOOLS_VERSION  	= 0x0000;
//----------------------------------------------------
enum{
    CHUNK_VERSION			= 0x1001ul,
};
//----------------------------------------------------
bool ESceneSoundSrcTool::LoadLTX(CInifile& ini)
{
	u32 version 	= ini.r_u32("main","version");
    if( version!=SOUND_SRC_TOOLS_VERSION )
    {
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
    }

	inherited::LoadLTX(ini);
	return true;
}
void ESceneSoundSrcTool::SaveLTX(CInifile& ini, int id)
{
	inherited::SaveLTX(ini, id);

	ini.w_u32		("main","version",SOUND_SRC_TOOLS_VERSION);
}

bool ESceneSoundSrcTool::LoadStream(IReader& F)
{
	u16 version 	= 0;
    if(F.r_chunk(CHUNK_VERSION,&version))
        if( version!=SOUND_SRC_TOOLS_VERSION ){
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
        }

	if (!inherited::LoadStream(F)) return false;

    return true;
}
//----------------------------------------------------

void ESceneSoundSrcTool::SaveStream(IWriter& F)
{
	inherited::SaveStream(F);

	F.w_chunk		(CHUNK_VERSION,(u16*)&SOUND_SRC_TOOLS_VERSION,sizeof(SOUND_SRC_TOOLS_VERSION));
}
//----------------------------------------------------

bool ESceneSoundSrcTool::LoadSelection(IReader& F)
{
	u16 version 	= 0;
    R_ASSERT(F.r_chunk(CHUNK_VERSION,&version));
    if( version!=SOUND_SRC_TOOLS_VERSION ){
        ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
        return false;
    }

	return inherited::LoadSelection(F);
}
//----------------------------------------------------

void ESceneSoundSrcTool::SaveSelection(IWriter& F)
{
	F.w_chunk		(CHUNK_VERSION,(u16*)&SOUND_SRC_TOOLS_VERSION,sizeof(SOUND_SRC_TOOLS_VERSION));

	inherited::SaveSelection(F);
}
//----------------------------------------------------

