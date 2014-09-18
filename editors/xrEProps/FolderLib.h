//---------------------------------------------------------------------------

#ifndef FolderLibH
#define FolderLibH
//---------------------------------------------------------------------------
#include "ElTree.hpp"
#include "ExtBtn.hpp"
#include "MxMenus.hpp"

typedef fastdelegate::FastDelegate2<LPCSTR,bool&> TFindObjectByName;     

class XR_EPROPS_API CFolderHelper{
    IC TElTreeItem*		LL_CreateFolder	(TElTree* tv, TElTreeItem* parent, const AnsiString& name, bool force_icon)
    {
    	TElTreeItem* N	= tv->Items->AddChildObject(parent,name,(void*)TYPE_FOLDER);
        N->ForceButtons	= force_icon;
        return N;
    }
    IC TElTreeItem*		LL_CreateObject	(TElTree* tv, TElTreeItem* parent, const AnsiString& name)
    {
    	TElTreeItem* N	= tv->Items->AddChildObject(parent,name,(void*)TYPE_OBJECT);
        return N;
    }
public:
    IC bool				IsFolder			(TElTreeItem* node){return node?(TYPE_FOLDER==(u32)node->Data):TYPE_INVALID;}
    IC bool				IsObject			(TElTreeItem* node){return node?(TYPE_OBJECT==(u32)node->Data):TYPE_INVALID;}

    bool 			 	MakeFullName		(TElTreeItem* begin_item, TElTreeItem* end_item, AnsiString& folder);
    bool 			 	MakeName			(TElTreeItem* begin_item, TElTreeItem* end_item, AnsiString& folder, bool bOnlyFolder);
	TElTreeItem* 		FindItemInFolder	(TElTree* tv, TElTreeItem* start_folder, const AnsiString& name);
	TElTreeItem* 		FindItemInFolder	(EItemType type, TElTree* tv, TElTreeItem* start_folder, const AnsiString& name);
    TElTreeItem* 		AppendFolder		(TElTree* tv, AnsiString full_name, bool force_icon);
	TElTreeItem*		AppendObject		(TElTree* tv, AnsiString full_name, bool allow_duplicate, bool force_icon);
    TElTreeItem* 		FindObject			(TElTree* tv, AnsiString full_name, TElTreeItem** last_valid_node=0, int* last_valid_idx=0);
    TElTreeItem* 		FindFolder			(TElTree* tv, AnsiString full_name, TElTreeItem** last_valid_node=0, int* last_valid_idx=0);
    TElTreeItem* 		FindItem			(TElTree* tv, AnsiString full_name, TElTreeItem** last_valid_node=0, int* last_valid_idx=0); 
    void 				GenerateFolderName	(TElTree* tv, TElTreeItem* node,AnsiString& name,AnsiString pref="folder", bool num_first=false);
	void 				GenerateObjectName	(TElTree* tv, TElTreeItem* node, AnsiString& name,AnsiString pref="object", bool num_first=false);
	AnsiString	 		GetFolderName		(const AnsiString& full_name, AnsiString& dest);
	AnsiString	 		GetObjectName		(const AnsiString& full_name, AnsiString& dest);
    AnsiString			ReplacePart			(AnsiString old_name, AnsiString ren_part, int level, LPSTR dest);
    bool 				RenameItem          (TElTree* tv, TElTreeItem* node, AnsiString& new_text, TOnItemRename OnRenameItem);
    void 				CreateNewFolder		(TElTree* tv, bool bEditAfterCreate);
    BOOL				RemoveItem			(TElTree* tv, TElTreeItem* pNode, TOnItemRemove OnRemoveItem, TOnItemAfterRemove OnAfterRemoveItem=0);
    // drag'n'drop
	void __fastcall		DragDrop			(TObject *Sender, TObject *Source, int X, int Y, TOnItemRename after_drag);
	void __fastcall		DragOver			(TObject *Sender, TObject *Source, int X, int Y, TDragState State, bool &Accept);
	void __fastcall		StartDrag			(TObject *Sender, TDragObject *&DragObject);
//	void __fastcall		StartDragNoFolder	(TObject *Sender, TDragObject *&DragObject);
    // popup menu
    void				ShowPPMenu			(TMxPopupMenu* M, TExtBtn* B=0);
    // name edit
	bool 				NameAfterEdit		(TElTreeItem* node, AnsiString value, AnsiString& N);
    // last selection
    TElTreeItem*		RestoreSelection	(TElTree* tv, TElTreeItem* node, bool bLeaveSel);
    TElTreeItem*		RestoreSelection	(TElTree* tv, AnsiString full_name, bool bLeaveSel);
    TElTreeItem*		ExpandItem			(TElTree* tv, TElTreeItem* node);
    TElTreeItem*		ExpandItem			(TElTree* tv, AnsiString full_name);

	bool 				DrawThumbnail		(HDC hdc, const Irect &R, u32* data, u32 w, u32 h);
	void 				FillRect			(HDC hdc, const Irect& r, u32 color);

    AnsiString			GenerateName		(LPCSTR pref, int dgt_cnt, TFindObjectByName cb, bool allow_pref_name, bool allow_);
//------------------------------------------------------------------------------
};

extern XR_EPROPS_API CFolderHelper FHelper;
#endif
