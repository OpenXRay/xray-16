//---------------------------------------------------------------------------

#ifndef newdialogH
#define newdialogH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Mask.hpp>
//---------------------------------------------------------------------------
class TNewEffectDialog : public TForm
{
__published:	// IDE-managed Components
    TMultiObjSpinEdit *Time;
    TMxLabel *RxLabel1;
    TButton *Button1;
    TButton *Button2;
    void __fastcall TimeChange(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TNewEffectDialog(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TNewEffectDialog *NewEffectDialog;
//---------------------------------------------------------------------------
#endif
