//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit7.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm7 *Form7;
//---------------------------------------------------------------------------
__fastcall TForm7::TForm7(TComponent* Owner)
    : TForm(Owner)
{
    ColorPanel->Color = (TColor)0;
    ClientHeight = Panel1->Height;
    m_iFunction = 0;
    tr = cr = br = 0.0f;
    tg = cg = bg = 0.0f;
    tb = cb = bb = 0.0f;
}
//---------------------------------------------------------------------------
void __fastcall TForm7::ColorPanelClick(TObject *Sender)
{
    if (ColorDialog->Execute() == false) return;
    ColorPanel->Color = ColorDialog->Color;
}
//---------------------------------------------------------------------------
void __fastcall TForm7::LinearFuncButtonClick(TObject *Sender)
{
    m_iFunction = dynamic_cast<TComponent*> (Sender)->Tag;
}
//---------------------------------------------------------------------------

