#include "stdafx.h"
#pragma hdrstop

#include "../ECore/Editor/ui_main.h"
#include "ui_leveltools.h"
#include "FrameWayPoint.h"
#include "Scene.h"
#include "WayPoint.h"
#include "ESceneWayControls.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TfraWayPoint::TfraWayPoint(TComponent* Owner)
        : TForm(Owner)
{
    DEFINE_INI(fsStorage);
}
//---------------------------------------------------------------------------
void __fastcall TfraWayPoint::PanelMinClick(TObject *Sender)
{
    PanelMinMaxClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ExpandClick(TObject *Sender)
{
    PanelMaximizeClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ebAdd1LinksClick(TObject *Sender)
{
	if (!ebModePoint->Down){
    	ELog.DlgMsg(mtInformation,"Before editing enter Point Mode.");
    	return;
    }
    bool bRes = false;
	ObjectList lst;
    Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
	// remove links
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++){
    	((CWayObject*)(*it))->RemoveLink();
    	bRes|=((CWayObject*)(*it))->Add1Link();
    }
	if (bRes) Scene->UndoSave();
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ebAdd2LinkClick(TObject *Sender)
{
	if (!ebModePoint->Down){
    	ELog.DlgMsg(mtInformation,"Before editing enter Point Mode.");
    	return;
    }
    bool bRes = false;
	ObjectList lst;
    Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++)
    	bRes|=((CWayObject*)(*it))->Add2Link();
	if (bRes) Scene->UndoSave();
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ebConvert1Click(TObject *Sender)
{
	ObjectList lst;
    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++)
    	((CWayObject*)(*it))->Convert1Link();
	if (cnt) Scene->UndoSave();
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ebConvert2Click(TObject *Sender)
{
	ObjectList lst;
    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++)
    	((CWayObject*)(*it))->Convert2Link();
	if (cnt) Scene->UndoSave();
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ebRemoveLinksClick(TObject *Sender)
{
	if (!ebModePoint->Down){
    	ELog.DlgMsg(mtInformation,"Before editing enter Point Mode.");
    	return;
    }
	ObjectList lst;
    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++)
    	((CWayObject*)(*it))->RemoveLink();
	if (cnt) Scene->UndoSave();
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ebInvertLinkClick(TObject *Sender)
{
	if (!ebModePoint->Down){
    	ELog.DlgMsg(mtInformation,"Before editing enter Point Mode.");
    	return;
    }
	ObjectList lst;
    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++)
    	((CWayObject*)(*it))->InvertLink();
	if (cnt) Scene->UndoSave();
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfraWayPoint::ebModeWayClick(TObject *Sender)
{
	LTools->SetTarget		(OBJCLASS_WAY,dynamic_cast<TExtBtn*>(Sender)->Tag);
}
//---------------------------------------------------------------------------


