#include "stdafx.h"
#pragma hdrstop

#include "SoundManager.h"
#include "../../../xrSound/soundrender_environment.h"
#include "EThumbnail.h"
#include "ui_main.h"

CSoundManager* SndLib=0;
//---------------------------------------------------------------------------
#pragma package(smart_init)

extern "C" int ogg_enc(const char* in_fn, const char* out_fn, float quality, void* comment, int size);
//------------------------------------------------------------------------------
xr_string CSoundManager::UpdateFileName(xr_string& fn)
{
	return EFS.AppendFolderToName(fn,-1,FALSE);
}

//------------------------------------------------------------------------------
// возвращает список всех звуков
//------------------------------------------------------------------------------
int CSoundManager::GetSounds(FS_FileSet& files, BOOL bFolders)
{
    return FS.file_list(files,_sounds_,(bFolders?FS_ListFolders:0)|FS_ListFiles|FS_ClampExt,"*.wav");
}

int CSoundManager::GetGameSounds(FS_FileSet& files)
{
    return FS.file_list(files,_game_sounds_,FS_ListFiles|FS_ClampExt,"*.ogg");
}

int	CSoundManager::GetSoundEnvs(AStringVec& items)
{
/*
	SoundEnvironment_LIB* Lib = Sound->get_env_library();
    if (Lib){
	    for (SoundEnvironment_LIB::SE_IT it=Lib->Library().begin(); it!=Lib->Library().end(); it++)
    	    items.push_back(*(*it)->name);
    }
	return items.size();
*/
return 0;    
}

bool CSoundManager::OnCreate()
{
//.	psSoundFreq			= sf_44K;
//	psSoundFlags.set	(ssHardware,FALSE);
    CSound_manager_interface::_create		(0);
    CSound_manager_interface::_create		(1);
    return true;
}

void CSoundManager::OnDestroy()
{
    CSound_manager_interface::_destroy		();
}

void CSoundManager::OnFrame()
{
	::psSoundVEffects		= psDeviceFlags.is(rsMuteSounds)?0.f:1.f;
	Sound->update			(EDevice.m_Camera.GetPosition(), EDevice.m_Camera.GetDirection(), EDevice.m_Camera.GetNormal());
}

void CSoundManager::MuteSounds(BOOL bVal)
{
	if (bVal) 	::psSoundVEffects = 0.f;
    else		::psSoundVEffects = psDeviceFlags.is(rsMuteSounds)?0.f:1.f;
}

void CSoundManager::RenameSound(LPCSTR nm0, LPCSTR nm1, EItemType type)
{
	if (TYPE_FOLDER==type){
    	FS.dir_delete			(_sounds_,nm0,FALSE);
    	FS.dir_delete			(_game_sounds_,nm0,FALSE);
    }else if (TYPE_OBJECT==type){
        string_path fn0,fn1,temp;
        // rename base file
        FS.update_path(fn0,_sounds_,nm0); 	strcat(fn0,".wav");
        FS.update_path(fn1,_sounds_,nm1);	strcat(fn1,".wav");
        FS.file_rename(fn0,fn1,false);

        // rename thm
        FS.update_path(fn0,_sounds_,nm0);	strcat(fn0,".thm");
        FS.update_path(fn1,_sounds_,nm1);	strcat(fn1,".thm");
        FS.file_rename(fn0,fn1,false);

        // rename ogg
        FS.update_path(fn0,_game_sounds_,nm0);	strcat(fn0,".ogg");
        FS.update_path(fn1,_game_sounds_,nm1);	strcat(fn1,".ogg");
        FS.file_rename(fn0,fn1,false);

	    Sound->refresh_sources();
	}
}

BOOL CSoundManager::RemoveSound(LPCSTR fname, EItemType type)
{
	if (TYPE_FOLDER==type){
    	FS.dir_delete			(_sounds_,fname,FALSE);
    	FS.dir_delete			(_game_sounds_,fname,FALSE);
		return TRUE;
    }else if (TYPE_OBJECT==type){
        xr_string src_name;
        src_name				= EFS.ChangeFileExt(fname,".wav");
        if (FS.exist(_sounds_,src_name.c_str())){
            xr_string thm_name = EFS.ChangeFileExt(fname,".thm");
            xr_string game_name= EFS.ChangeFileExt(fname,".ogg");
            // source
            FS.file_delete		(_sounds_,src_name.c_str());
            // thumbnail
            FS.file_delete		(_sounds_,thm_name.c_str());
            // game
            FS.file_delete		(_game_sounds_,game_name.c_str());
		    Sound->refresh_sources();
            return TRUE;
        }
    }
    return FALSE;
}

