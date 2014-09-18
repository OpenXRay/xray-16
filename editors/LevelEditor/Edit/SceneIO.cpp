//----------------------------------------------------
// file: Scene.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "scene.h"
#include "../ECore/Editor/ui_main.h"
#include "ui_leveltools.h"
#include "CustomObject.h"
#include "ESceneObjectTools.h"
#include "SceneObject.h"
#include "../ECore/Editor/ExportObjectOGF.h"
#include "Builder.h"
#include "SpawnPoint.h"
// file: SceneChunks.h
#define CURRENT_FILE_VERSION    	0x00000005

#define CURRENT_LEVELOP_VERSION 	0x0000000C
//0x00000008

#define CURRENT_LEVELOP_BP_VERSION 	0x00000009
#define CURRENT_ENV_VERSION	 		0x00000007

#define CHUNK_VERSION       	0x9df3
#define CHUNK_OBJECT_CLASS  	0x7703
#define CHUNK_OBJECT_LIST		0x7708
#define CHUNK_CAMERA        	0x7709
#define CHUNK_SNAPOBJECTS   	0x7710
#define CHUNK_LEVELOP       	0x7711
#define CHUNK_OBJECT_COUNT  	0x7712
#define CHUNK_LEVEL_TAG			0x7777

#define CHUNK_TOOLS_GUID		0x7000
#define CHUNK_TOOLS_DATA		0x8000

// level options
#define CHUNK_LO_VERSION		0x7801
#define CHUNK_LO_NAMES 			0x7802
#define CHUNK_LO_BOP		 	0x7803
#define CHUNK_LO_PREFIX 		0x7804
#define CHUNK_LO_BP_VERSION		0x7849
#define CHUNK_BUILD_PARAMS		0x7850
#define CHUNK_LIGHT_QUALITY		0x7851
#define CHUNK_MAP_USAGE			0x7852
#define CHUNK_LO_MAP_VER	 	0x7853
//------------------------------------------------------------------------------------------------
// Level Options
//------------------------------------------------------------------------------------------------
void st_LevelOptions::SaveLTX( CInifile& ini )
{
	LPCSTR section 	= "level_options";
	ini.w_u32		(section, "version", CURRENT_LEVELOP_VERSION);

	ini.w_string	(section, "level_path", m_FNLevelPath.c_str());
	ini.w_string	(section, "level_prefix", m_LevelPrefix.c_str());
	xr_string s		= "\"";
    s 				+= m_BOPText.c_str();
    s				+= "\"";
	ini.w_string	(section, "bop", s.c_str());
	ini.w_string	(section, "map_version", m_map_version.c_str());

	ini.w_u32		(section, "version_bp", CURRENT_LEVELOP_BP_VERSION);

	m_BuildParams.SaveLTX	(ini);

	ini.w_u8		(section, "light_hemi_quality", m_LightHemiQuality );
	ini.w_u8		(section, "light_sun_quality", m_LightSunQuality );

    m_mapUsage.SaveLTX(ini,section);
}

void st_LevelOptions::Save( IWriter& F )
{
    F.open_chunk( CHUNK_LO_VERSION );
	F.w_u32		( CURRENT_LEVELOP_VERSION );
    F.close_chunk();

    F.open_chunk( CHUNK_LO_NAMES );
	F.w_stringZ	( m_FNLevelPath.size()?m_FNLevelPath.c_str():"" );
    F.close_chunk();

    F.open_chunk( CHUNK_LO_PREFIX );
	F.w_stringZ	( m_LevelPrefix.size()?m_LevelPrefix.c_str():"");
    F.close_chunk();

    F.open_chunk( CHUNK_LO_BOP );
	F.w_stringZ	( m_BOPText.size()?m_BOPText.c_str():"" );
    F.close_chunk();

    F.open_chunk( CHUNK_LO_MAP_VER );
	F.w_stringZ	( m_map_version.size()?m_map_version.c_str():"1.0" );
    F.close_chunk();

    F.open_chunk( CHUNK_LO_BP_VERSION );
	F.w_u32		( CURRENT_LEVELOP_BP_VERSION );
    F.close_chunk();

    F.open_chunk( CHUNK_BUILD_PARAMS );
	F.w			( &m_BuildParams, sizeof(m_BuildParams) );
    F.close_chunk();

    F.open_chunk( CHUNK_LIGHT_QUALITY );
	F.w_u8		( m_LightHemiQuality );
	F.w_u8		( m_LightSunQuality );
    F.close_chunk();

    F.open_chunk( CHUNK_MAP_USAGE );
	F.w_u16		( m_mapUsage.m_GameType.get() );
    F.close_chunk();
}

void st_LevelOptions::ReadLTX(CInifile& ini)
{
	LPCSTR section 	= "level_options";

    u32 vers_op 		= ini.r_u32(section, "version");
    if( vers_op < 0x00000008 )
    {
        ELog.DlgMsg( mtError, "Skipping bad version of level options." );
        return;
    }

    m_FNLevelPath		= ini.r_string 		(section, "level_path");
    m_LevelPrefix		= ini.r_string 		(section, "level_prefix");
    m_BOPText			= ini.r_string_wb	(section, "bop");

    if(vers_op > 0x0000000B)
    	m_map_version		= ini.r_string		(section, "map_version");

    m_BuildParams.LoadLTX(ini);

    m_LightHemiQuality 				= ini.r_u8(section, "light_hemi_quality" );
    m_LightSunQuality 				= ini.r_u8(section, "light_sun_quality" );

    m_mapUsage.SetDefaults			();
    if(vers_op > 0x0000000A)
	{
     m_mapUsage.LoadLTX				(ini,section,false);
    }else
    {

    m_mapUsage.m_GameType.set		(eGameIDDeathmatch ,	ini.r_s32(section, "usage_deathmatch"));
    m_mapUsage.m_GameType.set		(eGameIDTeamDeathmatch, ini.r_s32(section, "usage_teamdeathmatch"));
    m_mapUsage.m_GameType.set		(eGameIDArtefactHunt,	ini.r_s32(section, "usage_artefacthunt"));


	if(vers_op > 0x00000008)
    {
        m_mapUsage.m_GameType.set	(eGameIDCaptureTheArtefact,	ini.r_s32(section, "usage_captretheartefact"));

        m_mapUsage.m_GameType.set	(eGameIDTeamDominationZone,	ini.r_s32(section, "usage_team_domination_zone"));
        if(vers_op==0x00000009)
        	m_mapUsage.m_GameType.set(eGameIDDominationZone,		ini.r_s32(section, "domination_zone"));
        else
        	m_mapUsage.m_GameType.set(eGameIDDominationZone,		ini.r_s32(section, "usage_domination_zone"));
     }
    }
}

