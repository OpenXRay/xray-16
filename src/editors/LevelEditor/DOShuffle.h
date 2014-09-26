//---------------------------------------------------------------------------

#ifndef DOShuffleH
#define DOShuffleH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ImgList.hpp>

#include "../ECore/Editor/Library.h"
#include <Dialogs.hpp>
#include "ElTree.hpp"
#include "ElXPThemedControl.hpp"
#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
#include "mxPlacemnt.hpp"
//---------------------------------------------------------------------------
// refs
class TfrmOneColor;
class EImageThumbnail;
class EDetailManager;

class TfrmDOShuffle : public TForm
{
friend class TfrmOneColor;
__published:	// IDE-managed Components
	TPanel *paTools;
	TFormStorage *fsStorage;
	TPanel *Panel3;
	TScrollBox *sbDO;
	TExtBtn *ebAppendIndex;
	TExtBtn *ebMultiClear;
	TPanel *Panel2;
	TElTree *tvItems;
	TPanel *paObject;
	TPanel *Panel1;
	TExtBtn *ebAddObject;
	TExtBtn *ebDelObject;
	TExtBtn *ebLoadList;
	TExtBtn *ebSaveList;
	TExtBtn *ebClearList;
	TPanel *paObjectProps;
	TSplitter *Splitter1;
	TMxPanel *paImage;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall paImagePaint(TObject *Sender);
	void __fastcall tvItemsKeyPress(TObject *Sender, char &Key);
	void __fastcall tvMultiDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall tvMultiDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall tvMultiStartDrag(TObject *Sender,
          TDragObject *&DragObject);
	void __fastcall ebAddObjectClick(TObject *Sender);
	void __fastcall ebDelObjectClick(TObject *Sender);
	void __fastcall ebAppendIndexClick(TObject *Sender);
	void __fastcall ebMultiClearClick(TObject *Sender);
	void __fastcall tvItemsDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall tvItemsDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall tvItemsStartDrag(TObject *Sender,
          TDragObject *&DragObject);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall tvItemsItemFocused(TObject *Sender);
	void __fastcall ebSaveListClick(TObject *Sender);
	void __fastcall ebLoadListClick(TObject *Sender);
	void __fastcall ebClearListClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
private:	// User declarations
	static TfrmDOShuffle* form;
    TElTreeItem* FDragItem;

    EImageThumbnail* m_Thm;
	EDetailManager* DM;

    TProperties* m_ObjectProps;
    void __stdcall  OnObjectPropsModified();
    
    xr_vector<TfrmOneColor*> color_indices;

    void InitItemsList(const char* nm=0);
	TElTreeItem* FindItem(const char* s);
    TElTreeItem* AddItem(TElTreeItem* node, const char* name, void* obj=(void*)1);

    void FillData		(); 
    bool ApplyChanges	();

    bool bColorIndModif;
    bool bObjectModif;

    void ClearIndexForms(); 

    bool bTHMLockRepaint;
    bool bLockFocused;
public:		
	void OnItemFocused	(TElTree* tv);
public:		// User declarations
    __fastcall TfrmDOShuffle(TComponent* Owner, EDetailManager* dm_tools);
// static function
    static bool __fastcall Run ();
    static bool __fastcall Visible(){return !!form;}
	static void __fastcall RemoveColorIndex(TfrmOneColor* p);
	static TfrmDOShuffle* __fastcall Form(){return form;}
};
//---------------------------------------------------------------------------
/*
struct SDOData{
    AnsiString	m_RefName;
    float		m_fMinScale;
    float		m_fMaxScale;
    float 		m_fDensityFactor;
    Flags32		m_Flags;
    SDOData		();
};
DEFINE_VECTOR(SDOData*,DDVec,DDIt);
*/
//---------------------------------------------------------------------------
#endif
