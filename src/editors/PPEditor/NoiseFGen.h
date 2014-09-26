//---------------------------------------------------------------------------

#ifndef NoiseFGenH
#define NoiseFGenH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Mask.hpp>
//---------------------------------------------------------------------------
class TNFGen : public TForm
{
__published:	// IDE-managed Components
    TMxLabel *RxLabel1;
    TMxLabel *RxLabel2;
    TMxLabel *RxLabel3;
    TMxLabel *RxLabel4;
    TMultiObjSpinEdit *First;
    TMultiObjSpinEdit *Period;
    TMultiObjSpinEdit *Second;
    TMultiObjSpinEdit *Time;
    TBevel *Bevel1;
    TButton *Button1;
    TButton *Button2;
    void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TNFGen(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TNFGen *NFGen;
//---------------------------------------------------------------------------
#endif