void st_LevelOptions::Read(IReader& F)
{
	R_ASSERT(F.find_chunk(CHUNK_LO_VERSION));
    DWORD vers = F.r_u32( );
    if( vers < 0x00000008 )
    {
        ELog.DlgMsg( mtError, "Skipping bad version of level options." );
        return;
    }

    R_ASSERT(F.find_chunk(CHUNK_LO_NAMES));
    F.r_stringZ 	(m_FNLevelPath);

    if (F.find_chunk(CHUNK_LO_PREFIX)) F.r_stringZ 	(m_LevelPrefix);

    R_ASSERT(F.find_chunk(CHUNK_LO_BOP));
    F.r_stringZ 	(m_BOPText); 

    if (F.find_chunk(CHUNK_LO_MAP_VER))
	    F.r_stringZ	(m_map_version);

    vers = 0;
    if (F.find_chunk(CHUNK_LO_BP_VERSION))
	    vers = F.r_u32( );

    if (CURRENT_LEVELOP_BP_VERSION==vers){
	    if (F.find_chunk(CHUNK_BUILD_PARAMS)) 	
        	F.r(&m_BuildParams, sizeof(m_BuildParams));
    }else{
        ELog.DlgMsg	(mtError, "Skipping bad version of build params.");
    	m_BuildParams.Init();
    }

    if (F.find_chunk(CHUNK_LIGHT_QUALITY))
    {
	    m_LightHemiQuality 	= F.r_u8();
	    m_LightSunQuality 	= F.r_u8();
    }
    if (F.find_chunk(CHUNK_MAP_USAGE))
    {
    	if(vers > 0x00000008)
        {
          m_mapUsage.m_GameType.assign	(F.r_u16());
        }else
        {
            m_mapUsage.m_GameType.zero					();
            m_mapUsage.m_GameType.set					(eGameIDDeathmatch ,	F.r_s32());
            m_mapUsage.m_GameType.set					(eGameIDTeamDeathmatch, F.r_s32());
            m_mapUsage.m_GameType.set					(eGameIDArtefactHunt,	F.r_s32());
        }
    }
}

//------------------------------------------------------------------------------------------------
// Scene
//------------------------------------------------------------------------------------------------
BOOL EScene::LoadLevelPartLTX(ESceneToolBase* M, LPCSTR mn)
{
	string_path map_name;
    strcpy(map_name, mn);
    
	if(!M->can_use_inifile())
    	return LoadLevelPart(M, map_name);

    int fnidx=0;
    while(FS.exist(map_name))
    {
        IReader* R		= FS.r_open	(map_name);
        VERIFY			(R);
        char 			ch;
        R->r			(&ch,sizeof(ch));
        bool b_is_inifile = (ch=='[');
        FS.r_close		(R);

        if(!b_is_inifile)
            return LoadLevelPart(M, map_name);

        M->m_EditFlags.set(ESceneToolBase::flReadonly,FALSE);

        CInifile			ini(map_name);

        // check level part GUID
        xrGUID				guid;
        guid.LoadLTX		(ini, "guid", "guid");

        if (guid!=m_GUID)
        {
            ELog.DlgMsg		(mtError,"Skipping invalid version of level part: '%s\\%s.part'",EFS.ExtractFileName(map_name).c_str(),M->ClassName());
            return 			FALSE;
        }
        // read data
        M->LoadLTX			(ini);

		++fnidx;
        sprintf(map_name, "%s%d", mn, fnidx);
    }

    return 					TRUE;
}

BOOL EScene::LoadLevelPart(ESceneToolBase* M, LPCSTR map_name)
{
	if(M->can_use_inifile())
	    return LoadLevelPartLTX(M, map_name);
        
	if (FS.exist(map_name))
    {
		// check locking
        M->m_EditFlags.set(ESceneToolBase::flReadonly,FALSE);

        IReader* R		= FS.r_open	(map_name);
        VERIFY			(R);
    	// check level part GUID
        R_ASSERT		(R->find_chunk	(CHUNK_TOOLS_GUID));
        xrGUID			guid;
        R->r			(&guid,sizeof(guid));

        if (guid!=m_GUID)
        {
            ELog.DlgMsg		(mtError,"Skipping invalid version of level part: '%s\\%s.part'",EFS.ExtractFileName(map_name).c_str(),M->ClassName());
        	FS.r_close		(R);
            return 			FALSE;
        }
        // read data
        IReader* chunk 	= R->open_chunk	(CHUNK_TOOLS_DATA+M->ClassID);
        if(chunk!=NULL)
        {
            M->LoadStream	(*chunk);
            chunk->close	();
        }else
        {
            ELog.DlgMsg		(mtError,"Skipping corrupted version of level part: '%s\\%s.part'",EFS.ExtractFileName(map_name).c_str(),M->ClassName());
            FS.r_close		(R);
            return 			FALSE;
        }
        //success
        FS.r_close			(R);
	    return 				TRUE;
    }
    return 					TRUE;
}

BOOL EScene::LoadLevelPart(LPCSTR map_name, ObjClassID cls)
{
	xr_string pn	= LevelPartName(map_name,cls);
    if (LoadLevelPart(GetTool(cls),pn.c_str()))
    	return 		TRUE;
    else
	    return 			FALSE;
}

BOOL EScene::UnloadLevelPart(ESceneToolBase* M)
{
	M->Clear		();
    return 			TRUE;
}

BOOL EScene::UnloadLevelPart(LPCSTR map_name, ObjClassID cls)
{
	xr_string pn	= LevelPartName(map_name,cls);
    if (UnloadLevelPart(GetTool(cls)))
    	return 		TRUE;
    else
    	return			FALSE;
}

