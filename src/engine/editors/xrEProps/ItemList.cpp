#include "stdafx.h"
#pragma hdrstop

#include "ItemList.h"
#include <ElVCLUtils.hpp>
#include <ElTools.hpp>

#include "ColorPicker.h"
#include "FolderLib.h"
#include "NumericVector.h"
#include "TextForm.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "multi_edit"
#pragma link "ElTreeStdEditors"
#pragma link "ElXPThemedControl"
#pragma link "MxMenus"
#pragma link "mxPlacemnt"
#pragma link "ElTree"
#pragma link "ElTreeStdEditors"
#pragma link "ElXPThemedControl"
#pragma link "MxMenus"
#pragma link "mxPlacemnt"
#pragma link "ElTreeAdvEdit"
#pragma link "ElBtnCtl"
#pragma link "ElPopBtn"
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
// IItemList vector
//---------------------------------------------------------------------------
DEFINE_VECTOR					(TItemList*,ILVec,ILVecIt);
static  ILVec					ILForms;

//---------------------------------------------------------------------------
TItemList* TItemList::CreateForm(LPCSTR title, TWinControl* parent, TAlign align, u32 flags)
{
	TItemList* props 			= xr_new<TItemList>(parent);
    // on create
	props->OnCreate				(title,parent, align, flags);
	ILForms.push_back			(props);
	return props;
}

TItemList* TItemList::CreateModalForm(LPCSTR title, u32 flags)
{
	return CreateForm			(title,0,alNone,flags);
}

void TItemList::DestroyForm(TItemList*& props)
{
	VERIFY(props);
    
    ILVecIt it					= std::find(ILForms.begin(),ILForms.end(),props); VERIFY(it!=ILForms.end());
    // destroy forms
	props->OnDestroy			();
    xr_delete					(props);
	ILForms.erase				(it);
}

//---------------------------------------------------------------------------
void TItemList::OnCreate(LPCSTR title, TWinControl* parent, TAlign align, u32 flags)
{
    m_Flags.assign				(flags);
    tvItems->MultiSelect		= m_Flags.is(ilMultiSelect);
    if (parent){
		Parent 					= parent;
    	Align 					= align;
	    BorderStyle 			= bsNone;
        ShowList				();
        fsStorage->Active		= false;
    }
    if (m_Flags.is(ilDragAllowed)){
	    tvItems->OnStartDrag 	= FHelper.StartDrag;
	    tvItems->OnDragOver 	= FHelper.DragOver;
        tvItems->DragAllowed	= true;
    }else if (m_Flags.is(ilDragCustom)){
        tvItems->DragAllowed	= true;
    }else{
	    tvItems->OnStartDrag 	= 0;
	    tvItems->OnDragOver 	= 0;
        tvItems->DragAllowed	= false;
    }
    if (Parent)	tvItems->HeaderSections->Item[0]->Text = title;
    else				Caption = title;
    fsStorage->IniSection   	= title;
    paStatus->Visible 			= !m_Flags.is(ilSuppressStatus);
}

void TItemList::OnDestroy()
{
	ClearList			();
	Close				();
}

