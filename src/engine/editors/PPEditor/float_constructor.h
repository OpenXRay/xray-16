//---------------------------------------------------------------------------

#ifndef float_constructorH
#define float_constructorH
//---------------------------------------------------------------------------
#include <xrCore.h>
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <stdio.h>

#include "float_param.h"
#include <Dialogs.hpp>
#include <ExtCtrls.hpp>
#include <Mask.hpp>

using namespace std;
//---------------------------------------------------------------------------
class TfrmConstructor : public TForm
{
__published:	// IDE-managed Components
    TButton *Button1;
    TButton *Button2;
private:	// User declarations
    int     							m_iTag;
    bool								b_locked;
public:		// User declarations
    vector<TfrmTimeConstructor*>        m_Entries;
    						__fastcall 	TfrmConstructor		(TComponent* Owner);
    TfrmTimeConstructor*    			AddEntryTemplate    (int iInsertAfter);
    void    				__fastcall  OnTimeChangeClick   	(TObject *Sender);
    void    				__fastcall  OnAddButtonClick   	(TObject *Sender);
    void    				__fastcall  OnDelButtonClick   	(TObject *Sender);
    void    				__fastcall  Reset              	();
    TfrmTimeConstructor* 	__fastcall  GetEntry           	(u32 index);
    void    							UpdatePositions      ();
    void								Lock				(bool b){b_locked=b;}
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmConstructor *frmConstructor;
//---------------------------------------------------------------------------
#endif
