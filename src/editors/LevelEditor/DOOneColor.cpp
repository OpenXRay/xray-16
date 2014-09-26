#include "stdafx.h"
#pragma hdrstop

#include "DOOneColor.h"
#include "DOShuffle.h"
#include "../ECore/Editor/ColorPicker.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElXPThemedControl"
#pragma link "ExtBtn"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TfrmOneColor::TfrmOneColor(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::ShowIndex(TfrmDOShuffle* parent)
{
	m_Parent = parent;
//	VERIFY(stage);
//	m_CurStage = stage;

    bLoadMode = true;
//    if (m_CurStage){
//    }
    bLoadMode = false;
    Visible=true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::HideIndex(){
    Visible=false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::mcColorMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	u32 color = ((TMultiObjColor*)Sender)->Brush->Color;
	if (SelectColorWin(&color)){
    	((TMultiObjColor*)Sender)->_Set(color);
		m_Parent->bColorIndModif = true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::FormClose(TObject *Sender,
      TCloseAction &Action)
{
	Action = caFree;
    m_Parent->RemoveColorIndex(this);
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::ebMultiRemoveClick(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::tvDOListDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
    Accept = false;
    if (Source == tvDOList) return;
	if (Source != m_Parent->tvItems) return;
    for ( TElTreeItem* node = tvDOList->Items->GetFirstNode(); node; node = node->GetNext())
        if (node->Text == m_Parent->FDragItem->Text) return;
	Accept = true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::tvDOListDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
    tvDOList->Items->AddObject(0,m_Parent->FDragItem->Text,m_Parent->FDragItem->Data);
	m_Parent->bColorIndModif = true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::tvDOListStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
	FDragItem = tvDOList->ItemFocused;
}
//---------------------------------------------------------------------------

void __fastcall TfrmOneColor::RemoveObject(LPCSTR text){
    for ( TElTreeItem* node = tvDOList->Items->GetFirstNode(); node; node = node->GetNext())
        if (node->Text == (WideString)text){
			node->Delete();
        	return;
        }
}

void __fastcall TfrmOneColor::AppendObject(LPCSTR text, LPVOID data)
{
    for ( TElTreeItem* node = tvDOList->Items->GetFirstNode(); node; node = node->GetNext())
        if (node->Text == (WideString)text) return;
    tvDOList->Items->AddObject(0,text,data);
}

void __fastcall TfrmOneColor::tvDOListItemFocused(TObject *Sender)
{
	m_Parent->OnItemFocused		(tvDOList);
}
//---------------------------------------------------------------------------


