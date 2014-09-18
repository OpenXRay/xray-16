//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "NoiseFGen.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TNFGen *NFGen;
//---------------------------------------------------------------------------
__fastcall TNFGen::TNFGen(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TNFGen::Button1Click(TObject *Sender)
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
