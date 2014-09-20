#include "stdafx.h"
#pragma hdrstop

#include "FrameSector.h"
#include "scene.h"
#include "sector.h"
#include "../ECore/Editor/ui_main.h"
#include "portal.h"
#include "portalutils.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mxPlacemnt"
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TfraSector::TfraSector(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TfraSector::PanelMinClick(TObject *Sender)
{
    PanelMinMaxClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TfraSector::TopClick(TObject *Sender)
{
    PanelMaximizeClick(Sender);           
}
//---------------------------------------------------------------------------

void __fastcall TfraSector::ebCaptureInsideVolumeClick(TObject *Sender)
{
	CSector* S = PortalUtils.GetSelectedSector();
	if (S){
    	S->CaptureInsideVolume();
        Scene->UndoSave();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraSector::ebDistributeObjectsClick(TObject *Sender)
{
	CSector* S = PortalUtils.GetSelectedSector();
	if (S){
    	S->DistributeInsideObjects	();
        Scene->UndoSave				();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraSector::ebCreateDefaultClick(TObject *Sender)
{
    CCustomObject* O=Scene->FindObjectByName(DEFAULT_SECTOR_NAME,OBJCLASS_SECTOR);
	if (O) ELog.DlgMsg(mtInformation,"Default sector already present. Remove this and try again.");
    else{
		if (!PortalUtils.CreateDefaultSector()) ELog.DlgMsg(mtInformation,"Default can't created.");
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraSector::ebRemoveDefaultClick(TObject *Sender)
{
	if (!PortalUtils.RemoveDefaultSector()) ELog.DlgMsg(mtInformation,"Default sector not found.");
}
//---------------------------------------------------------------------------

void __fastcall TfraSector::ebValidateClick(TObject *Sender)
{
	PortalUtils.Validate(true);
}
//---------------------------------------------------------------------------

void __fastcall TfraSector::ebBoxPickClick(TObject *Sender)
{
	if (ebCreateNewSingle->Down || ebCreateNewMultiple->Down) ebBoxPick->Down = false;
}
//---------------------------------------------------------------------------


