//---------------------------------------------------------------------------

#ifndef NumericVectorH
#define NumericVectorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Dialogs.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
#include "CSPIN.h"
#include "ExtBtn.hpp"
#include "multi_edit.hpp"
#include "MXCtrls.hpp"

class TfrmNumericVector : public TForm
{
__published:	// IDE-managed Components
    TExtBtn *ebOk;
    TExtBtn *ebCancel;
    TPanel *paBottom;
	TExtBtn *ebReset;
	TLabel *RxLabel3;
	TLabel *RxLabel1;
	TLabel *RxLabel2;
	TMultiObjSpinEdit *seX;
	TMultiObjSpinEdit *seY;
	TMultiObjSpinEdit *seZ;
	TBevel *Bevel1;
	TBevel *Bevel2;
	TBevel *Bevel3;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall ebCancelClick(TObject *Sender);
    void __fastcall ebOkClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall OnModified(TObject *Sender);
	void __fastcall ebResetClick(TObject *Sender);
private:	// User declarations
	Fvector* edit_data;
	Fvector* reset_value;
public:		// User declarations
    __fastcall TfrmNumericVector(TComponent* Owner);
    bool __fastcall Run(const char* title, Fvector* data, int decimal=3, Fvector* reset_value=0, Fvector* min=0, Fvector* max=0, int* X=0, int* Y=0);
};
//---------------------------------------------------------------------------
bool XR_EPROPS_API NumericVectorRun(const char* title, Fvector* data, int decimal=3, Fvector* reset_value=0, Fvector* min=0, Fvector* max=0, int* X=0, int* Y=0);
//---------------------------------------------------------------------------
#endif
