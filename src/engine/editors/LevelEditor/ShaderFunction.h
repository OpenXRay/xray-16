//---------------------------------------------------------------------------

#ifndef ShaderFunctionH
#define ShaderFunctionH
//---------------------------------------------------------------------------
#include "ExtBtn.hpp"
#include "multi_edit.hpp"
#include "MxMenus.hpp"
#include "mxPlacemnt.hpp"
#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
#include <StdCtrls.hpp>

// refs
struct xr_token;
struct WaveForm;

class TfrmShaderFunction : public TForm
{
__published:	// IDE-managed Components
    TPanel *Panel1;
	TStaticText *stFunction;
	TLabel *RxLabel1;
	TLabel *Label4;
	TMultiObjSpinEdit *seArg1;
	TLabel *Label1;
	TMultiObjSpinEdit *seArg2;
	TLabel *Label2;
	TMultiObjSpinEdit *seArg3;
	TLabel *Label3;
	TMultiObjSpinEdit *seArg4;
	TBevel *Bevel2;
	TImage *imDraw;
	TLabel *Label5;
	TLabel *lbMax;
	TLabel *lbCenter;
	TLabel *lbMin;
	TLabel *lbStart;
	TLabel *lbEnd;
	TExtBtn *ebOk;
	TExtBtn *ebCancel;
	TMxPopupMenu *pmFunction;
	TFormStorage *fsStorage;
	TMultiObjSpinEdit *seScale;
	TLabel *Label6;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall ebCancelClick(TObject *Sender);
    void __fastcall ebOkClick(TObject *Sender);
	void __fastcall stFunctionMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall stFunctionClick(TObject *Sender);
	void __fastcall seArgExit(TObject *Sender);
	void __fastcall seArgLWChange(TObject *Sender, int Val);
	void __fastcall seArgKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall FormShow(TObject *Sender);
private:	// User declarations
    bool bLoadMode;

	static WaveForm* m_CurFunc;
    static WaveForm* m_SaveFunc;

    void GetFuncData();
    void UpdateFuncData();

	void __fastcall DrawGraph();

    static TfrmShaderFunction *form;
public:		// User declarations
    __fastcall TfrmShaderFunction(TComponent* Owner);
    static int __fastcall Run(WaveForm* func);
};

typedef void __fastcall (__closure *ClickEvent)(System::TObject* Sender);
AnsiString& GetTokenNameFromVal_EQ	(u32 val, AnsiString& res, const xr_token *token_list);
u32 		GetTokenValFromName		(AnsiString& res, const xr_token *token_list);
void 		FillMenuFromToken		(TMxPopupMenu* menu, const xr_token *token_list, ClickEvent func);

//---------------------------------------------------------------------------
#endif
