//----------------------------------------------------
// file: PSLibrary.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "PSLibrary.h"
#include "ParticleEffect.h"
#include "ParticleGroup.h"

#ifdef _EDITOR
#	include "ParticleEffectActions.h"
#include "../ECore/Editor/ui_main.h"
#endif

#define _game_data_			"$game_data$"

bool ped_sort_pred	(const PS::CPEDef* a, 	const PS::CPEDef* b)	{	return xr_strcmp(a->Name(),b->Name())<0;}
bool pgd_sort_pred	(const PS::CPGDef* a, 	const PS::CPGDef* b)	{	return xr_strcmp(a->m_Name,b->m_Name)<0;}

bool ped_find_pred	(const PS::CPEDef* a, 	LPCSTR b)				{	return xr_strcmp(a->Name(),b)<0;}
bool pgd_find_pred	(const PS::CPGDef* a, 	LPCSTR b)				{	return xr_strcmp(a->m_Name,b)<0;}
//----------------------------------------------------
void CPSLibrary::OnCreate()
{
#ifdef _EDITOR
    if(pCreateEAction)
    {
        Load2();
    }else
#endif
    {
    	string_path		fn;
        FS.update_path	(fn,_game_data_,"particles.xr");
        Load			(fn);
    }
}
 
void CPSLibrary::OnDestroy()
{
	for (PS::PEDIt e_it = m_PEDs.begin(); e_it!=m_PEDs.end(); e_it++)
    	(*e_it)->DestroyShader();

	for (e_it = m_PEDs.begin(); e_it!=m_PEDs.end(); e_it++)
		xr_delete	(*e_it);
	m_PEDs.clear	();

	for (PS::PGDIt g_it = m_PGDs.begin(); g_it!=m_PGDs.end(); g_it++)
		xr_delete	(*g_it);
	m_PGDs.clear	();
}
//----------------------------------------------------
PS::PEDIt CPSLibrary::FindPEDIt(LPCSTR Name)
{
	if (!Name) return m_PEDs.end();
#ifdef _EDITOR
	for (PS::PEDIt it=m_PEDs.begin(); it!=m_PEDs.end(); it++)
    	if (0==xr_strcmp((*it)->Name(),Name)) return it;
	return m_PEDs.end();
#else
	PS::PEDIt I = std::lower_bound(m_PEDs.begin(),m_PEDs.end(),Name,ped_find_pred);
	if (I==m_PEDs.end() || (0!=xr_strcmp((*I)->m_Name,Name)))	return m_PEDs.end();
	else														return I;
#endif
}

PS::CPEDef* CPSLibrary::FindPED(LPCSTR Name)
{
	PS::PEDIt it = FindPEDIt(Name);
    return (it==m_PEDs.end())?0:*it;
}

PS::PGDIt CPSLibrary::FindPGDIt(LPCSTR Name)
{
	if (!Name) return m_PGDs.end();
#ifdef _EDITOR
	for (PS::PGDIt it=m_PGDs.begin(); it!=m_PGDs.end(); it++)
    	if (0==xr_strcmp((*it)->m_Name,Name)) return it;
	return m_PGDs.end();
#else
	PS::PGDIt I = std::lower_bound(m_PGDs.begin(),m_PGDs.end(),Name,pgd_find_pred);
	if (I==m_PGDs.end() || (0!=xr_strcmp((*I)->m_Name,Name)))	return m_PGDs.end();
	else														return I;
#endif
}

PS::CPGDef* CPSLibrary::FindPGD(LPCSTR Name)
{
	PS::PGDIt it = FindPGDIt(Name);
    return (it==m_PGDs.end())?0:*it;
}

void CPSLibrary::RenamePED(PS::CPEDef* src, LPCSTR new_name)
{
	R_ASSERT(src&&new_name&&new_name[0]);
	src->SetName(new_name);
}

void CPSLibrary::RenamePGD(PS::CPGDef* src, LPCSTR new_name)
{
	R_ASSERT(src&&new_name&&new_name[0]);
	src->SetName(new_name);
}

