#include "stdafx.h"
#pragma hdrstop

#include "ESceneSpawnControls.h"
#include "ui_leveltools.h"
#include "../ECore/Editor/ui_main.h"
#include "FrameSpawn.h"
#include "Scene.h"
#include "SpawnPoint.h"
//---------------------------------------------------------------------------
__fastcall TUI_ControlSpawnAdd::TUI_ControlSpawnAdd(int st, int act, ESceneToolBase* parent):TUI_CustomControl(st,act,parent){
}

bool __fastcall TUI_ControlSpawnAdd::AppendCallback(SBeforeAppendCallbackParams* p)
{
	LPCSTR ref_name = ((TfraSpawn*)parent_tool->pFrame)->Current();
    if (!ref_name){
    	ELog.DlgMsg(mtInformation,"Nothing selected.");
    	return false;
    }
    if(Scene->LevelPrefix().c_str())
    {
		p->name_prefix 	= 	Scene->LevelPrefix().c_str();
    	p->name_prefix 	+= 	"_";
    }
	p->name_prefix 	+= 	ref_name;
	p->data 		= (void*)ref_name;
    return (0!=p->name_prefix.length());
}

bool __fastcall TUI_ControlSpawnAdd::Start(TShiftState Shift)
{
    TfraSpawn* F = (TfraSpawn*)parent_tool->pFrame;
	if (F->ebAttachObject->Down){
		CCustomObject* from = Scene->RayPickObject(UI->ZFar(), UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_DUMMY, 0, 0);
        if (from->ClassID!=OBJCLASS_SPAWNPOINT){
            ObjectList 	lst;
            int cnt 	= Scene->GetQueryObjects(lst,OBJCLASS_SPAWNPOINT,1,1,0);
            if (1!=cnt)	ELog.DlgMsg(mtError,"Select one shape.");
            else{
                CSpawnPoint* base = dynamic_cast<CSpawnPoint*>(lst.back()); R_ASSERT(base);
                if (base->AttachObject(from)){
                    if (!Shift.Contains(ssAlt)){
                        F->ebAttachObject->Down	= false;
                        ResetActionToSelect		();
                    }
                }else{
		        	ELog.DlgMsg(mtError,"Attach impossible.");
                }
            }
        }else{
        	ELog.DlgMsg(mtError,"Attach impossible.");
        }
    }else{
	    DefaultAddObject(Shift,AppendCallback);             
    }
    return false;
}

