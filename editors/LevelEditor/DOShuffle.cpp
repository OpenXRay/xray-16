//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "DOShuffle.h"
#include "../ECore/Editor/EThumbnail.h"
#include "xr_trims.h"
#include "../ECore/Editor/Library.h"
#include "DOOneColor.h"
#include "../ECore/Editor/EditObject.h"
#include "Scene.h"
#include "../xrEProps/folderlib.h"
#include "ESceneDOTools.h"
#include "../ECore/Editor/ui_main.h"
#include "../ECore/Editor/ImageManager.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmDOShuffle *TfrmDOShuffle::form=0;

//static DDVec DOData;
/*
SDOData::SDOData(){
	DOData.push_back(this);
}
*/
//---------------------------------------------------------------------------
// Constructors
//---------------------------------------------------------------------------
bool __fastcall TfrmDOShuffle::Run()
{
	VERIFY(!form);
	form = xr_new<TfrmDOShuffle>((TComponent*)0,dynamic_cast<EDetailManager*>(Scene->GetTool(OBJCLASS_DO)));
	// show
    return (form->ShowModal()==mrOk);
}
//---------------------------------------------------------------------------

void TfrmDOShuffle::OnObjectPropsModified()
{
	bObjectModif = true;
//	TElTreeItem* N 		= tvItems->Selected;
//	tvItems->Selected 	= 0;
//	tvItems->Selected 	= N;
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::FormCreate(TObject *Sender)
{
	m_ObjectProps 		= TProperties::CreateForm("Objects",paObjectProps,alClient,fastdelegate::bind<TOnChooseClose>(this,&TfrmDOShuffle::OnObjectPropsModified));
    bTHMLockRepaint		= false;
    bLockFocused		= false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::FormDestroy(TObject *Sender)
{
	TProperties::DestroyForm(m_ObjectProps);
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::FormShow(TObject *Sender)
{
	bColorIndModif 		= false;
    bObjectModif		= false;
    FillData			();
	// check window position
    UI->CheckWindowPos	(this);
}
//---------------------------------------------------------------------------

void TfrmDOShuffle::FillData()
{
	// init
    tvItems->IsUpdating = true;
    tvItems->Selected = 0;
    tvItems->Items->Clear();
    // objects
    for (CDetailManager::DetailIt d_it=DM->objects.begin(); d_it!=DM->objects.end(); d_it++)
        AddItem(0,((EDetail*)(*d_it))->GetName(),(void*)(*d_it));
    // indices
    ColorIndexPairIt S = DM->m_ColorIndices.begin();
    ColorIndexPairIt E = DM->m_ColorIndices.end();
    ColorIndexPairIt it= S;
	for(; it!=E; it++){
    	TfrmOneColor* OneColor = xr_new<TfrmOneColor>((TComponent*)0);
		color_indices.push_back(OneColor);
		OneColor->Parent = form->sbDO;
	    OneColor->ShowIndex(this);
        OneColor->mcColor->Brush->Color = (TColor)rgb2bgr(it->first);
        for (DOIt do_it=it->second.begin(); do_it!=it->second.end(); do_it++){
        	EDetail* dd = 0;
            for (CDetailManager::DetailIt d_it=DM->objects.begin(); d_it!=DM->objects.end(); d_it++)
            	if (0==strcmp(((EDetail*)(*d_it))->GetName(),(*do_it)->GetName())){ dd = (EDetail*)*d_it; break; }
            VERIFY(dd);
	        OneColor->AppendObject(dd->GetName(),dd);
        }
    }
    // redraw
    tvItems->IsUpdating = false;
}
//---------------------------------------------------------------------------

bool TfrmDOShuffle::ApplyChanges()
{
	// update indices
	DM->RemoveColorIndices();
	for (u32 k=0; k<color_indices.size(); k++){
    	TfrmOneColor* OneColor = color_indices[k];
        if (OneColor->tvDOList->Items->Count){
	        u32 clr = bgr2rgb(OneColor->mcColor->Brush->Color);
		    for ( TElTreeItem* node = OneColor->tvDOList->Items->GetFirstNode(); node; node = node->GetNext())
	    		DM->AppendIndexObject(clr,AnsiString(node->Text).c_str(),false);
        }
    }
    if (/*bNeedUpdate||*/bColorIndModif||bObjectModif){
        ELog.DlgMsg(mtInformation,"Object or object list changed. Reinitialize needed!");
        DM->InvalidateSlots();
    	return true;
    }
    return false;
}
//---------------------------------------------------------------------------

void TfrmDOShuffle::ClearIndexForms()
{
	for (u32 k=0; k<color_indices.size(); k++)
    	xr_delete(color_indices[k]);
    color_indices.clear();
    xr_delete(m_Thm);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// implementation
//---------------------------------------------------------------------------
__fastcall TfrmDOShuffle::TfrmDOShuffle(TComponent* Owner, EDetailManager* dm_tools)
    : TForm(Owner)
{
    DEFINE_INI(fsStorage);
    DM = dm_tools; VERIFY(DM);
}
//---------------------------------------------------------------------------
TElTreeItem* TfrmDOShuffle::FindItem(const char* s)
{
    for ( TElTreeItem* node = tvItems->Items->GetFirstNode(); node; node = node->GetNext())
        if (node->Data && (AnsiString(node->Text) == s)) return node;
    return 0;
}
//---------------------------------------------------------------------------
TElTreeItem* TfrmDOShuffle::AddItem(TElTreeItem* node, const char* name, void* obj)
{
    TElTreeItem* obj_node = tvItems->Items->AddChildObject(node, name, obj);
    obj_node->ParentStyle = false;
    obj_node->Bold = false;
    return obj_node;
}
//---------------------------------------------------------------------------
void __fastcall TfrmDOShuffle::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
//    if (Key==VK_ESCAPE) ebCancel->Click();
//    if (Key==VK_RETURN) ebOk->Click();
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::FormClose(TObject *Sender, TCloseAction &Action)
{
    ModalResult = ApplyChanges()?mrOk:mrCancel;

	ClearIndexForms();

    if (ModalResult==mrOk)
		DM->InvalidateCache();

	Action = caFree;
    form = 0;
}
//---------------------------------------------------------------------------

void TfrmDOShuffle::OnItemFocused(TElTree* tv)
{
	if (bLockFocused)		return;
    bLockFocused			= true;

	// unselect before
    if (tvItems!=tv)		tvItems->Selected = 0;
	for (u32 k=0; k<color_indices.size(); k++){
    	TfrmOneColor* OneColor = color_indices[k];
        if (OneColor->tvDOList!=tv){
        	OneColor->tvDOList->IsUpdating 	= true;
        	OneColor->tvDOList->Selected 	= 0;
        	OneColor->tvDOList->IsUpdating 	= false;
        }
    }
    bLockFocused			= false;
    
    // select
	TElTreeItem* Item 		= tv->Selected;
    xr_delete(m_Thm);

    PropItemVec items;
	if (Item&&Item->Data){
		AnsiString nm 		= Item->Text;
        m_Thm			 	= ImageLib.CreateThumbnail(nm.c_str(),EImageThumbnail::ETObject);
        EDetail* dd			= (EDetail*)Item->Data;
		PHelper().CreateCaption	(items,"Ref Name",	dd->GetName());
		PHelper().CreateFloat	(items,"Density",	&dd->m_fDensityFactor, 	0.1f, 1.0f);
		PHelper().CreateFloat	(items,"Min Scale",	&dd->m_fMinScale, 		0.1f, 100.0f);
		PHelper().CreateFloat	(items,"Max Scale",	&dd->m_fMaxScale,		0.1f, 100.f);
		PHelper().CreateFlag32	(items,"No Waving",	&dd->m_Flags, DO_NO_WAVING);
    }
    m_ObjectProps->AssignItems	(items);
    if (!bTHMLockRepaint) 	paImage->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::tvItemsItemFocused(TObject *Sender)
{
	OnItemFocused		(tvItems);
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::paImagePaint(TObject *Sender)
{
	if (m_Thm) m_Thm->Draw(paImage);
}
//---------------------------------------------------------------------------


bool __fastcall LookupFunc(TElTreeItem* Item, void* SearchDetails){
    char s1 = *(char*)SearchDetails;
    char s2 = *AnsiString(Item->Text).c_str();
	return (s1==tolower(s2));
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::tvItemsKeyPress(TObject *Sender, char &Key)
{
	TElTreeItem* node = tvItems->Items->LookForItemEx(tvItems->Selected,-1,false,false,false,&Key,LookupFunc);
    if (!node) node = tvItems->Items->LookForItemEx(0,-1,false,false,false,&Key,LookupFunc);
    FHelper.RestoreSelection(tvItems,node,false);
}
//---------------------------------------------------------------------------

static TElTreeItem* DragItem=0;
void __fastcall TfrmDOShuffle::tvMultiDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
	TElTreeItem* node = ((TElTree*)Sender)->GetItemAtY(Y);
    if (node)
		DragItem->MoveToIns(0,node->Index);
	DragItem = 0;
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::tvMultiDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
	Accept = false;
	TElTreeItem* node = ((TElTree*)Sender)->GetItemAtY(Y);
	if ((Sender==Source)&&(node!=DragItem)) Accept=true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::tvMultiStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
	DragItem = ((TElTree*)Sender)->Selected;
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::ebAddObjectClick(TObject *Sender)
{
	LPCSTR S;
    if (TfrmChoseItem::SelectItem(smObject,S,8)){
	    AStringVec lst;
		_SequenceToList(lst, S);
        for (AStringIt s_it=lst.begin(); s_it!=lst.end(); s_it++)
        	if (!FindItem(s_it->c_str())){
                if (tvItems->Items->Count>=dm_max_objects){
                    ELog.DlgMsg(mtInformation,"Maximum detail objects in scene '%d'",dm_max_objects);
                    return;
                }
             	AddItem(0,s_it->c_str(),(void*)DM->AppendDO(s_it->c_str()));
            }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::ebDelObjectClick(TObject *Sender)
{
	if (tvItems->Selected){
        LPCSTR name			= AnsiString(tvItems->Selected->Text).c_str();
        DM->RemoveDO		(name);
        bObjectModif		= true;
    	bColorIndModif 		= true;
		for (u32 k=0; k<color_indices.size(); k++)
    		color_indices[k]->RemoveObject(name);
        tvItems->Selected->Delete();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::ebAppendIndexClick(TObject *Sender)
{
    bColorIndModif = true;
	color_indices.push_back(xr_new<TfrmOneColor>((TComponent*)0));
	color_indices.back()->Parent = sbDO;
    color_indices.back()->ShowIndex(this);
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::RemoveColorIndex(TfrmOneColor* p)
{
	form->bColorIndModif = true;
	form->color_indices.erase(std::find(form->color_indices.begin(),form->color_indices.end(),p));
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::ebMultiClearClick(TObject *Sender)
{
	bColorIndModif = true;
	for (u32 k=0; k<color_indices.size(); k++)
    	xr_delete(color_indices[k]);
    color_indices.clear();
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::tvItemsDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
	TfrmOneColor* OneColor = (TfrmOneColor*)((TElTree*)Source)->Parent;
    if (OneColor&&OneColor->FDragItem){
    	OneColor->FDragItem->Delete();
		bColorIndModif = true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::tvItemsDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
    Accept = false;
    if (Source == tvItems) return;
    Accept = true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::tvItemsStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
    FDragItem = tvItems->ItemFocused;
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::ebSaveListClick(TObject *Sender)
{
	xr_string fname;
	if (EFS.GetSaveName(_detail_objects_,fname)){
		DM->ExportColorIndices(fname.c_str());
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::ebLoadListClick(TObject *Sender)
{
	xr_string fname;
	if (EFS.GetOpenName(_detail_objects_,fname)){
		if (DM->ImportColorIndices(fname.c_str())){
			bColorIndModif 		= true;
	        DM->InvalidateSlots	();
			ClearIndexForms		();
        	FillData			();
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::ebClearListClick(TObject *Sender)
{
    DM->InvalidateSlots		();
    DM->ClearColorIndices	();
    ClearIndexForms			();
    FillData				();
	bColorIndModif 			= true;
    bObjectModif			= true;
    UI->RedrawScene			();
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::fsStorageRestorePlacement(TObject *Sender)
{
	m_ObjectProps->RestoreParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmDOShuffle::fsStorageSavePlacement(TObject *Sender)
{
	m_ObjectProps->SaveParams(fsStorage);
}
//---------------------------------------------------------------------------