//------------------------------------------------------------------------------
// возвращает список новых звуков
//------------------------------------------------------------------------------
int CSoundManager::GetLocalNewSounds(FS_FileSet& files)
{
    return FS.file_list	(files,_import_,FS_ListFiles|FS_RootOnly|FS_ClampExt,"*.wav");
}

//------------------------------------------------------------------------------
// копирует обновленные звуки с Import'a в Sounds
// files - список файлов для копирование
//------------------------------------------------------------------------------
/*
void CSoundManager::SafeCopyLocalToServer(FS_FileSet& files)
{
    xr_string p_import,p_sounds;
    xr_string src_name,dest_name;
    FS.update_path	  	(p_import,_import_,"");
    FS.update_path	  	(p_sounds,_sounds_,"");

    FS_FileSetIt it	= files.begin();
	FS_FileSetIt _E 	= files.end();
	for (; it!=_E; it++){
        // copy thm
		xr_string fn 	= EFS.ChangeFileExt(it->first,".thm");
		src_name 		= p_import	+ fn;
		UpdateFileName	(fn);
		dest_name 		= p_sounds	+ fn;
		FS.file_rename	(src_name.c_str(),dest_name.c_str(),true);
    	// copy sources
		fn 				= EFS.ChangeFileExt(it->first,".wav");
		src_name 		= p_import	+ fn;
		UpdateFileName	(fn);
		dest_name 		= p_sounds	+ fn;
        if (FS.exist(dest_name.c_str())){
	        EFS.BackupFile	(_sounds_,EFS.ChangeFileExt(fn,".wav").c_str());
	        EFS.BackupFile	(_sounds_,EFS.ChangeFileExt(fn,".thm").c_str());
        }
        FS.file_copy		(src_name.c_str(),dest_name.c_str());
        FS.set_file_age		(dest_name.c_str(),FS.get_file_age(src_name.c_str()));
        EFS.WriteAccessLog	(dest_name.c_str(),"Replace");
        EFS.MarkFile		(src_name.c_str(),true);
    }
}    
*/
//------------------------------------------------------------------------------
// создает тхм
//------------------------------------------------------------------------------
void CSoundManager::CreateSoundThumbnail(ESoundThumbnail* THM, const AnsiString& src_name, LPCSTR initial, bool bSetDefParam)
{
	R_ASSERT(src_name.Length());
	string_path base_name;
    if (initial)	FS.update_path(base_name,initial,src_name.c_str());
    else			FS.update_path(base_name,_sounds_,src_name.c_str());
	strcpy			(base_name, EFS.ChangeFileExt(base_name,".wav").c_str());

    // выставить начальные параметры
	if (bSetDefParam){
		THM->m_Age 				= FS.get_file_age(base_name);
		THM->m_fQuality 		= 0.f;
		THM->m_fMinDist 		= 1.f;
        THM->m_uGameType		= 0;
    }
}                             

void CSoundManager::MakeGameSound(ESoundThumbnail* THM, LPCSTR src_name, LPCSTR game_name)
{
	VerifyPath		(game_name);
    CMemoryWriter 	F;
    F.w_u32			(OGG_COMMENT_VERSION);
    F.w_float		(THM->m_fMinDist);
    F.w_float		(THM->m_fMaxDist);
    F.w_float		(THM->m_fBaseVolume);
    F.w_u32			(THM->m_uGameType);
    F.w_float		(THM->m_fMaxAIDist);

	if (!ogg_enc(src_name, game_name, THM->m_fQuality, F.pointer(), F.size()))
    {
    	FS.file_delete(game_name);
    	ELog.DlgMsg(mtError,"Can't make game sound '%s'.",game_name);
    }
}

