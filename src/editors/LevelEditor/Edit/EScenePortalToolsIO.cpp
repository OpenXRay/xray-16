#include "stdafx.h"
#pragma hdrstop

#include "EScenePortalTools.h"

// chunks
static const u16 PORTAL_TOOLS_VERSION  	= 0x0000;
//----------------------------------------------------
enum{
    CHUNK_VERSION			= 0x1001ul,
    CHUNK_FLAGS				= 0x1002ul,
};
//----------------------------------------------------
bool EScenePortalTool::LoadLTX(CInifile& ini)
{
	u32 version 	= ini.r_u32("main","version");
    if( version!=PORTAL_TOOLS_VERSION )
    {
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
    }

	inherited::LoadLTX(ini);

  	m_Flags.assign	(ini.r_u32("main","flags"));

	return true;
}
void EScenePortalTool::SaveLTX(CInifile& ini, int id)
{
	inherited::SaveLTX	(ini, id);
	ini.w_u32		("main", "version", PORTAL_TOOLS_VERSION);
    ini.w_u32		("main", "flags", m_Flags.get());
}

bool EScenePortalTool::LoadStream(IReader& F)
{
	u16 version 	= 0;
    if(F.r_chunk(CHUNK_VERSION,&version))
        if( version!=PORTAL_TOOLS_VERSION ){
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
        }

	if (!inherited::LoadStream(F)) return false;

    if (F.find_chunk(CHUNK_FLAGS))
    	m_Flags.assign(F.r_u32());

    return true;
}
//----------------------------------------------------

void EScenePortalTool::SaveStream(IWriter& F)
{
	inherited::SaveStream	(F);

	F.w_chunk		(CHUNK_VERSION,(u16*)&PORTAL_TOOLS_VERSION,sizeof(PORTAL_TOOLS_VERSION));

	F.open_chunk	(CHUNK_FLAGS);
    F.w_u32			(m_Flags.get());
	F.close_chunk	();
}
//----------------------------------------------------

bool EScenePortalTool::LoadSelection(IReader& F)
{
	u16 version 	= 0;
    R_ASSERT(F.r_chunk(CHUNK_VERSION,&version));
    if( version!=PORTAL_TOOLS_VERSION ){
        ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
        return false;
    }

	return inherited::LoadSelection(F);
}
//----------------------------------------------------

void EScenePortalTool::SaveSelection(IWriter& F)
{
	F.w_chunk		(CHUNK_VERSION,(u16*)&PORTAL_TOOLS_VERSION,sizeof(PORTAL_TOOLS_VERSION));

	inherited::SaveSelection(F);
}
//----------------------------------------------------

