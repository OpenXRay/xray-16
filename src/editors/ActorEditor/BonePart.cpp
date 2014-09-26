//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "../ECore/Editor/EditObject.h"
#include "BonePart.h"
#include "Bone.h"
#include "UI_ActorTools.h"
#include "../ECore/Editor/UI_Main.h"
#include "../xrEProps/FolderLib.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmBonePart *frmBonePart;        
TElTree* 	T[4];                       
TEdit* 		E[4];
TLabel* 	L[4];
//---------------------------------------------------------------------------
bool TfrmBonePart::Run(CEditableObject* object)
{
	int res = mrCancel;
	if (object){            
        m_EditObject = object;
        m_BoneParts = &object->BoneParts();
        res = ShowModal();
    }else{
    	ELog.DlgMsg(mtError,"Scene empty. Load object first.");
    }
    return (res==mrOk);
}
//---------------------------------------------------------------------------
__fastcall TfrmBonePart::TfrmBonePart(TComponent* Owner)
	: TForm(Owner)
{
    T[0] = tvPart1;
    T[1] = tvPart2;
    T[2] = tvPart3;
    T[3] = tvPart4;
    E[0] = edPart1Name;                    
    E[1] = edPart2Name;
    E[2] = edPart3Name;
    E[3] = edPart4Name;
    L[0] = lbPart1;
    L[1] = lbPart2;                      
    L[2] = lbPart3;
    L[3] = lbPart4;
}
//---------------------------------------------------------------------------
void __fastcall TfrmBonePart::FormShow(TObject *Sender)
{
	FillBoneParts();
	// check window position
	UI->CheckWindowPos(this);
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::FillBoneParts()
{
    for (int k=0; k<4; k++) T[k]->IsUpdating = true;
    for (k=0; k<4; k++){T[k]->Items->Clear();E[k]->Text="";}
	for (BPIt it=m_BoneParts->begin(); it!=m_BoneParts->end(); it++){
        E[it-m_BoneParts->begin()]->Text = it->alias.c_str();
        for (RStringVecIt w_it=it->bones.begin(); w_it!=it->bones.end(); w_it++)
        	FHelper.AppendObject(T[it-m_BoneParts->begin()],w_it->c_str(),false,true);
    }
    for (k=0; k<4; k++) T[k]->IsUpdating = false;
    lbTotalBones->Caption = m_EditObject->BoneCount();
    UpdateCount();
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::UpdateCount()
{
    for (int k=0; k<4; k++) L[k]->Caption = AnsiString().sprintf("(%d B)",T[k]->Items->Count);
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if (Key==VK_ESCAPE)	return;
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::tvPartStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
	for (TElTreeItem* node=((TElTree*)Sender)->GetNextSelected(0); node; node=((TElTree*)Sender)->GetNextSelected(node))
		FDragItems.push_back(node);
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::tvPartDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
	for (int k=0; k<(int)FDragItems.size(); k++){
		FHelper.AppendObject(((TElTree*)Sender),FDragItems[k]->Text,false,true);
        FDragItems[k]->Delete();
    }
    FDragItems.clear();
    UpdateCount();
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::tvPartDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
    Accept = false;
    if (Source == Sender) return;
    Accept = true;
}
//---------------------------------------------------------------------------


void __fastcall TfrmBonePart::ebSaveClick(TObject *Sender)
{
	for (int k=0; k<4; k++){
    	if (T[k]->Items->Count&&E[k]->Text.IsEmpty()){
        	ELog.DlgMsg(mtError,"Verify parts name.");
        	return;
        }
        for (int i=k-1; i>=0; i--){
	    	if (!T[k]->Items->Count) continue;
        	if (E[k]->Text.UpperCase()==E[i]->Text.UpperCase()){
                ELog.DlgMsg(mtError,"Unique name required.");
                return;
            }
        }
    }

    // verify
	U8Vec b_use(m_EditObject->BoneCount(),0);
    for (k=0; k<4; k++){
    	if (T[k]->Items->Count)
		    for ( TElTreeItem* node = T[k]->Items->GetFirstNode(); node; node = node->GetNext())
	        	b_use[m_EditObject->FindBoneByNameIdx(AnsiString(node->Text).c_str())]++;
    }
    for (U8It u_it=b_use.begin(); u_it!=b_use.end(); u_it++)
    	if (*u_it!=1){ 
            ELog.DlgMsg	(mtError,"Invalid bone part found (missing or duplicate bones).");
        	return;
        }

	// save    
	m_BoneParts->clear();
    for (k=0; k<4; k++){
    	if (T[k]->Items->Count){
            m_BoneParts->push_back(SBonePart());
            SBonePart& BP = m_BoneParts->back();
            BP.alias = E[k]->Text.c_str();
		    for ( TElTreeItem* node = T[k]->Items->GetFirstNode(); node; node = node->GetNext())
            	BP.bones.push_back(AnsiString(node->Text).c_str());
        }
    }
    ATools->OnMotionDefsModified();
    Close();
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::ebCancelClick(TObject *Sender)
{
    ModalResult = mrCancel;
	Close();
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::ExtBtn1Click(TObject *Sender)
{
    for (int k=0; k<4; k++) T[k]->IsUpdating = true;
    for (k=0; k<4; k++){T[k]->Items->Clear();E[k]->Text="";}
    E[0]->Text = "default";
    for (BoneIt it=m_EditObject->FirstBone(); it!=m_EditObject->LastBone(); it++)
        FHelper.AppendObject(T[0],(*it)->Name().c_str(),false,true);
    for (k=0; k<4; k++) T[k]->IsUpdating = false;
    UpdateCount();
}
//---------------------------------------------------------------------------


/*
void __fastcall TfrmBonePart::ebAllClick(TObject *Sender)
{
	TExtBtn* B = dynamic_cast<TExtBtn*>(Sender); VERIFY(B); int idx = B->Tag;
    T[idx]->IsUpdating = true;
    T[idx]->Items->Clear();
    for (BoneIt it=m_EditObject->FirstBone(); it!=m_EditObject->LastBone(); it++)
        FHelper.AppendObject(T[idx],(*it)->Name(),false,true);
    T[idx]->IsUpdating = false;
    UpdateCount();
}
*/
void __fastcall TfrmBonePart::ebClearClick(TObject *Sender)
{
	TExtBtn* B = dynamic_cast<TExtBtn*>(Sender); VERIFY(B); int idx = B->Tag;
    T[idx]->IsUpdating = true;
    T[idx]->Items->Clear();
    T[idx]->IsUpdating = false;
    UpdateCount();
}
//---------------------------------------------------------------------------



void __fastcall TfrmBonePart::ebSaveToClick(TObject *Sender)
{
    xr_string temp_fn;
    if (EFS.GetSaveName( _import_, temp_fn) )
    {
        CInifile ini	(temp_fn.c_str(), FALSE, FALSE, TRUE);
        string64		buff;
        for(int i=0; i<4;++i)
        {
            sprintf	(buff,"part_%d",i);
            ini.w_string(buff, "partition_name", E[i]->Text.c_str());
            if (T[i]->Items->Count)
            {
                for ( TElTreeItem* node = T[i]->Items->GetFirstNode(); node; node = node->GetNext())
                    ini.w_string(buff, AnsiString(node->Text).c_str(), NULL );
			}
        }
        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmBonePart::ebLoadFromClick(TObject *Sender)
{
//.
    xr_string temp_fn;
    if (EFS.GetOpenName	( _import_, temp_fn, false, NULL, 0))
    {
        ebClearClick	(Sender);
        CInifile ini	(temp_fn.c_str(), TRUE, TRUE, FALSE);
        string64		buff;
        for(int i=0; i<4;++i)
        {
            sprintf	(buff,"part_%d",i);
            LPCSTR part_name = ini.r_string(buff, "partition_name");
            E[i]->Text = part_name;
            CInifile::Sect& S 	= ini.r_section(buff);
            CInifile::SectCIt it = S.Data.begin();
            CInifile::SectCIt e = S.Data.end();
            for (; it!=e; ++it)
            {
                if( 0!= stricmp(it->first.c_str(), "partition_name") )
                {
                    FHelper.AppendObject(T[i], it->first.c_str(), false, true);
                }
            }

        }
    }
}
//---------------------------------------------------------------------------

