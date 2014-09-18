#include "stdafx.h"
#pragma hdrstop

#include "ESceneLightTools.h"
#include "elight.h"
#include "scene.h"
#include "../ECore/Editor/ui_main.h"

// chunks
static const u16 LIGHT_TOOLS_VERSION  	= 0x0000;
//----------------------------------------------------
enum{
    CHUNK_VERSION			= 0x1001ul,
    CHUNK_LCONTROLS			= 0x1002ul,
    CHUNK_LCONTROLS_LAST	= 0x1003ul,
    CHUNK_FLAGS				= 0x1004ul,
//	CHUNK_HEMI				= 0x1005ul, // obsolette
    CHUNK_SUN_SHADOW		= 0x1006ul,
    CHUNK_HEMI2				= 0x1007ul,
};
//----------------------------------------------------
bool ESceneLightTool::LoadLTX(CInifile& ini)
{
	u32 version 	= ini.r_u32("main", "version");
    if( version!=LIGHT_TOOLS_VERSION )
    {
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
        }

	inherited::LoadLTX(ini);

  	m_Flags.assign		(ini.r_u32("main","flags"));

    m_SunShadowDir		= ini.r_fvector2	("main", "sun_shadow_dir");
	lcontrol_last_idx	= ini.r_u32			("main", "lcontrol_last_idx");

	CInifile::Sect S = ini.r_section		("lcontrols");
	CInifile::SectCIt it = S.Data.begin();
	CInifile::SectCIt it_e = S.Data.end();
    for(; it!=it_e; ++it)
    {
    	u32 idx = ini.r_u32	("lcontrols", it->first.c_str());
       AppendLightControl(it->first.c_str(),&idx);
    }
	return true;
}

void ESceneLightTool::SaveLTX(CInifile& ini, int id)
{
	inherited::SaveLTX	(ini, id);

	ini.w_u32		("main", "version", LIGHT_TOOLS_VERSION);

    ini.w_u32		("main", "flags", m_Flags.get());

    ini.w_fvector2	("main", "sun_shadow_dir", m_SunShadowDir);

	ini.w_u32			("main", "lcontrol_last_idx", lcontrol_last_idx);

	RTokenVecIt		_I 	= lcontrols.begin();
    RTokenVecIt		_E 	= lcontrols.end();
    for (;_I!=_E; ++_I)
    {
        ini.w_u32	("lcontrols", _I->name.c_str(), _I->id);
    }
}

bool ESceneLightTool::LoadStream(IReader& F)
{
	u16 version 	= 0;
    if(F.r_chunk(CHUNK_VERSION,&version))
        if( version!=LIGHT_TOOLS_VERSION ){
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
        }

	if (!inherited::LoadStream(F)) return false;

    if (F.find_chunk(CHUNK_FLAGS))
    	m_Flags.assign		(F.r_u32());

    if (F.find_chunk(CHUNK_SUN_SHADOW)){
     	F.r_u8				();
        F.r_fvector2		(m_SunShadowDir);
    }

    if (F.find_chunk(CHUNK_LCONTROLS_LAST))
		lcontrol_last_idx	= F.r_u32();

	IReader* R 		= F.open_chunk(CHUNK_LCONTROLS);
    if (R){
        while (!R->eof()){
        	shared_str		l_name;
            R->r_stringZ(l_name);
            u32 l_idx	= R->r_u32();
            AppendLightControl(l_name.c_str(),&l_idx);
        }
        R->close		();
    }

    return true;
}
//----------------------------------------------------

void ESceneLightTool::SaveStream(IWriter& F)
{
	inherited::SaveStream	(F);

	F.w_chunk		(CHUNK_VERSION,(u16*)&LIGHT_TOOLS_VERSION,sizeof(LIGHT_TOOLS_VERSION));

	F.open_chunk	(CHUNK_FLAGS);
    F.w_u32			(m_Flags.get());
	F.close_chunk	();

	F.open_chunk	(CHUNK_SUN_SHADOW);
    F.w_u8			(0);
    F.w_fvector2	(m_SunShadowDir);
    F.close_chunk	();

	F.open_chunk	(CHUNK_LCONTROLS_LAST);
	F.w_u32			(lcontrol_last_idx);
    F.close_chunk	();

	F.open_chunk	(CHUNK_LCONTROLS);
	RTokenVecIt		_I 	= lcontrols.begin();
    RTokenVecIt		_E 	= lcontrols.end();
    for (;_I!=_E; _I++){
        F.w_stringZ	(_I->name);
        F.w_u32		(_I->id);
    }
    F.close_chunk	();
}
//----------------------------------------------------
 
bool ESceneLightTool::LoadSelection(IReader& F)
{
	u16 version 	= 0;
    R_ASSERT(F.r_chunk(CHUNK_VERSION,&version));
    if( version!=LIGHT_TOOLS_VERSION ){
        ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
        return false;
    }

	return inherited::LoadSelection(F);
}
//----------------------------------------------------

void ESceneLightTool::SaveSelection(IWriter& F)
{
	F.w_chunk		(CHUNK_VERSION,(u16*)&LIGHT_TOOLS_VERSION,sizeof(LIGHT_TOOLS_VERSION));
    
	inherited::SaveSelection(F);
}
//----------------------------------------------------


