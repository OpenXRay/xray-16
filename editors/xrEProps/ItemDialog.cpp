#include "stdafx.h"
#pragma hdrstop

#include "ItemDialog.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mxPlacemnt"
#pragma resource "*.dfm"
TfrmItemDialog* TfrmItemDialog::form=0;
int 			TfrmItemDialog::res=-1;

//---------------------------------------------------------------------------
__fastcall TfrmItemDialog::TfrmItemDialog(TComponent* Owner)
    : TForm(Owner)
{
	res		= -1;
}
//---------------------------------------------------------------------------

void __fastcall TfrmItemDialog::FormKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (Key==VK_ESCAPE) ModalResult = -1;
}

//----------------------------------------------------

void __fastcall TfrmItemDialog::ebClick(TObject *Sender)
{
	TExtBtn* E 	= dynamic_cast<TExtBtn*>(Sender); VERIFY(E);
    res 		= E->Tag;
    Close		();
}
//---------------------------------------------------------------------------

int __fastcall TfrmItemDialog::Run(LPCSTR caption, LPCSTR msg, LPCSTR buttons)
{
	R_ASSERT(buttons&&buttons[0]);
	form = xr_new<TfrmItemDialog>((TComponent*)0);
	form->Caption 		= caption;
    form->lbMsg->Caption= msg;
    int cnt				= _GetItemCount(buttons); R_ASSERT(cnt<9);
    AnsiString 			nm;
    for (int k=0; k<cnt; k++){
    	_GetItem		(buttons,k,nm);
        TExtBtn* E		= dynamic_cast<TExtBtn*>(form->FindComponent(AnsiString().sprintf("eb%d",k))); VERIFY(E);
        E->Caption		= nm;
        E->Visible		= true;
        E->Tag			= k;
    }
    form->ShowModal		();
    return res;
}
//---------------------------------------------------------------------------

void __fastcall TfrmItemDialog::FormClose(TObject *Sender,
      TCloseAction &Action)
{
	Action = caFree;	
}
//---------------------------------------------------------------------------


