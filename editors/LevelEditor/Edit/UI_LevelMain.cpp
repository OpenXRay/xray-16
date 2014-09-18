//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "UI_LevelMain.h"

#include "UI_LevelTools.h"
#include "EditLibrary.h"
#include "../ECore/Editor/ImageEditor.h"
#include "leftbar.h"
#include "topbar.h"
#include "scene.h"
#include "sceneobject.h"
#include "Cursor3D.h"
#include "bottombar.h"
#include "xr_trims.h"
#include "main.h"
#include "xr_input.h"
#include "../ECore/Editor/ui_main.h"
#include "d3dutils.h"
#include "EditLightAnim.h"
#include "builder.h"
#include "SoundManager_LE.h"
#include "../xrEProps/NumericVector.h"
#include "LevelPreferences.h"
#include "LEClipEditor.h"

#ifdef _LEVEL_EDITOR
//.    if (m_Cursor->GetVisible()) RedrawScene();
#endif

CLevelMain*&	LUI=(CLevelMain*)UI;

CLevelMain::CLevelMain()
{
    m_Cursor        = xr_new<C3DCursor>();
    EPrefs			= xr_new<CLevelPreferences>();
}

CLevelMain::~CLevelMain()
{
    xr_delete		(EPrefs);
    xr_delete		(m_Cursor);
    TClipMaker::DestroyForm(g_clip_maker);
}



//------------------------------------------------------------------------------
// Tools commands
//------------------------------------------------------------------------------
CCommandVar CLevelTool::CommandChangeTarget(CCommandVar p1, CCommandVar p2)
{
	if (Scene->GetTool(p1)->IsEnabled())
    {
	    SetTarget	(p1,p2);
	    ExecCommand	(COMMAND_UPDATE_PROPERTIES);
        return 		TRUE;
    }else{
    	return 		FALSE;
    }
}
CCommandVar CLevelTool::CommandShowObjectList(CCommandVar p1, CCommandVar p2)
{
    if (LUI->GetEState()==esEditScene) ShowObjectList();
    return TRUE;
}

//------------------------------------------------------------------------------
// Main commands
//------------------------------------------------------------------------------
CCommandVar CommandLibraryEditor(CCommandVar p1, CCommandVar p2)
{
    if (Scene->ObjCount()||(LUI->GetEState()!=esEditScene)){
        if (LUI->GetEState()==esEditLibrary)	TfrmEditLibrary::ShowEditor();
        else									ELog.DlgMsg(mtError, "Scene must be empty before editing library!");
    }else{
        TfrmEditLibrary::ShowEditor();
    }
    return TRUE;
}
CCommandVar CommandLAnimEditor(CCommandVar p1, CCommandVar p2)
{
    TfrmEditLightAnim::ShowEditor();
    return TRUE;
}
CCommandVar CommandFileMenu(CCommandVar p1, CCommandVar p2)
{
    FHelper.ShowPPMenu(fraLeftBar->pmSceneFile,0);
    return TRUE;
}
CCommandVar CLevelTool::CommandEnableTarget(CCommandVar p1, CCommandVar p2)
{
	ESceneToolBase* M 	= Scene->GetTool(p1);
    VERIFY					(M);
    BOOL res				= FALSE;
	if (p2)
    {
    	res 				= ExecCommand(COMMAND_LOAD_LEVEL_PART,M->ClassID,TRUE);
        if(res)
		    M->m_EditFlags.set(ESceneToolBase::flEnable,TRUE);
    }else
    {
        if (!Scene->IfModified())
        {
		    M->m_EditFlags.set(ESceneToolBase::flEnable,TRUE);
            res				= FALSE;
        }else
        {
	    	res				= ExecCommand(COMMAND_UNLOAD_LEVEL_PART,M->ClassID,TRUE);
            if(res)
		    	M->m_EditFlags.set(ESceneToolBase::flEnable,FALSE);
        }
		if (res)        	
        	ExecCommand(COMMAND_CHANGE_TARGET,OBJCLASS_SCENEOBJECT);
    }
    ExecCommand				(COMMAND_REFRESH_UI_BAR);
    return res;
}

CCommandVar CLevelTool::CommandShowTarget(CCommandVar p1, CCommandVar p2)
{
	ESceneToolBase* M 	= Scene->GetTool(p1);
    if(p2)
    	M->m_EditFlags.set(ESceneToolBase::flVisible,TRUE);
    else
    	M->m_EditFlags.set(ESceneToolBase::flVisible,FALSE);
        
    return TRUE;
}

CCommandVar CLevelTool::CommandReadonlyTarget(CCommandVar p1, CCommandVar p2)
{
	ESceneToolBase* M 		= Scene->GetTool(p1); VERIFY(M);
    BOOL res				= TRUE;
	if (p2){
        if (!Scene->IfModified())
        {
		    M->m_EditFlags.set(ESceneToolBase::flForceReadonly,FALSE);
            res				= FALSE;
        }else
        {
//.            xr_string pn	= Scene->LevelPartName(LTools->m_LastFileName.c_str(),M->ClassID);
        }
    }else
    {
//.        xr_string pn		= Scene->LevelPartName(LTools->m_LastFileName.c_str(), M->ClassID);
    }
    if (res)
    {
    	Reset				();
	    ExecCommand			(COMMAND_REFRESH_UI_BAR);
    }
    return res;
}