void TItemList::ClearParams(TElTreeItem* node)
{
	if (node){
    	FATAL("ClearParams - node");
    }else{
        // save last selected items
        last_selected_items.clear();
        GetSelected		(last_selected_items);
        // store
        if (m_Flags.is(ilFolderStore)&&tvItems->Items->Count){
	        FolderStore.clear();
            for (TElTreeItem* item=tvItems->Items->GetFirstNode(); item; item=item->GetNext()){
            	if (item->ChildrenCount){
                	AnsiString nm;
                	FHelper.MakeFullName(item,0,nm);
                    SFolderStore 		st_item;
                    st_item.expand		= item->Expanded;
                    FolderStore[nm]		= st_item;
                }
            }
        }
        // real clear
	    for (ListItemsIt it=m_Items.begin(); it!=m_Items.end(); it++)
    		xr_delete			(*it);
		m_Items.clear();
        // fill list
        LockUpdating			();
	    tvItems->Items->Clear	();
        UnlockUpdating			();
    }
}
//---------------------------------------------------------------------------
void __fastcall TItemList::ClearList()
{
    ClearParams			();
}
//---------------------------------------------------------------------------
void __fastcall TItemList::DeselectAll()
{
    if (tvItems->MultiSelect) 	tvItems->DeselectAll();
    else 						tvItems->Selected   = 0;
}
//---------------------------------------------------------------------------
ListItem* TItemList::FindItem(LPCSTR full_name)
{
	TElTreeItem* item    		= FHelper.FindObject(tvItems,full_name,&item);
    return item?(ListItem*)item->Tag:0;
}
//---------------------------------------------------------------------------
void TItemList::SelectItem(LPCSTR full_name, bool bVal, bool bLeaveSel, bool bExpand)
{
    // select item
	TElTreeItem* item;              
    FHelper.FindItem			(tvItems,full_name,&item);
    if (!bLeaveSel)				tvItems->DeselectAll();
    if (item){
        if (bExpand) 			FHelper.ExpandItem(tvItems,item);
        if (tvItems->MultiSelect) 	item->Selected 		= bVal;
        else 						tvItems->Selected   = item;
		tvItems->EnsureVisible	(item); 
		if (tvItems->OnAfterSelectionChange) tvItems->OnAfterSelectionChange(0);
    }else{
        if (!tvItems->MultiSelect) 	tvItems->Selected   = item;
    }
}
//---------------------------------------------------------------------------

__fastcall TItemList::TItemList(TComponent* Owner) : TForm(Owner)
{
    m_Flags.zero		();
    OnItemFocusedEvent	= 0;
    OnItemsFocusedEvent	= 0;
    OnCloseEvent		= 0;
    OnItemRenameEvent	= 0;
    OnItemRemoveEvent	= 0;
    iLocked				= 0;
}
//---------------------------------------------------------------------------

void TItemList::ShowList()
{
	Show();
}

void __fastcall TItemList::ShowListModal()
{
	ShowModal();
}

void __fastcall TItemList::HideList()
{
	Hide();
}

