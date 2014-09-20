//---------------------------------------------------------------------------
#ifndef PropertiesListH
#define PropertiesListH
//---------------------------------------------------------------------------

#include "ElTree.hpp"
#include "ElTreeStdEditors.hpp"
#include "ElXPThemedControl.hpp"
#include "multi_edit.hpp"
#include "MxMenus.hpp"
#include "mxPlacemnt.hpp"
#include <Classes.hpp>
#include <Controls.hpp>
#include <Menus.hpp>
#include <StdCtrls.hpp>
#include <ExtCtrls.hpp>
#include "ElTreeAdvEdit.hpp"
#include <Mask.hpp>

#include "../../xrServerEntities/PropertiesListHelper.h"
#include "RenderWindow.hpp"
#include "MxShortcut.hpp"
#include "ExtBtn.hpp"

#define TElFString ::TElFString

// refs
class TItemList;

class XR_EPROPS_API TProperties : public TForm
{
__published:	// IDE-managed Components
	TElTree 			*tvProperties;
	TMxPopupMenu 		*pmEnum;
	TFormStorage 		*fsStorage;
	TMultiObjSpinEdit 	*seNumber;
	TBevel 				*Bevel1;
	TMxPopupMenu *pmSystem;
	TMenuItem *ExpandAll1;
	TMenuItem *N1;
	TMenuItem *CollapseAll1;
	TMenuItem *N2;
	TMenuItem *miDrawThumbnails;
	TMxPopupMenu *pmItems;
	TMenuItem *ExpandSelected1;
	TMenuItem *CollapseSelected1;
	TMenuItem *N3;
	TPanel *paButtons;
	TBevel *Bevel2;
	TExtBtn *ebOK;
	TExtBtn *ebCancel;
	TMaskEdit *edText;
	TPanel *paFolders;
	TSplitter *spFolders;
	TElTreeInplaceEdit *ElTreeInplaceEdit1;
	TMenuItem *miAutoExpand;
	TMxHotKey *hkShortcut;
	void __fastcall 	FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall 	tvPropertiesClick(TObject *Sender);
	void __fastcall 	tvPropertiesItemDraw(TObject *Sender, TElTreeItem *Item, TCanvas *Surface, TRect &R, int SectionIndex);
	void __fastcall 	tvPropertiesMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall 	seNumberExit(TObject *Sender);
	void __fastcall 	seNumberKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall 	edTextExit(TObject *Sender);
	void __fastcall 	edTextKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall 	tvPropertiesItemFocused(TObject *Sender);
	void __fastcall 	edTextDblClick(TObject *Sender);
	void __fastcall 	tvPropertiesHeaderColumnResize(TObject *Sender, int SectionIndex);
	void __fastcall 	FormDeactivate(TObject *Sender);
	void __fastcall 	FormShow(TObject *Sender);
	void __fastcall 	tvPropertiesItemChange(TObject *Sender, TElTreeItem *Item, TItemChangeMode ItemChangeMode);
	void __fastcall 	FormDestroy(TObject *Sender);
	void __fastcall 	fsStorageRestorePlacement(TObject *Sender);
	void __fastcall 	fsStorageSavePlacement(TObject *Sender);
	void __fastcall 	CollapseAll1Click(TObject *Sender);
	void __fastcall 	ExpandAll1Click(TObject *Sender);
	void __fastcall 	miDrawThumbnailsClick(TObject *Sender);
	void __fastcall 	tvPropertiesMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall 	tvPropertiesMouseMove(TObject *Sender, TShiftState Shift, int X, int Y);
	void __fastcall ExpandSelected1Click(TObject *Sender);
	void __fastcall CollapseSelected1Click(TObject *Sender);
	void __fastcall ebOKClick(TObject *Sender);
	void __fastcall ebCancelClick(TObject *Sender);
	void __fastcall tvPropertiesShowLineHint(TObject *Sender,
          TElTreeItem *Item, TElHeaderSection *Section, TElFString &Text,
          THintWindow *HintWindow, TPoint &MousePos, bool &DoShowHint);
	void __fastcall tvPropertiesCompareItems(TObject *Sender,
          TElTreeItem *Item1, TElTreeItem *Item2, int &res);
	void __fastcall miAutoExpandClick(TObject *Sender);
	void __fastcall hkShortcut_KeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall hkShortcut_Exit(TObject *Sender);
private:	// User declarations
    void __fastcall 	PMItemClick		(TObject *Sender);
	void __fastcall 	WaveFormClick	(TElTreeItem* item);
	void __fastcall 	ColorClick		(TElTreeItem* item);
	void __fastcall 	VectorClick		(TElTreeItem* item);
	void __fastcall 	ChooseClick		(TElTreeItem* item);
	void __fastcall 	GameTypeClick	(TElTreeItem* item);
	TElTreeItem* 		m_FirstClickItem;