xr_string EScene::LevelPartPath(LPCSTR full_name)
{
    return 			EFS.ExtractFilePath(full_name)+EFS.ExtractFileName(full_name)+"\\";
}

xr_string EScene::LevelPartName(LPCSTR map_name, ObjClassID cls)
{
    return 			LevelPartPath(map_name)+GetTool(cls)->ClassName() + ".part";
}

void EScene::SaveLTX(LPCSTR map_name, bool bForUndo, bool bForceSaveAll)
{
	VERIFY			(map_name);
    R_ASSERT		(!bForUndo);

    CTimer 			T;
    T.Start			();
    xr_string 		full_name;
	full_name		= map_name;
    
    xr_string 		part_prefix;

    bool bSaveMain	= true;
    
	if (!bForUndo)
    {
        if (bSaveMain)
        {
    		EFS.MarkFile	(full_name.c_str(),true);
        }
    	part_prefix		= LevelPartPath(full_name.c_str());
    }

    
    CInifile ini(full_name.c_str(),FALSE,FALSE,TRUE);

    if (bSaveMain)
    {
        ini.w_u32			("version","value",CURRENT_FILE_VERSION);

        m_LevelOp.SaveLTX	(ini);

        m_GUID.SaveLTX		(ini,"guid","guid");

		ini.w_string		("level_tag","owner",m_OwnerName.c_str());
        ini.w_u32			("level_tag","create_time",m_CreateTime);

        ini.w_fvector3		("camera","hpb",EDevice.m_Camera.GetHPB());
        ini.w_fvector3		("camera","pos",EDevice.m_Camera.GetPosition());

        for(ObjectIt SO=m_ESO_SnapObjects.begin(); SO!=m_ESO_SnapObjects.end(); ++SO)
        {
            ini.w_string	("snap_objects",(*SO)->Name,NULL);
        }
    }

    m_SaveCache.clear		();

    SceneToolsMapPairIt _I = m_SceneTools.begin();
    SceneToolsMapPairIt _E = m_SceneTools.end();

    for (; _I!=_E; ++_I)
    {
        if (		(_I->first!=OBJCLASS_DUMMY) && 
        			_I->second && 
                    _I->second->IsEnabled() && 
                    _I->second->IsEditable()
            )
        {
            if (bForUndo)
            {
            	if (_I->second->IsNeedSave())
                    _I->second->SaveStream	(m_SaveCache);
            }else
            {
            	// !ForUndo
                    xr_string part_name 	= part_prefix + _I->second->ClassName() + ".part";
                    if(_I->second->can_use_inifile())
                    {
                        EFS.MarkFile			(part_name.c_str(),true);
                    	SaveToolLTX				(_I->second->ClassID, part_name.c_str());
                    }  //can_use_ini_file
                    else
                    {
						_I->second->SaveStream	(m_SaveCache);

						EFS.MarkFile			(part_name.c_str(),true);

						IWriter* FF				= FS.w_open	(part_name.c_str());
						R_ASSERT			(FF);
                        FF->open_chunk		(CHUNK_TOOLS_GUID);
                        FF->w				(&m_GUID,sizeof(m_GUID));
                        FF->close_chunk		();

                        FF->open_chunk		(CHUNK_TOOLS_DATA+_I->first);
                        FF->w				(m_SaveCache.pointer(),m_SaveCache.size());
                        FF->close_chunk		();

                        FS.w_close			(FF);

                    }//  ! can_use_ini_file
            }
			m_SaveCache.clear	();
        }
    }
        
	if (!bForUndo)
    {
    	m_RTFlags.set	(flRT_Unsaved,FALSE);
    	Msg				("Saving time: %3.2f sec",T.GetElapsed_sec());
    }
}
//--------------------------------------------------------------------------------------------------
void EScene::SaveToolLTX(ObjClassID clsid, LPCSTR fn)
{
    ESceneToolBase* tool 	= GetTool(clsid);
	int fc = tool->SaveFileCount();
	if(fc==1)
    {
      CInifile ini_part		(fn, FALSE, FALSE, FALSE);
      tool->SaveLTX			(ini_part, 0);
      m_GUID.SaveLTX			(ini_part,"guid","guid");
      ini_part.save_as		();
    }else
    {
    	for(int i=0; i<fc; ++i)
        {
        	
        	string_path 			filename;
            if(i)
            	sprintf				(filename, "%s%d", fn, i);
            else
            	strcpy				(filename, fn);

            CInifile ini_part		(filename, FALSE, FALSE, FALSE);
            tool->SaveLTX			(ini_part, i);
            m_GUID.SaveLTX			(ini_part,"guid","guid");
            ini_part.save_as		();
        }
    }
}

bool EScene::LoadToolLTX(ObjClassID clsid, LPCSTR fn)
{
    ESceneToolBase* tool 	= GetTool(clsid);
    tool->Clear				(true);
	bool res 				= LoadLevelPartLTX(tool, fn);
	return 					res;
}