//------------------------------------------------------------------------------
// возвращает список не синхронизированных (модифицированных) текстур
// source_list - содержит список текстур с расширениями
// sync_list - реально сохраненные файлы (после использования освободить)
//------------------------------------------------------------------------------
void CSoundManager::SynchronizeSounds(bool sync_thm, bool sync_game, bool bForceGame, FS_FileSet* source_list, AStringVec* sync_list, FS_FileSet* modif_map)
{        
	FS_FileSet M_BASE;
	FS_FileSet M_THUM;
    FS_FileSet M_GAME;

    if (source_list)
    	M_BASE = *source_list;
    else
    	FS.file_list(M_BASE,_sounds_,FS_ListFiles|FS_ClampExt,"*.wav");

    if (M_BASE.empty())
    	return;

    if (sync_thm)
    	FS.file_list(M_THUM,_sounds_,FS_ListFiles|FS_ClampExt,"*.thm");
        
    if (sync_game)
    	FS.file_list(M_GAME,_game_sounds_,FS_ListFiles|FS_ClampExt,"*.ogg");

    bool bProgress 				= (M_BASE.size()>1);

    int m_age					= time(NULL);

    SPBItem* pb = 0;
    if (bProgress)
    	pb = UI->ProgressStart(M_BASE.size(),"Synchronize sounds...");

    FS_FileSetIt it				= M_BASE.begin();
	FS_FileSetIt it_e 			= M_BASE.end();
	for (; it!=it_e; ++it)
    {
	    BOOL bUpdated 			= FALSE;
        xr_string base_name		= EFS.ChangeFileExt(it->name,"");
        xr_strlwr				(base_name);
        string_path				fn;
        FS.update_path			(fn,_sounds_,EFS.ChangeFileExt(base_name,".wav").c_str());
    	if (!FS.exist(fn))
        	continue;

		FS_FileSetIt th 		= M_THUM.find(base_name);
    	bool bThm 				= ((th==M_THUM.end()) || ((th!=M_THUM.end())&&(th->time_write!=it->time_write)));
  		FS_FileSetIt gm 		= M_GAME.find(base_name);
    	bool bGame				= bThm || ((gm==M_GAME.end()) || ((gm!=M_GAME.end())&&(gm->time_write!=it->time_write)));

		ESoundThumbnail* THM	= 0;

        // backup base sound
    	// check thumbnail
    	if (sync_thm&&bThm){
        	THM 				= xr_new<ESoundThumbnail>(it->name.c_str());
            THM->Save			(m_age);
            bUpdated 			= TRUE;
        }
        // check game sounds
    	if (bForceGame||(sync_game&&bGame))
        {
        	if (!THM) THM 			= xr_new<ESoundThumbnail>(it->name.c_str());
            R_ASSERT(THM);
            string_path 			src_name;
            strconcat				(sizeof(src_name),src_name, base_name.c_str(), ".wav");

            FS.update_path			(src_name,_sounds_,src_name);
            string_path 			game_name;
            strconcat				(sizeof(game_name),game_name, base_name.c_str(), ".ogg");
            FS.update_path			(game_name,_game_sounds_,game_name);
            MakeGameSound			(THM,src_name,game_name);

            if (sync_list)
            	sync_list->push_back(base_name.c_str());

            if (modif_map)
            	modif_map->insert(*it);

            bUpdated = TRUE;
		}

		if (THM)
        	xr_delete(THM);

		if (UI->NeedAbort())
        	break;

        if (bProgress)
		    pb->Inc		(bUpdated?xr_string(base_name+" - UPDATED.").c_str():"",bUpdated);

        if (bUpdated)
        {
            string_path				wav_fn,thm_fn,ogg_fn;
            FS.update_path			(wav_fn,_sounds_,		EFS.ChangeFileExt(base_name,".wav").c_str());
            FS.update_path			(thm_fn,_sounds_,		EFS.ChangeFileExt(base_name,".thm").c_str());
            FS.update_path			(ogg_fn,_game_sounds_,	EFS.ChangeFileExt(base_name,".ogg").c_str());
            FS.set_file_age			(wav_fn,m_age);
            FS.set_file_age			(thm_fn,m_age);
            FS.set_file_age			(ogg_fn,m_age);
        }
    }
    if (bProgress)
    	UI->ProgressEnd(pb);
}

