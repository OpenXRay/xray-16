//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "newdialog.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TNewEffectDialog *NewEffectDialog;
//---------------------------------------------------------------------------
__fastcall TNewEffectDialog::TNewEffectDialog(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TNewEffectDialog::TimeChange(TObject *Sender)
{
    if (Time->Value > 0.0f)
       Button1->Enabled = true;
    else
       Button1->Enabled = false;
}
//---------------------------------------------------------------------------
