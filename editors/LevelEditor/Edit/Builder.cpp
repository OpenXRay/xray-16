//----------------------------------------------------
// file: Builder.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "Builder.h"
#include "Scene.h"
#include "PortalUtils.h"
#include "ESceneDOTools.h"
#include "../ECore/Editor/ui_main.h"
#include "../ECore/Editor/ui_toolscustom.h"
#include "xrHemisphere.h"
//----------------------------------------------------

SceneBuilder Builder;
//----------------------------------------------------

ICF static void simple_hemi_callback(float x, float y, float z, float E, LPVOID P)
{
    SceneBuilder::BLVec* dst 	= (SceneBuilder::BLVec*)P;
    SceneBuilder::SBuildLight 	T;
    T.energy     				= E;
    T.light.direction.set		(x,y,z);
    dst->push_back  			(T);
}

SceneBuilder::SceneBuilder()
{
    m_iDefaultSectorNum = 0;
    l_scene_stat		= 0;
    l_verts				= 0;
    l_faces				= 0;
    l_smgroups			= 0;
    l_vert_cnt			= 0;
    l_face_cnt 			= 0;
	l_vert_it	 		= 0;
	l_face_it			= 0;
    object_for_render	= 0;
	m_save_as_object	= false;
}

SceneBuilder::~SceneBuilder()
{
}

//------------------------------------------------------------------------------
#define CHECK_BREAK     	if (UI->NeedAbort()) break;
#define VERIFY_COMPILE(x,c1,c2) CHECK_BREAK \
							if (!x){error_text.sprintf("ERROR: %s %s", c1,c2); break;}
//------------------------------------------------------------------------------
BOOL SceneBuilder::Compile(bool b_selected_only)
{
	if(m_save_as_object)
	{
		EvictResource	();
        GetBounding		();
        CompileStatic	(b_selected_only);
        EvictResource	();
		return TRUE;
	}

	AnsiString error_text		= "";
	UI->ResetBreak				();
	if(UI->ContainEState(esBuildLevel)) return false;
	ELog.Msg( mtInformation, "Building started..." );

    UI->BeginEState(esBuildLevel);
    try{
        do{
	        // check debug
            bool bTestPortal = Scene->ObjCount(OBJCLASS_SECTOR)||Scene->ObjCount(OBJCLASS_PORTAL);
	        // validate scene
    	    VERIFY_COMPILE(Scene->Validate(false,bTestPortal,true,true,true,true),"Validation failed.","Invalid scene.");
			// fill simple hemi
            simple_hemi.clear	();
	        xrHemisphereBuild	(1,2.f,simple_hemi_callback,&simple_hemi);
        	// build
            VERIFY_COMPILE		(PreparePath(),				"Failed to prepare level path","");
            VERIFY_COMPILE		(PrepareFolders(),			"Failed to prepare level folders","");
            VERIFY_COMPILE		(EvictResource(),	  		"Failed to evict resource","");
            VERIFY_COMPILE		(GetBounding(),				"Failed to acquire level bounding volume","");
            VERIFY_COMPILE		(RenumerateSectors(), 		"Failed to renumerate sectors","");
            VERIFY_COMPILE		(CompileStatic(false),	  		"Failed static remote build","");
            VERIFY_COMPILE		(EvictResource(),	  		"Failed to evict resource","");
            VERIFY_COMPILE		(BuildLTX(),		  		"Failed to build level description","");
            VERIFY_COMPILE		(BuildGame(),		  		"Failed to build game","");
            VERIFY_COMPILE		(BuildSceneStat(),			"Failed to build scene statistic","");
            BuildHOMModel		();
            BuildSOMModel		();
    	    // build tools
            SceneToolsMapPairIt _I 	= Scene->FirstTool();
            SceneToolsMapPairIt _E	= Scene->LastTool();
            for (; _I!=_E; ++_I)
            {
            	if (_I->first!=OBJCLASS_DUMMY)
                {
                    if (_I->second->Valid())
                    {
                        VERIFY_COMPILE(_I->second->Export(m_LevelPath),_I->second->ClassDesc(),"export failed.");
                        ELog.Msg(mtInformation,"Process %s - done.",_I->second->ClassDesc());
                    }else
                    {
                        ELog.Msg(mtError,"Process %s - failed.",_I->second->ClassDesc());
                    }
                }
            }
		    Clear			();
        } while(0);

        if (!error_text.IsEmpty()) 	ELog.DlgMsg(mtError,error_text.c_str());
        else if (UI->NeedAbort())	ELog.DlgMsg(mtInformation,"Building terminated.");
        else						ELog.DlgMsg(mtInformation,"Building OK.");
    }catch(...){
    	ELog.DlgMsg(mtError,"Error has occured in builder routine. Editor aborted.");
        abort();
    }
    UI->EndEState();

	return error_text.IsEmpty();
}
//------------------------------------------------------------------------------

