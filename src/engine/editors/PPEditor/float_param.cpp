#include "stdafx.h"
#pragma hdrstop

#include "float_param.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "multi_edit"
#pragma link "MXCtrls"
#pragma resource "*.dfm"
TfrmTimeConstructor *frmTimeConstructor;
//---------------------------------------------------------------------------
__fastcall TfrmTimeConstructor::TfrmTimeConstructor(TComponent* Owner)
    : TForm(Owner)
{
    ClientHeight = Panel1->Height;
    
}
//---------------------------------------------------------------------------

