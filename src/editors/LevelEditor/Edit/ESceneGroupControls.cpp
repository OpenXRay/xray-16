#include "stdafx.h"
#pragma hdrstop

#include "ESceneGroupControls.h"
#include "ESceneGroupTools.h"
#include "GroupObject.h"
#include "Scene.h"

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
__fastcall TUI_ControlGroupAdd::TUI_ControlGroupAdd(int st, int act, ESceneToolBase* parent):TUI_CustomControl(st,act,parent)
{
}

bool __fastcall TUI_ControlGroupAdd::AfterAppendCallback(TShiftState Shift, CCustomObject* obj)
{
	bool result 			= false;
    ESceneGroupTool* ot		= dynamic_cast<ESceneGroupTool*>(parent_tool);
    if (ot->GetCurrentObject())
    {
        CGroupObject* group	= dynamic_cast<CGroupObject*>(obj); R_ASSERT(group);
        LPCSTR short_name	= ot->GetCurrentObject();
        result 				= group->SetReference(short_name);
        if (result){
            string256 			namebuffer;
            Scene->GenObjectName(OBJCLASS_GROUP, namebuffer, short_name);
            group->Name 		= namebuffer;
        }
    }
    return result;
}
bool __fastcall TUI_ControlGroupAdd::Start(TShiftState Shift)
{
	DefaultAddObject	(Shift,0,AfterAppendCallback);
    return false;
}
void __fastcall TUI_ControlGroupAdd::Move(TShiftState _Shift)
{
}
bool __fastcall TUI_ControlGroupAdd::End(TShiftState _Shift)
{
    return true;
}


