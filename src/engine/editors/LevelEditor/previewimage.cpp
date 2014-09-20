#include "stdafx.h"
#pragma hdrstop

#include "PreviewImage.h"
#include "../ECore/Engine/Texture.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TfrmPreviewImage* TfrmPreviewImage::form = 0;
//---------------------------------------------------------------------------
__fastcall TfrmPreviewImage::TfrmPreviewImage(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TfrmPreviewImage::FormKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (Key==VK_ESCAPE) Close();
}

//----------------------------------------------------
int __fastcall TfrmPreviewImage::Run(){
	form = xr_new<TfrmPreviewImage>((TComponent*)0);

//S
/*
	VERIFY(T);
	form->tex = T;
	form->ClientHeight 	= T->height()+4;
    form->ClientWidth 	= T->width()+4;
    form->mult = 1;
*/
    return form->ShowModal();
}

//----------------------------------------------------
void __fastcall TfrmPreviewImage::FormClose(TObject *Sender,
      TCloseAction &Action)
{
	Action = caFree;
    form = 0;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPreviewImage::pbImagePaint(TObject *Sender)
{
    if (tex){
        int w = paImage->Width-4;
        int h = paImage->Height-4;
        RECT r;
        r.left  = 2; r.top = 2;
        r.right = w+2; r.bottom = h+2;
//S        tex->DrawStretch(paImage->Handle, &r);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPreviewImage::pbImageMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (Shift.Contains(ssShift)){
        if (Button==mbLeft){
            mult *= 2;
        }else if (Button==mbRight){
            mult /= 2;
        }
        if (mult<=0.0625) mult = 0.0625;
        if (mult>=2) mult = 2;
//S        ClientHeight 	= tex->height()*mult+4;
//S        ClientWidth 	= tex->width()*mult+4;
    }
}
//---------------------------------------------------------------------------

