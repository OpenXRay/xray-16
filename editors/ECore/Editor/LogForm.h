//---------------------------------------------------------------------------
#ifndef LogFormH
#define LogFormH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include <StdCtrls.hpp>

#include "ExtBtn.hpp"
#include "mxPlacemnt.hpp"
#include "MxMenus.hpp"
#include <Menus.hpp>

class ECORE_API TfrmLog : public TForm
{
__published:	// IDE-managed Components
    TPanel *Panel1;
	TPanel *Panel2;
	TListBox *lbLog;
	TExtBtn *ebClear;
	TExtBtn *ebClearSelected;
	TExtBtn *ebClose;
	TFormStorage *fsStorage;
	TExtBtn *ebFlush;
	TMxPopupMenu *MxPopupMenu1;
	TMenuItem *imCopy;
	TMenuItem *imSelectAll;
	TMenuItem *ClearAll1;
	TMenuItem *ClearSelected1;
	TMenuItem *N1;
	void __fastcall ebClearClick(TObject *Sender);
	void __fastcall lbLogDrawItem(TWinControl *Control, int Index,
          TRect &Rect, TOwnerDrawState State);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall ebClearSelectedClick(TObject *Sender);
	void __fastcall ebCloseClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall ebFlushClick(TObject *Sender);
	void __fastcall lbLogKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall imCopyClick(TObject *Sender);
	void __fastcall imSelectAllClick(TObject *Sender);
	void __fastcall lbLogKeyPress(TObject *Sender, char &Key);
private:	// User declarations
	static TfrmLog *form;
public:		// User declarations
    __fastcall TfrmLog(TComponent* Owner);
    static void __fastcall AddMessage	(TMsgDlgType mt, const AnsiString& msg);
    static void __fastcall AddMessage	(const AnsiString& msg){AddMessage(mtCustom,msg);}
    static void __fastcall AddDlgMessage(TMsgDlgType mt, const AnsiString& msg);
    static void __fastcall AddDlgMessage(const AnsiString& msg){AddDlgMessage(mtCustom,msg);}
    static void __fastcall ShowLog		();
    static void __fastcall HideLog		();
    static void __fastcall CreateLog	();
    static void __fastcall DestroyLog	();
    static void __fastcall ChangeVisible(){if (form->Visible) HideLog(); else ShowLog();}
    static bool __fastcall IsVisible	(){return form->Visible;}
};
//---------------------------------------------------------------------------
#endif