void EScene::Save(LPCSTR map_name, bool bUndo, bool bForceSaveAll)
{
	R_ASSERT		(bUndo);
	VERIFY			(map_name);

    CTimer 			T;
    T.Start			();
    xr_string 		full_name;
	full_name		= map_name;
    
    xr_string 		part_prefix;

    bool bSaveMain	= true;
    
	IWriter* F			= 0;

    if (bSaveMain)
    {
	    F				= FS.w_open(full_name.c_str()); R_ASSERT(F);
        
        F->open_chunk	(CHUNK_VERSION);
        F->w_u32		(CURRENT_FILE_VERSION);
        F->close_chunk	();

        F->open_chunk	(CHUNK_LEVELOP);
        m_LevelOp.Save	(*F);
        F->close_chunk	();

        F->open_chunk	(CHUNK_TOOLS_GUID);
        F->w			(&m_GUID,sizeof(m_GUID));
        F->close_chunk	();

        F->open_chunk	(CHUNK_LEVEL_TAG);
        F->w_stringZ	(m_OwnerName);
        F->w			(&m_CreateTime,sizeof(m_CreateTime));
        F->close_chunk	();
    
		F->open_chunk	(CHUNK_CAMERA);
        F->w_fvector3	(EDevice.m_Camera.GetHPB());
        F->w_fvector3	(EDevice.m_Camera.GetPosition());
        F->close_chunk	();

        F->open_chunk		(CHUNK_SNAPOBJECTS);
        F->w_u32			(m_ESO_SnapObjects.size());

        for(ObjectIt _F=m_ESO_SnapObjects.begin();_F!=m_ESO_SnapObjects.end();++_F)
            F->w_stringZ	((*_F)->Name);

        F->close_chunk		();
    }

    m_SaveCache.clear		();

    SceneToolsMapPairIt _I = m_SceneTools.begin();
    SceneToolsMapPairIt _E = m_SceneTools.end();

    for (; _I!=_E; ++_I)
    {
        if (	(_I->first!=OBJCLASS_DUMMY) && 
        		_I->second 					&& 
                _I->second->IsEnabled()		&&
                _I->second->IsEditable() 	&&
                (_I->second->IsChanged()||bForceSaveAll)	)
        {

            if (_I->second->IsEnabled()&&_I->second->IsEditable())
            {
            	if (_I->second->IsNeedSave())
                {
                    _I->second->SaveStream	(m_SaveCache);
                    F->open_chunk			(CHUNK_TOOLS_DATA+_I->first);
                    F->w					(m_SaveCache.pointer(),m_SaveCache.size());
                    F->close_chunk			();
                }
            }
			m_SaveCache.clear	();
        }
    }
        
    // save data
    if (bSaveMain)		FS.w_close(F);
}
//--------------------------------------------------------------------------------------------------

void EScene::SaveObjectLTX(CCustomObject* O, LPCSTR sect_name, CInifile& ini)
{
	ini.w_u32	(sect_name,"clsid",O->ClassID);
	O->SaveLTX	(ini, sect_name);
}

void EScene::SaveObjectStream( CCustomObject* O, IWriter& F )
{
    F.open_chunk	(CHUNK_OBJECT_CLASS);
    F.w_u32			(O->ClassID);
    F.close_chunk	();
    F.open_chunk	(CHUNK_OBJECT_BODY);
    O->SaveStream	(F);
    F.close_chunk	();
}
//--------------------------------------------------------------------------------------------------

void EScene::SaveObjectsLTX(ObjectList& lst, LPCSTR sect_name_parent, LPCSTR sect_name_prefix, CInifile& ini)
{
    u32 i 				= 0;
    string256			buff;
    for(ObjectIt _F = lst.begin(); _F!=lst.end(); ++_F, ++i)
    {
    	sprintf				(buff,"%s_%s_%d",sect_name_parent,sect_name_prefix,i);
        SaveObjectLTX		(*_F,buff,ini);
    }
	sprintf					(buff,"%s_count",sect_name_prefix);
    ini.w_u32				(sect_name_parent, buff, lst.size());
}

void EScene::SaveObjectsStream( ObjectList& lst, u32 chunk_id, IWriter& F )
{
    F.open_chunk			(chunk_id);
    int count 				= 0;
    for(ObjectIt _F = lst.begin();_F!=lst.end();++_F)
    {
        F.open_chunk		(count);
        ++count;
        SaveObjectStream	(*_F,F);
        F.close_chunk		();
    }
	F.close_chunk			();
}
//--------------------------------------------------------------------------------------------------

bool EScene::ReadObjectStream(IReader& F, CCustomObject*& O)
{
    ObjClassID clsid		=OBJCLASS_DUMMY;
    R_ASSERT				(F.find_chunk(CHUNK_OBJECT_CLASS));
    clsid 					= ObjClassID(F.r_u32());
	O 						= GetOTool(clsid)->CreateObject(0,0);

    IReader* S 				= F.open_chunk(CHUNK_OBJECT_BODY);
    R_ASSERT				(S);
    bool bRes 				= O->LoadStream(*S);
    S->close				();

	if (!bRes)
    	xr_delete			(O);

	return bRes;
}
//----------------------------------------------------
bool EScene::ReadObjectLTX(CInifile& ini, LPCSTR sect_name, CCustomObject*& O)
{
    ObjClassID clsid		= OBJCLASS_DUMMY;
    clsid 					= ObjClassID(ini.r_u32(sect_name,"clsid"));
	O 						= GetOTool(clsid)->CreateObject(0,0);

    bool bRes 				= O->LoadLTX(ini, sect_name);

	if (!bRes)
    	xr_delete			(O);

	return bRes;
}

#include "AppendObjectInfoForm.h"
bool EScene::ReadObjectsLTX(CInifile& ini,  LPCSTR sect_name_parent, LPCSTR sect_name_prefix, TAppendObject on_append, SPBItem* pb)
{
	string128			buff;
	R_ASSERT			(on_append);
	sprintf				(buff, "%s_count", sect_name_prefix);
    u32 count			= ini.r_u32(sect_name_parent, buff);
	bool bRes 			= true;

	for(u32 i=0; i<count; ++i)
    {
    	sprintf				(buff, "%s_%s_%d", sect_name_parent, sect_name_prefix, i);
        CCustomObject* obj	= NULL;

		if(ReadObjectLTX(ini, buff, obj))
        {
            LPCSTR obj_name = obj->Name;
            CCustomObject* existing = FindObjectByName(obj_name,obj->ClassID);
            if(existing)
            {

                if(g_frmConflictLoadObject->m_result!=2 && g_frmConflictLoadObject->m_result!=4 && g_frmConflictLoadObject->m_result!=6)
                {
                    g_frmConflictLoadObject->m_existing_object 	= existing;
                    g_frmConflictLoadObject->m_new_object 		= obj;
                    g_frmConflictLoadObject->Prepare			();
                    g_frmConflictLoadObject->ShowModal			();
                }
                switch(g_frmConflictLoadObject->m_result)
                {
                    case 1: //Overwrite
                    case 2: //Overwrite All
                    {
                       bool res = RemoveObject		(existing, true, true);
                        if(!res)
                            Msg("! RemoveObject [%s] failed", existing->Name);
                         else
                            xr_delete(existing);
                    }break;
                    case 3: //Insert new
                    case 4: //Insert new All
                    {
                        string256 				buf;
                        GenObjectName			(obj->ClassID, buf, obj->Name);
                        obj->Name				= buf;
                    }break;
                    case 0: //Cancel
                    case 5: //Skip
                    case 6: //Skip All
                    {
                        xr_delete(obj);
                    }break;
                } //switch
            } //if exist
            if (obj && !on_append(obj))
                xr_delete(obj);}
        
        else
        	bRes = false;

        if (pb)
			pb->Inc();
    }
    return bRes;
}

