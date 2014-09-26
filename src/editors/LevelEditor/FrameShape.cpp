#include "stdafx.h"
#pragma hdrstop

#include "FrameShape.h"
#include "ui_leveltools.h"
#include "scene.h"
#include "EShape.h"
#include "ESceneShapeTools.h"
#include "../ECore/Editor/ui_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TfraShape::TfraShape(TComponent* Owner)
        : TForm(Owner)
{
    DEFINE_INI(fsStorage);
}
//---------------------------------------------------------------------------
void __fastcall TfraShape::PaneMinClick(TObject *Sender)
{
    PanelMinMaxClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraShape::ExpandClick(TObject *Sender)
{
    PanelMaximizeClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraShape::ebDetachAllShapesClick(TObject *Sender)
{
    ObjectList lst;
    if (Scene->GetQueryObjects(lst,OBJCLASS_SHAPE,1,1,0)){
		Scene->SelectObjects(false,OBJCLASS_SHAPE);
    	for (ObjectIt it=lst.begin(); it!=lst.end(); it++)
        	((CEditShape*)*it)->Detach();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraShape::ebAttachShapeClick(TObject *Sender)
{
	if (ebAttachShape->Down) ExecCommand(COMMAND_CHANGE_ACTION, etaAdd);
}
//---------------------------------------------------------------------------




void __fastcall TfraShape::ebEditLevelBoundModeClick(TObject *Sender)
{
	ebRecalcLB->Enabled = ebEditLevelBoundMode->Down;
	if(ebEditLevelBoundMode->Down)
    {
       	tool->OnEditLevelBounds(false);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraShape::ebRecalcLBClick(TObject *Sender)
{
     	tool->OnEditLevelBounds(true);
}
//---------------------------------------------------------------------------

