//---------------------------------------------------------------------------

#ifndef Unit8H
#define Unit8H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <stdio.h>

#include "Unit9.h"
#include <Dialogs.hpp>
#include <ExtCtrls.hpp>
#include <Mask.hpp>

#include <vector>

using namespace std;
//---------------------------------------------------------------------------
class TForm8 : public TForm
{
__published:	// IDE-managed Components
    TButton *Button1;
    TButton *Button2;
    TOpenDialog *OpenDialog;
    TSaveDialog *SaveDialog;
    TPanel *Panel2;
    TLabel *Label1;
    TMultiObjSpinEdit *InitValue;
private:	// User declarations
    void    AddEntryTemplate    (int iInsertAfter);
    void    RecalcSize          ();
    void    ResetPositions      ();
    int     m_iTag;
public:		// User declarations
    vector<TForm9*>         m_Entries;
    __fastcall TForm8(TComponent* Owner);
    void    __fastcall  OnAddButtonClick   (TObject *Sender);
    void    __fastcall  OnDelButtonClick   (TObject *Sender);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm8 *Form8;
//---------------------------------------------------------------------------
#endif
