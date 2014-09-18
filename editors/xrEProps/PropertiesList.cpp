//------------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "PropertiesList.h"
#include <ElVCLUtils.hpp>
#include <ElTools.hpp>

#include "ShaderFunction.h"
#include "ColorPicker.h"
#include "ChoseForm.h"
#include "FolderLib.h"
#include "NumericVector.h"
#include "TextForm.h"
#include "GameTypeForm.h"
#include "ItemList.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "multi_edit"
#pragma link "ElTreeStdEditors"
#pragma link "ElXPThemedControl"
#pragma link "multi_edit"
#pragma link "MxMenus"
#pragma link "mxPlacemnt"
#pragma link "ElTree"
#pragma link "ElTreeStdEditors"
#pragma link "ElXPThemedControl"
#pragma link "multi_edit"
#pragma link "MxMenus"
#pragma link "mxPlacemnt"
#pragma link "ElTreeAdvEdit"
#pragma link "ElBtnCtl"
#pragma link "ElPopBtn"
#pragma link "ExtBtn"
#pragma link "ElEdits"
#pragma link "ElHotKey"
#pragma link "RenderWindow"
#pragma link "MxShortcut"
#pragma link "ExtBtn"
#pragma resource "*.dfm"

#define TSTRING_COUNT 	4
const LPSTR TEXTUREString[TSTRING_COUNT]={"Custom...","-","$null","$base0"};
//---------------------------------------------------------------------------
void TProperties::ClearParams(TElTreeItem* node)
{
	if (node){
    	FATAL("ClearParams - node");
    	//S когда будут все итемы удалить у каждого
/*
//s
    	for (TElTreeItem* item=node; item; item=item->GetNext()){
			PropValue* V = (PropValue*)GetItemData(item);
            if (V){
	            PropValuePairIt it=std::find(m_Values.begin(),m_Values.end(),V); VERIFY(it!=m_Values.end());
    	        if (it){
					m_Values.erase(it);
					xr_delete(V);
                }
            }
		}
*/
    }else{
	    if (tvProperties->Selected) FHelper.MakeFullName(tvProperties->Selected,0,last_selected_item);
        // store
        if (!m_Flags.is(plNoClearStore)) FolderStorage.clear();
        FolderStore			();
        // clear
	    for (PropItemIt it=m_Items.begin(); it!=m_Items.end(); it++)
    		xr_delete	(*it);
		m_Items.clear				();
        m_ViewItems.clear			();
		LockUpdating				();
	    tvProperties->Items->Clear	();
		UnlockUpdating				();
    }
}
//---------------------------------------------------------------------------
void __fastcall TProperties::ResetItems()
{
    for (PropItemIt it=m_Items.begin(); it!=m_Items.end(); it++)
        (*it)->ResetValues();
}
//---------------------------------------------------------------------------
void __fastcall TProperties::ClearProperties()
{
	CancelEditControl	();
    ClearParams			();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::SelectFolder(const AnsiString& folder_name)
{
	m_Folders->SelectItem(folder_name.c_str(),true,true,true);
//.    tvProperties->se
}

void __fastcall TProperties::SelectItem(const AnsiString& full_name)
{
	m_FirstClickItem	= FHelper.RestoreSelection	(tvProperties,FHelper.ExpandItem(tvProperties,full_name),false);
}
//---------------------------------------------------------------------------

__fastcall TProperties::TProperties(TComponent* Owner) : TForm(Owner)
{
	m_FirstClickItem= 0;
	bModified 		= false;
	m_BMCheck 		= xr_new<Graphics::TBitmap>();
    m_BMDot 		= xr_new<Graphics::TBitmap>();
    m_BMEllipsis 	= xr_new<Graphics::TBitmap>();
	m_BMCheck->LoadFromResourceName		((u32)HInstance,"CHECK");
	m_BMDot->LoadFromResourceName		((u32)HInstance,"DOT");
	m_BMEllipsis->LoadFromResourceName	((u32)HInstance,"ELLIPSIS");
    seNumber->Parent	= tvProperties;
    seNumber->Hide		();
    edText->Parent		= tvProperties;
    edText->Hide		();
    hkShortcut->Parent	= tvProperties;
    hkShortcut->Hide	();
    m_Flags.zero		();
    m_Folders			= 0;
}
//---------------------------------------------------------------------------

TProperties* TProperties::CreateForm(const AnsiString& title, TWinControl* parent, TAlign align, TOnModifiedEvent modif, TOnItemFocused focused, TOnCloseEvent on_close, u32 flags)
{
	TProperties* props 			= xr_new<TProperties>(parent);
    props->OnModifiedEvent 		= modif;
    props->OnItemFocused    	= focused;
    props->OnCloseEvent			= on_close;
    if (parent){
		props->Parent 			= parent;
    	props->Align 			= align;
	    props->BorderStyle 		= bsNone;
        props->ShowProperties	();
        props->fsStorage->Active= false;
    }
	props->Caption				= title;	
    props->fsStorage->IniSection= title;
    props->m_Flags.assign		(flags);   

    if (props->m_Flags.is_any(plItemFolders)){
    	if (props->m_Flags.is(plIFTop)){
            props->paFolders->Align	= alTop;
            props->spFolders->Align	= alTop;
            props->spFolders->Top	= props->paFolders->Top+props->paFolders->Height;
        }else{
            props->paFolders->Align	= alLeft;
            props->spFolders->Align	= alLeft;
            props->spFolders->Left	= props->paFolders->Left+props->paFolders->Width;
        }
    	props->spFolders->Show	();
    	props->paFolders->Show	();
        props->paFolders->Refresh();
    	props->m_Folders		= TItemList::CreateForm("Folders",props->paFolders,alClient,TItemList::ilSuppressIcon|TItemList::ilFolderStore|TItemList::ilSuppressStatus|(props->m_Flags.is(plMultiSelect)?TItemList::ilMultiSelect:0));
        props->m_Folders->OnItemFocusedEvent.bind(props,&TProperties::OnFolderFocused);
    }else{
    	props->spFolders->Hide	();
    	props->paFolders->Hide	();
    }
	return props;
}

TProperties* TProperties::CreateModalForm(const AnsiString& title, bool bShowButtonsBar, TOnModifiedEvent modif, TOnItemFocused focused, TOnCloseEvent on_close, u32 flags)
{
	TProperties* props 			= xr_new<TProperties>((TComponent*)0);
    props->OnModifiedEvent 		= modif;
    props->OnItemFocused    	= focused;
    props->OnCloseEvent			= on_close;
    props->paButtons->Visible	= bShowButtonsBar;
	props->Caption				= title;	
    props->fsStorage->IniSection= title;
    props->m_Flags.assign		(flags);
    if (props->m_Flags.is(plItemFolders)){
    	if (props->m_Flags.is(plIFTop)){
            props->paFolders->Align	= alTop;
            props->spFolders->Align	= alTop;
            props->spFolders->Top	= props->paFolders->Top+props->paFolders->Height;
        }else{
            props->paFolders->Align	= alLeft;
            props->spFolders->Align	= alLeft;
            props->spFolders->Left	= props->paFolders->Left+props->paFolders->Width;
        }
    	props->spFolders->Show	();
    	props->paFolders->Show	();
        props->paFolders->Refresh();
    	props->m_Folders		= TItemList::CreateForm("Folders",props->paFolders,alClient,TItemList::ilSuppressIcon|TItemList::ilFolderStore|TItemList::ilSuppressStatus|(props->m_Flags.is(plMultiSelect)?TItemList::ilMultiSelect:0)); 
        props->m_Folders->OnItemFocusedEvent.bind(props,&TProperties::OnFolderFocused);
    }else{
    	props->spFolders->Hide	();
    	props->paFolders->Hide	();
    }
	return props;
}

void TProperties::DestroyForm(TProperties*& props)
{
	VERIFY(props);
	props->FolderStorage.clear();
	// apply edit controls
	props->ApplyEditControl();
    // destroy forms
	props->ClearProperties();
	props->Close();
    xr_delete(props);
}
void __fastcall TProperties::ShowProperties()
{
	Show					();
}

int __fastcall TProperties::ShowPropertiesModal()
{
	return ShowModal		();
}

void __fastcall TProperties::HideProperties()
{
	Hide();
}

int __fastcall TProperties::EditPropertiesModal(PropItemVec& values, LPCSTR title, bool bShowButtonsBar, TOnModifiedEvent modif, TOnItemFocused focused, TOnCloseEvent close, u32 flags)
{
	TProperties* P 	= CreateModalForm(title,bShowButtonsBar,modif,focused,close,flags);
    P->AssignItems	(values);
    int res 		= P->ShowPropertiesModal();
    DestroyForm		(P);
    return res;
}

void __fastcall TProperties::FormClose(TObject *Sender,
      TCloseAction &Action)
{
	ApplyEditControl	();
    if (Visible&&!OnCloseEvent.empty()) 	OnCloseEvent();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::FormDestroy(TObject *Sender)
{
	ClearParams		();
    xr_delete		(m_BMCheck);
    xr_delete		(m_BMDot);
    xr_delete		(m_BMEllipsis);
}
//---------------------------------------------------------------------------

void TProperties::FillElItems(PropItemVec& items, LPCSTR startup_pref)
{
	m_ViewItems.clear();
    tvProperties->Items->Clear();
	for (PropItemIt it=items.begin(); it!=items.end(); it++){
    	PropItem* prop		= *it;
        AnsiString 	key 	= *prop->key;
	    if (m_Flags.is(plItemFolders)&&(startup_pref&&startup_pref[0])){
        	if (startup_pref&&startup_pref[0]){
                AnsiString k	= key;		
                LPCSTR k0		= k.c_str();
                LPCSTR k1		= startup_pref;
                while (k0[0]&&k1[0]&&(k0[0]==k1[0]))	{k0++;k1++; if(k0[0]=='\\')key=k0+1;}
                if (!((k0[0]==0)&&(k1[0]==0)))			if ((k0[0]!='\\')||(k1[0]!=0))	continue;
            }else{
            	if (1!=_GetItemCount(key.c_str(),'\\')) continue;
            }
        }
        m_ViewItems.push_back	(prop);
        prop->m_Owner 		= this; 
        prop->item			= FHelper.AppendObject(tvProperties,key,false,false); 
        R_ASSERT3			(prop->item,"Duplicate properties key found:",key.c_str());
        prop->Item()->Hint	= ".";
        prop->Item()->Tag 	= (int)prop;
        prop->Item()->UseStyles=true;
        prop->Item()->CheckBoxEnabled = prop->m_Flags.is(PropItem::flShowCB);
        prop->Item()->ShowCheckBox 	= prop->m_Flags.is(PropItem::flShowCB);
        prop->Item()->CheckBoxState 	= (TCheckBoxState)prop->m_Flags.is(PropItem::flCBChecked);
        // if thumbnail draw
        if (prop->m_Flags.is(PropItem::flDrawThumbnail)){
        	prop->Item()->Height 		= 64;
        	prop->Item()->OwnerHeight = !miDrawThumbnails->Checked;
        }                             
        // if canvas value
        if (PROP_CANVAS==prop->type){
        	prop->Item()->Height 		= ((CanvasValue*)prop->GetFrontValue())->height;
        	prop->Item()->OwnerHeight = false;
        }
        // main text set style
        prop->Item()->MainStyle->Style= ElhsOwnerDraw;
        
        // set style
        TElCellStyle* CS    = prop->Item()->AddStyle();
        CS->OwnerProps 		= true;
        CS->CellType 		= sftUndef;
        CS->Style 			= ElhsOwnerDraw;
        prop->Item()->ColumnText->Add(prop->GetDrawText().c_str());
    }
    if (m_Flags.is(plFullExpand)||miAutoExpand->Checked) tvProperties->FullExpand();
    if (m_Flags.is(plFullSort)){
        tvProperties->ShowColumns	= false;
    	tvProperties->Sort			(true);
        tvProperties->SortMode 		= smAdd;
        tvProperties->ShowColumns	= true;
    }else{
        for (PropItemIt it=m_ViewItems.begin(); it!=m_ViewItems.end(); it++){
            PropItem* prop = *it;
            if (prop->m_Flags.is(PropItem::flSorted)) 
            	((TElTreeItem*)prop->item)->Sort(true);
        }
    }

    FolderRestore		();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::AssignItems(PropItemVec& items)
{
	// begin fill mode
	LockUpdating		();
	CancelEditControl	();

    // clear values
//    if (tvProperties->Selected) FHelper.MakeFullName(tvProperties->Selected,0,last_selected_item);
    ClearParams();

    // copy values
    m_Items				= items;

    // folder
    ListItemsVec		folder_items;

    if (m_Flags.is(plItemFolders)){
        for (PropItemIt it=m_Items.begin(); it!=m_Items.end(); it++){
            PropItem* prop 	= *it;
            int cnt 		= _GetItemCount(prop->key.c_str(),'\\');
            if (cnt>1){	
                AnsiString 	folder;
                _ReplaceItem(prop->key.c_str(),cnt-1,"",folder,'\\');
                if (0==LHelper().FindItem(folder_items,folder.c_str())){
                	PropItem* P		= PHelper().FindItem(m_Items,prop->key.c_str());
                    ListItem* I		= LHelper().CreateItem(folder_items,folder.c_str(),0);
                    if (P) I->prop_color = P->prop_color;
                }
            }
        }
    }

    // create EL items
	if (m_Flags.is(plItemFolders))	m_Folders->AssignItems	(folder_items,m_Flags.is(plFullExpand),m_Flags.is(plFullSort));
	// fill 
	AnsiString full_sel_item;
    if (m_Flags.is(plItemFolders)&&m_Folders->GetSelected())
    	FHelper.MakeFullName(m_Folders->GetSelected(),0,full_sel_item);
    FillElItems			(m_Items, full_sel_item.c_str());

    // end fill mode
    bModified			= false;

	UnlockUpdating		();

    FolderRestore		();
    SelectItem			(last_selected_item);
}
//---------------------------------------------------------------------------

void TProperties::FolderStore()
{
    if (m_Flags.is(plFolderStore)&&tvProperties->Items->Count){
        for (TElTreeItem* item=tvProperties->Items->GetFirstNode(); item; item=item->GetNext()){
            if (item->ChildrenCount)
            {
                AnsiString nm;
                FHelper.MakeFullName(item,0,nm);
                SFolderStore 		st_item;
                st_item.expand		= item->Expanded;
                FolderStorage[nm]		= st_item;
            }
        }
    }
}
void TProperties::FolderRestore()
{
    if (m_Flags.is(plFolderStore)&&!FolderStorage.empty()){
        for (TElTreeItem* item=tvProperties->Items->GetFirstNode(); item; item=item->GetNext()){
            if (item->ChildrenCount){   
                AnsiString nm;
                FHelper.MakeFullName		(item,0,nm);
                FolderStorePairIt it 		= FolderStorage.find(nm);
                if (it!=FolderStorage.end()){
                    SFolderStore& st_item 	= it->second;
                    if (st_item.expand) 	item->Expand	(false);
                    else					item->Collapse	(false);
                }
            }
        }
    }
}
void __fastcall TProperties::OnFolderFocused(TElTreeItem* item)
{
	AnsiString s, lfsi;
    if (tvProperties->Selected) FHelper.MakeFullName(tvProperties->Selected,0,lfsi);
	FHelper.MakeFullName(item,0,s);
    LockUpdating	();
    FolderStore		();
    FillElItems		(m_Items, s.c_str());
    UnlockUpdating	();
    if (lfsi.Length()){
		AnsiString 	lfsi_new,new_part;
        int cnt		= _GetItemCount(s.c_str(),'\\');
        if (cnt)	_GetItem(s.c_str(),cnt-1,new_part,'\\',"",false);
        _ReplaceItem(lfsi.c_str(),0,new_part.c_str(),lfsi_new,'\\');
	    SelectItem	(lfsi_new);
	}
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesClick(TObject *Sender)
{
	if (m_Flags.is(plReadOnly)) return;

	TSTItemPart 	IP=(TSTItemPart)0;
    int				HC=0;
    TElTreeItem* 	Item;
    TPoint			P;

  	GetCursorPos(&P);
  	P = tvProperties->ScreenToClient(P);
  	Item = tvProperties->GetItemAt(P.x, P.y, IP, HC);
	if (HC==1)
    	tvProperties->EditItem(Item, HC);
}
//---------------------------------------------------------------------------

void TProperties::OutBOOL(BOOL val, TCanvas* Surface, TRect& R, bool bEnable)
{
	if (bEnable){
        Surface->CopyMode 	= cmSrcAnd;//cmSrcErase;
        if (val)			Surface->Draw(R.Left,R.Top+3,m_BMCheck);
        else 				Surface->Draw(R.Left,R.Top+3,m_BMDot);
    }else{
	    DrawText			(Surface->Handle, val?"on":"off", -1, &R, DT_LEFT | DT_SINGLELINE);
    }
}

void TProperties::OutText(LPCSTR text, TCanvas* Surface, TRect& R, bool bEnable, TGraphic* g, bool bArrow)
{
	if (bEnable&&(g||bArrow)){
	    R.Right	-=	g->Width+2;
    	R.Left 	+= 	1;
    }else{
        R.Right-= 1;
        R.Left += 1;
    }
    DrawText	(Surface->Handle, text, -1, &R, DT_LEFT | DT_SINGLELINE);
	if (bEnable){
        if (g){
            R.Left 	= 	R.Right;
            Surface->CopyMode = cmSrcAnd;
            Surface->Draw(R.Left+1,R.Top+5,g);
        }else if (bArrow){
            R.Left 	= 	R.Right;
            R.Right += 	10;
            DrawArrow	(Surface, eadDown, R, clWindowText, true);
        }
    }
}

void DrawButton(TRect R, TCanvas* Surface, LPCSTR caption, bool bDown, bool bSelected)
{
    TColor a 				= bDown?TColor(0x00707070):TColor(0x00A0A0A0);
    TColor b 				= bDown?TColor(0x00C0C0C0):TColor(0x00707070);
    R.Bottom				+= 	1;
    Surface->Pen->Color 	= a;                     
    Surface->MoveTo			(R.left,R.bottom);
    Surface->LineTo			(R.left,R.top);
    Surface->LineTo			(R.right,R.top);
    Surface->Pen->Color 	= b;
    Surface->LineTo			(R.right,R.bottom);
    Surface->LineTo			(R.left-1,R.bottom);
    R.Left 					+= 	2;
    R.Right					-=	1;
    R.Bottom				-= 	1;
    R.Top					+=	1;
	Surface->Brush->Style 	= bsSolid;
	Surface->Brush->Color 	= bSelected?TColor(0x00858585):TColor(0x00808080);
	Surface->FillRect		(R);
    DrawText				(Surface->Handle, caption, -1, &R, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
}
void DrawButtons(TRect R, TCanvas* Surface, RStringVec& lst, int down_btn, bool bSelected)
{
	if (!lst.empty()){
        TRect r				= R;
        float dx 			= float(R.Width())/float(lst.size());
        for (RStringVecIt it=lst.begin(); it!=lst.end(); it++){
        	int k			= it-lst.begin();
    		r.left			= R.left+iFloor(k*dx);
    		r.right			= r.left+dx-1;
            DrawButton		(r,Surface,it->c_str(),(down_btn==k),bSelected);
        }
    }
}

int DrawText(HDC hDC, LPCSTR text, LPRECT R, UINT uFormat)
{
	return DrawText(hDC,text,-1,R,uFormat);
}

void __fastcall TProperties::tvPropertiesItemDraw(TObject *Sender,
      TElTreeItem *Item, TCanvas *Surface, TRect &R, int SectionIndex)
{
    PropItem* prop 					= (PropItem*)Item->Tag;
    if (!prop)						return;

    R.left							+= 	4;
    
	TRect  R1;
	Surface->Brush->Style 			= bsClear;
	if (SectionIndex == 0){
        Surface->Font->Style 		= TFontStyles();           
        Surface->Font->Color 		= (TColor)prop->prop_color;
        DrawText					(Surface->Handle, AnsiString(Item->Text).c_str(), -1, &R, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	}else if (SectionIndex == 1){
        u32 type 					= prop->type;
        if (prop->Enabled()){
            Surface->Font->Color 	= (TColor)prop->val_color;
            Surface->Font->Style 	= TFontStyles();           
        }else{
            Surface->Font->Color 	= clSilver;
            Surface->Font->Style 	= TFontStyles()<< fsBold;
        }
        // check mixed
        prop->CheckMixed();
        // out caption mixed 
        if (prop->m_Flags.is(PropItem::flMixed)){
            TColor C 		= Surface->Brush->Color;
            TBrushStyle S 	= Surface->Brush->Style;
            Surface->Brush->Style = bsSolid;
            Surface->Brush->Color = Item->Selected?(TColor)0x00A0A0A0:(TColor)0x008E8E8E;
            TRect r	=	R;
            r.Left 	-= 	1;
            r.Right	+=	1;
            r.Bottom+= 	2;
            r.Top	-=	1;
            Surface->FillRect(r);
            Surface->Brush->Color 	= C;
            Surface->Brush->Style	= S;
            R.Right-= 1;
            R.Left += 1;
            DrawText	(Surface->Handle, prop->GetDrawText().c_str(), &R, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }else{
            TRect src_rect 	= R;
            prop->draw_rect.set(R.left,R.top,R.Right,R.Bottom);
            switch(type){
            case PROP_BUTTON:{
                ButtonValue* V			= dynamic_cast<ButtonValue*>(prop->GetFrontValue()); R_ASSERT(V);
                DrawButtons				(R,Surface,V->value,V->btn_num,Item->Selected);
            }break;
            case PROP_CAPTION:
                Surface->Font->Color 	= clSilver;
                R.Right-= 1;
                R.Left += 1;
                DrawText	(Surface->Handle, prop->GetDrawText().c_str(), &R, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            break;
            case PROP_CANVAS:{
                Surface->Font->Color 	= clSilver;
                R.Right-= 1;
                R.Left += 1;
                CanvasValue* val = dynamic_cast<CanvasValue*>(prop->GetFrontValue()); R_ASSERT(val);
                if (!val->OnDrawCanvasEvent.empty())
	                val->OnDrawCanvasEvent(val,Surface,Irect().set(R.left,R.top,R.right,R.bottom));
                DrawText	(Surface->Handle, prop->GetDrawText().c_str(), &R, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            }break;
            case PROP_FCOLOR:{
            	R.Left	-= 	3;
            	R.Bottom+= 	1;
                Surface->Brush->Style = bsSolid;
                Surface->Brush->Color = TColor(0x00000000);
                Surface->FrameRect(R);
                R.Right	-=	1;
                R.Left 	+= 	1;
                R.Top	+=	1;
                R.Bottom-= 	1;
                ColorValue* V			= dynamic_cast<ColorValue*>(prop->GetFrontValue()); R_ASSERT(V);
                Surface->Brush->Color 	= (TColor)(V->GetValue()).get_windows();
                Surface->FillRect(R);
            }break;
            case PROP_VCOLOR:{
            	R.Left	-= 	3;
            	R.Bottom+= 	1;
                Surface->Brush->Style = bsSolid;
                Surface->Brush->Color = TColor(0x00000000);
                Surface->FrameRect(R);
                R.Right	-=	1;
                R.Left 	+= 	1;
                R.Top	+=	1;
                R.Bottom-= 	1;
                VectorValue* V			= dynamic_cast<VectorValue*>(prop->GetFrontValue()); R_ASSERT(V);
                Fcolor C; C.set			(V->value->z,V->value->y,V->value->x,0.f);
                Surface->Brush->Color 	= (TColor)C.get();
                Surface->FillRect(R);
            }break;
            case PROP_COLOR:{
            	R.Left	-= 	3;
            	R.Bottom+= 	1;
                Surface->Brush->Style = bsSolid;
                Surface->Brush->Color = TColor(0x00000000);
                Surface->FrameRect(R);
                R.Right	-=	1;
                R.Left 	+= 	1;
                R.Top	+=	1;
                R.Bottom-= 	1;
                U32Value* V=dynamic_cast<U32Value*>(prop->GetFrontValue()); R_ASSERT(V);
                u32 C 	= (U32Value::TYPE)V->GetValue();
                Surface->Brush->Color = (TColor)rgb2bgr(C);
                Surface->FillRect(R);
            }break;
            case PROP_FLAG:{
                FlagValueCustom*  V	= dynamic_cast<FlagValueCustom*>(prop->GetFrontValue()); R_ASSERT(V);
                if (V->HaveCaption())	OutText	(prop->GetDrawText().c_str(),Surface,R,prop->Enabled(),m_BMEllipsis);
                else	        		OutBOOL	(V->m_Flags.is(FlagValueCustom::flInvertedDraw)?!V->GetValueEx():V->GetValueEx(),Surface,R,prop->Enabled());
            }break;
            case PROP_BOOLEAN:{
                BOOLValue* V		= dynamic_cast<BOOLValue*>(prop->GetFrontValue()); R_ASSERT(V);
                OutBOOL				(V->GetValue(),Surface,R,prop->Enabled());
            }break;
            case PROP_CHOOSE:{
				ChooseValue* V		= dynamic_cast<ChooseValue*>(prop->GetFrontValue()); R_ASSERT(V);
                OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled(),m_BMEllipsis);
                if (miDrawThumbnails->Checked&&prop->m_Flags.is(PropItem::flDrawThumbnail)){ 
                    R.top			+=	tvProperties->LineHeight-4;
                    R.left 			= 	R.Right-(R.bottom-R.top);
                    if (!V->OnDrawThumbnailEvent.empty())
                    	V->OnDrawThumbnailEvent(prop->GetDrawText().c_str(),Surface->Handle,Irect().set(R.left,R.top,R.right,R.bottom));
                }
            }break;
            case PROP_TEXTURE2:{
                OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled(),m_BMEllipsis);
                if (miDrawThumbnails->Checked){ 
                    R.top			+=	tvProperties->LineHeight-4;
                    R.left 			= 	R.Right-(R.bottom-R.top);
                    SChooseEvents* E= TfrmChoseItem::GetEvents(smTexture); 
                    if (E&&!E->on_thm.empty())
                    	E->on_thm(prop->GetDrawText().c_str(),Surface->Handle,Irect().set(R.left,R.top,R.right,R.bottom));
                }
            }break;
            case PROP_WAVE:
                OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled(),m_BMEllipsis);
            break;
            case PROP_TOKEN:
            case PROP_RTOKEN:
            case PROP_SH_TOKEN:
            case PROP_RLIST:
            case PROP_CLIST:
                OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled(),m_BMEllipsis);
            break;
            case PROP_TIME:{
                FloatValue* V = dynamic_cast<FloatValue*>(prop->GetFrontValue()); R_ASSERT(V);
                OutText(FloatTimeToStrTime(V->GetValue()).c_str(),Surface,R,prop->Enabled());
            }break;
            case PROP_CTEXT:
            case PROP_RTEXT:
            case PROP_STEXT:
                if (edText->Tag!=(int)Item)
                    OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled(),m_BMEllipsis);
            break;
            case PROP_SHORTCUT:
                if (hkShortcut->Tag!=(int)Item)
                    OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled());
            break;
            case PROP_VECTOR:
            case PROP_GAMETYPE:
                OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled());
            break;
            case PROP_NUMERIC:
                if (seNumber->Tag!=(int)Item)
                    OutText(prop->GetDrawText().c_str(),Surface,R,prop->Enabled());
            break;
            default:
                FATAL("Unknown prop type");
            };
        }
        // show LW edit
        if (!prop->m_Flags.is(PropItem::flDisabled)){
            switch(type){
            case PROP_SHORTCUT:
                if (hkShortcut->Tag==(int)Item) if (!hkShortcut->Visible) ShowSCText(R);
            break;
            case PROP_TIME:
            case PROP_CTEXT:
            case PROP_STEXT:
            case PROP_RTEXT:
                if (edText->Tag==(int)Item) if (!edText->Visible) ShowLWText(R);
            break;
            case PROP_NUMERIC:
                if (seNumber->Tag==(int)Item) if (!seNumber->Visible) ShowLWNumber(R);
            break;
            };
        }
  	}
}
//---------------------------------------------------------------------------

template <class T>
BOOL FlagOnEdit				(PropItem* prop, BOOL& bRes)
{                                                     
    FlagValue<_flags<T> >* V= dynamic_cast<FlagValue<_flags<T> >*>(prop->GetFrontValue());
    if (!V)					return FALSE;
    _flags<T> new_val 		= V->GetValue(); 
    prop->BeforeEdit<FlagValue<_flags<T> >,_flags<T> >(new_val);
    new_val.invert			(V->mask); 
    if (prop->AfterEdit<FlagValue<_flags<T> >,_flags<T> >(new_val))
	    bRes = prop->ApplyValue<FlagValue<_flags<T> >,_flags<T> >	(new_val);
    return TRUE;
}                                                            
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (m_Flags.is(plReadOnly)) return;
    
	CancelEditControl();

	TSTItemPart 	IP=(TSTItemPart)0;
    int				HC=0;
	TElTreeItem* item = tvProperties->GetItemAt(X,Y,IP,HC);
  	if (item){
    	if ((HC==1)&&(Button==mbLeft)){
//        	Log("Shift",(int)Shift.Contains(ssDouble));
            PropItem* prop = (PropItem*)item->Tag;
            // Проверить чтобы не нажимать 2 раза для кнопок
            if (prop&&(PROP_BUTTON==prop->type)) m_FirstClickItem=item;
            if (m_FirstClickItem==item){
				if (!prop||(prop&&!prop->Enabled())) return;
                pmEnum->Tag = (int)item;
                switch(prop->type){
                case PROP_CAPTION: break;
                case PROP_CANVAS: break;
                case PROP_BUTTON:{
                    ButtonValue* FV				= dynamic_cast<ButtonValue*>(prop->GetFrontValue()); R_ASSERT(FV);
                    int btn_num					= iFloor((X-prop->draw_rect.x1)*(float(FV->value.size())/float(prop->draw_rect.width())));
                    for (PropItem::PropValueIt it=prop->values.begin(); it!=prop->values.end(); it++){
                        ButtonValue* V			= dynamic_cast<ButtonValue*>(*it); R_ASSERT(V);
                        V->btn_num				= btn_num;
                    }
                    item->RedrawItem			(true);
                }break;
                case PROP_FLAG:{
                    BOOL bRes 					= FALSE;
                    if (!FlagOnEdit<u8>(prop,bRes))
                        if (!FlagOnEdit<u16>(prop,bRes))
                            if (!FlagOnEdit<u32>(prop,bRes))
                                FATAL			("Unknown flag type");
                    if (bRes){
                        Modified				();
                        RefreshForm				();
                    }
                }break;
                case PROP_BOOLEAN:{
                    BOOLValue* V				= dynamic_cast<BOOLValue*>(prop->GetFrontValue()); R_ASSERT(V);
                    BOOL new_val 				= V->GetValue		();
				    prop->BeforeEdit<BOOLValue,BOOL>(new_val);
                    new_val						= !new_val;
				    if (prop->AfterEdit<BOOLValue,BOOL>(new_val))
                        if (prop->ApplyValue<BOOLValue,BOOL>(new_val)){
                            Modified				();
                            RefreshForm				();
                        }
                }break;
                case PROP_TOKEN:{
                    pmEnum->Items->Clear();
                    TokenValueCustom* T			= dynamic_cast<TokenValueCustom*>(prop->GetFrontValue()); R_ASSERT(T);
                    xr_token* token_list 		= T->token;
                    TMenuItem* mi 				= xr_new<TMenuItem>((TComponent*)0);
                    mi->Caption 				= "-";
                    pmEnum->Items->Add			(mi);
                    for(int i=0; token_list[i].name; i++){
                        mi 			= xr_new<TMenuItem>((TComponent*)0);
                        mi->Tag		= i;
                        mi->Caption = token_list[i].name;
                        mi->OnClick = PMItemClick;
                        pmEnum->Items->Add(mi);
                    }
                }break;
                case PROP_RTOKEN:{
                    pmEnum->Items->Clear();
                    RTokenValueCustom* T		= dynamic_cast<RTokenValueCustom*>(prop->GetFrontValue()); R_ASSERT(T);
                    TMenuItem* mi 				= xr_new<TMenuItem>((TComponent*)0);
                    mi->Caption 				= "-";
                    pmEnum->Items->Add			(mi);
                    for(u32 k=0; k<T->token_count; k++){
                    	xr_rtoken& t= T->token[k];
                        mi 			= xr_new<TMenuItem>((TComponent*)0);
                        mi->Tag		= k;
                        mi->Caption = *t.name;
                        mi->OnClick = PMItemClick;
                        pmEnum->Items->Add(mi);
                    }
                }break;
                case PROP_SH_TOKEN:{
                    pmEnum->Items->Clear();
                    TokenValueSH* T	= dynamic_cast<TokenValueSH*>(prop->GetFrontValue()); R_ASSERT(T);
                    TMenuItem* mi 	= xr_new<TMenuItem>((TComponent*)0);
                    mi->Caption 	= "-";
                    pmEnum->Items->Add(mi);
                    for (u32 i=0; i<T->cnt; i++){
                        mi 			= xr_new<TMenuItem>((TComponent*)0);
                        mi->Tag		= i;
                        mi->Caption = T->items[i].str;
                        mi->OnClick = PMItemClick;
                        pmEnum->Items->Add(mi);
                    }
                }break;
                case PROP_CLIST:{
                    pmEnum->Items->Clear();
                    CListValue* T				= dynamic_cast<CListValue*>(prop->GetFrontValue()); R_ASSERT(T);
                    TMenuItem* mi	= xr_new<TMenuItem>((TComponent*)0);
                    mi->Caption 	= "-";
                    pmEnum->Items->Add(mi);
                    for(u32 k=0; k<T->item_count; k++){
                        mi 			= xr_new<TMenuItem>((TComponent*)0);
                        mi->Tag		= k;
                        mi->Caption = T->items[k].c_str();
                        mi->OnClick = PMItemClick;
                        pmEnum->Items->Add(mi);
                    }
                }break;
                case PROP_RLIST:{
                    pmEnum->Items->Clear();
                    RListValue* T				= dynamic_cast<RListValue*>(prop->GetFrontValue()); R_ASSERT(T);
                    TMenuItem* mi	= xr_new<TMenuItem>((TComponent*)0);
                    mi->Caption 	= "-";
                    pmEnum->Items->Add(mi);
                    for(u32 k=0; k<T->item_count; k++){
                        mi 			= xr_new<TMenuItem>((TComponent*)0);
                        mi->Tag		= k;
                        mi->Caption = T->items[k].c_str();
                        mi->OnClick = PMItemClick;
                        pmEnum->Items->Add(mi);
                    }
                }break;
                case PROP_VECTOR: 			VectorClick		(item); 	break;
                case PROP_WAVE: 			WaveFormClick	(item); 	break;
                case PROP_VCOLOR:
                case PROP_FCOLOR:
                case PROP_COLOR: 			ColorClick		(item); 	break;
                case PROP_CHOOSE:			ChooseClick		(item); 	break;
                case PROP_NUMERIC:			PrepareLWNumber	(item);		break;
                case PROP_GAMETYPE:			GameTypeClick	(item);		break;
	            case PROP_CTEXT:
                case PROP_STEXT:
                case PROP_RTEXT:
                	if ((X-prop->draw_rect.x1)<(prop->draw_rect.width()-(m_BMEllipsis->Width+2)))	PrepareLWText(item);
                    else								                   							ExecTextEditor(prop);
                break;
                case PROP_SHORTCUT:
                	PrepareSCText(item);
                break;
                case PROP_TIME:
                	PrepareLWText(item);
                break;
                case PROP_TEXTURE2:{
                    pmEnum->Items->Clear();
                    TMenuItem* mi	= xr_new<TMenuItem>((TComponent*)0);
                    mi->Caption 	= "-";
                    pmEnum->Items->Add(mi);
                    for (u32 i=0; i<TSTRING_COUNT; i++){
                        mi = xr_new<TMenuItem>((TComponent*)0);
                        mi->Tag		= i;
                        mi->Caption = TEXTUREString[i];
                        mi->OnClick = PMItemClick;
                        pmEnum->Items->Add(mi);
                    }
                }break;
                default:
                    FATAL("Unknown prop type");
                };
                switch(prop->type){
                case PROP_TOKEN:
                case PROP_RTOKEN:
                case PROP_SH_TOKEN:
                case PROP_CLIST:
                case PROP_RLIST:
                case PROP_TEXTURE2:
                    TPoint P; P.x = X; P.y = Y;
                    P=tvProperties->ClientToScreen(P);
                    pmEnum->Popup(P.x,P.y-10);
                    break;
                };
            }
	        if (prop&&!prop->OnClickEvent.empty()) prop->OnClickEvent(prop);
        }else if (Button==mbRight){
            TPoint P; P.x = X; P.y = Y;
            P=tvProperties->ClientToScreen(P);
            pmItems->Popup(P.x,P.y-10);
        }
    };
    m_FirstClickItem = item;
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
	if (m_Flags.is(plReadOnly)) return;
	if (tvProperties->Selected){
		TElTreeItem* item 	= tvProperties->Selected;
		PropItem* prop 		= (PropItem*)item->Tag;
		if (!prop||(prop&&!prop->Enabled())) return;
        switch(prop->type){
        case PROP_BUTTON:{
        	if (Shift.Contains(ssLeft)){
                ButtonValue* V				= dynamic_cast<ButtonValue*>(prop->GetFrontValue()); R_ASSERT(V);
                Y							-= tvProperties->HeaderHeight;
                int btn_num					= -1;
                if (((Y>prop->draw_rect.y1)&&(Y<prop->draw_rect.y2))&&((X>prop->draw_rect.x1)&&(Y<prop->draw_rect.x2)))
                	btn_num					= iFloor((X-prop->draw_rect.x1)*(float(V->value.size())/float(prop->draw_rect.width())));
                for (PropItem::PropValueIt it=prop->values.begin(); it!=prop->values.end(); it++){
                    ButtonValue* V			= dynamic_cast<ButtonValue*>(*it); R_ASSERT(V);
                    V->btn_num				= btn_num;
                }
                item->RedrawItem			(true);
            }
        }break;
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (m_Flags.is(plReadOnly)) return;
	if (tvProperties->Selected){
		TElTreeItem* item 	= tvProperties->Selected;
		PropItem* prop 		= (PropItem*)item->Tag;
		if (!prop||(prop&&!prop->Enabled())) return;
        switch(prop->type){
        case PROP_BUTTON:{
        	if (Button==mbLeft){
				bool bRes 	= false;
                bool bSafe	= false;
                for (PropItem::PropValueIt it=prop->Values().begin(); it!=prop->Values().end(); it++){
                    ButtonValue* V			= dynamic_cast<ButtonValue*>(*it); R_ASSERT(V);
                    if (V->btn_num>-1){
	                    bRes 				|= V->OnBtnClick(bSafe);
    	                V->btn_num			= -1;
        	            if (V->m_Flags.is(ButtonValue::flFirstOnly)) break;
                    }
                }
                if (bRes){
                    Modified			();
                    if (!bSafe)			RefreshForm		();
                }
                if (!bSafe)				item->RedrawItem(true);
            }
        }break;
        }
    }
}
//---------------------------------------------------------------------------
template <class T>
BOOL TokenOnEdit	   		(PropItem* prop, u32 _new_val, BOOL& bRes)
{                                                     
    TokenValue<T>* V		= dynamic_cast<TokenValue<T>*>(prop->GetFrontValue());
    if (!V)					return FALSE;
    T new_val				= _new_val;
    if (prop->AfterEdit<TokenValue<T>,T>(new_val))
    	bRes 				= prop->ApplyValue<TokenValue<T>,T>	(new_val);
    return TRUE;
}
template <class T>
BOOL RTokenOnEdit	   		(PropItem* prop, u32 _new_val, BOOL& bRes)
{                                                     
    RTokenValue<T>* V		= dynamic_cast<RTokenValue<T>*>(prop->GetFrontValue());
    if (!V)					return FALSE;
    T new_val				= _new_val;
    if (prop->AfterEdit<RTokenValue<T>,T>(new_val))
	    bRes 				= prop->ApplyValue<RTokenValue<T>,T>	(new_val);
    return TRUE;
}
//---------------------------------------------------------------------------

void __fastcall TProperties::PMItemClick(TObject *Sender)
{
    TMenuItem* mi = dynamic_cast<TMenuItem*>(Sender);
    if (mi){
        TElTreeItem* item = (TElTreeItem*)pmEnum->Tag;
		PropItem* prop = (PropItem*)item->Tag;
        switch(prop->Type()){
		case PROP_TOKEN:{
			TokenValueCustom* V		= dynamic_cast<TokenValueCustom*>(prop->GetFrontValue()); R_ASSERT(V);
            xr_token* token_list   	= V->token;
            BOOL bRes 				= FALSE;
            u32 new_val				= token_list[mi->Tag].id;
            if (!TokenOnEdit<u8>(prop,new_val,bRes))
                if (!TokenOnEdit<u16>(prop,new_val,bRes))
                    if (!TokenOnEdit<u32>(prop,new_val,bRes))
                        FATAL		("Unknown token type");
            if (bRes){
                Modified			();
            }
			item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
        }break;
		case PROP_RTOKEN:{
			RTokenValueCustom* V	= dynamic_cast<RTokenValueCustom*>(prop->GetFrontValue()); R_ASSERT(V);
            BOOL bRes 				= FALSE;
            u32 new_val				= V->token[mi->Tag].id;
            if (!RTokenOnEdit<u8>(prop,new_val,bRes))
                if (!RTokenOnEdit<u16>(prop,new_val,bRes))
                    if (!RTokenOnEdit<u32>(prop,new_val,bRes))
                        FATAL		("Unknown rtoken type");
            if (bRes){
                Modified			();
            }
			item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
        }break;
		case PROP_SH_TOKEN:{
			TokenValueSH* V			= dynamic_cast<TokenValueSH*>(prop->GetFrontValue()); R_ASSERT(V);
            u32 new_val				= V->items[mi->Tag].ID;
		    if (prop->AfterEdit<TokenValueSH,u32>(new_val))
                if (prop->ApplyValue<TokenValueSH,u32>(new_val)){
                    Modified			();
                }
			item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
        }break;
		case PROP_RLIST:{
			RListValue* V			= dynamic_cast<RListValue*>(prop->GetFrontValue()); R_ASSERT(V);
            shared_str new_val			= V->items[mi->Tag];
            if (prop->AfterEdit<RListValue,shared_str>(new_val))
                if (prop->ApplyValue<RListValue,shared_str>(new_val)){
                    Modified			();
                }
			item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
        }break;
		case PROP_CLIST:{
			CListValue* V			= dynamic_cast<CListValue*>(prop->GetFrontValue()); R_ASSERT(V);
            xr_string new_val		= V->items[mi->Tag];
            if (prop->AfterEdit<CListValue,xr_string>(new_val))
                if (prop->ApplyValue<CListValue,LPCSTR>(new_val.c_str())){
                    Modified			();
                }
			item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
        }break;
		case PROP_TEXTURE2:{
			CTextValue* T			= dynamic_cast<CTextValue*>(prop->GetFrontValue()); R_ASSERT(T);
			xr_string edit_val	 = T->GetValue();
		    prop->BeforeEdit<CTextValue,xr_string>(edit_val);
            LPCSTR new_val 		 	= 0;
            bool bRes				= true;
        	if (mi->Tag==0){
            	bRes				= TfrmChoseItem::SelectItem(smTexture,new_val,8,edit_val.c_str());
            }else if (mi->Tag>=2){
            	new_val			 	= TEXTUREString[mi->Tag];
            }
            if (bRes){
                edit_val		 	= new_val;
                if (prop->AfterEdit<CTextValue,xr_string>(edit_val)){
                    if (prop->ApplyValue<CTextValue,LPCSTR>(edit_val.c_str()))  
                        Modified		();
                    item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
                }
            }
        }break;
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TProperties::WaveFormClick(TElTreeItem* item)
{
	PropItem* prop 			= (PropItem*)item->Tag;
    R_ASSERT(PROP_WAVE==prop->type);

	WaveValue* V			= dynamic_cast<WaveValue*>(prop->GetFrontValue()); R_ASSERT(V);
    WaveForm edit_val		= V->GetValue();
    prop->BeforeEdit<WaveValue,WaveForm>(edit_val);
	if (TfrmShaderFunction::Run(&edit_val)==mrOk){
        if (prop->AfterEdit<WaveValue,WaveForm>(edit_val))
            if (prop->ApplyValue<WaveValue,WaveForm>(edit_val)){
                Modified		();
            }
        item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
    }
}
//---------------------------------------------------------------------------
extern "C" DLL_API bool FSColorPickerExecute(u32* currentColor, LPDWORD originalColor, const int initialExpansionState);

void __fastcall TProperties::ColorClick(TElTreeItem* item)
{
	PropItem* prop = (PropItem*)item->Tag;
    switch (prop->type){
    case PROP_FCOLOR:{
		ColorValue* V		= dynamic_cast<ColorValue*>(prop->GetFrontValue()); R_ASSERT(V);
        Fcolor edit_val		= V->GetValue();
        prop->BeforeEdit<ColorValue,Fcolor>(edit_val);
        u32 ev 				= edit_val.get();
        u32 a 				= color_get_A(ev);
        if (SelectColor(&ev)){
        	ev		= subst_alpha(ev,a);
	        edit_val.set	(ev);
            if (prop->AfterEdit<ColorValue,Fcolor>(edit_val))
                if (prop->ApplyValue<ColorValue,Fcolor>(edit_val)){
                    item->RedrawItem(true);
                    Modified		();
                }
        }
    }break;
    case PROP_VCOLOR:{
		VectorValue* V		= dynamic_cast<VectorValue*>(prop->GetFrontValue()); R_ASSERT(V);
        Fvector edit_val	= V->GetValue();
        prop->BeforeEdit<VectorValue,Fvector>(edit_val);
		Fcolor C; 			C.set(edit_val.x,edit_val.y,edit_val.z,1.f);
        u32 ev 				= C.get();
        if (SelectColor(&ev)){
        	C.set			(ev);
	        edit_val.set	(C.r,C.g,C.b);
            if (prop->AfterEdit<VectorValue,Fvector>(edit_val))
                if (prop->ApplyValue<VectorValue,Fvector>(edit_val)){
                    item->RedrawItem(true);
                    Modified		();
                }
        }
    }break;
    case PROP_COLOR:{
		U32Value* V			= dynamic_cast<U32Value*>(prop->GetFrontValue()); R_ASSERT(V);
        u32 edit_val		= V->GetValue();
        prop->BeforeEdit<U32Value,u32>(edit_val);
        u32 a 				= color_get_A(edit_val);
        if (SelectColor(&edit_val)){
        	edit_val		= subst_alpha(edit_val,a);
            if (prop->AfterEdit<U32Value,u32>(edit_val))
                if (prop->ApplyValue<U32Value,u32>(edit_val)){
                    item->RedrawItem(true);
                    Modified		();
                }
        }
    }break;
    default: FATAL("Unsupported type");
    }
}
//---------------------------------------------------------------------------
void __fastcall TProperties::GameTypeClick(TElTreeItem* item)
{
    PropItem* prop 				= (PropItem*)item->Tag;
	GameTypeValue* V			= dynamic_cast<GameTypeValue*>(prop->GetFrontValue());
    R_ASSERT					(V);
    GameTypeChooser edit_val	= V->GetValue();

    prop->BeforeEdit<GameTypeValue,GameTypeChooser>(edit_val);
	if (gameTypeRun(AnsiString(item->Text).c_str(),&edit_val))
    {
        if (prop->AfterEdit<GameTypeValue,GameTypeChooser>(edit_val))
            if (prop->ApplyValue<GameTypeValue,GameTypeChooser>(edit_val))
            {
                item->RedrawItem(true);
                Modified		();
            }
    }
}

void __fastcall TProperties::VectorClick(TElTreeItem* item)
{
    PropItem* prop 	= (PropItem*)item->Tag;
	VectorValue* V	= dynamic_cast<VectorValue*>(prop->GetFrontValue()); R_ASSERT(V);
    Fvector edit_val= V->GetValue();
    prop->BeforeEdit<VectorValue,Fvector>(edit_val);
	if (NumericVectorRun(AnsiString(item->Text).c_str(),&edit_val,V->dec,&edit_val,&V->lim_mn,&V->lim_mx)){
        if (prop->AfterEdit<VectorValue,Fvector>(edit_val))
            if (prop->ApplyValue<VectorValue,Fvector>(edit_val)){
                item->RedrawItem(true);
                Modified		();
            }
    }
}
//---------------------------------------------------------------------------

void __fastcall TProperties::ChooseClick(TElTreeItem* item)
{
	PropItem* prop			= (PropItem*)item->Tag;

    ChooseValue* V			= dynamic_cast<ChooseValue*>(prop->GetFrontValue()); VERIFY(V);
    shared_str	edit_val  	= V->GetValue();
	if (!edit_val.size()) 	edit_val = V->m_StartPath;
    prop->BeforeEdit<ChooseValue,shared_str>(edit_val);
	//
    ChooseItemVec			items;
    if (!V->OnChooseFillEvent.empty()){
        V->m_Items			= &items;
        V->OnChooseFillEvent(V);
    }    
    //
    LPCSTR new_val			= 0;
    if (TfrmChoseItem::SelectItem(V->m_ChooseID,new_val,V->subitem,edit_val.c_str(),0,V->m_FillParam,0,items.size()?&items:0,V->m_ChooseFlags)){
        edit_val			= new_val;
        if (prop->AfterEdit<ChooseValue,shared_str>(edit_val))
            if (prop->ApplyValue<ChooseValue,shared_str>(edit_val)){
                Modified   	();
            }
        item->ColumnText->Strings[0]= prop->GetDrawText().c_str();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// LW style inplace editor
//---------------------------------------------------------------------------
void TProperties::CancelLWNumber()
{
    HideLWNumber();
}

void TProperties::HideLWNumber()
{
	// последовательность важна (может быть 2 Apply)
    seNumber->Tag	= 0;
    if (seNumber->Visible&&Visible) 	tvProperties->SetFocus();
    seNumber->Hide	();
}
//---------------------------------------------------------------------------
template <class T>
BOOL NumericBeforeEdit 		(PropItem* prop, TMultiObjSpinEdit* seNumber)
{                                                     
    NumericValue<T>* V		= dynamic_cast<NumericValue<T>*>(prop->GetFrontValue());
    if (!V)					return FALSE;
    T edit_val        		= V->GetValue();
    prop->BeforeEdit<NumericValue<T>,T>(edit_val);
    seNumber->MinValue 		= V->lim_mn;
    seNumber->MaxValue	 	= V->lim_mx;
    seNumber->Increment		= V->inc;
    seNumber->LWSensitivity	= V->dec?0.1f:0.01f;
    seNumber->Decimal  		= V->dec;
    seNumber->ValueType		= V->dec?vtFloat:vtInt;
    seNumber->Value 		= edit_val;
    return TRUE;
}
void TProperties::PrepareLWNumber(TElTreeItem* item)
{
	PropItem* prop = (PropItem*)item->Tag;
    if (!NumericBeforeEdit<u8>(prop,seNumber))
        if (!NumericBeforeEdit<u16>(prop,seNumber))
            if (!NumericBeforeEdit<u32>(prop,seNumber))
                if (!NumericBeforeEdit<s8>(prop,seNumber))
                    if (!NumericBeforeEdit<s16>(prop,seNumber))
                        if (!NumericBeforeEdit<s32>(prop,seNumber))
                            if (!NumericBeforeEdit<float>(prop,seNumber))
                                FATAL("Unknown numeric type");
    seNumber->Tag 	= (int)item;
    tvProperties->Refresh();
}
void TProperties::ShowLWNumber(TRect& R)
{
    seNumber->Left 	= R.Left;
    seNumber->Top  	= R.Top+tvProperties->HeaderHeight;
    seNumber->Width	= R.Right-R.Left+2;
    seNumber->Height= R.Bottom-R.Top+2;
	seNumber->ButtonWidth = seNumber->Height;
    seNumber->Show();
    seNumber->SetFocus();
}

template <class T>
BOOL NumericOnEdit	   		(PropItem* prop, T new_val, BOOL& bRes)
{                                                     
    NumericValue<T>* V		= dynamic_cast<NumericValue<T>*>(prop->GetFrontValue());
    if (!V)					return FALSE;
    if (prop->AfterEdit<NumericValue<T>,T>	(T(new_val)))
	    bRes 				= prop->ApplyValue<NumericValue<T>,T>	(T(new_val));
    return TRUE;
}

void TProperties::ApplyLWNumber()
{
	TElTreeItem* item 			= (TElTreeItem*)seNumber->Tag;
    seNumber->Tag				= 0;
    if (item){
		PropItem* prop 			= (PropItem*)item->Tag;
        seNumber->Update();
        BOOL bRes				= FALSE;
        if (!NumericOnEdit<u8>(prop,seNumber->Value,bRes))
            if (!NumericOnEdit<u16>(prop,seNumber->Value,bRes))
                if (!NumericOnEdit<u32>(prop,seNumber->Value,bRes))
                    if (!NumericOnEdit<s8>(prop,seNumber->Value,bRes))
                        if (!NumericOnEdit<s16>(prop,seNumber->Value,bRes))
                            if (!NumericOnEdit<s32>(prop,seNumber->Value,bRes))
	                            if (!NumericOnEdit<float>(prop,seNumber->Value,bRes))
				                    FATAL("Unknown numeric type");
        if (bRes){
            Modified			();
        }
		item->ColumnText->Strings[0] = prop->GetDrawText().c_str();
    }
}

void __fastcall TProperties::seNumberExit(TObject *Sender)
{
	ApplyLWNumber();
	HideLWNumber();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::seNumberKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if (VK_RETURN==Key){
		ApplyLWNumber();
		HideLWNumber();
    }else if (VK_ESCAPE==Key){
		CancelLWNumber();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Textinplace editor
//---------------------------------------------------------------------------
void TProperties::CancelLWText()
{
    HideLWText();
}

void TProperties::HideLWText()
{
	// последовательность важна (может быть 2 Apply)
    edText->Tag		= 0;
    if (edText->Visible&&Visible) 	tvProperties->SetFocus();
    edText->Hide	();
}
//---------------------------------------------------------------------------
void TProperties::PrepareLWText(TElTreeItem* item)
{
	PropItem* prop 		= (PropItem*)item->Tag;
    switch (prop->type){
    case PROP_CTEXT:{
		CTextValue* V		= dynamic_cast<CTextValue*>(prop->GetFrontValue()); R_ASSERT(V);
        xr_string edit_val= V->GetValue();
	    prop->BeforeEdit<CTextValue,xr_string>(edit_val);
        edText->EditMask	= "";
		edText->Text 		= edit_val.c_str();
		edText->MaxLength	= V->lim;
	}break;
    case PROP_STEXT:{
		STextValue* V		= dynamic_cast<STextValue*>(prop->GetFrontValue()); R_ASSERT(V);
        xr_string edit_val= V->GetValue();
	    prop->BeforeEdit<STextValue,xr_string>(edit_val);
        edText->EditMask	= "";
		edText->Text 		= edit_val.c_str();
		edText->MaxLength	= 0;
    }break;
    case PROP_RTEXT:{
		RTextValue* V		= dynamic_cast<RTextValue*>(prop->GetFrontValue()); R_ASSERT(V);
        shared_str edit_val	= V->GetValue();
	    prop->BeforeEdit<RTextValue,shared_str>(edit_val);
        edText->EditMask	= "";
		edText->Text 		= edit_val.c_str();
		edText->MaxLength	= 0;
    }break;
    case PROP_TIME:{
		FloatValue* V		= dynamic_cast<FloatValue*>(prop->GetFrontValue()); R_ASSERT(V);
        float edit_val		= V->GetValue();
	    prop->BeforeEdit<FloatValue,float>(edit_val);
        edText->EditMask	= "!90:00:00;1;_";
		edText->Text 		= FloatTimeToStrTime(edit_val);
		edText->MaxLength	= 0;
    }break;
    }
    edText->Tag 	= (int)item;
    tvProperties->Refresh();
}
void TProperties::ShowLWText(TRect& R)
{
    edText->Left 	= R.Left-1;
    edText->Top  	= R.Top+tvProperties->HeaderHeight;
    edText->Width	= R.Right-R.Left+0;
    edText->Height	= R.Bottom-R.Top+2;
    edText->Show	();
    edText->SetFocus();
}

void TProperties::ApplyLWText()
{
	TElTreeItem* item 			= (TElTreeItem*)edText->Tag;
    edText->Tag					= 0;
    if (item){
		PropItem* prop 			= (PropItem*)item->Tag;
        edText->Update();
	    switch (prop->type){
        case PROP_CTEXT:{
			CTextValue* V		= dynamic_cast<CTextValue*>(prop->GetFrontValue()); R_ASSERT(V);
			xr_string new_val	= AnsiString(edText->Text).c_str();
            if (prop->AfterEdit<CTextValue,xr_string>(new_val))
                if (prop->ApplyValue<CTextValue,LPCSTR>(new_val.c_str())){
                    Modified();
                }
            item->ColumnText->Strings[0] = prop->GetDrawText().c_str();
        }break;
        case PROP_STEXT:{
			STextValue* V		= dynamic_cast<STextValue*>(prop->GetFrontValue()); R_ASSERT(V);
			xr_string new_val	= AnsiString(edText->Text).c_str();
            if (prop->AfterEdit<STextValue,xr_string>(new_val))
                if (prop->ApplyValue<STextValue,xr_string>(new_val)){
                    Modified();
                }
            item->ColumnText->Strings[0] = prop->GetDrawText().c_str();
        }break;
        case PROP_RTEXT:{
			RTextValue* V		= dynamic_cast<RTextValue*>(prop->GetFrontValue()); R_ASSERT(V);
			shared_str new_val	= AnsiString(edText->Text).c_str();
            if (prop->AfterEdit<RTextValue,shared_str>(new_val))
                if (prop->ApplyValue<RTextValue,shared_str>(new_val)){
                    Modified();
                }
            item->ColumnText->Strings[0] = prop->GetDrawText().c_str();
        }break;
        case PROP_TIME:{
			FloatValue* V		= dynamic_cast<FloatValue*>(prop->GetFrontValue()); R_ASSERT(V);
			float new_val		= StrTimeToFloatTime(edText->Text.c_str());
            if (prop->AfterEdit<FloatValue,float>(new_val))
                if (prop->ApplyValue<FloatValue,float>(new_val)){
                    Modified		();
                }
            item->ColumnText->Strings[0] = FloatTimeToStrTime(V->GetValue());
		}break;        
    	}
    }
}

void __fastcall TProperties::edTextExit(TObject *Sender)
{
	ApplyLWText();
	HideLWText();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::edTextKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if (VK_RETURN==Key){
		ApplyLWText();
		HideLWText();
    }else if (VK_ESCAPE==Key){
		CancelLWText();
    }
}
//---------------------------------------------------------------------------

void __fastcall TProperties::edTextDblClick(TObject *Sender)
{
	TElTreeItem* item 		= (TElTreeItem*)edText->Tag;
    if (item){
		PropItem* prop 		= (PropItem*)item->Tag;
        edText->Update();
	    switch (prop->type){
    	case PROP_CTEXT:
        case PROP_STEXT:
    	case PROP_RTEXT:{
			AnsiString new_val	= edText->Text;
			if (TfrmText::RunEditor(new_val,AnsiString(item->Text).c_str()))
            	edText->Text = new_val;
            ApplyLWText();
            HideLWText();
        }break;
    	}
    }
}
//---------------------------------------------------------------------------

void TProperties::ExecTextEditor(PropItem* prop)
{
    if (prop){
	    switch (prop->type){
    	case PROP_CTEXT:{
            CTextValue* V	= dynamic_cast<CTextValue*>(prop->GetFrontValue()); R_ASSERT(V);
            xr_string edit_val= V->GetValue();
		    prop->BeforeEdit<CTextValue,xr_string>(edit_val);
            AnsiString tmp	= edit_val.c_str();
            if (TfrmText::RunEditor(tmp,AnsiString(prop->Item()->Text).c_str(),false)){
            	edit_val	= tmp.c_str();
			    if (prop->AfterEdit<CTextValue,xr_string>(edit_val))
                    if (prop->ApplyValue<CTextValue,LPCSTR>(edit_val.c_str()))
                        Modified();
                prop->Item()->ColumnText->Strings[0] = prop->GetDrawText().c_str();
            }
        }break;
    	case PROP_STEXT:{
            STextValue* V	= dynamic_cast<STextValue*>(prop->GetFrontValue()); R_ASSERT(V);
            xr_string edit_val= V->GetValue();
		    prop->BeforeEdit<STextValue,xr_string>(edit_val);
            AnsiString tmp	= edit_val.c_str();
            if (TfrmText::RunEditor(tmp,AnsiString(prop->Item()->Text).c_str(),false)){
            	edit_val	= tmp.c_str();
			    if (prop->AfterEdit<STextValue,xr_string>(edit_val))
                    if (prop->ApplyValue<STextValue,xr_string>(edit_val))
                        Modified();
                prop->Item()->ColumnText->Strings[0] = prop->GetDrawText().c_str();
            }
        }break;
    	case PROP_RTEXT:{
            RTextValue* V	= dynamic_cast<RTextValue*>(prop->GetFrontValue()); R_ASSERT(V);
            shared_str edit_val= V->GetValue();
		    prop->BeforeEdit<RTextValue,shared_str>(edit_val);
            AnsiString tmp	= edit_val.c_str();
            if (TfrmText::RunEditor(tmp,AnsiString(prop->Item()->Text).c_str(),false)){
            	edit_val	= tmp.c_str();
			    if (prop->AfterEdit<RTextValue,shared_str>(edit_val))
                    if (prop->ApplyValue<RTextValue,shared_str>(edit_val))
                        Modified();
                prop->Item()->ColumnText->Strings[0] = prop->GetDrawText().c_str();
            }
        }break;
    	}
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Shortcut editor
//---------------------------------------------------------------------------
void TProperties::CancelSCText()
{
    HideSCText();
}

void TProperties::HideSCText()
{
	// последовательность важна (может быть 2 Apply)
    hkShortcut->Tag		= 0;
    if (hkShortcut->Visible&&Visible) 	tvProperties->SetFocus();
    hkShortcut->Hide	();
}
void TProperties::PrepareSCText(TElTreeItem* item)
{
	PropItem* prop 		= (PropItem*)item->Tag;
    switch (prop->type){
    case PROP_SHORTCUT:{
		ShortcutValue* V		= dynamic_cast<ShortcutValue*>(prop->GetFrontValue()); R_ASSERT(V);
        xr_shortcut edit_val	= V->GetValue();
	    prop->BeforeEdit<ShortcutValue,xr_shortcut>(edit_val);
        hkShortcut->HotKey		= edit_val.hotkey;
    }break;
    }
    hkShortcut->Tag 	= (int)item;
    tvProperties->Refresh();
}
void TProperties::ShowSCText(TRect& R)
{
    hkShortcut->Left 	= R.Left;
    hkShortcut->Top  	= R.Top+tvProperties->HeaderHeight;
    hkShortcut->Width	= R.Right-R.Left+0;
    hkShortcut->Height	= R.Bottom-R.Top+2;

    hkShortcut->Show	();
    hkShortcut->SetFocus();
}

void TProperties::ApplySCText()
{
	TElTreeItem* item 			= (TElTreeItem*)hkShortcut->Tag;
    hkShortcut->Tag					= 0;
    if (item){
		PropItem* prop 			= (PropItem*)item->Tag;
        hkShortcut->Update();
        switch (prop->type){
        case PROP_SHORTCUT:{
            ShortcutValue* V  	= dynamic_cast<ShortcutValue*>(prop->GetFrontValue()); R_ASSERT(V);
            xr_shortcut new_val;
            new_val.hotkey		= hkShortcut->HotKey;
            if (prop->AfterEdit<ShortcutValue,xr_shortcut>(new_val))
                if (prop->ApplyValue<ShortcutValue,xr_shortcut>(new_val)){
                    Modified		();
                }
            item->ColumnText->Strings[0] = V->GetDrawText(0).c_str();
		}break;        
    	}
    }
}
void __fastcall TProperties::hkShortcut_Exit(TObject *Sender)
{
	ApplySCText();
	HideSCText();
}
void __fastcall TProperties::hkShortcut_KeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
/*
	if (VK_RETURN==Key){
		ApplySCText();
		HideSCText();
    }else if (VK_ESCAPE==Key){
		CancelSCText();
    }
*/
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesItemFocused(TObject *Sender)
{
	if (!OnItemFocused.empty()) 	OnItemFocused(tvProperties->Selected);
	if (tvProperties->Selected){
        PropItem* prop 		= (PropItem*)tvProperties->Selected->Tag;
        if (prop&&!prop->OnItemFocused.empty())prop->OnItemFocused(prop);
    }
	m_FirstClickItem 	= 0;
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesHeaderColumnResize(
      TObject *Sender, int SectionIndex)
{
	ApplyEditControl();
}
//---------------------------------------------------------------------------

void TProperties::ApplyEditControl()
{
	ApplyLWText		();
	HideLWText		();
	ApplyLWNumber	();
	HideLWNumber	();
	ApplySCText		();
	HideSCText		();
}
//---------------------------------------------------------------------------

void TProperties::CancelEditControl()
{
	CancelLWNumber	();
	CancelLWText	();
	CancelSCText	();
}
//---------------------------------------------------------------------------

bool __fastcall TProperties::IsModified()
{
	ApplyEditControl();
	return bModified;
}
//---------------------------------------------------------------------------

void __fastcall TProperties::FormDeactivate(TObject *Sender)
{
	ApplyEditControl();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::FormShow(TObject *Sender)
{
	// check window position
	CheckWindowPos	(this);
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesItemChange(TObject *Sender,
      TElTreeItem *Item, TItemChangeMode ItemChangeMode)
{
	if (Item&&(icmCheckState==ItemChangeMode)){
		PropItem* prop 			= (PropItem*)Item->Tag;
	    if (prop){
        	prop->m_Flags.set	(PropItem::flCBChecked,Item->Checked);
            prop->OnChange		();
			Modified			();
    	}
	    tvProperties->Refresh	();
    }
}
//---------------------------------------------------------------------------

void __fastcall TProperties::fsStorageRestorePlacement(TObject *Sender)
{
    RestoreParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TProperties::fsStorageSavePlacement(TObject *Sender)
{
	SaveParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TProperties::ExpandSelected1Click(TObject *Sender)
{
	if (tvProperties->Selected) tvProperties->Selected->Expand(false);
}
//---------------------------------------------------------------------------

void __fastcall TProperties::CollapseSelected1Click(TObject *Sender)
{
	if (tvProperties->Selected) tvProperties->Selected->Collapse(false);
}
//---------------------------------------------------------------------------

void __fastcall TProperties::ExpandAll1Click(TObject *Sender)
{
	tvProperties->FullExpand();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::CollapseAll1Click(TObject *Sender)
{
	tvProperties->FullCollapse();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::miDrawThumbnailsClick(TObject *Sender)
{
	RefreshForm();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::miAutoExpandClick(TObject *Sender)
{
	RefreshForm();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::RefreshForm()
{
	LockUpdating		();
    for (PropItemIt it=m_ViewItems.begin(); it!=m_ViewItems.end(); it++){
    	PropItem* prop = *it;
    	if (prop&&prop->item&&prop->m_Flags.is(PropItem::flDrawThumbnail)) 
        	prop->Item()->OwnerHeight = !miDrawThumbnails->Checked;
    }
    if (miAutoExpand->Checked) tvProperties->FullExpand();
	UnlockUpdating		();
	tvProperties->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TProperties::ebOKClick(TObject *Sender)
{
	ModalResult = mrOk;	
}
//---------------------------------------------------------------------------

void __fastcall TProperties::ebCancelClick(TObject *Sender)
{
	ModalResult = mrCancel;	
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesShowLineHint(TObject *Sender,
      TElTreeItem *Item, TElHeaderSection *Section, TElFString &Text,
      THintWindow *HintWindow, TPoint &MousePos, bool &DoShowHint)
{
    PropItem* prop 				= (PropItem*)Item->Tag;
    if (prop){
//    	HintWindow->Brush->Color= clGray;
		Text					= prop->GetDrawText().c_str();
    }
}
//---------------------------------------------------------------------------

void __fastcall TProperties::tvPropertiesCompareItems(TObject *Sender,
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

PropItem* TProperties::FindItem(const shared_str& name)
{
	return PHelper().FindItem(m_Items,name,PROP_UNDEF);
}
//---------------------------------------------------------------------------