bool EScene::ReadObjectsStream(IReader& F, u32 chunk_id, TAppendObject on_append, SPBItem* pb)
{
	R_ASSERT			(on_append);
	bool bRes 			= true;
    IReader* OBJ 		= F.open_chunk(chunk_id);
    if (OBJ)
    {
        IReader* O   	= OBJ->open_chunk(0);
        for (int count=1; O; ++count)
        {
            CCustomObject* obj	=NULL;
            if (ReadObjectStream(*O, obj))
            {
                LPCSTR obj_name = obj->Name;
                CCustomObject* existing = FindObjectByName(obj_name,obj->ClassID);
                if(existing)
                {
                	if(g_frmConflictLoadObject->m_result!=2 && g_frmConflictLoadObject->m_result!=4 && g_frmConflictLoadObject->m_result!=6)
                    {
                        g_frmConflictLoadObject->m_existing_object 	= existing;
                        g_frmConflictLoadObject->m_new_object 		= obj;
                        g_frmConflictLoadObject->Prepare			();
                        g_frmConflictLoadObject->ShowModal			();
                    }
                    switch(g_frmConflictLoadObject->m_result)
                    {
                    	case 1: //Overwrite
                    	case 2: //Overwrite All
                        {
                           bool res = RemoveObject		(existing, true, true);
							if(!res)
                            	Msg("! RemoveObject [%s] failed", existing->Name);
                             else
                             	xr_delete(existing);
                        }break;
                    	case 3: //Insert new
                    	case 4: //Insert new All
                        {
                            string256 				buf;
    						GenObjectName			(obj->ClassID, buf, obj->Name);
    						obj->Name				= buf;
                        }break;
                    	case 0: //Cancel
                    	case 5: //Skip
                    	case 6: //Skip All
                        {
                        	xr_delete(obj);
                        }break;
                    }
                }
            	if (obj && !on_append(obj))
                	xr_delete(obj);}
            else
            	bRes = false;

            O->close	();
            O 			= OBJ->open_chunk(count);

            if (pb)
            pb->Inc();
        }
        OBJ->close();
    }
    return bRes;
}

bool EScene::OnLoadAppendObject(CCustomObject* O)
{
	AppendObject	(O,false);
    return true;
}

//----------------------------------------------------
bool EScene::LoadLTX(LPCSTR map_name, bool bUndo)
{
    DWORD version = 0;
	if (!map_name||(0==map_name[0])) return false;

    xr_string 		full_name;
    full_name 		= map_name;

	ELog.Msg( mtInformation, "EScene: loading '%s'", map_name);
    if (FS.exist(full_name.c_str()))
    {
        CTimer T; T.Start();

        // lock main level
		CInifile	ini(full_name.c_str());
        version 	= ini.r_u32("version","value");

        if (version!=CURRENT_FILE_VERSION)
        {
            ELog.DlgMsg( mtError, "EScene: unsupported file version. Can't load Level.");
            UI->UpdateScene();
            return false;
        }

        m_LevelOp.ReadLTX	(ini);

       	Fvector hpb, pos;
        pos					= ini.r_fvector3("camera","pos");
        hpb					= ini.r_fvector3("camera","hpb");
        EDevice.m_Camera.Set(hpb,pos);
        EDevice.m_Camera.SetStyle(EDevice.m_Camera.GetStyle());
		EDevice.m_Camera.SetStyle(EDevice.m_Camera.GetStyle());

        m_GUID.LoadLTX			(ini,"guid","guid");

		m_OwnerName				= ini.r_string("level_tag","owner");
        m_CreateTime			= ini.r_u32("level_tag","create_time");


        SceneToolsMapPairIt _I 	= m_SceneTools.begin();
        SceneToolsMapPairIt _E 	= m_SceneTools.end();
        for (; _I!=_E; ++_I)
        {
            if (_I->second)
            {
                {
                    if (!bUndo && _I->second->IsEnabled() && (_I->first!=OBJCLASS_DUMMY))
                    {
                        xr_string fn 		 = LevelPartName(map_name, _I->first).c_str();
                        LoadLevelPartLTX	(_I->second, fn.c_str());
                    }
                }
            }
		}

        if(ini.section_exist("snap_objects"))
        {
			CInifile::Sect& S 		= ini.r_section("snap_objects");
            CInifile::SectCIt Si 	= S.Data.begin();
            CInifile::SectCIt Se 	= S.Data.end();
            for(;Si!=Se; ++Si)
            {
                CCustomObject* 	O = FindObjectByName(Si->first.c_str(),OBJCLASS_SCENEOBJECT);
                if (!O)
                    ELog.Msg(mtError,"EScene: Can't find snap object '%s'.",Si->second.c_str());
                else
                	m_ESO_SnapObjects.push_back(O);
            }
            UpdateSnapList();
        }

        Msg("EScene: %d objects loaded, %3.2f sec", ObjCount(), T.GetElapsed_sec() );

    	UI->UpdateScene(true);

        SynchronizeObjects();

	    if (!bUndo)
        	m_RTFlags.set(flRT_Unsaved|flRT_Modified,FALSE);
        
		return true;
    }else
    {
    	ELog.Msg(mtError,"Can't find file: '%s'",map_name);
    }
	return false;
}

