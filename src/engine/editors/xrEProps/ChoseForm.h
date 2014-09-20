//---------------------------------------------------------------------------

#ifndef ChoseFormH
#define ChoseFormH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ImgList.hpp>

#include <Dialogs.hpp>
#include "ElTree.hpp"
#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
#include "mxPlacemnt.hpp"
#include "ElXPThemedControl.hpp"
#include "Gradient.hpp"
#include "ChooseTypes.H"     

//---------------------------------------------------------------------------
// refs
class TProperties;

class XR_EPROPS_API TfrmChoseItem : public TForm
{
__published:	// IDE-managed Components
	TPanel *paRight;
	TFormStorage *fsStorage;
	TPanel *paMulti;
	TElTree *tvMulti;
	TExtBtn *ebMultiUp;
	TExtBtn *ebMultiDown;
	TExtBtn *ebMultiRemove;
	TExtBtn *ebMultiClear;
	TPanel *Panel2;
	TExtBtn *sbSelect;
	TExtBtn *sbCancel;
	TPanel *Panel4;
	TElTree *tvItems;
	TPanel *paFind;
	TGradient *grdFon;
	TLabel *mxLabel2;
	TLabel *lbHint;
	TLabel *lbItemName;
	TLabel *mxLabel1;
	TMxPanel *paImage;
	TExtBtn *ebExt;
	TEdit *edFind;
	TSplitter *Splitter1;
	TSplitter *Splitter2;
	TPanel *paInfo;
    void __fastcall sbSelectClick(TObject *Sender);
    void __fastcall sbCancelClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall tvItemsDblClick(TObject *Sender);
	void __fastcall tvItemsKeyPress(TObject *Sender, char &Key);
	void __fastcall tvItemsItemChange(TObject *Sender, TElTreeItem *Item,
          TItemChangeMode ItemChangeMode);
	void __fastcall ebMultiUpClick(TObject *Sender);
	void __fastcall ebMultiDownClick(TObject *Sender);
	void __fastcall tvMultiDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall tvMultiDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall tvMultiStartDrag(TObject *Sender,
          TDragObject *&DragObject);
	void __fastcall ebMultiRemoveClick(TObject *Sender);
	void __fastcall ebMultiClearClick(TObject *Sender);
	void __fastcall tvItemsItemFocused(TObject *Sender);
	void __fastcall paImagePaint(TObject *Sender);
	void __fastcall ebExtClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall tvItemsCompareItems(TObject *Sender, TElTreeItem *Item1,
          TElTreeItem *Item2, int &res);
	void __fastcall tmRepaintTimer(TObject *Sender);
	void __fastcall edFindChange(TObject *Sender);
private:	// User declarations
	static TfrmChoseItem* form;
    static AnsiString select_item;

    TProperties*   	m_Props;
    void 		   	InitItemsList	(const char* nm=0);

    int 			iMultiSelLimit;
private:
    Flags32			m_Flags;

    SChooseEvents 	E;

    ChooseItemVec	m_Items;
	void __fastcall FillItems	(u32 choose_id);
    void __fastcall AppendItem	(SChooseItem* item, bool b_check_duplicate);
    void 			DrawImage	();
protected:
    static AnsiString 			m_LastSelection; 

    DEFINE_MAP(u32,SChooseEvents,EventsMap,EventsMapIt);     
    static EventsMap			m_Events;
public:
	static TOnChooseFillEvents 	fill_events;
public:		// User declarations
    __fastcall 					TfrmChoseItem	(TComponent* Owner);
	static int	 	__fastcall 	SelectItem		(u32 choose_ID, LPCSTR& dest, int sel_cnt=1, LPCSTR init_name=0, TOnChooseFillItems item_fill=0, void* fill_param=0, TOnChooseSelectItem item_select=0, ChooseItemVec* items=0, u32 flags=cfAllowNone);
    static void __fastcall		OnFrame			();

    static void					AppendEvents	(u32 choose_ID, LPCSTR caption, TOnChooseFillItems on_fill, TOnChooseSelectItem on_sel, TOnDrawThumbnail on_thm, TOnChooseClose on_close, u32 flags);
    static void					ClearEvents		();
    static SChooseEvents*		GetEvents		(u32 choose_ID);
};
//---------------------------------------------------------------------------
#endif
