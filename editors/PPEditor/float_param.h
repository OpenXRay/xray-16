//---------------------------------------------------------------------------

#ifndef float_paramH
#define float_paramH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Mask.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <ToolWin.hpp>
#include <ImgList.hpp>
#include "multi_edit.hpp"
#include "MXCtrls.hpp"
//---------------------------------------------------------------------------
class TfrmTimeConstructor : public TForm
{
__published:	// IDE-managed Components
    TPanel *Panel1;
    TMultiObjSpinEdit *WorkTime;
    TSpeedButton *AddButton;
    TSpeedButton *DeleteButton;
	TMxLabel *RxLabel2;
	TMxLabel *MxLabel1;
	TMultiObjSpinEdit *StartTime;
private:	// User declarations
public:		// User declarations
    float   t, c, b;
    __fastcall TfrmTimeConstructor(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmTimeConstructor *frmTimeConstructor;
//---------------------------------------------------------------------------
#endif