void CSoundManager::CleanupSounds()
{
	FS_FileSet 	    M_BASE;
	FS_FileSet 	    M_THUM;
    FS_FileSet 	    M_GAME;
    FS_FileSet 	    M_GAME_DEL;
    FS_FileSet 	    M_THM_DEL;

    FS.file_list	(M_BASE,_sounds_,		FS_ListFiles|FS_ClampExt,"*.wav");
    FS.file_list	(M_THUM,_sounds_,		FS_ListFiles|FS_ClampExt,"*.thm");
    FS.file_list	(M_GAME,_game_sounds_,	FS_ListFiles|FS_ClampExt,"*.ogg");

    FS_FileSetIt 	it;
	FS_FileSetIt 	it_e;

    // check source exist
    it				= M_GAME.begin();
	it_e 			= M_GAME.end();

	for (; it!=it_e; ++it)
    {
        xr_string base_name		= EFS.ChangeFileExt(it->name,"");
        xr_strlwr				(base_name);
		FS_FileSetIt bs 		= M_BASE.find(base_name);

    	if (bs==M_BASE.end())
        	M_GAME_DEL.insert	(*it);
    }
    it				= M_THUM.begin();
	it_e 			= M_THUM.end();

	for (; it!=it_e; ++it)
    {
        xr_string base_name		= EFS.ChangeFileExt(it->name,"");
        xr_strlwr				(base_name);
		FS_FileSetIt bs 		= M_BASE.find(base_name);
    	if (bs==M_BASE.end())
        	M_THM_DEL.insert	(*it);
    }

    SPBItem* pb = UI->ProgressStart(M_GAME_DEL.size()+M_THM_DEL.size(),"Cleanup sounds...");
    // mark game del sounds
    it							= M_GAME_DEL.begin();
	it_e 						= M_GAME_DEL.end();

	for (; it!=it_e; ++it)
    {
        xr_string base_name		= EFS.ChangeFileExt(it->name,"");
        xr_strlwr				(base_name);
        string_path 			fn;
        FS.update_path			(fn,_game_sounds_,EFS.ChangeFileExt(base_name,".ogg").c_str());
        EFS.MarkFile			(fn,true);
        pb->Inc					();
    }
    // mark thm sounds
    it							= M_THM_DEL.begin();
	it_e 						= M_THM_DEL.end();
	for (; it!=it_e; ++it)
    {
        xr_string base_name		= EFS.ChangeFileExt(it->name,"");
        xr_strlwr				(base_name);
        string_path 			fn;
        FS.update_path			(fn,_sounds_,EFS.ChangeFileExt(base_name,".thm").c_str());
        EFS.MarkFile			(fn,true);
        pb->Inc					();
    }
    UI->ProgressEnd				(pb);
}

/*
void CSoundManager::ChangeFileAgeTo(FS_FileSet* tgt_map, int age)
{
	VERIFY(tgt_map);
	FS_FileSet* 	M_BASE 		= tgt_map;
    
    // lock rescanning
    FS.lock_rescan	();
    // change
	SPBItem* pb=0;
    if (M_BASE->size()>1) pb	= UI->ProgressStart(M_BASE->size(),"Change sounds age...");
    FS_FileSetIt it			= M_BASE->begin();
	FS_FileSetIt _E 			= M_BASE->end();
	for (; it!=_E; it++){
        xr_string base_name	= EFS.ChangeFileExt(it->first,""); xr_strlwr(base_name);
        xr_string	wav_fn,thm_fn,ogg_fn;
        FS.update_path			(wav_fn,_sounds_,		EFS.ChangeFileExt(base_name,".wav").c_str());
        FS.update_path			(thm_fn,_sounds_,		EFS.ChangeFileExt(base_name,".thm").c_str());
        FS.update_path			(ogg_fn,_game_sounds_,	EFS.ChangeFileExt(base_name,".ogg").c_str());
        FS.set_file_age			(wav_fn.c_str(),age);
        FS.set_file_age			(thm_fn.c_str(),age);
        FS.set_file_age			(ogg_fn.c_str(),age);
        if (pb) 			    pb->Inc();
    }
    if (pb) 					UI->ProgressEnd(pb);
    // lock rescanning
    FS.unlock_rescan			();
}
*/
//------------------------------------------------------------------------------
// если передан параметр modif - обновляем DX-Surface only и только из списка
// иначе полная синхронизация
//------------------------------------------------------------------------------
void CSoundManager::RefreshSounds(bool bSync)
{
    if (FS.can_write_to_alias(_sounds_)){
        UI->SetStatus("Refresh sounds...");
        if (bSync){
            SynchronizeSounds	(true,true,false,0,0);
            CleanupSounds		();
        }
        Sound->refresh_sources();
        UI->SetStatus("");
    }else{
        Log("#!You don't have permisions to modify sounds.");
    }
}