void CPSLibrary::Remove(const char* nm)
{
	PS::PEDIt it = FindPEDIt(nm);
	if (it!=m_PEDs.end())
    {
		(*it)->DestroyShader();
		xr_delete		(*it);
		m_PEDs.erase	(it);
	}else
    {
		PS::PGDIt it = FindPGDIt(nm);
		if (it!=m_PGDs.end())
        {
			xr_delete	(*it);
			m_PGDs.erase(it);
		}
	}
}
//----------------------------------------------------
bool CPSLibrary::Load2()
{
	FS_FileSet					files;
	string_path					_path;
    FS.update_path				(_path, "$game_particles$", "");

	FS.file_list				(files, _path, FS_ListFiles, "*.pe,*.pg");

#ifdef _EDITOR
	SPBItem* pb = NULL;
	if(UI->m_bReady)
    pb 							= UI->ProgressStart(files.size(),"Loading particles...");
#endif
	FS_FileSet::iterator it		= files.begin();
	FS_FileSet::iterator it_e	= files.end();

    string_path 				p_path, p_name, p_ext;
	for(;it!=it_e;++it)
	{
		const FS_File& f		= (*it);
	    _splitpath				(f.name.c_str(), 0, p_path, p_name, p_ext );
        FS.update_path			(_path, "$game_particles$",f.name.c_str());
        CInifile				ini (_path,TRUE,TRUE,FALSE);

#ifdef _EDITOR
        if(pb) pb->Inc					();
#endif

        xr_sprintf				(_path, sizeof(_path),"%s%s",p_path, p_name);
        if(0==stricmp(p_ext,".pe"))
        {
            PS::CPEDef*	def		= xr_new<PS::CPEDef>();
            def->m_Name			= _path;
            if (def->Load2(ini)) 
            	m_PEDs.push_back(def);
            else
            	xr_delete		(def);
        }else
        if(0==stricmp(p_ext,".pg"))
        {
            PS::CPGDef*	def		= xr_new<PS::CPGDef>();
            def->m_Name			= _path;
            if (def->Load2(ini)) 
            	m_PGDs.push_back(def);
            else
            	xr_delete		(def);
        }else
        {
        	R_ASSERT(0);
        }
	}

	std::sort			(m_PEDs.begin(),m_PEDs.end(),ped_sort_pred);
	std::sort			(m_PGDs.begin(),m_PGDs.end(),pgd_sort_pred);

	for (PS::PEDIt e_it = m_PEDs.begin(); e_it!=m_PEDs.end(); e_it++)
    	(*e_it)->CreateShader();

#ifdef _EDITOR
    if(pb) UI->ProgressEnd		(pb);
#endif
	Msg				("Loaded particles :%d", files.size());
	return true;
}


bool CPSLibrary::Load(const char* nm)
{
    if (!FS.exist(nm))
    {
        Msg("Can't find file: '%s'",nm);
        return 				false;
    }
    
	IReader*	F			= FS.r_open(nm);
	bool bRes 				= true;
    R_ASSERT(F->find_chunk(PS_CHUNK_VERSION));
    u16 ver					= F->r_u16();
    if (ver!=PS_VERSION) return false;
    // second generation
    IReader* OBJ;
    OBJ			 			= F->open_chunk(PS_CHUNK_SECONDGEN);
    if (OBJ){
        IReader* O   		= OBJ->open_chunk(0);
        for (int count=1; O; count++) {
            PS::CPEDef*	def	= xr_new<PS::CPEDef>();
            if (def->Load(*O)) m_PEDs.push_back(def);
            else{ bRes = false; xr_delete(def); }
            O->close();
            if (!bRes)	break;
            O 			= OBJ->open_chunk(count);
        }
        OBJ->close();
    }
    // second generation
    OBJ 					= F->open_chunk(PS_CHUNK_THIRDGEN);
    if (OBJ){
        IReader* O   		= OBJ->open_chunk(0);
        for (int count=1; O; count++) {
            PS::CPGDef*	def	= xr_new<PS::CPGDef>();
            if (def->Load(*O)) m_PGDs.push_back(def);
            else{ bRes = false; xr_delete(def); }
            O->close();
            if (!bRes) break;
            O 			= OBJ->open_chunk(count);
        }
        OBJ->close();
    }

    // final
	FS.r_close			(F);

	std::sort			(m_PEDs.begin(),m_PEDs.end(),ped_sort_pred);
	std::sort			(m_PGDs.begin(),m_PGDs.end(),pgd_sort_pred);

	for (PS::PEDIt e_it = m_PEDs.begin(); e_it!=m_PEDs.end(); e_it++)
    	(*e_it)->CreateShader();

    return			bRes;
}
//----------------------------------------------------
void CPSLibrary::Reload()
{
	OnDestroy();
    OnCreate();
	Msg( "PS Library was succesfully reloaded." );
}
//----------------------------------------------------

using PS::CPGDef;

CPGDef const* const* CPSLibrary::particles_group_begin	() const
{
	return	(m_PGDs.size() ? &*m_PGDs.begin() : 0);
}

CPGDef const* const* CPSLibrary::particles_group_end	() const
{
	return	(m_PGDs.size() ? &*m_PGDs.end() : 0);
}

void CPSLibrary::particles_group_next			(PS::CPGDef const* const*& iterator) const
{
	VERIFY	(iterator);
	VERIFY	(iterator >= particles_group_begin());
	VERIFY	(iterator <  particles_group_end());
	++iterator;
}

shared_str const& CPSLibrary::particles_group_id(CPGDef const& particles_group) const
{
	return	(particles_group.m_Name);
}