void __fastcall TItemList::FormClose(TObject *Sender,
      TCloseAction &Action)
{

    if (!OnCloseEvent.empty()) 	OnCloseEvent();
	ClearParams					();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::AssignItems(ListItemsVec& items, bool full_expand, bool full_sort)
{
	// begin fill mode
	LockUpdating			();
    // clear values
//    if (tvItems->Selected) FHelper.MakeFullName(tvItems->Selected,0,last_selected_item);
    if (!m_Items.empty())	ClearParams();
    // fill values
    m_Items					= items;
	for (ListItemsIt it=m_Items.begin(); it!=m_Items.end(); it++){
    	ListItem* prop		= *it;
        if (prop->key.size()&&(prop->key[prop->key.size()-1]=='\\')){
        	prop->item		= FHelper.AppendFolder(tvItems,*prop->key,!m_Flags.is(ilSuppressIcon));
            TElTreeItem* prop_item	= (TElTreeItem*)prop->item;
            prop_item->CheckBoxEnabled 		= false;
            prop_item->UseStyles		   	= true;
            prop_item->MainStyle->TextColor		= (TColor)prop->prop_color;         
            prop_item->MainStyle->OwnerProps 	= true;
            prop_item->MainStyle->Style 		= ElhsOwnerDraw;
        }else{
            prop->item		= FHelper.AppendObject(tvItems,*prop->key,false,!m_Flags.is(ilSuppressIcon));
            if (!prop->item){
				Msg				("#!Duplicate item name found: '%s'",*prop->key);
                break;
            }
            TElTreeItem* prop_item	= (TElTreeItem*)prop->item;
            prop_item->ImageIndex	= prop->icon_index;
            prop_item->Tag	    	= (int)prop;
            prop_item->UseStyles	= true;
            prop_item->CheckBoxEnabled = prop->m_Flags.is(ListItem::flShowCB);
            prop_item->ShowCheckBox 	= prop->m_Flags.is(ListItem::flShowCB);
            prop_item->CheckBoxState 	= (TCheckBoxState)prop->m_Flags.is(ListItem::flCBChecked);

            // set flags                                        
            if (prop->m_Flags.is(ListItem::flDrawThumbnail)){
                prop_item->Height 		= 64;
                prop_item->OwnerHeight = !miDrawThumbnails->Checked;
            }
            // set style
            prop_item->MainStyle->OwnerProps 	= true;
            prop_item->MainStyle->Style 		= ElhsOwnerDraw;
        }
    }

    // end fill mode
	if (full_expand) tvItems->FullExpand();

    // folder restore
    if (m_Flags.is(ilFolderStore)&&!FolderStore.empty()){
        for (TElTreeItem* item=tvItems->Items->GetFirstNode(); item; item=item->GetNext()){
            if (item->ChildrenCount){
                AnsiString nm;
                FHelper.MakeFullName		(item,0,nm);
                FolderStorePairIt it 		= FolderStore.find(nm);
                if (it!=FolderStore.end()){
                    SFolderStore& st_item 	= it->second;
                    if (st_item.expand) 	item->Expand	(false);
                    else					item->Collapse	(false);
                }
            }
        }
    }

    // sorting
    if (full_sort){
        tvItems->ShowColumns	= false;
    	tvItems->Sort			(true);
        tvItems->ShowColumns	= true;
    }else{
        for (ListItemsIt it=m_Items.begin(); it!=m_Items.end(); it++){
            ListItem* prop		= *it;
            if (prop->m_Flags.is(ListItem::flSorted)) ((TElTreeItem*)prop->item)->Sort(true);
        }
    }

    // expand sel items
    for (RStringVecIt s_it=last_selected_items.begin(); s_it!=last_selected_items.end(); s_it++)
    	FHelper.ExpandItem	(tvItems,**s_it);

	UnlockUpdating			();

    // restore selection
    tvItems->DeselectAll	();

    
    for (s_it=last_selected_items.begin(); s_it!=last_selected_items.end(); s_it++)
	    FHelper.RestoreSelection(tvItems,**s_it,true);

    // check size
	tvItemsResize			(0);

    paStatus->Caption		= AnsiString(" Items count: ")+m_Items.size();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsClick(TObject *Sender)
{
	TElTreeItem* item = dynamic_cast<TElTreeItem*>(Sender);
  	if (item){
        ListItem* prop = (ListItem*)item->Tag;
        if (prop&&!prop->OnClickEvent.empty()) prop->OnClickEvent(prop);
    };
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	TSTItemPart 	IP=(TSTItemPart)0;
    int				HC=0;
	TElTreeItem* item = tvItems->GetItemAt(X,Y,IP,HC);
  	if (item){
        if (Button==mbRight){
            TPoint P; P.x = X; P.y = Y;
            P=tvItems->ClientToScreen(P);
            pmItems->Popup(P.x,P.y-10);
        }
    };
    if (m_Flags.is(ilEditMenu)){
        if (Button==mbRight){
            TPoint P; P.x = X; P.y = Y;
            P=tvItems->ClientToScreen(P);
            pmEdit->Popup(P.x,P.y-10);
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
//
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
//
}
//---------------------------------------------------------------------------

int __fastcall TItemList::GetSelected(RStringVec& items)
{
    for (TElTreeItem* item = tvItems->GetNextSelected(0); item; item = tvItems->GetNextSelected(item)){
        if (item->Hidden)	continue;
    	AnsiString 			nm;
    	FHelper.MakeFullName(item,0,nm);
        items.push_back		(nm.c_str());
    }
    return items.size();
}

int __fastcall TItemList::GetSelected(LPCSTR pref, ListItemsVec& items, bool bOnlyObject)
{
    for (TElTreeItem* item = tvItems->GetNextSelected(0); item; item = tvItems->GetNextSelected(item))
    {
        ListItem* prop 		= (ListItem*)item->Tag;

        if (item->Hidden) continue;

        if (prop && (!bOnlyObject || (bOnlyObject && prop->m_Object) ) )
        {
        	AnsiString key	= *prop->key;
        	if (pref){
            	if (1==key.Pos(pref))
                	items.push_back	(prop);
            }else
				items.push_back	(prop);
        }
    }
    return items.size();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsAfterSelectionChange(TObject *Sender)
{
	if (IsLocked()) return;
    // hack, нода не должна была быть выделенной
    for (TElTreeItem* item = tvItems->GetNextSelected(0); item; item = tvItems->GetNextSelected(item))
        if (item->Hidden)	item->Selected	= false;
    // получаем выделение и обрабатываем ивенты
    ListItemsVec sel_items;
    GetSelected	(0,sel_items,false);
    if (!OnItemFocusedEvent.empty())	OnItemFocusedEvent(GetSelected());
    if (!OnItemsFocusedEvent.empty()) 	OnItemsFocusedEvent(sel_items);
    for (ListItemsIt it=sel_items.begin(); it!=sel_items.end(); it++){
        if (!(*it)->OnItemFocused.empty())(*it)->OnItemFocused(*it);
    }
}
//---------------------------------------------------------------------------

void __fastcall TItemList::FormShow(TObject *Sender)
{
    InplaceEdit->Editor->Color		= TColor(0x00A0A0A0);
    InplaceEdit->Editor->AutoSelect	= true;
	// check window position
	CheckWindowPos					(this);
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsItemChange(TObject *Sender,
      TElTreeItem *Item, TItemChangeMode ItemChangeMode)
{
	if (Item){
    	if (icmCheckState==ItemChangeMode){
            ListItem* prop 			= (ListItem*)Item->Tag;
            if (prop){
                prop->m_Flags.set	(ListItem::flCBChecked,Item->Checked);
    //			prop->OnChange		();
    //			Modified			();
            }
	        tvItems->Refresh		();
	    }
    }
}
//---------------------------------------------------------------------------

void __fastcall TItemList::ExpandAll1Click(TObject *Sender)
{
	tvItems->FullExpand();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::CollapseAll1Click(TObject *Sender)
{
	tvItems->FullCollapse();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::miDrawThumbnailsClick(TObject *Sender)
{
	RefreshForm();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::RefreshForm()
{
    LockUpdating					();
    for (TElTreeItem* item=tvItems->Items->GetFirstNode(); item; item=item->GetNext()){
        ListItem* prop				= (ListItem*)item->Tag;
        if (prop){
            item->Hidden			= prop->m_Flags.is(ListItem::flHidden);
            if (prop->m_Flags.is(ListItem::flHidden)){
                item->Selected		= false;
            }else{
                item->OwnerHeight 	= prop->m_Flags.is(ListItem::flDrawThumbnail)?!miDrawThumbnails->Checked:true;
            }
        }
    }
    for (item=tvItems->Items->GetFirstNode(); item; item=item->GetNext()){
        ListItem* prop				= (ListItem*)item->Tag;
        if (!prop) item->Hidden		= !item->HasVisibleChildren;
    }

//	for (it=m_Items.begin(); it!=m_Items.end(); it++){
//		ListItem* prop 		= *it;
//		((TElTreeItem*)prop->item)->Hidden	= prop->m_Flags.is(ListItem::flHidden);
//	}
	UnlockUpdating			();
	tvItems->Repaint		();
}

void __fastcall TItemList::tvItemsItemDraw(TObject *Sender,
      TElTreeItem *Item, TCanvas *Surface, TRect &R, int SectionIndex)
{
    ListItem* prop 			= (ListItem*)Item->Tag;
    if (prop){
    	Surface->Font->Color= (TColor)prop->prop_color;
        R.left				+= 	4;
        DrawText			(Surface->Handle, AnsiString(Item->Text).c_str(), -1, &R, DT_LEFT | DT_SINGLELINE);
        if (miDrawThumbnails->Checked&&prop->m_Flags.is(ListItem::flDrawThumbnail)){ 
            R.top			+=	tvItems->LineHeight-4;
            R.left 			= 	R.Right-(R.bottom-R.top);
            if (!prop->OnDrawThumbnail.empty())
            	prop->OnDrawThumbnail(prop->Key(),Surface->Handle,Irect().set(R.left,R.top,R.right,R.bottom));
        }
    }else{
    	Surface->Font->Color= Item->MainStyle->TextColor;
        R.left				+= 	4;
        DrawText			(Surface->Handle, AnsiString(Item->Text).c_str(), -1, &R, DT_LEFT | DT_SINGLELINE);
    }
}
//---------------------------------------------------------------------------

void __fastcall TItemList::InplaceEditValidateResult(
      TObject *Sender, bool &InputValid)
{
	R_ASSERT(m_Flags.is(ilEditMenu));
	TElTreeInplaceAdvancedEdit* IE	= InplaceEdit;
    AnsiString new_text 			= AnsiString(IE->Editor->Text).LowerCase();
    InputValid						= false;
    if (!new_text.IsEmpty()){
	    IE->Editor->Text 			= new_text;
		AnsiString old_name, new_name;
		FHelper.MakeName			(IE->Item,0,old_name,false);
	    _ReplaceItem				(old_name.c_str(),IE->Item->Level,new_text.c_str(),new_name,'\\');
	    TElTreeItem* find_item		= FHelper.FindItem(tvItems,new_name);
    	InputValid 					= (find_item==IE->Item)||(!find_item);//.(!find_item); нужно для того чтобы принимало 
    }
}
//---------------------------------------------------------------------------

void __fastcall TItemList::InplaceEditAfterOperation(TObject *Sender,
      bool &Accepted, bool &DefaultConversion)
{
	if (Accepted){
        R_ASSERT(m_Flags.is(ilEditMenu));
        TElTreeInplaceAdvancedEdit* IE	= InplaceEdit;
        AnsiString new_text 			= AnsiString(IE->Editor->Text).LowerCase();
        bool bRes						= FHelper.RenameItem(tvItems,IE->Item,new_text,TOnItemRename(this,&TItemList::RenameItem)); 
        if (bRes){
	        if (tvItems->OnAfterSelectionChange)tvItems->OnAfterSelectionChange(0);
            if (!OnModifiedEvent.empty())OnModifiedEvent();
            // ensure visible
            IE->Item->Text				= new_text;
			tvItems->EnsureVisible		(IE->Item); 
        }
    }
}
//---------------------------------------------------------------------------

void TItemList::RenameItem(LPCSTR fn0, LPCSTR fn1, EItemType type)
{
	if (!OnItemRenameEvent.empty())	OnItemRenameEvent(fn0,fn1,type);
    if (type==TYPE_OBJECT){
        TElTreeItem* item0			= FHelper.FindObject(tvItems,fn0); 	VERIFY(item0);
        ListItem* prop				= (ListItem*)item0->Tag; 			VERIFY(prop);
		prop->SetName				(fn1);
        TElTreeItem* item1			= FHelper.FindObject(tvItems,fn1);
        if (item1) prop->item		= item1;
    }
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
	R_ASSERT(m_Flags.is(ilEditMenu));
    FHelper.DragDrop(Sender,Source,X,Y,TOnItemRename(this,&TItemList::RenameItem));
	if (tvItems->OnAfterSelectionChange) tvItems->OnAfterSelectionChange(0);
}
//---------------------------------------------------------------------------

void __fastcall TItemList::miCreateFolderClick(TObject *Sender)
{
	FHelper.CreateNewFolder(tvItems,true);
}
//---------------------------------------------------------------------------

void __fastcall TItemList::Rename1Click(TObject *Sender)
{
	RenameSelItem();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::Delete1Click(TObject *Sender)
{
	RemoveSelItems();
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if (m_Flags.is(ilEditMenu))
		if (Key==VK_DELETE)
            RemoveSelItems();
}
//---------------------------------------------------------------------------

void TItemList::LoadSelection(TFormStorage* storage)
{
	last_selected_items.clear();
    int cnt 			= storage->ReadInteger("sel_cnt",0);
    for (int k=0; k<cnt;k++){
    	AnsiString tmp = storage->ReadString(AnsiString().sprintf("sel%d",k),"");
        if (!tmp.IsEmpty())last_selected_items.push_back(tmp.c_str());
    }
}
//---------------------------------------------------------------------------

void TItemList::SaveSelection(TFormStorage* storage)
{
    RStringVec items;
    if (GetSelected(items)){
	    storage->WriteInteger("sel_cnt",items.size());
        for (RStringVecIt l_it=items.begin(); l_it!=items.end(); l_it++)
	    	storage->WriteString(AnsiString().sprintf("sel%d",l_it-items.begin()),**l_it);
    }
//    for (AStringIt s_it=last_selected_items.begin(); s_it!=last_selected_items.end(); s_it++)
//    	storage->WriteString(AnsiString().sprintf("sel%d",s_it-last_selected_items.begin()),*s_it);
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsResize(TObject *Sender)
{
    tvItems->HeaderSections->Item[0]->Width = tvItems->Width;
    if (tvItems->VertScrollBarVisible)
    	tvItems->HeaderSections->Item[0]->Width -= tvItems->VertScrollBarStyles->Width;
}
//---------------------------------------------------------------------------

void __fastcall TItemList::RefreshForm1Click(TObject *Sender)
{
	RefreshForm();	
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsHeaderResize(TObject *Sender)
{
    tvItems->HeaderSections->Item[0]->Width = tvItems->Width;
    if (tvItems->VertScrollBarVisible)
    	tvItems->HeaderSections->Item[0]->Width -= tvItems->VertScrollBarStyles->Width;
}
//---------------------------------------------------------------------------

void TItemList::RemoveSelItems(TOnItemRemove on_remove)
{
	if (mrYes==MessageDlg("Remove selected item(s)?", mtConfirmation, TMsgDlgButtons() << mbYes << mbNo, 0)){
        on_remove 	= on_remove.empty()?OnItemRemoveEvent:on_remove;
        VERIFY		(!on_remove.empty());
        RStringVec sel_items;
        if (GetSelected(sel_items)){
            tvItems->IsUpdating = true; // LockUpdating нельзя
            DeselectAll					();
            tvItemsAfterSelectionChange	(0);
            bool bSelChanged=false;
            bool bRes=false;
            for (RStringVecIt it=sel_items.begin(); it!=sel_items.end(); it++){
                TElTreeItem* pNode	= FHelper.FindItem(tvItems,**it);
                if (!FHelper.RemoveItem(tvItems,pNode,on_remove.empty()?OnItemRemoveEvent:on_remove)){
                    SelectItem(**it,true,true,false);
                    bSelChanged=true;
                }else{
                    bRes = true;
                }
            }
            if (bSelChanged||bRes){
                tvItemsAfterSelectionChange	(0);
                if (bRes&&!OnModifiedEvent.empty())	OnModifiedEvent(); 
            }
            tvItems->IsUpdating 		= false;
        }
    }
}
//---------------------------------------------------------------------------

void TItemList::RenameSelItem()
{
	VERIFY(m_Flags.is(ilEditMenu));
    RStringVec sel_items;
    if (1==GetSelected(sel_items)){
		if (sel_items.back().size()){ 
        	TElTreeItem* pNode	= FHelper.FindItem(tvItems,*sel_items.back());
        	tvItems->EditItem	(pNode,-1);
        }
        tvItemsAfterSelectionChange	(0);
    }
}
//---------------------------------------------------------------------------

void __fastcall TItemList::tvItemsCompareItems(TObject *Sender,
      TElTreeItem *Item1, TElTreeItem *Item2, int &res)
{
	u32 type1 = (u32)Item1->Data;
	u32 type2 = (u32)Item2->Data;
    if (type1==type2){
        if (Item1->Text<Item2->Text) 		res = -1;
        else if (Item1->Text>Item2->Text) 	res =  1;
        else if (Item1->Text==Item2->Text) 	res =  0;
    }else if (type1==TYPE_FOLDER)	    	res = -1;
    else if (type2==TYPE_FOLDER)	    	res =  1;
}
//---------------------------------------------------------------------------

void TItemList::FireOnItemFocused()
{
	tvItemsAfterSelectionChange(0);
}
//---------------------------------------------------------------------------

void TItemList::GetFolders(RStringVec& folders)
{
    for (TElTreeItem* item=tvItems->Items->GetFirstNode(); item; item=item->GetNext()){
        if (FHelper.IsFolder(item)){
            AnsiString nm;
            FHelper.MakeFullName(item,0,nm);
            folders.push_back	(AnsiString(nm+'\\').c_str());
        }
    }
}
//---------------------------------------------------------------------------

void TItemList::GenerateObjectName(shared_str& name, LPCSTR start_node, LPCSTR pref, bool num_first)
{
	AnsiString _name;
	TElTreeItem* node 		  	= FHelper.FindItem(tvItems, start_node);
	FHelper.GenerateObjectName	(tvItems, node, _name, pref, num_first);
    name					  	= _name.c_str();
}
//---------------------------------------------------------------------------

