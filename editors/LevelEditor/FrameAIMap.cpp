#include "stdafx.h"
#pragma hdrstop

#include "ui_leveltools.h"
#include "FrameAIMap.h"
#include "Scene.h"
#include "ESceneAIMapTools.h"
#include "ESceneAIMapControls.h"
#include "../ECore/Editor/ui_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mxPlacemnt"
#pragma link "ElTree"
#pragma link "ElXPThemedControl"
#pragma link "ElPanel"
#pragma link "ElSplit"
#pragma link "multi_edit"
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TfraAIMap::TfraAIMap(TComponent* Owner, ESceneAIMapTool* _tools)
: TForm(Owner)
{   
	tools = _tools;  R_ASSERT(tools);         
}
//---------------------------------------------------------------------------
void __fastcall TfraAIMap::PanelMinClick(TObject *Sender)
{
    PanelMinMaxClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ExpandClick(TObject *Sender)
{
    PanelMaximizeClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ebGenerateMapClick(TObject *Sender)
{
	tools->GenerateMap(false);
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ebGenerateSelectedClick(TObject *Sender)
{
	tools->GenerateMap(true);
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ebResetSelectedClick(TObject *Sender)
{
	tools->ResetNodes();
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ebSmoothNodesClick(TObject *Sender)
{
	tools->SmoothNodes();
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ExtBtn6Click(TObject *Sender)
{
	if (ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Are you sure to clear AI Map?")==mrYes){
		tools->Clear();   
		Scene->UndoSave();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ebDrawSnapObjectsClick(TObject *Sender)
{
    UI->RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ebInvertLinkClick(TObject *Sender)
{
	tools->InvertLinks();
    Scene->UndoSave();
    UI->RedrawScene();
}
//---------------------------------------------------------------------------

static const int idx[5][4]={
	{0,1,2,3},
	{1,2,3,0},
	{2,3,0,1},
	{3,0,1,2},
	{4,4,4,4},
};

int ConvertV2L(int side)
{
	if (side<4){
        const Fvector& HPB = EDevice.m_Camera.GetHPB();
        float h = angle_normalize(HPB.x)/PI; R_ASSERT((h>=0.f)&&(h<=2.f));
        if (h>0.25f&&h<=0.75f)		return idx[3][side];
        else if (h>0.75f&&h<=1.25f)	return idx[2][side];
        else if (h>1.25f&&h<=1.75f)	return idx[1][side];
        else return idx[0][side];
    }else return side;
}
                        
static const u8 fl[5]	=	{SAINode::flN1,SAINode::flN2,SAINode::flN3,SAINode::flN4,
    					 	 SAINode::flN1|SAINode::flN2|SAINode::flN3|SAINode::flN4,
//    					 	 SAINode::flN1|SAINode::flN2,SAINode::flN2|SAINode::flN3,
//    					 	 SAINode::flN3|SAINode::flN4,SAINode::flN4|SAINode::flN1
                             };
void __fastcall TfraAIMap::ebSideClick(TObject *Sender)
{
	ESceneAIMapTool::EMode mode;
    if (ebModeAppend->Down) 		mode = ESceneAIMapTool::mdAppend;
    else if (ebModeRemove->Down)	mode = ESceneAIMapTool::mdRemove;
    else if (ebModeInvert->Down)	mode = ESceneAIMapTool::mdInvert;
    TExtBtn* btn = dynamic_cast<TExtBtn*>(Sender); R_ASSERT(btn);
	tools->MakeLinks(fl[ConvertV2L(btn->Tag)], mode, ebIgnoreConstraints->Down);
    Scene->UndoSave();
    UI->RedrawScene();      
}
//---------------------------------------------------------------------------

void __fastcall TfraAIMap::ebSelLinkClick(TObject *Sender)
{
    TExtBtn* btn = dynamic_cast<TExtBtn*>(Sender); R_ASSERT(btn);
	tools->SelectNodesByLink	(btn->Tag);
    Scene->UndoSave();
}
//---------------------------------------------------------------------------





void __fastcall TfraAIMap::btnAddIgnoredMaterialClick(TObject *Sender)
{
    LPCSTR new_val = 0;
    if ( TfrmChoseItem::SelectItem(smGameMaterial,new_val,1) )
    {
        lbIgnoreMaterialsList->AddItem(new_val, NULL);
        SGameMtl* mtl		= GMLib.GetMaterial(new_val);
        tools->m_ignored_materials.push_back(mtl->GetID());
    }
}

//---------------------------------------------------------------------------

void __fastcall TfraAIMap::btnIgnoreMaterialClearClick(TObject *Sender)
{
    lbIgnoreMaterialsList->Clear();
    tools->m_ignored_materials.clear();
}
//---------------------------------------------------------------------------

