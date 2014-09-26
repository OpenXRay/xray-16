#include "stdafx.h"
#pragma hdrstop

#include "NumericVector.h"
//#include "ui_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmNumericVector *frmNumericVector=0;

bool NumericVectorRun(const char* title, Fvector* data, int decimal, Fvector* reset_value, Fvector* min, Fvector* max, int* X, int* Y){
	frmNumericVector = xr_new<TfrmNumericVector>((TComponent*)0);
    bool res = frmNumericVector->Run(title, data, decimal, reset_value, min, max, X, Y);
    xr_delete(frmNumericVector);
    return res;
}

//---------------------------------------------------------------------------
__fastcall TfrmNumericVector::TfrmNumericVector(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TfrmNumericVector::FormKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (Key==VK_ESCAPE) ebCancel->Click();
    if (Key==VK_RETURN) ebOk->Click();
}

//----------------------------------------------------
void __fastcall TfrmNumericVector::FormShow(TObject *Sender)
{
    ebOk->Enabled       = false;
	// check window position
	CheckWindowPos		(this);
}
//---------------------------------------------------------------------------

void __fastcall TfrmNumericVector::OnModified(TObject *Sender)
{
    ebOk->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmNumericVector::ebOkClick(TObject *Sender)
{
	edit_data->x = seX->Value;
	edit_data->y = seY->Value;
	edit_data->z = seZ->Value;

    Close();
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------

void __fastcall TfrmNumericVector::ebCancelClick(TObject *Sender)
{
    Close();
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------

//----------------------------------------------------
bool __fastcall TfrmNumericVector::Run(const char* title, Fvector* data, int decimal, Fvector* rv, Fvector* MN, Fvector* MX, int* X, int* Y)
{
	frmNumericVector->Caption = title;

	VERIFY(data);
    edit_data = data;

    reset_value = rv;
	ebReset->Enabled = !!reset_value;

    seX->Decimal = decimal;
    seY->Decimal = decimal;
    seZ->Decimal = decimal;

	seX->Value = data->x;
    seY->Value = data->y;
	seZ->Value = data->z;

    if (MN){
	    seX->MinValue = MN->x;
    	seY->MinValue = MN->y;
	    seZ->MinValue = MN->z;
    }else{
	    seX->MinValue = 0;
    	seY->MinValue = 0;
	    seZ->MinValue = 0;
    }
    if (MX){
	    seX->MaxValue = MX->x;
    	seY->MaxValue = MX->y;
	    seZ->MaxValue = MX->z;
    }else{
	    seX->MaxValue = 0;
    	seY->MaxValue = 0;
	    seZ->MaxValue = 0;
    }

    if (!X||!Y){
	    POINT pt;
    	GetCursorPos(&pt);
        int w=GetSystemMetrics(SM_CXSCREEN);
        int h=GetSystemMetrics(SM_CYSCREEN);
	    Left = pt.x-(Width*0.5f);
    	Top = pt.y;
        if (((Left+Width*0.5f)>w)||((Top+Height)>h)){ Left=w*0.5f-Width*0.5f; Top=h*0.5f; }
    }else{
	    Left = *X-(Width*0.5f);
    	Top = *Y;
    }

    return (ShowModal()==mrOk);
}

void __fastcall TfrmNumericVector::ebResetClick(TObject *Sender)
{
	seX->Value = reset_value->x;
    seY->Value = reset_value->y;
	seZ->Value = reset_value->z;
}
//---------------------------------------------------------------------------




