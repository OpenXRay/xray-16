//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "frmSearch.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TSearchForm *SearchForm;
//---------------------------------------------------------------------------
__fastcall TSearchForm::TSearchForm(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TSearchForm::CloseBtnClick(TObject *Sender)
{
   SearchForm->Close();
}
//---------------------------------------------------------------------------

