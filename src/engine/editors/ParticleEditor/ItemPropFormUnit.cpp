//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ItemPropFormUnit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mxPlacemnt"
#pragma resource "*.dfm"
TfmItemProp *fmItemProp;
//---------------------------------------------------------------------------
__fastcall TfmItemProp::TfmItemProp(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TfmItemProp::FormCloseQuery(TObject *Sender,
      bool &CanClose)
{
	CanClose = false;
}
//---------------------------------------------------------------------------
void __fastcall TfmItemProp::FormClose(TObject *Sender,
      TCloseAction &Action)
{
	Action = caMinimize;
}
//---------------------------------------------------------------------------