bool EScene::Load(LPCSTR map_name, bool bUndo)
{
    u32 version = 0;

	if (!map_name||(0==map_name[0])) return false;

    xr_string 		full_name;
    full_name 		= map_name;

	ELog.Msg( mtInformation, "EScene: loading '%s'", map_name);
    if (FS.exist(full_name.c_str()))
    {
        CTimer T; T.Start();
            
        // read main level
        IReader* F 	= FS.r_open(full_name.c_str()); VERIFY(F);
        // Version
        R_ASSERT	(F->r_chunk(CHUNK_VERSION, &version));
        if (version!=CURRENT_FILE_VERSION)
        {
            ELog.DlgMsg( mtError, "EScene: unsupported file version. Can't load Level.");
            UI->UpdateScene();
            FS.r_close(F);
            return false;
        }

        // Lev. ops.
        IReader* LOP = F->open_chunk(CHUNK_LEVELOP);
        if (LOP)
        {
	        m_LevelOp.Read	(*LOP);
        	LOP->close		();
        }else
        {
			ELog.DlgMsg		(mtError, "Skipping old version of level options.\nCheck level options after loading.");
	    }

        //
        if (F->find_chunk(CHUNK_CAMERA))
        {
        	Fvector hpb, pos;
	        F->r_fvector3	(hpb);
    	    F->r_fvector3	(pos);
            EDevice.m_Camera.Set(hpb,pos);
			EDevice.m_Camera.SetStyle(EDevice.m_Camera.GetStyle());
        }

	    if (F->find_chunk(CHUNK_TOOLS_GUID))
        {
		    F->r			(&m_GUID,sizeof(m_GUID));
        }

        if (F->find_chunk(CHUNK_LEVEL_TAG))
        {
            F->r_stringZ	(m_OwnerName);
            F->r			(&m_CreateTime,sizeof(m_CreateTime));
        }else
        {
            m_OwnerName		= "";
            m_CreateTime	= 0;
        }

        DWORD obj_cnt 		= 0;

        if (F->find_chunk(CHUNK_OBJECT_COUNT))
        	obj_cnt 		= F->r_u32();

        SPBItem* pb 		= UI->ProgressStart(obj_cnt,"Loading objects...");
        ReadObjectsStream	(*F,CHUNK_OBJECT_LIST,OnLoadAppendObject,pb);
        UI->ProgressEnd		(pb);

        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; ++_I)
        {
            if (_I->second)
            {
			    IReader* chunk 		= F->open_chunk(CHUNK_TOOLS_DATA+_I->first);
            	if (chunk){
	                _I->second->LoadStream(*chunk);
    	            chunk->close	();
                }else{
                    if (!bUndo && _I->second->IsEnabled() && (_I->first!=OBJCLASS_DUMMY))
                    {
                        LoadLevelPart	(_I->second,LevelPartName(map_name,_I->first).c_str());
                    }
                }
            }
		}
        
        // snap list
        if (F->find_chunk(CHUNK_SNAPOBJECTS))
        {
        	shared_str 	buf;
            int cnt 	= F->r_u32();
            if (cnt)
            {
                for (int i=0; i<cnt; ++i)
                {
                    F->r_stringZ		(buf);
                    CCustomObject* 	O = FindObjectByName(buf.c_str(),OBJCLASS_SCENEOBJECT);
                    if (!O)
                    	ELog.Msg(mtError,"EScene: Can't find snap object '%s'.",buf.c_str());

                    else
                    m_ESO_SnapObjects.push_back(O);
                }
            }
            UpdateSnapList();
        }

        Msg("EScene: %d objects loaded, %3.2f sec", ObjCount(), T.GetElapsed_sec() );

    	UI->UpdateScene(true); 

		FS.r_close(F);

        SynchronizeObjects();

	    if (!bUndo)
        	m_RTFlags.set(flRT_Unsaved|flRT_Modified,FALSE);
        
		return true;
    }else
    {
    	ELog.Msg(mtError,"Can't find file: '%s'",map_name);
    }
	return false;
}

//---------------------------------------------------------------------------------------
//copy/paste utils
//---------------------------------------------------------------------------------------
void EScene::SaveSelection( ObjClassID classfilter, LPCSTR fname )
{
	VERIFY			( fname );

    xr_string 		full_name;
    full_name 		= fname;

    IWriter* F		= FS.w_open(full_name.c_str());  R_ASSERT(F);

    F->open_chunk	(CHUNK_VERSION);
    F->w_u32	   	(CURRENT_FILE_VERSION);
    F->close_chunk	();

    m_SaveCache.clear();
    if (OBJCLASS_DUMMY==classfilter)
    {
        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; ++_I)
            if (_I->second&&_I->second->IsNeedSave())
            {
                F->open_chunk				(CHUNK_TOOLS_DATA+_I->first);
                _I->second->SaveSelection	(m_SaveCache);
                F->w						(m_SaveCache.pointer(),m_SaveCache.size());
                m_SaveCache.clear			();
                F->close_chunk				();
            }
    }else{
    	ESceneToolBase* mt = GetTool(classfilter); VERIFY(mt);
        F->open_chunk	(CHUNK_TOOLS_DATA+classfilter);
        mt->SaveSelection(m_SaveCache);
        F->w			(m_SaveCache.pointer(),m_SaveCache.size());
        m_SaveCache.clear();
        F->close_chunk	();
    }
        
    FS.w_close		(F);
}

//----------------------------------------------------
bool EScene::OnLoadSelectionAppendObject(CCustomObject* obj)
{
    string256 				buf;
    GenObjectName			(obj->ClassID,buf,obj->Name);
    obj->Name				= buf;
    AppendObject			(obj, false);
    obj->Select				(true);
    return 					true;
}
//----------------------------------------------------

