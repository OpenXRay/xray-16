#include "stdafx.h"
#pragma hdrstop

#include "EScenePSControls.h"
#include "ui_leveltools.h"
#include "..\..\Layers\xrRender\PSLibrary.h"
#include "EParticlesObject.h"
#include "scene.h"
#include "FramePS.h"
#include "../ECore/Editor/ui_main.h"

//----------------------------------------------------------------------
//
//------------------------------------------------------------------------------
__fastcall TUI_ControlPSAdd::TUI_ControlPSAdd(int st, int act, ESceneToolBase* parent):TUI_CustomControl(st,act,parent){
}

bool __fastcall TUI_ControlPSAdd::AfterAppendCallback(TShiftState Shift, CCustomObject* obj)
{
	EParticlesObject* pg= dynamic_cast<EParticlesObject*>(obj); R_ASSERT(pg);
    LPCSTR ref_name		= ((TfraPS*)parent_tool->pFrame)->Current();
    if (!ref_name){
    	ELog.DlgMsg(mtInformation,"Nothing selected.");
    	return false;
    }
	if (!pg->Compile(ref_name)){
    	ELog.DlgMsg(mtInformation,"Can't compile particle system '%s'.",ref_name);
        return false;
    }
    return true;
}

bool __fastcall TUI_ControlPSAdd::Start(TShiftState Shift)
{
    DefaultAddObject(Shift,0,AfterAppendCallback);
    return false;
}

void __fastcall TUI_ControlPSAdd::Move(TShiftState _Shift)
{
}
bool __fastcall TUI_ControlPSAdd::End(TShiftState _Shift)
{
    return true;
}


