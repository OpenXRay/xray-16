#include "stdafx.h"
#pragma hdrstop

#include "FrameFogVol.h"
#include "ESceneFogVolumeTools.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ExtBtn"
#pragma resource "*.dfm"


//---------------------------------------------------------------------------
__fastcall TfraFogVol::TfraFogVol(TComponent* Owner, ESceneFogVolumeTool* gt)
	: TForm(Owner)
{
	ParentTools = gt;
}
//---------------------------------------------------------------------------
void __fastcall TfraFogVol::ebGroupClick(TObject *Sender)
{
	ParentTools->GroupSelected();
}
//---------------------------------------------------------------------------
void __fastcall TfraFogVol::ebUngroupClick(TObject *Sender)
{
	ParentTools->UnGroupCurrent();
}
//---------------------------------------------------------------------------