bool EScene::LoadSelection( LPCSTR fname )
{
    u32 version = 0;

	VERIFY( fname );

    xr_string 		full_name;
    full_name 		= fname;

	ELog.Msg( mtInformation, "EScene: loading part %s...", fname );

    bool res = true;

    if (FS.exist(full_name.c_str())){
		SelectObjects( false );

        IReader* F = FS.r_open(full_name.c_str());

        // Version
        R_ASSERT(F->r_chunk(CHUNK_VERSION, &version));
        if (version!=CURRENT_FILE_VERSION){
            ELog.DlgMsg( mtError, "EScene: unsupported file version. Can't load Level.");
            UI->UpdateScene();
            FS.r_close(F);
            return false;
        }

        // Objects
        if (!ReadObjectsStream(*F,CHUNK_OBJECT_LIST,OnLoadSelectionAppendObject,0))
        {
            ELog.DlgMsg(mtError,"EScene. Failed to load selection.");
            res = false;
        }

        SceneToolsMapPairIt _I = m_SceneTools.begin();
        SceneToolsMapPairIt _E = m_SceneTools.end();
        for (; _I!=_E; _I++)
            if (_I->second&&_I->second->IsEnabled()&&_I->second->IsEditable()){
			    IReader* chunk 		= F->open_chunk(CHUNK_TOOLS_DATA+_I->first);
            	if (chunk){
	                _I->second->LoadSelection(*chunk);
    	            chunk->close	();
                }
            }
        // Synchronize
		SynchronizeObjects();
		FS.r_close(F);
    }
	return res;
}
//----------------------------------------------------

#pragma pack(push,1)
struct SceneClipData {
	int m_ClassFilter;
	char m_FileName[MAX_PATH];
};
#pragma pack(pop)

void EScene::CopySelection( ObjClassID classfilter )
{
	HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, sizeof(SceneClipData) );
	SceneClipData *sceneclipdata = (SceneClipData *)GlobalLock(hmem);

	sceneclipdata->m_ClassFilter = classfilter;
	GetTempFileName( FS.get_path(_temp_)->m_Path, "clip", 0, sceneclipdata->m_FileName );
	SaveSelection( classfilter, sceneclipdata->m_FileName );

	GlobalUnlock( hmem );

	int clipformat = RegisterClipboardFormat( "CF_XRAY_CLASS_LIST" );
	if( OpenClipboard( 0 ) ){
		SetClipboardData( clipformat, hmem );
		CloseClipboard();
	} else {
		ELog.DlgMsg( mtError, "Failed to open clipboard" );
		GlobalFree( hmem );
	}
}

void EScene::PasteSelection() 
{
	int clipformat = RegisterClipboardFormat( "CF_XRAY_CLASS_LIST" );
	if( OpenClipboard( 0 ) ){

		HGLOBAL hmem = GetClipboardData(clipformat);
		if( hmem ){
			SceneClipData *sceneclipdata = (SceneClipData *)GlobalLock(hmem);
			LoadSelection( sceneclipdata->m_FileName );
			GlobalUnlock( hmem );
		} else {
			ELog.DlgMsg( mtError, "No data in clipboard" );
		}

		CloseClipboard();

	} else {
		ELog.DlgMsg( mtError, "Failed to open clipboard" );
	}
}

void EScene::CutSelection( ObjClassID classfilter )
{
	CopySelection( classfilter );
	RemoveSelection( classfilter );
}
//----------------------------------------------------

void EScene::LoadCompilerError(LPCSTR fn)
{
    Tools->ClearDebugDraw();
/*
	CInifile		ini(fn);
   	string256		buff;
    LPCSTR			sect;
	u32				sz, i;

    sect			= "t-junction";
	sz 				= ini.r_u32(sect,"count");
    Tools->m_DebugDraw.m_Points.resize(sz);
	for(i=0; i<sz; ++i)
    {
    	CLevelTool::SDebugDraw::Point& pt = Tools->m_DebugDraw.m_Points[i];
        sprintf		(buff,"%d_p",i);
		pt.p[0]		= ini.r_fvector3(sect,buff);

        sprintf		(buff,"%d_c",i);
		pt.c		= ini.r_u32	(sect,buff);

        sprintf		(buff,"%d_i",i);
		pt.i		= ini.r_bool(sect,buff);

        sprintf		(buff,"%d_m",i);
		pt.m		= ini.r_bool(sect,buff);
    }

    sect			= "m-edje";
	sz 			= ini.r_u32(sect,"count");
    Tools->m_DebugDraw.m_Lines.resize(sz);
	for(i=0; i<sz; ++i)
    {
    	CLevelTool::SDebugDraw::Line& pt = Tools->m_DebugDraw.m_Lines[i];
        sprintf		(buff,"%d_p0",i);
		pt.p[0]		= ini.r_fvector3(sect,buff);

        sprintf		(buff,"%d_p1",i);
		pt.p[1]		= ini.r_fvector3(sect,buff);

        sprintf		(buff,"%d_c",i);
		pt.c		= ini.r_u32	(sect,buff);

        sprintf		(buff,"%d_i",i);
		pt.i		= ini.r_bool(sect,buff);

        sprintf		(buff,"%d_m",i);
		pt.m		= ini.r_bool(sect,buff);
    }
    sect			= "invalid_face";
	sz 			= ini.r_u32(sect,"count");
    Tools->m_DebugDraw.m_WireFaces.resize(sz);
	for(i=0; i<sz; ++i)
    {
    	CLevelTool::SDebugDraw::Face& pt = Tools->m_DebugDraw.m_WireFaces[i];
        sprintf		(buff,"%d_p0",i);
		pt.p[0]		= ini.r_fvector3(sect,buff);

        sprintf		(buff,"%d_p1",i);
		pt.p[1]		= ini.r_fvector3(sect,buff);

        sprintf		(buff,"%d_p2",i);
		pt.p[2]		= ini.r_fvector3(sect,buff);

        sprintf		(buff,"%d_c",i);
		pt.c		= ini.r_u32	(sect,buff);

        sprintf		(buff,"%d_i",i);
		pt.i		= ini.r_bool(sect,buff);

        sprintf		(buff,"%d_m",i);
		pt.m		= ini.r_bool(sect,buff);
    }
*/

	IReader* F	= FS.r_open(fn);
    Tools->ClearDebugDraw();
    Fvector 		pt[3];
    if (F->find_chunk(10)){ // lc error (TJ)
        Tools->m_DebugDraw.m_Points.resize(F->r_u32());
        F->r(Tools->m_DebugDraw.m_Points.begin(),sizeof(CLevelTool::SDebugDraw::Point)*Tools->m_DebugDraw.m_Points.size());
    }else if (F->find_chunk(0)){ // lc error (TJ)
    	u32 cnt			= F->r_u32();
        for (u32 k=0;k<cnt; k++){ F->r(pt,sizeof(Fvector)); Tools->m_DebugDraw.AppendPoint(pt[0],0xff00ff00,true,true,"TJ"); }
    }
/*    
    if (F->find_chunk(11)){ // lc error (multiple edges)
        Tools->m_DebugDraw.m_Lines.resize(F->r_u32());
        F->r(Tools->m_DebugDraw.m_Lines.begin(),sizeof(CLevelTool::SDebugDraw::Line)*Tools->m_DebugDraw.m_Lines.size());
    }else if (F->find_chunk(1)){ // lc error (multiple edges)
    	u32 cnt			= F->r_u32();
        for (u32 k=0;k<cnt; k++){ F->r(pt,sizeof(Fvector)*2); Tools->m_DebugDraw.AppendLine(pt[0],pt[1],0xff0000ff,false,false); }
    }
*/
    if (F->find_chunk(12)){ // lc error (invalid faces)
        Tools->m_DebugDraw.m_WireFaces.resize(F->r_u32());
        F->r(Tools->m_DebugDraw.m_WireFaces.begin(),sizeof(CLevelTool::SDebugDraw::Face)*Tools->m_DebugDraw.m_WireFaces.size());
    }else if (F->find_chunk(2)){ // lc error (invalid faces)
    	u32 cnt			= F->r_u32();
        for (u32 k=0;k<cnt; k++){ F->r(pt,sizeof(Fvector)*3); Tools->m_DebugDraw.AppendWireFace(pt[0],pt[1],pt[2]); }
    }
    FS.r_close(F);

}