CCommandVar CLevelTool::CommandMultiRenameObjects(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() )
    {
        if (mrYes==ELog.DlgMsg(mtConfirmation, TMsgDlgButtons()<<mbYes<<mbNo, "Are you sure to rename selected objects?"))
        {
			int cnt			= Scene->MultiRenameObjects();
            if (cnt)
            {
			    ExecCommand	(COMMAND_UPDATE_PROPERTIES);
            	Scene->UndoSave();
            }
	        ELog.DlgMsg		( mtInformation, "%d - objects are renamed.", cnt );
        }
    }else
    {
        ELog.DlgMsg			( mtError, "Scene sharing violation" );
    }
    return 					FALSE;
}
CCommandVar CommandLoadLevelPart(CCommandVar p1, CCommandVar p2)
{
    xr_string temp_fn	= LTools->m_LastFileName.c_str();
    if (!temp_fn.empty())
        return			Scene->LoadLevelPart(temp_fn.c_str(),p1);
    return				TRUE;
}
CCommandVar CommandUnloadLevelPart(CCommandVar p1, CCommandVar p2)
{
    xr_string temp_fn	= LTools->m_LastFileName.c_str();
    if (!temp_fn.empty())
        return			Scene->UnloadLevelPart(temp_fn.c_str(),p1);
    return				TRUE;
}
CCommandVar CommandLoad(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() )
    {
    	if (!p1.IsString())
        {
        	xr_string temp_fn	= LTools->m_LastFileName.c_str();
        	if (EFS.GetOpenName	( _maps_, temp_fn ))
            	return 			ExecCommand(COMMAND_LOAD,temp_fn);
        }else
        {
	        xr_string temp_fn		= p1;
            xr_strlwr				(temp_fn);

            if (!Scene->IfModified())
            	return FALSE;
            
            UI->SetStatus			("Level loading...");
            ExecCommand				(COMMAND_CLEAR);

			IReader* R = FS.r_open	(temp_fn.c_str());
            char ch;
            R->r(&ch, sizeof(ch));
            bool is_ltx = (ch=='[');
            FS.r_close(R);
            bool res;
            LTools->m_LastFileName	= temp_fn.c_str();

            if(is_ltx)
            	res = Scene->LoadLTX(temp_fn.c_str(), false);
            else
            	res = Scene->Load(temp_fn.c_str(), false);

            if (res)
            {
                UI->ResetStatus		();
                Scene->UndoClear	();
                
                BOOL bk1 			= Scene->m_RTFlags.test(EScene::flRT_Unsaved);
                BOOL bk2 			= Scene->m_RTFlags.test(EScene::flRT_Modified);

                Scene->UndoSave		();

                 Scene->m_RTFlags.set(EScene::flRT_Unsaved,bk1);
                 Scene->m_RTFlags.set(EScene::flRT_Modified,bk2);

				ExecCommand			(COMMAND_CLEAN_LIBRARY);
                ExecCommand			(COMMAND_UPDATE_CAPTION);
                ExecCommand			(COMMAND_CHANGE_ACTION,etaSelect);
                EPrefs->AppendRecentFile(temp_fn.c_str());
            }else
            {
                ELog.DlgMsg	( mtError, "Can't load map '%s'", temp_fn.c_str() );
                LTools->m_LastFileName = "";
            }
            // update props
            ExecCommand			(COMMAND_UPDATE_PROPERTIES);
            UI->RedrawScene		();             
        }
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
        return FALSE;
    }
    return TRUE;
}
CCommandVar CommandSaveBackup(CCommandVar p1, CCommandVar p2)
{
    string_path 	fn;
    strconcat(sizeof(fn),fn,Core.CompName,"_",Core.UserName,"_backup.level");
    FS.update_path	(fn,_maps_,fn);
    return 			ExecCommand(COMMAND_SAVE,xr_string(fn));
}
CCommandVar CommandSave(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() )
    {
        if (p2==1)
        {
            xr_string temp_fn	= LTools->m_LastFileName.c_str();
            if (EFS.GetSaveName	( _maps_, temp_fn ))
                return 			ExecCommand(COMMAND_SAVE,temp_fn, 66);
            else
                return          FALSE;
        }else{
            if (p1.IsInteger())
            	return 			ExecCommand(COMMAND_SAVE,xr_string(LTools->m_LastFileName.c_str()),0);
                
            xr_string temp_fn	= xr_string(p1);
            if (temp_fn.empty())
            {
                return 			ExecCommand(COMMAND_SAVE,temp_fn,1);
            }else
            {
                xr_strlwr		(temp_fn);

                UI->SetStatus	("Level saving...");

                if(LUI->m_rt_object_props->section_exist(temp_fn.c_str()))
                {
                    CInifile::Sect& S 	= LUI->m_rt_object_props->r_section(temp_fn.c_str());
                    S.Data.clear		();
                }

                Scene->SaveLTX	(temp_fn.c_str(), false, (p2==66));

                UI->ResetStatus	();
                // set new name
                if (0!=xr_strcmp(Tools->m_LastFileName.c_str(),temp_fn.c_str()))
                {
    	            Tools->m_LastFileName 	= temp_fn.c_str();
                }
                ExecCommand		(COMMAND_UPDATE_CAPTION);
                EPrefs->AppendRecentFile(temp_fn.c_str());
                return 			TRUE;
            }
        }
    } else {
        ELog.DlgMsg			( mtError, "Scene sharing violation" );
        return				FALSE;
    }
}

CCommandVar CommandClear(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if (!Scene->IfModified()) return TRUE;
        EDevice.m_Camera.Reset	();
        Scene->Reset			();
        Scene->m_LevelOp.Reset	();
        Tools->m_LastFileName 		= "";
        LTools->m_LastSelectionName = "";
        Scene->UndoClear		();
        ExecCommand				(COMMAND_UPDATE_CAPTION);
        ExecCommand				(COMMAND_CHANGE_TARGET,OBJCLASS_SCENEOBJECT);
        ExecCommand				(COMMAND_CHANGE_ACTION,etaSelect,estDefault);
	    ExecCommand				(COMMAND_UPDATE_PROPERTIES);
        Scene->UndoSave			();
        return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
        return					FALSE;
    }
}
CCommandVar CommandLoadFirstRecent(CCommandVar p1, CCommandVar p2)
{
    if (EPrefs->FirstRecentFile())
        return 					ExecCommand(COMMAND_LOAD,xr_string(EPrefs->FirstRecentFile()));
    return 						FALSE;
}

