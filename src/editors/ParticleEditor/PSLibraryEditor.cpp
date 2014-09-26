#include "stdafx.h"
#pragma hdrstop

#include "..\..\Layers\xrRender\PSLibrary.h"
#include "..\..\Layers\xrRender\ParticleEffect.h"
#include "..\..\Layers\xrRender\ParticleGroup.h"
#include "..\ECore\Editor\ui_main.h"
//------------------------------------------------------------------------------

void __fastcall CPSLibrary::FindByName(LPCSTR new_name, bool& res)
{
	res = (FindPED(new_name)||FindPGD(new_name));
}

PS::CPEDef* CPSLibrary::AppendPED(PS::CPEDef* src)
{
	m_PEDs.push_back(xr_new<PS::CPEDef>());
#ifdef _PARTICLE_EDITOR
    if (src) m_PEDs.back()->Copy(*src);
#endif
    return m_PEDs.back();
}
//------------------------------------------------------------------------------

PS::CPGDef* CPSLibrary::AppendPGD(PS::CPGDef* src)
{
	m_PGDs.push_back(xr_new<PS::CPGDef>());
    if (src) m_PGDs.back()->Clone(src);
    return m_PGDs.back();
}
//------------------------------------------------------------------------------

bool CPSLibrary::Save()
{
	xr_string temp_fn;
    if (EFS.GetSaveName	( "$game_data$", temp_fn))
	{    
    	return Save		(temp_fn.c_str());
    }else
    	return 			false;
}
//------------------------------------------------------------------------------
 bool CPSLibrary::Save2()
{
	FS.dir_delete("$game_particles$","",TRUE);
	string_path				fn;
    SPBItem* pb 		= UI->ProgressStart(m_PEDs.size()+m_PGDs.size(),"Saving particles...");
    for (PS::PEDIt it=m_PEDs.begin(); it!=m_PEDs.end(); ++it)
    {
       pb->Inc				();
    	PS::CPEDef*	pe 		= (*it);
        FS.update_path		(fn, "$game_particles$", pe->m_Name.c_str());
        strcat				(fn,".pe");
		CInifile 			ini(fn,FALSE,FALSE,FALSE);
		pe->Save2			(ini);
		ini.save_as			(fn);
    }

    for (PS::PGDIt g_it=m_PGDs.begin(); g_it!=m_PGDs.end(); ++g_it)
    {
       pb->Inc				();
    	PS::CPGDef*	pg 		= (*g_it);
        FS.update_path		(fn, "$game_particles$", pg->m_Name.c_str());
        strcat				(fn,".pg");
		CInifile 			ini(fn,FALSE,FALSE,FALSE);
        pg->Save2			(ini);
		ini.save_as			(fn);
    }
    UI->ProgressEnd		(pb);
    return true;
}


bool CPSLibrary::Save(const char* nm)
{
    CMemoryWriter F;

    F.open_chunk	(PS_CHUNK_VERSION);
    F.w_u16		(PS_VERSION);
    F.close_chunk	();

    F.open_chunk	(PS_CHUNK_SECONDGEN);
    u32 chunk_id = 0;
    for (PS::PEDIt it=m_PEDs.begin(); it!=m_PEDs.end(); ++it,++chunk_id)
    {
        F.open_chunk	(chunk_id);
        (*it)->Save	(F);
        F.close_chunk	();
    }
    F.close_chunk	();

    
    F.open_chunk	(PS_CHUNK_THIRDGEN);
    chunk_id = 0;
    for (PS::PGDIt g_it=m_PGDs.begin(); g_it!=m_PGDs.end(); ++g_it,++chunk_id)
    {
        F.open_chunk	(chunk_id);
        (*g_it)->Save	(F);
        F.close_chunk	();
    }
    F.close_chunk	();

    return F.save_to(nm);
}
//------------------------------------------------------------------------------

