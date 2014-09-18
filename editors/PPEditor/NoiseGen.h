//---------------------------------------------------------------------------

#ifndef NoiseGenH
#define NoiseGenH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Mask.hpp>
#include <Dialogs.hpp>
//---------------------------------------------------------------------------
class TNGen : public TForm
{
__published:	// IDE-managed Components
    TGroupBox *GroupBox1;
    TMultiObjSpinEdit *FR;
    TMultiObjSpinEdit *FG;
    TMultiObjSpinEdit *FB;
    TMxLabel *RxLabel1;
    TMxLabel *RxLabel2;
    TMxLabel *RxLabel3;
    TPanel *FColor;
    TGroupBox *GroupBox2;
    TMxLabel *RxLabel4;
    TMxLabel *RxLabel5;
    TMxLabel *RxLabel6;
    TMultiObjSpinEdit *SR;
    TMultiObjSpinEdit *SG;
    TMultiObjSpinEdit *SB;
    TPanel *SColor;
    TMxLabel *RxLabel7;
    TMxLabel *RxLabel8;
    TMultiObjSpinEdit *Period;
    TMultiObjSpinEdit *Time;
    TButton *Button1;
    TButton *Button2;
    TBevel *Bevel1;
    TColorDialog *ColorDialog;
    void __fastcall FColorClick(TObject *Sender);
    void __fastcall SColorClick(TObject *Sender);
    void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TNGen(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TNGen *NGen;
//---------------------------------------------------------------------------
#endif