CCommandVar CommandClearDebugDraw(CCommandVar p1, CCommandVar p2)
{
    Tools->ClearDebugDraw		();
    UI->RedrawScene				();
    return 						TRUE;
}
#include "SpawnPoint.h"
CCommandVar CommandShowClipEditor(CCommandVar p1, CCommandVar p2)
{
	if(g_clip_maker==NULL)
       g_clip_maker = TClipMaker::CreateForm();

    if(!g_clip_maker->Visible)	
    {
		ESceneCustomOTool* st = Scene->GetOTool(OBJCLASS_SPAWNPOINT);

    	ObjectList&	ol = st->GetObjects();
        ObjectList::iterator it = ol.begin();    
        ObjectList::iterator it_e = ol.end();    

		CCustomObject* CO = NULL;
        for(;it!=it_e;++it)
        {
        	if((*it)->Selected()==true)
            {
            	CO=*it;
                break;
            }
        }
        if(!CO)
        	return TRUE;
            
    	CSpawnPoint* sp = dynamic_cast<CSpawnPoint*>(CO);

        
    	CKinematicsAnimated* KA 	= PKinematicsAnimated(sp->m_SpawnData.m_Visual->visual);
        R_ASSERT					(KA);
   		g_clip_maker->ShowEditor	(KA);
    }
    return 							TRUE;
}

CCommandVar CommandImportCompilerError(CCommandVar p1, CCommandVar p2)
{
    xr_string fn;
    if(EFS.GetOpenName("$logs$", fn, false, NULL, 0)){
        Scene->LoadCompilerError(fn.c_str());
    }
    UI->RedrawScene		();
    return TRUE;
}
CCommandVar CommandExportCompilerError(CCommandVar p1, CCommandVar p2)
{
    xr_string fn;
    if(EFS.GetSaveName("$logs$", fn, NULL, 0)){
        Scene->SaveCompilerError(fn.c_str());
    }
    return TRUE;
}
CCommandVar CommandValidateScene(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->Validate	(true,true,true,true,true,true);
	    return 			TRUE;
    } else {
        ELog.DlgMsg		( mtError, "Scene sharing violation" );
	    return 			FALSE;
    }
}
CCommandVar CommandCleanLibrary(CCommandVar p1, CCommandVar p2)
{
    if ( !Scene->locked() ){
        Lib.CleanLibrary();
        return 			TRUE;
    }else{
        ELog.DlgMsg		(mtError, "Scene must be empty before refreshing library!");
        return 			FALSE;
    }
}

CCommandVar CommandReloadObjects(CCommandVar p1, CCommandVar p2)
{
    Lib.ReloadObjects	();
    return 				TRUE;
}

CCommandVar CommandCut(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->CutSelection(LTools->CurrentClassID());
        fraLeftBar->miPaste->Enabled = true;
        fraLeftBar->miPaste2->Enabled = true;
        Scene->UndoSave	();
        return 			TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
        return 			FALSE;
    }
}
CCommandVar CommandCopy(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->CopySelection(LTools->CurrentClassID());
        fraLeftBar->miPaste->Enabled = true;
        fraLeftBar->miPaste2->Enabled = true;
        return 			TRUE;
    } else {
        ELog.DlgMsg		( mtError, "Scene sharing violation" );
        return 			FALSE;
    }
}

CCommandVar CommandPaste(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->PasteSelection();
        Scene->UndoSave	();
        return 			TRUE;
    } else {
        ELog.DlgMsg		( mtError, "Scene sharing violation" );
        return  		FALSE;
    }
}

#include "AppendObjectInfoForm.h"
CCommandVar CommandLoadSelection(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() )
    {
        xr_string fn			= LTools->m_LastSelectionName;
        if( EFS.GetOpenName( _maps_, fn ) )
        {
        	LPCSTR maps_path	= FS.get_path(_maps_)->m_Path;
        	if (fn.c_str()==strstr(fn.c_str(),maps_path))
		        LTools->m_LastSelectionName = fn.c_str()+xr_strlen(maps_path);
            UI->SetStatus		("Fragment loading...");

			g_frmConflictLoadObject->m_result = 0;
            Scene->LoadSelection(fn.c_str());
			g_frmConflictLoadObject->m_result = 4; //auto-rename

            UI->ResetStatus		();
            Scene->UndoSave		();
            ExecCommand			(COMMAND_CHANGE_ACTION,etaSelect);
            ExecCommand			(COMMAND_UPDATE_PROPERTIES);
            UI->RedrawScene		();
	        return 				TRUE;
        }               	
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 				FALSE;
}        
CCommandVar CommandSaveSelection(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        xr_string fn			= LTools->m_LastSelectionName;
        if( EFS.GetSaveName		( _maps_, fn ) ){
        	LPCSTR maps_path	= FS.get_path(_maps_)->m_Path;
        	if (fn.c_str()==strstr(fn.c_str(),maps_path))
		        LTools->m_LastSelectionName = fn.c_str()+xr_strlen(maps_path);
            UI->SetStatus		("Fragment saving...");
            Scene->SaveSelection(LTools->CurrentClassID(),fn.c_str());
            UI->ResetStatus		();
	        return 				TRUE;
        }
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}

CCommandVar CommandUndo(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if( !Scene->Undo() ) 	ELog.DlgMsg( mtInformation, "Undo buffer empty" );
        else{
            LTools->Reset		();
            ExecCommand			(COMMAND_CHANGE_ACTION, etaSelect);
	        return 				TRUE;
        }
    } else {
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}

CCommandVar CommandRedo(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if( !Scene->Redo() ) 	ELog.DlgMsg( mtInformation, "Redo buffer empty" );
        else{
            LTools->Reset		();
            ExecCommand			(COMMAND_CHANGE_ACTION, etaSelect);
		    return 				TRUE;
        }
    } else {
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}