void EScene::SaveCompilerError(LPCSTR fn)
{
/*
	CInifile		ini(fn,FALSE,FALSE,TRUE);
   	string256		buff;
    LPCSTR			sect;
    u32				sz, i;

    sz 				= Tools->m_DebugDraw.m_Points.size();
    sect			= "t-junction";
    ini.w_u32		(sect,"count",sz);
    for(i=0; i<sz; ++i)
    {
        sprintf		(buff,"%d_p",i);
        ini.w_fvector3(sect,buff,Tools->m_DebugDraw.m_Points[i].p[0]);

        sprintf		(buff,"%d_c",i);
        ini.w_u32	(sect,buff,Tools->m_DebugDraw.m_Points[i].c);

        sprintf		(buff,"%d_i",i);
        ini.w_bool	(sect,buff,Tools->m_DebugDraw.m_Points[i].i);

        sprintf		(buff,"%d_m",i);
        ini.w_bool	(sect,buff,Tools->m_DebugDraw.m_Points[i].m);
    }

    sz 				= Tools->m_DebugDraw.m_Lines.size();
    sect			= "m-edje";
    ini.w_u32		("sect","count",sz);
    for(i=0; i<sz; ++i)
    {
        sprintf		(buff,"%d_p0",i);
        ini.w_fvector3(sect,buff,Tools->m_DebugDraw.m_Lines[i].p[0]);

        sprintf		(buff,"%d_p1",i);
        ini.w_fvector3(sect,buff,Tools->m_DebugDraw.m_Lines[i].p[1]);

        sprintf		(buff,"%d_c",i);
        ini.w_u32	(sect,buff,Tools->m_DebugDraw.m_Lines[i].c);

        sprintf		(buff,"%d_i",i);
        ini.w_bool	(sect,buff,Tools->m_DebugDraw.m_Lines[i].i);

        sprintf		(buff,"%d_m",i);
        ini.w_bool	(sect,buff,Tools->m_DebugDraw.m_Lines[i].m);
    }

    sz 				= Tools->m_DebugDraw.m_WireFaces.size();
    sect			= "invalid_face";
    ini.w_u32		(sect,"count",sz);
    for(i=0; i<sz; ++i)
    {
        sprintf		(buff,"%d_p0",i);
        ini.w_fvector3(sect,buff,Tools->m_DebugDraw.m_WireFaces[i].p[0]);

        sprintf		(buff,"%d_p1",i);
        ini.w_fvector3(sect,buff,Tools->m_DebugDraw.m_WireFaces[i].p[1]);

        sprintf		(buff,"%d_p2",i);
        ini.w_fvector3(sect,buff,Tools->m_DebugDraw.m_WireFaces[i].p[2]);

        sprintf		(buff,"%d_c",i);
        ini.w_u32	(sect,buff,Tools->m_DebugDraw.m_WireFaces[i].c);

        sprintf		(buff,"%d_i",i);
        ini.w_bool	(sect,buff,Tools->m_DebugDraw.m_WireFaces[i].i);

        sprintf		(buff,"%d_m",i);
        ini.w_bool	(sect,buff,Tools->m_DebugDraw.m_WireFaces[i].m);
    }
*/
	IWriter*		fs	= FS.w_open(fn);  R_ASSERT(fs);
	IWriter&		err = *fs;

	// t-junction
	err.open_chunk	(10);
	err.w_u32		(Tools->m_DebugDraw.m_Points.size());
	err.w			(Tools->m_DebugDraw.m_Points.begin(), Tools->m_DebugDraw.m_Points.size()*sizeof(CLevelTool::SDebugDraw::Point));
	err.close_chunk	();

	// m-edje
	err.open_chunk	(11);
	err.w_u32		(Tools->m_DebugDraw.m_Lines.size());
	err.w			(Tools->m_DebugDraw.m_Lines.begin(), Tools->m_DebugDraw.m_Lines.size()*sizeof(CLevelTool::SDebugDraw::Line));
	err.close_chunk	();

	// invalid
	err.open_chunk	(12);
	err.w_u32		(Tools->m_DebugDraw.m_WireFaces.size());
	err.w			(Tools->m_DebugDraw.m_WireFaces.begin(), Tools->m_DebugDraw.m_WireFaces.size()*sizeof(CLevelTool::SDebugDraw::Face));
	err.close_chunk	();

    FS.w_close		(fs);

}


void EScene::ExportObj(bool b_selected_only)
{
	Builder.m_save_as_object 	= true;
	Builder.Compile				(b_selected_only);
	Builder.m_save_as_object 	= false;

}