	Graphics::TBitmap* 	m_BMCheck;
	Graphics::TBitmap* 	m_BMDot;
	Graphics::TBitmap* 	m_BMEllipsis;
    bool 				bModified;
    AnsiString 			last_selected_item;
	// LW style inpl editor
    void 				HideLWNumber	();
    void 				PrepareLWNumber(TElTreeItem* node);
    void 				ShowLWNumber	(TRect& R);
    void 				ApplyLWNumber	();
    void 				CancelLWNumber	();
	// text inplace editor
    void 				ExecTextEditor	(PropItem* prop);
    void 				HideLWText		();
    void 				PrepareLWText	(TElTreeItem* node);
    void 				ShowLWText		(TRect& R);
    void 				ApplyLWText		();
    void 				CancelLWText	();
    // shortcut
    void 				HideSCText		();
    void 				PrepareSCText	(TElTreeItem* node);
    void 				ShowSCText		(TRect& R);
    void 				ApplySCText		();
    void 				CancelSCText	();

    PropItemVec 		m_Items;
    PropItemVec 		m_ViewItems;
	void 				FillElItems		(PropItemVec& items, LPCSTR startup_pref=0);
    
    typedef 			fastdelegate::FastDelegate1<TElTreeItem*> TOnItemFocused;    
    TOnItemFocused      OnItemFocused;
    TOnModifiedEvent 	OnModifiedEvent;
    TOnCloseEvent		OnCloseEvent;
    void 				ClearParams				(TElTreeItem* node=0);
    void 				ApplyEditControl		();
    void 				CancelEditControl		();

	void 				OutBOOL					(BOOL val, TCanvas* Surface, TRect& R, bool bEnable);
	void 				OutText					(LPCSTR text, TCanvas* Surface, TRect& R, bool bEnable, TGraphic* g=0, bool bArrow=false);

public:
    void 				Modified				(){bModified=true; if (!OnModifiedEvent.empty()) OnModifiedEvent();}
public:
	enum{
        plFolderStore	= (1<<0),
        plItemFolders	= (1<<1),
        plFullExpand	= (1<<2),
        plFullSort		= (1<<3), 
        plIFTop			= (1<<4),
        plNoClearStore	= (1<<5),
        plReadOnly		= (1<<6),
        plMultiSelect	= (1<<7),
    };
protected:
    Flags32				m_Flags;

	// RT store
    struct SFolderStore{
    	bool expand;
    };
    DEFINE_MAP(AnsiString,SFolderStore,FolderStoreMap,FolderStorePairIt);
    FolderStoreMap		FolderStorage;
    void				FolderStore				();
    void				FolderRestore			();

    TItemList*			m_Folders;
    void 	__stdcall  	OnFolderFocused			(TElTreeItem* item);
public:		// User declarations
	__fastcall TProperties		        		(TComponent* Owner);
	static TProperties* CreateForm				(const AnsiString& title, TWinControl* parent=0, TAlign align=alNone, TOnModifiedEvent modif=0, TOnItemFocused focused=0, TOnCloseEvent close=0, u32 flags=plFolderStore|plFullExpand);
	static TProperties* CreateModalForm			(const AnsiString& title, bool bShowButtonsBar=true, TOnModifiedEvent modif=0, TOnItemFocused focused=0, TOnCloseEvent close=0, u32 flags=plFolderStore|plFullExpand);
	static void 		DestroyForm				(TProperties*& props);
    static int 			EditPropertiesModal		(PropItemVec& values, LPCSTR title, bool bShowButtonsBar=true, TOnModifiedEvent modif=0, TOnItemFocused focused=0, TOnCloseEvent close=0, u32 flags=plFolderStore|plFullExpand);
    int __fastcall 		ShowPropertiesModal		();
    void __fastcall 	ShowProperties			();
    void __fastcall 	HideProperties			();
    void __fastcall 	ClearProperties			();
    bool __fastcall 	IsModified				();
    void __fastcall 	ResetModified			(){bModified = false;}
    void __fastcall 	RefreshForm				();