BOOL SceneBuilder::MakeGame( )
{
	AnsiString error_text="";
	UI->ResetBreak();
	if(UI->ContainEState(esBuildLevel)) return false;
	ELog.Msg( mtInformation, "Making started..." );

    UI->BeginEState(esBuildLevel);
    try{
        do{
	        // clear error
            Tools->ClearDebugDraw();
	        // validate scene
    	    VERIFY_COMPILE(Scene->Validate(false,false,false,false,false,false),	"Validation failed.","Invalid scene.");
        	// build
            VERIFY_COMPILE(PreparePath(),				"Failed to prepare level path.","");
            VERIFY_COMPILE(GetBounding(),				"Failed to acquire level bounding volume.","");
            VERIFY_COMPILE(RenumerateSectors(), 		"Failed to renumerate sectors","");
            VERIFY_COMPILE(BuildLTX(),					"Failed to build level description.","");
            VERIFY_COMPILE(BuildGame(),					"Failed to build game.","");
        } while(0);

        if (!error_text.IsEmpty()) 	ELog.DlgMsg(mtError,error_text.c_str());
        else if (UI->NeedAbort())	ELog.DlgMsg(mtInformation,"Making terminated.");
        else						ELog.DlgMsg(mtInformation,"Making finished.");
    }catch(...){
    	ELog.DlgMsg(mtError,"Error has occured in builder routine. Editor aborted.");
        abort();
    }
    UI->EndEState();

	return error_text.IsEmpty();
}
//------------------------------------------------------------------------------

BOOL SceneBuilder::MakeAIMap()
{
	AnsiString error_text;
    do{
		VERIFY_COMPILE(PreparePath(),				"Failed to prepare level path.","");
		VERIFY_COMPILE(BuildAIMap(),				"Failed to build AI-Map.","");
    }while(0);
    if (!error_text.IsEmpty()) 	ELog.DlgMsg(mtError,error_text.c_str());
    else if (UI->NeedAbort())	ELog.DlgMsg(mtInformation,"Building terminated.");
    else						ELog.DlgMsg(mtInformation,"AI-Map succesfully exported.");

	return error_text.IsEmpty();
}
//------------------------------------------------------------------------------

BOOL SceneBuilder::MakeDetails()
{
	AnsiString error_text;
    do{
		VERIFY_COMPILE(PreparePath(),				"Failed to prepare level path.","");
        // save details
		VERIFY_COMPILE(Scene->GetTool(OBJCLASS_DO)->Export(m_LevelPath), "Export failed.","");
    }while(0);
    if (!error_text.IsEmpty()) 	ELog.DlgMsg(mtError,error_text.c_str());
    else if (UI->NeedAbort())	ELog.DlgMsg(mtInformation,"Building terminated.");
    else						ELog.DlgMsg(mtInformation,"Details succesfully exported.");

	return error_text.IsEmpty();
}
//------------------------------------------------------------------------------

BOOL SceneBuilder::MakeHOM( )
{
	AnsiString error_text="";
	UI->ResetBreak();
	if(UI->ContainEState(esBuildLevel)) return false;
	ELog.Msg( mtInformation, "Making started..." );

    UI->BeginEState(esBuildLevel);
    try{
        do{
        	// build
            VERIFY_COMPILE(PreparePath(),				"Failed to prepare level path.","");
            VERIFY_COMPILE(BuildHOMModel(),				"Failed to build HOM model.","");
        } while(0);

        if (!error_text.IsEmpty()) 	ELog.DlgMsg(mtError,error_text.c_str());
        else if (UI->NeedAbort())	ELog.DlgMsg(mtInformation,"Building terminated...");
        else						ELog.DlgMsg(mtInformation,"Building OK...");
    }catch(...){
    	ELog.DlgMsg(mtError,"Error has occured in builder routine. Editor aborted.");
        abort();
    }
    UI->EndEState();

	return error_text.IsEmpty();
}
//------------------------------------------------------------------------------

BOOL SceneBuilder::MakeSOM( )
{
	AnsiString error_text="";
	UI->ResetBreak();
	if(UI->ContainEState(esBuildLevel)) return false;
	ELog.Msg( mtInformation, "Making started..." );

    UI->BeginEState(esBuildLevel);
    try{
        do{
        	// build
            VERIFY_COMPILE(PreparePath(),				"Failed to prepare level path.","");
            VERIFY_COMPILE(BuildSOMModel(),				"Failed to build SOM model.","");
        } while(0);

        if (!error_text.IsEmpty()) 	ELog.DlgMsg(mtError,error_text.c_str());
        else if (UI->NeedAbort())	ELog.DlgMsg(mtInformation,"Building terminated...");
        else						ELog.DlgMsg(mtInformation,"Building OK...");
    }catch(...){
    	ELog.DlgMsg(mtError,"Error has occured in builder routine. Editor aborted.");
        abort();
    }
    UI->EndEState();

	return error_text.IsEmpty();
}
//------------------------------------------------------------------------------
#include "../ECore/Editor/EditObject.h"
void SceneBuilder::OnRender()
{
	if (object_for_render){
        object_for_render->OnFrame();
        object_for_render->RenderSingle(Fidentity);
    }
}

