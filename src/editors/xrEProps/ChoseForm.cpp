//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "ChoseForm.h"
#include "PropertiesList.h"               
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElXPThemedControl"
#pragma link "ExtBtn"
#pragma link "MXCtrls"
#pragma link "mxPlacemnt"
#pragma link "ElXPThemedControl"
#pragma link "Gradient"
#pragma resource "*.dfm"
TfrmChoseItem*				TfrmChoseItem::form			= 0;
AnsiString 					TfrmChoseItem::select_item	= "";
TfrmChoseItem::EventsMap	TfrmChoseItem::m_Events;
TOnChooseFillEvents 		TfrmChoseItem::fill_events	= 0;      
AnsiString 					TfrmChoseItem::m_LastSelection; 

//---------------------------------------------------------------------------
SChooseEvents* TfrmChoseItem::GetEvents	(u32 choose_ID)
{
	EventsMapIt it 	= m_Events.find(choose_ID);
    if (it!=m_Events.end()){
    	return &it->second;
    }else return 0;
}
void TfrmChoseItem::AppendEvents(u32 choose_ID, LPCSTR caption, TOnChooseFillItems on_fill, TOnChooseSelectItem on_sel, TOnDrawThumbnail on_thm, TOnChooseClose on_close, u32 flags)
{
	EventsMapIt it 	= m_Events.find(choose_ID); VERIFY(it==m_Events.end());
    m_Events.insert	(std::make_pair(choose_ID,SChooseEvents(caption,on_fill,on_sel,on_thm,on_close,flags)));
}
void TfrmChoseItem::ClearEvents()
{
	m_Events.clear	();
}
//---------------------------------------------------------------------------
void __fastcall TfrmChoseItem::FormCreate(TObject *Sender)
{
    m_Props = TProperties::CreateForm("Info",paInfo,alClient);
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::FormDestroy(TObject *Sender)
{
	TProperties::DestroyForm		(m_Props);
}
//---------------------------------------------------------------------------

int __fastcall TfrmChoseItem::SelectItem(u32 choose_ID, LPCSTR& dest, int sel_cnt, LPCSTR init_name, TOnChooseFillItems item_fill, void* fill_param, TOnChooseSelectItem item_select, ChooseItemVec* items, u32 mask)
{
	VERIFY(!form);
	form 							= xr_new<TfrmChoseItem>((TComponent*)0);
    form->m_Flags.assign			(mask);
    form->m_Flags.set				(cfMultiSelect,sel_cnt>1);
    form->iMultiSelLimit 			= sel_cnt;

	// init
//.	if (init_name&&init_name[0]) 
    	m_LastSelection 			= init_name;
    form->tvItems->Selected 		= 0;

    // fill items
    if (items){
    	VERIFY2(item_fill.empty(),"ChooseForm: Duplicate source.");
    	form->m_Items				= *items;
        form->E.Set					("Select Item",0,item_select,0,0,0);
    }else if (!item_fill.empty()){
    	// custom
        form->E.Set					("Select Item",item_fill,item_select,0,0,0);
    }else{
    	SChooseEvents* e			= GetEvents(choose_ID); VERIFY2(e,"Can't find choose event.");
    	form->E						= *e;
    }
	// set & fill
    form->Caption					= form->E.caption.c_str();

    if (!form->E.on_fill.empty())	
    	form->E.on_fill(form->m_Items,fill_param);
    
    form->FillItems					(choose_ID);
    
//.	form->paItemsCount->Caption		= AnsiString(" Items in list: ")+AnsiString(form->tvItems->Items->Count);

	// show
    bool bRes 						= (form->ShowModal()==mrOk);
    dest							= 0;
    if (bRes){
		int item_cnt				= _GetItemCount(select_item.c_str(),',');
    	dest 						= (select_item==NONE_CAPTION)?0:select_item.c_str();
	    m_LastSelection				= select_item;
        return 						item_cnt?item_cnt:1;
    }
    return 0;
}
// Constructors
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::AppendItem(SChooseItem* item, bool b_check_duplicate)
{
    TElTreeItem* node				= FHelper.AppendObject(form->tvItems,item->name.c_str(), !b_check_duplicate/*false*/,true);
    node->Tag						= (int)item;
    node->Hint						= item->hint.size()?item->hint.c_str():"-";
    node->CheckBoxEnabled 			= form->m_Flags.is(cfMultiSelect);
    node->ShowCheckBox 				= form->m_Flags.is(cfMultiSelect);
}

void __fastcall TfrmChoseItem::FillItems(u32 choose_id)
{
	form->tvItems->IsUpdating		= true;
    tvItems->Items->Clear			();

    if (m_Flags.is(cfAllowNone)&&!m_Flags.is(cfMultiSelect))	
    	FHelper.AppendObject(tvItems,NONE_CAPTION,false,true);

	u32 ss = m_Items.size();
    ChooseItemVecIt  it				= m_Items.begin();
    ChooseItemVecIt  _E				= m_Items.end();

    bool b_check_duplicate = (choose_id!=15);// smSkeletonAnimations is already unique set !!!
    for (; it!=_E; it++)   			
    	AppendItem(&(*it), b_check_duplicate);
        
    form->tvItems->Sort				(true);
	form->tvItems->IsUpdating 		= false;
    if (m_Flags.is(cfFullExpand))	form->tvItems->FullExpand		();
}

//---------------------------------------------------------------------------
// implementation
//---------------------------------------------------------------------------
__fastcall TfrmChoseItem::TfrmChoseItem(TComponent* Owner)
    : TForm(Owner)
{
	tvItems->MultiSelect 	= false;
    m_Flags.assign			(cfAllowNone);
    tvItems->ShowCheckboxes = false;
    grdFon->Caption 		= "";
}
//---------------------------------------------------------------------------
void __fastcall TfrmChoseItem::sbSelectClick(TObject *Sender)
{
	if (m_Flags.is(cfMultiSelect)){
        select_item = "";
        AnsiString nm;
	    for ( TElTreeItem* node = tvMulti->Items->GetFirstNode(); node; node = node->GetNext()){
    	    FHelper.MakeName(node,0,nm,false);
            select_item += nm+AnsiString(",");
        }
        if (!select_item.IsEmpty()) select_item.Delete(select_item.Length(),1);

        Close();
        ModalResult = mrOk;
    }else{
	    if (tvItems->Selected&&FHelper.IsObject(tvItems->Selected)){
    	    FHelper.MakeName(tvItems->Selected,0,select_item,false);
	        Close();
    	    ModalResult = mrOk;
	    }else{
			Msg			("#Select item first.");
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TfrmChoseItem::sbCancelClick(TObject *Sender)
{
    Close();
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TfrmChoseItem::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key==VK_ESCAPE) sbCancel->Click();
    if (Key==VK_RETURN) sbSelect->Click();
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::FormShow(TObject *Sender)
{
    tvItems->ShowCheckboxes 	= m_Flags.is(cfMultiSelect);
	int itm_cnt = _GetItemCount(m_LastSelection.c_str());
	if (m_Flags.is(cfMultiSelect)){
	    string256 T;
        for (int i=0; i<itm_cnt; i++){
            TElTreeItem* itm_node = FHelper.FindObject(tvItems,_GetItem(m_LastSelection.LowerCase().c_str(),i,T,',',"",false),0,0);//,bIgnoreExt);
	        TElTreeItem* fld_node = 0;
            if (itm_node){
				tvMulti->Items->AddObject(0,_GetItem(m_LastSelection.c_str(),i,T,',',"",false),(void*)TYPE_OBJECT);
            	itm_node->Checked = true;
                tvItems->EnsureVisible(itm_node);
                fld_node=itm_node->Parent;
                if (fld_node) fld_node->Expand(false);
            }
        }
    }else{
        TElTreeItem* itm_node = FHelper.FindItem(tvItems,m_LastSelection.LowerCase().c_str(),0,0);//,bIgnoreExt);
        TElTreeItem* fld_node = 0;
        if (itm_node){
            tvItems->Selected = itm_node;
            tvItems->EnsureVisible(itm_node);
            fld_node=itm_node->Parent;
            if (fld_node) fld_node->Expand(false);
        }else if (fld_node){
            tvItems->EnsureVisible(fld_node);
            fld_node->Expand(false);
            tvItems->Selected = fld_node;
        }
    }
    paMulti->Visible = m_Flags.is(cfMultiSelect);
	// check window position
	CheckWindowPos	(this);
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::FormClose(TObject *Sender, TCloseAction &Action)
{
    if (tvItems->Selected) 	m_LastSelection=tvItems->Selected->Text;
    if (!E.on_close.empty())E.on_close();
	Action 			= caFree;
    form 			= 0;
}
//---------------------------------------------------------------------------


void __fastcall TfrmChoseItem::tvItemsDblClick(TObject *Sender)
{
	if (!m_Flags.is(cfMultiSelect)&&FHelper.IsObject(tvItems->Selected)) sbSelectClick(Sender);
}
//---------------------------------------------------------------------------

bool __fastcall LookupFunc(TElTreeItem* Item, void* SearchDetails){
    char s1 = *(char*)SearchDetails;
    char s2 = *AnsiString(Item->Text).c_str();
	return (s1==tolower(s2));
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::tvItemsKeyPress(TObject *Sender, char &Key)
{
	TElTreeItem* node = tvItems->Items->LookForItemEx(tvItems->Selected,-1,false,false,false,&Key,LookupFunc);
    if (!node) node = tvItems->Items->LookForItemEx(0,-1,false,false,false,&Key,LookupFunc);
    if (node){
    	tvItems->Selected = node;
		tvItems->EnsureVisible(node);
    }
}
//---------------------------------------------------------------------------


void __fastcall TfrmChoseItem::tvItemsItemChange(TObject *Sender,
      TElTreeItem *Item, TItemChangeMode ItemChangeMode)
{
	if (Item&&(ItemChangeMode==icmCheckState)){
        AnsiString fn;
        FHelper.MakeName(Item,0,fn,false);
	    TElTreeItem *node = tvMulti->Items->LookForItem(0,fn.c_str(),0,0,false,true,false,true,true);
        if (node&&!Item->Checked) node->Delete();
        if (!node&&Item->Checked){
	    	if ((tvMulti->Items->Count+1)<=iMultiSelLimit){
    	        tvMulti->Items->AddObject(0,fn,(void*)TYPE_OBJECT);
            }else{
            	Item->Checked 	= false;
	        	Msg				("#Limit %d item(s).",iMultiSelLimit);
            }
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::ebMultiUpClick(TObject *Sender)
{
    if (tvMulti->Selected&&tvMulti->Selected->GetPrev())
    	tvMulti->Selected->MoveToIns(0,tvMulti->Selected->Index-1);
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::ebMultiDownClick(TObject *Sender)
{
    if (tvMulti->Selected&&tvMulti->Selected->GetNext())
    	tvMulti->Selected->MoveToIns(0,tvMulti->Selected->Index+1);
}
//---------------------------------------------------------------------------
static TElTreeItem* DragItem=0;
void __fastcall TfrmChoseItem::tvMultiDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
	TElTreeItem* node = ((TElTree*)Sender)->GetItemAtY(Y);
    if (node)
		DragItem->MoveToIns(0,node->Index);
	DragItem = 0;
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::tvMultiDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
	Accept = false;
	TElTreeItem* node = ((TElTree*)Sender)->GetItemAtY(Y);
	if ((Sender==Source)&&(node!=DragItem)) Accept=true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::tvMultiStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
	DragItem = ((TElTree*)Sender)->Selected;
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::ebMultiRemoveClick(TObject *Sender)
{
	if (tvMulti->Selected){
    	TElTreeItem *cur = tvMulti->Selected;
	    TElTreeItem *node = tvItems->Items->LookForItem(0,cur->Text,0,0,false,true,false,true,true);
        if (node) node->Checked = false;
        tvMulti->Selected = cur->GetNext()?cur->GetNext():cur->GetPrev();
    	cur->Delete();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::ebMultiClearClick(TObject *Sender)
{
	tvMulti->Items->Clear();
    for ( TElTreeItem* node = tvItems->Items->GetFirstNode(); node; node = node->GetNext())
    	if (node->Checked) node->Checked=false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::tvItemsItemFocused(TObject *Sender)
{
	TElTreeItem* Item 	= tvItems->Selected;
    PropItemVec 		items;
    lbItemName->Caption	= "";
    lbHint->Caption    	= "";
	if (Item&&FHelper.IsObject(Item)&&Item->Tag){
    	if (ebExt->Down&&!E.on_sel.empty())	
        	E.on_sel	((SChooseItem*)Item->Tag,items);
        lbItemName->Caption 	= Item->Text;
        lbHint->Caption 		= Item->Hint;
    }
    m_Props->AssignItems		(items);
    paInfo->Visible				= !items.empty();
	paImage->Repaint			();
}
//---------------------------------------------------------------------------

void TfrmChoseItem::DrawImage	()
{
	TElTreeItem* Item 	= tvItems->Selected;
	if (Item&&FHelper.IsObject(Item)&&Item->Tag){
    	if (ebExt->Down){//&&!E.on_sel.empty()){
        	SChooseItem* itm 	= (SChooseItem*)Item->Tag;
            if (!E.on_thm.empty()){
            	E.on_thm(*itm->name,paImage->Canvas->Handle,Irect().set(0,0,paImage->Width,paImage->Height));
            }
        }        	
    }
}

void __fastcall TfrmChoseItem::paImagePaint(TObject *Sender)
{
	DrawImage();
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::ebExtClick(TObject *Sender)
{
	tvItemsItemFocused				(0);
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::fsStorageRestorePlacement(TObject *Sender)
{
	m_Props->RestoreParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::fsStorageSavePlacement(TObject *Sender)
{
	m_Props->SaveParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::tvItemsCompareItems(TObject *Sender,
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

void __fastcall TfrmChoseItem::tmRepaintTimer(TObject *Sender)
{
	DrawImage();
}
//---------------------------------------------------------------------------

void __fastcall TfrmChoseItem::edFindChange(TObject *Sender)
{
	AnsiString txt = edFind->Text;
    FHelper.RestoreSelection(tvItems,txt,false);
}
//---------------------------------------------------------------------------

void __fastcall	TfrmChoseItem::OnFrame()
{
	if (form){ 
	    if (form->E.flags.is(SChooseEvents::flAnimated))
    		form->DrawImage();
    }
}
//---------------------------------------------------------------------------