    void __fastcall		SelectFolder			(const AnsiString& folder_name);
    void __fastcall		SelectItem				(const AnsiString& full_name);
    void __fastcall 	AssignItems				(PropItemVec& values);
    void __fastcall 	ResetItems				();
    bool __fastcall 	IsFocused				(){return tvProperties->Focused()||seNumber->Focused()||edText->Focused();}
    void __fastcall 	SetModifiedEvent		(TOnModifiedEvent modif=0){OnModifiedEvent=modif;}
    void __fastcall 	GetColumnWidth			(int& c0, int& c1)
    {
    	c0=tvProperties->HeaderSections->Item[0]->Width;
	    c1=tvProperties->HeaderSections->Item[1]->Width;
	}
    void __fastcall 	SetColumnWidth			(int c0, int c1)
    {
    	tvProperties->HeaderSections->Item[0]->Width=c0;
	    tvProperties->HeaderSections->Item[1]->Width=c1;
	}
    void __fastcall 	SaveParams				(TFormStorage* fs)
    {
		fs->WriteInteger(AnsiString().sprintf("%s_column0_width",Caption.c_str()),tvProperties->HeaderSections->Item[0]->Width);
		fs->WriteInteger(AnsiString().sprintf("%s_column1_width",Caption.c_str()),tvProperties->HeaderSections->Item[1]->Width);
		fs->WriteInteger(AnsiString().sprintf("%s_draw_thm",Caption.c_str()),miDrawThumbnails->Checked);
		fs->WriteInteger(AnsiString().sprintf("%s_auto_expand",Caption.c_str()),miAutoExpand->Checked);
		fs->WriteInteger(AnsiString().sprintf("%s_fp_width",Caption.c_str()),paFolders->Width);
		fs->WriteInteger(AnsiString().sprintf("%s_fp_height",Caption.c_str()),paFolders->Height);
    }
    void __fastcall 	RestoreParams			(TFormStorage* fs)
    {                                      	
		tvProperties->HeaderSections->Item[0]->Width 	= fs->ReadInteger(AnsiString().sprintf("%s_column0_width",Caption.c_str()),tvProperties->HeaderSections->Item[0]->Width);
		tvProperties->HeaderSections->Item[1]->Width 	= fs->ReadInteger(AnsiString().sprintf("%s_column1_width",Caption.c_str()),tvProperties->HeaderSections->Item[1]->Width);
        miDrawThumbnails->Checked						= fs->ReadInteger(AnsiString().sprintf("%s_draw_thm",Caption.c_str()),false);
        miAutoExpand->Checked							= fs->ReadInteger(AnsiString().sprintf("%s_auto_expand",Caption.c_str()),false);
		paFolders->Width								= fs->ReadInteger(AnsiString().sprintf("%s_fp_width",Caption.c_str()),paFolders->Width);
		paFolders->Height								= fs->ReadInteger(AnsiString().sprintf("%s_fp_height",Caption.c_str()),paFolders->Height);
        RefreshForm			();
    }

    void 				LockUpdating			(){ tvProperties->IsUpdating = true; }
    void 				UnlockUpdating			(){ tvProperties->IsUpdating = false; }

    // auxiliary routines
	static IC LPVOID	GetItemData				(TElTreeItem* item){return (void*)item->Tag;}
	static IC bool 		IsItemType				(TElTreeItem* item, EPropType type){return item->Tag&&(((PropItem*)item->Tag)->Type()==type);}
	static IC EPropType GetItemType				(TElTreeItem* item){return item->Tag?((PropItem*)item->Tag)->Type():(EPropType)0;}
	static IC LPCSTR	GetItemColumn			(TElTreeItem* item, int col){
    	static AnsiString t;
        if (col<item->ColumnText->Count) t=item->ColumnText->Strings[col];
        return t.c_str();
    }

    void				SetReadOnly				(BOOL val){m_Flags.set(plReadOnly,val);}
    PropItem*			FindItem 				(const shared_str& name);
};
//---------------------------------------------------------------------------
#endif
