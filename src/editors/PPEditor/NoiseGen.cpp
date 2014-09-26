//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "NoiseGen.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TNGen *NGen;
//---------------------------------------------------------------------------
__fastcall TNGen::TNGen(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TNGen::FColorClick(TObject *Sender)
{
    if (ColorDialog->Execute () == false) return;
    FColor->Color = ColorDialog->Color;
    FR->Value = (float)(ColorDialog->Color & 0x0000ff) / 255.0f;
    FG->Value = (float)((ColorDialog->Color & 0x00ff00) >> 8) / 255.0f;
    FB->Value = (float)((ColorDialog->Color & 0xff0000) >> 16) / 255.0f;
}
//---------------------------------------------------------------------------
void __fastcall TNGen::SColorClick(TObject *Sender)
{
    if (ColorDialog->Execute () == false) return;
    SColor->Color = ColorDialog->Color;
    SR->Value = (float)(ColorDialog->Color & 0x0000ff) / 255.0f;
    SG->Value = (float)((ColorDialog->Color & 0x00ff00) >> 8) / 255.0f;
    SB->Value = (float)((ColorDialog->Color & 0xff0000) >> 16) / 255.0f;
}
//---------------------------------------------------------------------------
void __fastcall TNGen::Button1Click(TObject *Sender)
{
    if (Period->Value <= 0.0f || Time->Value <= 0.0f)
       {
       Application->MessageBox ("Invalid period or time for effect", "Error", MB_OK | MB_ICONSTOP);
       return;
       }
    if (Period->Value >= Time->Value)
       {
       Application->MessageBox ("Period must be less than time effect", "Error", MB_OK | MB_ICONSTOP);
       return;
       }
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
