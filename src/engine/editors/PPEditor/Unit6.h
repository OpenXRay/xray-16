//---------------------------------------------------------------------------

#ifndef Unit6H
#define Unit6H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Dialogs.hpp>
#include <stdio.h>

#include "Unit7.h"
#include <ExtCtrls.hpp>

#include <vector>

using namespace std;

//---------------------------------------------------------------------------
class TForm6 : public TForm
{
__published:	// IDE-managed Components
    TButton *Button1;
    TButton *Button2;
    TColorDialog *ColorDialog;
    TOpenDialog *OpenDialog;
    TSaveDialog *SaveDialog;
    TPanel *Panel2;
    TLabel *Label1;
    TPanel *Panel3;
    void __fastcall Panel3Click(TObject *Sender);
private:	// User declarations
    void    RecalcSize          ();
    void    ResetPositions      ();
    int     m_iTag;
public:		// User declarations
    void    AddEntryTemplate    (int iInsertAfter);
    TColor                  m_InitColor;
    vector<TForm7*>         m_Entries;
    __fastcall TForm6(TComponent* Owner);
//    __fastcall TForm6 (HWND handle);
    void    __fastcall  OnAddButtonClick   (TObject *Sender);
    void    __fastcall  OnDelButtonClick   (TObject *Sender);
    void    __fastcall  SaveData           (FILE *file);
    void    __fastcall  LoadData           (FILE *file);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm6 *Form6;
//---------------------------------------------------------------------------
#endif
