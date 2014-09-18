//---------------------------------------------------------------------------
#ifndef TextFormH
#define TextFormH
#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include "multi_check.hpp"
#include <StdCtrls.hpp>
#include "multi_edit.hpp"
//---------------------------------------------------------------------------
#include <Forms.hpp>
#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
#include "mxPlacemnt.hpp"
#include "ElACtrls.hpp"
#include "ElStatBar.hpp"
#include "ElXPThemedControl.hpp"
#include "MxMenus.hpp"
#include <Menus.hpp>

// refs
class CCustomObject;

typedef fastdelegate::FastDelegate1<LPCSTR,bool>							TOnApplyClick;
typedef fastdelegate::FastDelegate0<bool> 									TOnCloseClick;
typedef fastdelegate::FastDelegate3<const AnsiString&, AnsiString&, bool&> 	TOnCodeInsight;

class XR_EPROPS_API TfrmText : public TForm
{
__published:	// IDE-managed Components
	TPanel *paBottomBar;
	TExtBtn *ebOk;
	TExtBtn *ebCancel;
	TFormStorage *fsStorage;
	TExtBtn *ebApply;
	TExtBtn *ebLoad;
	TExtBtn *ebSave;
	TElStatusBar *sbStatusPanel;
	TMxPopupMenu *pmTextMenu;
	TExtBtn *ebClear;
	TTimer *tmIdle;
	TMemo *mmText;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall ebCancelClick(TObject *Sender);
    void __fastcall ebOkClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
	void __fastcall mmTextChange(TObject *Sender);
	void __fastcall ebApplyClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall ebLoadClick(TObject *Sender);
	void __fastcall ebSaveClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall mmTextKeyUp(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall ebClearClick(TObject *Sender);
	void __fastcall tmIdleTimer(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall FormDeactivate(TObject *Sender);
private:	// User declarations
	AnsiString* 	m_Text;
    TOnApplyClick 	OnApplyClick;
    TOnCloseClick 	OnCloseClick;
    TOnCodeInsight  OnCodeInsight;
    void			OutLineNumber();
public:		// User declarations
	enum{
    	flReadOnly 	= (1<<0),
    	flOurPPMenu	= (1<<1),
    };
public:
    __fastcall TfrmText(TComponent* Owner);
    static TfrmText* 	__fastcall CreateForm		(AnsiString& text, LPCSTR caption="Text", u32 flags=0, int lim=0, LPCSTR apply_name="Apply", TOnApplyClick on_apply=0, TOnCloseClick on_close=0, TOnCodeInsight on_insight=0);
    static bool		 	__fastcall RunEditor		(AnsiString& text, LPCSTR caption="Text", u32 flags=0, int lim=0, LPCSTR apply_name="Apply", TOnApplyClick on_apply=0, TOnCloseClick on_close=0, TOnCodeInsight on_insight=0);
    static void 		__fastcall DestroyForm		(TfrmText* form);
    bool 	Modified	(){return mmText->Modified;}
    void 	ApplyEdit	(){ebApplyClick(0);}
    void 	InsertLine	(const AnsiString& line);
    void 	InsertTextCP(const AnsiString& line, bool bCommas=true);
    void	SetText		(AnsiString& text);
};
//---------------------------------------------------------------------------
#endif