CCommandVar CommandClearSceneSummary(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->ClearSummaryInfo	();
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandCollectSceneSummary(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->CollectSummaryInfo();
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandShowSceneSummary(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->ShowSummaryInfo();
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandExportSceneSummary(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->ExportSummaryInfo(xr_string(p1).c_str());
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}

CCommandVar CommandSceneHighlightTexture(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        LPCSTR new_val 		 	= 0;
		if (TfrmChoseItem::SelectItem(smTexture,new_val,1)){
	       	Scene->HighlightTexture(new_val,false,0,0,false);
		    return 				TRUE;
        }
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}

CCommandVar CommandOptions(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        ExecCommand				(COMMAND_SHOW_PROPERTIES, p1, p2);
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}

CCommandVar CommandBuild(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if (mrYes==ELog.DlgMsg(mtConfirmation, TMsgDlgButtons()<<mbYes<<mbNo, "Are you sure to build level?"))
            return				Builder.Compile(false);
    }else{
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}
CCommandVar CommandMakeAIMap(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if (mrYes==ELog.DlgMsg(mtConfirmation, TMsgDlgButtons()<<mbYes<<mbNo, "Are you sure to export ai-map?"))
            return 				Builder.MakeAIMap( );
    }else{
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}
CCommandVar CommandMakeGame(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if (mrYes==ELog.DlgMsg(mtConfirmation, TMsgDlgButtons()<<mbYes<<mbNo, "Are you sure to export game?"))
            return				Builder.MakeGame( );
    }else{
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}
CCommandVar CommandMakeDetails(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if (mrYes==ELog.DlgMsg(mtConfirmation, TMsgDlgButtons()<<mbYes<<mbNo, "Are you sure to export details?"))
            return 				Builder.MakeDetails();
    }else{
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}
CCommandVar CommandMakeHOM(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if (mrYes==ELog.DlgMsg(mtConfirmation, TMsgDlgButtons()<<mbYes<<mbNo, "Are you sure to export HOM?"))
            return				Builder.MakeHOM();
    }else{
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}
CCommandVar CommandMakeSOM(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        if (mrYes==ELog.DlgMsg(mtConfirmation, TMsgDlgButtons()<<mbYes<<mbNo, "Are you sure to export Sound Occlusion Model?"))
            return				Builder.MakeSOM();
    }else{
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}
CCommandVar CommandInvertSelectionAll(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->InvertSelection	(LTools->CurrentClassID());
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}

CCommandVar CommandSelectAll(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->SelectObjects	(true,LTools->CurrentClassID());
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
    }
    return 						FALSE;
}

CCommandVar CommandDeselectAll(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->SelectObjects	(false,LTools->CurrentClassID());
	    return 					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}

CCommandVar CommandDeleteSelection(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->RemoveSelection	( LTools->CurrentClassID() );
        Scene->UndoSave			();
        return					TRUE;
    } else {
        ELog.DlgMsg( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}

CCommandVar CommandHideUnsel(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->ShowObjects		( false, LTools->CurrentClassID(), true, false );
        Scene->UndoSave			();
        ExecCommand				(COMMAND_UPDATE_PROPERTIES);
	    return 					TRUE;
    } else {
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandHideSel(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->ShowObjects		( bool(p1), LTools->CurrentClassID(), true, true );
        Scene->UndoSave			();
        ExecCommand				(COMMAND_UPDATE_PROPERTIES);
	    return 					TRUE;
    } else {
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandHideAll(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->ShowObjects		( bool(p1), LTools->CurrentClassID(), false );
        Scene->UndoSave			();
        ExecCommand				(COMMAND_UPDATE_PROPERTIES);
	    return 					TRUE;
    }else{
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandSetSnapObjects(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->SetSnapList		();
	    return 					TRUE;
    }else{
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandAddSelSnapObjects(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->AddSelToSnapList	();
	    return 					TRUE;
    }else{
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandDelSelSnapObjects(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->DelSelFromSnapList();
	    return 					TRUE;
    }else{
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandClearSnapObjects(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->ClearSnapList	(true);
	    return 					TRUE;
    }else{
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandSelectSnapObjects(CCommandVar p1, CCommandVar p2)
{
    if( !Scene->locked() ){
        Scene->SelectSnapList	();
	    return 					TRUE;
    }else{
        ELog.DlgMsg				( mtError, "Scene sharing violation" );
	    return 					FALSE;
    }
}
CCommandVar CommandRefreshSnapObjects(CCommandVar p1, CCommandVar p2)
{
    fraLeftBar->UpdateSnapList();
    return 						TRUE;
}
/*
CCommandVar CommandRefreshSoundEnvs(CCommandVar p1, CCommandVar p2)
{
    ::Sound->refresh_env_library();
    return 						TRUE;
//		::Sound->_restart();
}
*/

CCommandVar CommandRefreshSoundEnvGeometry(CCommandVar p1, CCommandVar p2)
{
    LSndLib->RefreshEnvGeometry();
    return 						TRUE;
}
CCommandVar CommandShowContextMenu(CCommandVar p1, CCommandVar p2)
{
    LUI->ShowContextMenu		(p1);
    return 						TRUE;
}
//------        
CCommandVar CommandRefreshUIBar(CCommandVar p1, CCommandVar p2)
{
    fraTopBar->RefreshBar		();
    fraLeftBar->RefreshBar		();
    fraBottomBar->RefreshBar	();
    return 						TRUE;
}
CCommandVar CommandRestoreUIBar(CCommandVar p1, CCommandVar p2)
{
    fraTopBar->fsStorage->RestoreFormPlacement();
    fraLeftBar->fsStorage->RestoreFormPlacement();
    fraBottomBar->fsStorage->RestoreFormPlacement();
    return 						TRUE;
}
CCommandVar CommandSaveUIBar(CCommandVar p1, CCommandVar p2)
{
    fraTopBar->fsStorage->SaveFormPlacement();
    fraLeftBar->fsStorage->SaveFormPlacement();
    fraBottomBar->fsStorage->SaveFormPlacement();
    return 						TRUE;
}
CCommandVar CommandUpdateToolBar(CCommandVar p1, CCommandVar p2)
{
    fraLeftBar->UpdateBar		();
    return 						TRUE;
}
CCommandVar CommandUpdateCaption(CCommandVar p1, CCommandVar p2)
{
    frmMain->UpdateCaption		();
    return 						TRUE;
}
//------
CCommandVar CommandCreateSoundLib(CCommandVar p1, CCommandVar p2)
{
    SndLib						= xr_new<CLevelSoundManager>();
    return 						TRUE;
}

extern BOOL ai_map_shown;
CCommandVar CommandToggleAiMapVisibility(CCommandVar p1, CCommandVar p2)
{
	ai_map_shown 				= !ai_map_shown;
    return 						TRUE;
}

void CLevelMain::RegisterCommands()
{
	inherited::RegisterCommands	();
    // tools
	REGISTER_SUB_CMD_CE	(COMMAND_CHANGE_TARGET,             "Change Target", 		LTools,CLevelTool::CommandChangeTarget, true);
		APPEND_SUB_CMD	("Object", 							OBJCLASS_SCENEOBJECT,	0);
		APPEND_SUB_CMD	("Light", 							OBJCLASS_LIGHT, 		0);
		APPEND_SUB_CMD	("Sound Source",					OBJCLASS_SOUND_SRC, 	0);
		APPEND_SUB_CMD	("Sound Env", 		                OBJCLASS_SOUND_ENV, 	0);
		APPEND_SUB_CMD	("Glow", 			                OBJCLASS_GLOW, 			0);
		APPEND_SUB_CMD	("Shape", 			                OBJCLASS_SHAPE, 		0);
		APPEND_SUB_CMD	("Spawn Point", 	                OBJCLASS_SPAWNPOINT, 	0);
		APPEND_SUB_CMD	("Way", 			                OBJCLASS_WAY, 			0);
		APPEND_SUB_CMD	("Way Point", 		                OBJCLASS_WAY, 			1);
		APPEND_SUB_CMD	("Toggle Way Mode",	                OBJCLASS_WAY, 			2);
		APPEND_SUB_CMD	("Sector", 			                OBJCLASS_SECTOR, 		0);
		APPEND_SUB_CMD	("Portal", 			                OBJCLASS_PORTAL, 		0);
		APPEND_SUB_CMD	("Group", 			                OBJCLASS_GROUP, 		0);
		APPEND_SUB_CMD	("Particle System",                 OBJCLASS_PS, 			0);
		APPEND_SUB_CMD	("Detail Objects", 	                OBJCLASS_DO, 			0);
		APPEND_SUB_CMD	("AI Map", 			                OBJCLASS_AIMAP, 		0);
		APPEND_SUB_CMD	("Static Wallmark",                 OBJCLASS_WM, 			0);
    REGISTER_SUB_CMD_END;    
	REGISTER_CMD_C	    (COMMAND_ENABLE_TARGET,           	LTools,CLevelTool::CommandEnableTarget);
	REGISTER_CMD_C	    (COMMAND_SHOW_TARGET,           	LTools,CLevelTool::CommandShowTarget);
	REGISTER_CMD_C	    (COMMAND_READONLY_TARGET,          	LTools,CLevelTool::CommandReadonlyTarget);
	REGISTER_CMD_C	    (COMMAND_MULTI_RENAME_OBJECTS,     	LTools,CLevelTool::CommandMultiRenameObjects);

	REGISTER_CMD_CE	    (COMMAND_SHOW_OBJECTLIST,           "Scene\\Show Object List",		LTools,CLevelTool::CommandShowObjectList, false);
	// common
	REGISTER_CMD_S	    (COMMAND_LIBRARY_EDITOR,           	CommandLibraryEditor);
	REGISTER_CMD_S	    (COMMAND_LANIM_EDITOR,            	CommandLAnimEditor);
	REGISTER_CMD_SE	    (COMMAND_FILE_MENU,              	"File\\Menu",					CommandFileMenu, 		true);
    REGISTER_CMD_S		(COMMAND_LOAD_LEVEL_PART,			CommandLoadLevelPart);
    REGISTER_CMD_S		(COMMAND_UNLOAD_LEVEL_PART,			CommandUnloadLevelPart);
	REGISTER_CMD_SE	    (COMMAND_LOAD,              		"File\\Load Level", 			CommandLoad, 			true);
    REGISTER_SUB_CMD_SE (COMMAND_SAVE, 						"File",							CommandSave,			true);
    	APPEND_SUB_CMD	("Save",							0,								0);
    	APPEND_SUB_CMD	("Save As",							0,								1);
    REGISTER_SUB_CMD_END;
	REGISTER_CMD_S	    (COMMAND_SAVE_BACKUP,              	CommandSaveBackup);
	REGISTER_CMD_SE	    (COMMAND_CLEAR,              		"File\\Clear Scene", 			CommandClear,			true);
	REGISTER_CMD_SE	    (COMMAND_LOAD_FIRSTRECENT,          "File\\Load First Recent",		CommandLoadFirstRecent, true);
	REGISTER_CMD_S	    (COMMAND_CLEAR_DEBUG_DRAW, 		    CommandClearDebugDraw);
	REGISTER_CMD_S	    (COMMAND_IMPORT_COMPILER_ERROR,     CommandImportCompilerError);
	REGISTER_CMD_S	    (COMMAND_EXPORT_COMPILER_ERROR,     CommandExportCompilerError);
	REGISTER_CMD_S	    (COMMAND_VALIDATE_SCENE,            CommandValidateScene);
	REGISTER_CMD_S	    (COMMAND_CLEAN_LIBRARY,           	CommandCleanLibrary);
	REGISTER_CMD_S	    (COMMAND_RELOAD_OBJECTS,            CommandReloadObjects);
	REGISTER_CMD_SE	    (COMMAND_CUT,              			"Edit\\Cut",					CommandCut,false);
	REGISTER_CMD_SE	    (COMMAND_COPY,              		"Edit\\Copy",					CommandCopy,false);
	REGISTER_CMD_SE	    (COMMAND_PASTE,              		"Edit\\Paste",					CommandPaste,false);
	REGISTER_CMD_S	    (COMMAND_LOAD_SELECTION,            CommandLoadSelection);
	REGISTER_CMD_S	    (COMMAND_SAVE_SELECTION,            CommandSaveSelection);
	REGISTER_CMD_SE	    (COMMAND_UNDO,              		"Edit\\Undo",					CommandUndo,false);
	REGISTER_CMD_SE	    (COMMAND_REDO,              		"Edit\\Redo",					CommandRedo,false);
	REGISTER_CMD_S	    (COMMAND_CLEAR_SCENE_SUMMARY,	    CommandClearSceneSummary);
	REGISTER_CMD_S	    (COMMAND_COLLECT_SCENE_SUMMARY,     CommandCollectSceneSummary);
	REGISTER_CMD_S	    (COMMAND_SHOW_SCENE_SUMMARY,        CommandShowSceneSummary);
	REGISTER_CMD_S	    (COMMAND_EXPORT_SCENE_SUMMARY,      CommandExportSceneSummary);
	REGISTER_CMD_S	    (COMMAND_SCENE_HIGHLIGHT_TEXTURE,	CommandSceneHighlightTexture);
	REGISTER_CMD_SE	    (COMMAND_OPTIONS,              		"Scene\\Options",		        CommandOptions,false);
	REGISTER_CMD_SE	    (COMMAND_BUILD,              		"Compile\\Build",		        CommandBuild,false);
	REGISTER_CMD_SE	    (COMMAND_MAKE_GAME,              	"Compile\\Make Game",	        CommandMakeGame,false);
	REGISTER_CMD_SE	    (COMMAND_MAKE_AIMAP,              	"Compile\\Make AI Map",	        CommandMakeAIMap,false);
	REGISTER_CMD_SE	    (COMMAND_MAKE_DETAILS,              "Compile\\Make Details",        CommandMakeDetails,false);
	REGISTER_CMD_SE	    (COMMAND_MAKE_HOM,              	"Compile\\Make HOM",	        CommandMakeHOM,false);
	REGISTER_CMD_SE	    (COMMAND_MAKE_SOM,              	"Compile\\Make SOM",	        CommandMakeSOM,false);
	REGISTER_CMD_SE	    (COMMAND_INVERT_SELECTION_ALL,      "Selection\\Invert", 			CommandInvertSelectionAll,false);
	REGISTER_CMD_SE	    (COMMAND_SELECT_ALL,              	"Selection\\Select All", 		CommandSelectAll,false);
	REGISTER_CMD_SE	    (COMMAND_DESELECT_ALL,              "Selection\\Unselect All", 		CommandDeselectAll,false);
	REGISTER_CMD_SE	    (COMMAND_DELETE_SELECTION,          "Edit\\Delete", 				CommandDeleteSelection,false);
	REGISTER_CMD_SE	    (COMMAND_HIDE_UNSEL,              	"Visibility\\Hide Unselected",	CommandHideUnsel,false);
	REGISTER_CMD_SE	    (COMMAND_HIDE_SEL,              	"Visibility\\Hide Selected", 	CommandHideSel,false);
	REGISTER_CMD_SE	    (COMMAND_HIDE_ALL,              	"Visibility\\Hide All", 		CommandHideAll,false);
	REGISTER_CMD_S		(COMMAND_SET_SNAP_OBJECTS,          CommandSetSnapObjects);
	REGISTER_CMD_S	    (COMMAND_ADD_SEL_SNAP_OBJECTS,      CommandAddSelSnapObjects);
	REGISTER_CMD_S	    (COMMAND_DEL_SEL_SNAP_OBJECTS,      CommandDelSelSnapObjects);
	REGISTER_CMD_S	    (COMMAND_CLEAR_SNAP_OBJECTS,        CommandClearSnapObjects);
	REGISTER_CMD_S	    (COMMAND_SELECT_SNAP_OBJECTS,       CommandSelectSnapObjects);
	REGISTER_CMD_S	    (COMMAND_REFRESH_SNAP_OBJECTS,      CommandRefreshSnapObjects);
//	REGISTER_CMD_S	    (COMMAND_REFRESH_SOUND_ENVS,        CommandRefreshSoundEnvs);
	REGISTER_CMD_S	    (COMMAND_REFRESH_SOUND_ENV_GEOMETRY,CommandRefreshSoundEnvGeometry);
	REGISTER_CMD_S	    (COMMAND_SHOWCONTEXTMENU,           CommandShowContextMenu);
	REGISTER_CMD_S	    (COMMAND_REFRESH_UI_BAR,            CommandRefreshUIBar);
	REGISTER_CMD_S	    (COMMAND_RESTORE_UI_BAR,            CommandRestoreUIBar);
	REGISTER_CMD_S	    (COMMAND_SAVE_UI_BAR,              	CommandSaveUIBar);
	REGISTER_CMD_S	    (COMMAND_UPDATE_TOOLBAR,            CommandUpdateToolBar);
	REGISTER_CMD_S	    (COMMAND_UPDATE_CAPTION,            CommandUpdateCaption);
	REGISTER_CMD_S	    (COMMAND_CREATE_SOUND_LIB,          CommandCreateSoundLib);
	REGISTER_CMD_SE	    (COMMAND_TOGGLE_AIMAP_VISIBILITY,   "Visibility\\Toggle AIMap",			CommandToggleAiMapVisibility,true);
	REGISTER_CMD_S	    (COMMAND_SHOW_CLIP_EDITOR,			CommandShowClipEditor);
    
}

char* CLevelMain::GetCaption()
{
	return Tools->m_LastFileName.IsEmpty()?"noname":Tools->m_LastFileName.c_str();
}

bool __fastcall CLevelMain::ApplyShortCut(WORD Key, TShiftState Shift)
{
    return inherited::ApplyShortCut(Key,Shift);
}
//---------------------------------------------------------------------------

bool __fastcall CLevelMain::ApplyGlobalShortCut(WORD Key, TShiftState Shift)
{
    return inherited::ApplyGlobalShortCut(Key,Shift);
}
//---------------------------------------------------------------------------
void RetrieveSceneObjPointAndNormal( Fvector& hitpoint, Fvector* hitnormal, const SRayPickInfo &pinf, int bSnap )
{
    if(pinf.e_mesh == 0)
    {
      hitpoint = pinf.pt;
       if (hitnormal && pinf.visual_inf.K )
        	*hitnormal = pinf.visual_inf.normal;
       return;
    }
	if ( Tools->GetSettings(etfVSnap) && bSnap )
    {
    	Fvector pn;
        float u = pinf.inf.u;
        float v = pinf.inf.v;
        float w = 1-(u+v);
        Fvector verts[3];
        pinf.e_obj->GetFaceWorld( pinf.s_obj->_Transform(), pinf.e_mesh, pinf.inf.id, verts );
        
        if ((w>u) && (w>v))
        	pn.set(verts[0]);
        else if ((u>w) && (u>v))
        	pn.set(verts[1]);
        else
        	pn.set(verts[2]);
            
        if (pn.distance_to(pinf.pt) < LTools->m_MoveSnap)
        	hitpoint.set(pn);
        else
        	hitpoint.set(pinf.pt);
            
    }
    else
    {
    	hitpoint.set(pinf.pt);
    }

    if (hitnormal)
    {
    	Fvector verts[3];
        pinf.e_obj->GetFaceWorld(pinf.s_obj->_Transform(),pinf.e_mesh,pinf.inf.id,verts);
        hitnormal->mknormal(verts[0],verts[1],verts[2]);
    }
}


bool EditLibPickObjectGeometry(  Fvector& hitpoint,  const Fvector& start, const Fvector& direction, int bSnap, Fvector* hitnormal )
{
	SRayPickInfo pinf;
	if( TfrmEditLibrary::RayPick( start, direction, &pinf ) )
    {
    	RetrieveSceneObjPointAndNormal( hitpoint,  hitnormal, pinf, bSnap );
        return true;
    }
    return false;
}

bool ScenePickObjectGeometry( Fvector& hitpoint,  const Fvector& start, const Fvector& direction, int bSnap, Fvector* hitnormal )
{

	SRayPickInfo pinf;

   
    SRayPickInfo l_pinf;
    bool bResult = false;

    {
      SRayPickInfo l_pinf;
      bool l_bres = Scene->RayPickObject( l_pinf.inf.range, start,direction, OBJCLASS_SPAWNPOINT , &l_pinf, Scene->GetSnapList(false) );
      
      if( l_bres )
      {
          pinf = l_pinf;
          bResult = true;
      }
      
    }
    {
    
     SRayPickInfo l_pinf;
     bool l_bres = Scene->RayPickObject( l_pinf.inf.range, start, direction, OBJCLASS_SCENEOBJECT , &l_pinf, Scene->GetSnapList(false) );

     if( !bResult||(l_bres && l_pinf.inf.range < pinf.inf.range) )
          pinf = l_pinf;
     if( l_bres )
           bResult = true;
           
    }


     if( bResult )
     	    RetrieveSceneObjPointAndNormal( hitpoint,  hitnormal, pinf, bSnap );
            
     return  bResult;

}


bool PickObjectGeometry( EEditorState est, Fvector& hitpoint,  const Fvector& start, const Fvector& direction, int bSnap, Fvector* hitnormal )
{

	switch(est)
    {
       case esEditLibrary:
         	return EditLibPickObjectGeometry( hitpoint, start, direction, bSnap, hitnormal );
       case esEditScene:
         	return ScenePickObjectGeometry( hitpoint, start, direction, bSnap, hitnormal );
        default:
         	NODEFAULT;
    }
	return false;
}

bool PickGrid(  Fvector& hitpoint,  const Fvector& start, const Fvector& direction, int bSnap, Fvector* hitnormal )
{
     
    // pick grid
	Fvector normal;
	normal.set( 0, 1, 0 );
	float clcheck = direction.dotproduct( normal );

	if( fis_zero( clcheck ) )
    	 return false;

	float alpha = - start.dotproduct(normal) / clcheck;
    
	if( alpha <= 0 )
    	return false;

	hitpoint.x = start.x + direction.x * alpha;
	hitpoint.y = start.y + direction.y * alpha;
	hitpoint.z = start.z + direction.z * alpha;

    if (Tools->GetSettings(etfGSnap) && bSnap)
    {
        hitpoint.x = snapto( hitpoint.x, LTools->m_MoveSnap );
        hitpoint.z = snapto( hitpoint.z, LTools->m_MoveSnap );
        hitpoint.y = 0.f;
    }
    
	if (hitnormal)
    	hitnormal->set(0,1,0);
	return true;
}

bool CLevelMain::PickGround(Fvector& hitpoint, const Fvector& start, const Fvector& direction, int bSnap, Fvector* hitnormal){
	VERIFY(m_bReady);
    
    EEditorState est = GetEState();
    if( est!= esEditLibrary && est != esEditScene )
    	return false;
   
    // pick object geometry
    if( (bSnap==-1) || ( Tools->GetSettings(etfOSnap) && (bSnap==1) ) )
    {
      bool b =  PickObjectGeometry( est, hitpoint, start, direction, bSnap,  hitnormal );
      if(b)
      return true;
    }

    return   PickGrid( hitpoint, start, direction, bSnap,  hitnormal );

}
//----------------------------------------------------

bool CLevelMain::SelectionFrustum(CFrustum& frustum)
{
	VERIFY(m_bReady);
    Fvector st,d,p[4];
    Ivector2 pt[4];

    float depth = 0;

    float x1=m_StartCp.x, x2=m_CurrentCp.x;
    float y1=m_StartCp.y, y2=m_CurrentCp.y;

	if(!(x1!=x2&&y1!=y2)) return false;

	pt[0].set(_min(x1,x2),_min(y1,y2));
	pt[1].set(_max(x1,x2),_min(y1,y2));
	pt[2].set(_max(x1,x2),_max(y1,y2));
	pt[3].set(_min(x1,x2),_max(y1,y2));

    SRayPickInfo pinf;
    for (int i=0; i<4; i++){
	    EDevice.m_Camera.MouseRayFromPoint(st, d, pt[i]);
        if (EPrefs->bp_lim_depth){
			pinf.inf.range = EDevice.m_Camera._Zfar(); // max pick range
            if (Scene->RayPickObject(pinf.inf.range, st, d, OBJCLASS_SCENEOBJECT, &pinf, 0))
	            if (pinf.inf.range > depth) depth = pinf.inf.range;
        }
    }
    if (depth<EDevice.m_Camera._Znear()) depth = EDevice.m_Camera._Zfar();
    else depth += EPrefs->bp_depth_tolerance;

    for (i=0; i<4; i++){
	    EDevice.m_Camera.MouseRayFromPoint(st, d, pt[i]);
        p[i].mad(st,d,depth);
    }

    Fvector pos = EDevice.m_Camera.GetPosition();
    frustum.CreateFromPoints(p,4,pos);

    Fplane P; P.build(p[0],p[1],p[2]);
    if (P.classify(st)>0) P.build(p[2],p[1],p[0]);
	frustum._add(P);

	return true;
}
//----------------------------------------------------
void CLevelMain::RealUpdateScene()
{
	inherited::RealUpdateScene	();
	if (GetEState()==esEditScene){
	    Scene->OnObjectsUpdate	();
    	LTools->OnObjectsUpdate	(); // обновить все что как-то связано с объектами
	    RedrawScene				();
    }
}
//---------------------------------------------------------------------------


void CLevelMain::ShowContextMenu(int cls)
{
	VERIFY(m_bReady);
    POINT pt;
    GetCursorPos(&pt);
    fraLeftBar->miProperties->Enabled = false;
    if (Scene->SelectionCount( true, cls )) fraLeftBar->miProperties->Enabled = true;
    RedrawScene(true);
    fraLeftBar->pmObjectContext->TrackButton = tbRightButton;
    fraLeftBar->pmObjectContext->Popup(pt.x,pt.y);
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Common
//---------------------------------------------------------------------------
void CLevelMain::ResetStatus()
{
	VERIFY(m_bReady);
    if (fraBottomBar->paStatus->Caption!=""){
	    fraBottomBar->paStatus->Caption=""; fraBottomBar->paStatus->Repaint();
    }
}
void CLevelMain::SetStatus(LPSTR s, bool bOutLog)
{
	VERIFY(m_bReady);
    if (fraBottomBar->paStatus->Caption!=s){
	    fraBottomBar->paStatus->Caption=s; fraBottomBar->paStatus->Repaint();
    	if (bOutLog&&s&&s[0]) ELog.Msg(mtInformation,s);
    }
}
void CLevelMain::ProgressDraw()
{
	fraBottomBar->RedrawBar();
}
//---------------------------------------------------------------------------
void CLevelMain::OutCameraPos()
{
	if (m_bReady){
        AnsiString s;
        const Fvector& c 	= EDevice.m_Camera.GetPosition();
        s.sprintf("C: %3.1f, %3.1f, %3.1f",c.x,c.y,c.z);
    //	const Fvector& hpb 	= EDevice.m_Camera.GetHPB();
    //	s.sprintf(" Cam: %3.1f°, %3.1f°, %3.1f°",rad2deg(hpb.y),rad2deg(hpb.x),rad2deg(hpb.z));
        fraBottomBar->paCamera->Caption=s; fraBottomBar->paCamera->Repaint();
    }
}
//---------------------------------------------------------------------------
void CLevelMain::OutUICursorPos()
{
	VERIFY(fraBottomBar);
    AnsiString s; POINT pt;
    GetCursorPos(&pt);
    s.sprintf("Cur: %d, %d",pt.x,pt.y);
    fraBottomBar->paUICursor->Caption=s; fraBottomBar->paUICursor->Repaint();
}
//---------------------------------------------------------------------------
void CLevelMain::OutGridSize()
{
	VERIFY(fraBottomBar);
    AnsiString s;
    s.sprintf("Grid: %1.1f",EPrefs->grid_cell_size);
    fraBottomBar->paGridSquareSize->Caption=s; fraBottomBar->paGridSquareSize->Repaint();
}
//---------------------------------------------------------------------------
void CLevelMain::OutInfo()
{
	fraBottomBar->paSel->Caption = Tools->GetInfo();
}
//---------------------------------------------------------------------------
void CLevelMain::RealQuit()
{
	frmMain->Close();
}
//---------------------------------------------------------------------------

#define INI_RTP_NAME(buf) 		{FS.update_path(buf,"$local_root$","rt_object_props.ltx");}

void CLevelMain::SaveSettings(CInifile* I)
{
	m_rt_object_props->save_as();
    
	inherited::SaveSettings(I);
    SSceneSummary::Save(I);
}
void CLevelMain::LoadSettings(CInifile* I)
{
	string_path			fn;
	INI_RTP_NAME		(fn);
	m_rt_object_props = CInifile::Create(fn,FALSE);
	m_rt_object_props->save_at_end(FALSE);
    
	inherited::LoadSettings(I);
    SSceneSummary::Load(I);
}

void CLevelMain::store_rt_flags(const CCustomObject* CO)
{
    if(LTools->m_LastFileName.Length() && CO->Name && xr_strlen(CO->Name) )
    {
   	m_rt_object_props->remove_line(LTools->m_LastFileName.c_str(), CO->Name);
	if(CO->Selected() || !CO->Visible())
    	m_rt_object_props->w_u32(LTools->m_LastFileName.c_str(), CO->Name, CO->m_RT_Flags.get()&(CCustomObject::flRT_Selected|CCustomObject::flRT_Visible));
    }
}

void CLevelMain::restore_rt_flags(CCustomObject* CO)
{
	if(CO->Name && LTools->m_LastFileName.Length() && m_rt_object_props->line_exist(LTools->m_LastFileName.c_str(), CO->Name))
    {
		u32 fl = m_rt_object_props->r_u32(LTools->m_LastFileName.c_str(), CO->Name);
        CO->m_RT_Flags.set	(CCustomObject::flRT_Visible|CCustomObject::flRT_Selected|CCustomObject::flRT_SelectedLast, FALSE);
        CO->m_RT_Flags.or	(fl);
    }
}

//---------------------------------------------------------------------------

