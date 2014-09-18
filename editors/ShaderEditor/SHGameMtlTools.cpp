//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SHGameMtlTools.h"
#include "../../xrServerEntities/PropertiesListHelper.h"
#include "ui_shadermain.h"
#include "../xrEProps/folderlib.h"
#include "UI_ShaderTools.h"
#include "../xrEProps/ChoseForm.h"
#include "leftbar.h"
#include "../xrEProps/ItemList.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
CSHGameMtlTools::CSHGameMtlTools(ISHInit& init):ISHTools(init)
{
    m_Mtl 				= 0;
    m_GameMtlPairTools	= 0;
}

CSHGameMtlTools::~CSHGameMtlTools()
{
}
//---------------------------------------------------------------------------

void CSHGameMtlTools::OnActivate()
{
    // fill items
    FillItemList		();
    Ext.m_Items->SetOnModifiedEvent		(fastdelegate::bind<TOnModifiedEvent>(this,&CSHGameMtlTools::Modified));
    Ext.m_Items->SetOnItemRenameEvent	(fastdelegate::bind<TOnItemRename>(this,&CSHGameMtlTools::OnRenameItem));
    Ext.m_Items->SetOnItemRemoveEvent	(fastdelegate::bind<TOnItemRemove>(this,&CSHGameMtlTools::OnRemoveItem));
    inherited::OnActivate		();
}

void CSHGameMtlTools::OnDeactivate()
{
    inherited::OnDeactivate		();
}

void CSHGameMtlTools::OnFrame()
{
	inherited::OnFrame();
}
//---------------------------------------------------------------------------

bool CSHGameMtlTools::OnCreate()
{
	m_GameMtlPairTools	= STools->FindTools(aeMtlPair); R_ASSERT(m_GameMtlPairTools);
    Load();
    return true;
}

void CSHGameMtlTools::OnDestroy()
{
    m_bModified = FALSE;
}

void CSHGameMtlTools::Reload()
{
	// mtl
    ResetCurrentItem();
    // mtl pair
    m_GameMtlPairTools->ResetCurrentItem();
    // load
    Load();
    FillItemList		();
}

void CSHGameMtlTools::FillItemList()
{
	// store folders
	RStrVec folders;
	Ext.m_Items->GetFolders(folders);
    // fill items
	ListItemsVec items;
    for (GameMtlIt m_it=GMLib.FirstMaterial(); m_it!=GMLib.LastMaterial(); m_it++)
        LHelper().CreateItem(items,*(*m_it)->m_Name,0);
    // fill folders
    for (RStringVecIt s_it=folders.begin(); s_it!=folders.end(); s_it++)
        LHelper().CreateItem(items,**s_it,0);
    // assign items
	Ext.m_Items->AssignItems(items,false,true);
}

void CSHGameMtlTools::Load()
{
    m_bLockUpdate		= TRUE;

    GMLib.Unload	();
    GMLib.Load		();
    ResetCurrentItem();

	m_bLockUpdate		= FALSE;
}

bool CSHGameMtlTools::Save()
{
	ResetCurrentItem	();
    m_bLockUpdate		= TRUE;

    // save
    string_path 		fn;
    FS.update_path		(fn,_game_data_,GAMEMTL_FILENAME);
    EFS.MarkFile		(fn,false);
    bool bRes			= GMLib.Save();
    
	m_bLockUpdate		= FALSE;

    if (bRes) 			m_bModified	= FALSE;
    return bRes;
}

SGameMtl* CSHGameMtlTools::FindItem(LPCSTR name)
{
	if (name && name[0]){
    	return GMLib.GetMaterial(name);
    }else return 0;
}

void CSHGameMtlTools::FillChooseMtlType(ChooseItemVec& items, void* param)
{
    items.push_back(SChooseItem("Dynamic",	"Dynamic material"));
    items.push_back(SChooseItem("Static",	"Static material"));
}

LPCSTR CSHGameMtlTools::AppendItem(LPCSTR folder_name, LPCSTR parent_name)
{
    LPCSTR M=0;
	SGameMtl* parent 	= FindItem(parent_name);
    if (!parent){
	    if (!TfrmChoseItem::SelectItem(smCustom,M,1,0,fastdelegate::bind<TOnChooseFillItems>(this,&CSHGameMtlTools::FillChooseMtlType))||!M) return 0;
    }
    AnsiString pref		= parent_name?AnsiString(parent_name):AnsiString(folder_name)+M;
    m_LastSelection		= FHelper.GenerateName(pref.c_str(),2,fastdelegate::bind<TFindObjectByName>(this,&CSHGameMtlTools::ItemExist),false,true);
    SGameMtl* S 		= GMLib.AppendMaterial(parent);
    S->m_Name			= m_LastSelection.c_str();
    if (!parent)		S->Flags.set (SGameMtl::flDynamic,0==strcmp(M,"Dynamic"));
    ExecCommand			(COMMAND_UPDATE_LIST);
    ExecCommand			(COMMAND_UPDATE_PROPERTIES);
	Modified			();
    return *S->m_Name;
}

void CSHGameMtlTools::OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type)
{
	if (type==TYPE_OBJECT){
        SGameMtl* S = FindItem(old_full_name); R_ASSERT(S);
        S->m_Name		= new_full_name;
        if (S==m_Mtl){
            ExecCommand	(COMMAND_UPDATE_PROPERTIES);
            ExecCommand	(COMMAND_UPDATE_LIST);
        }
    }
}

void CSHGameMtlTools::OnRemoveItem(LPCSTR name, EItemType type, bool& res)
{
	if (type==TYPE_OBJECT){
        R_ASSERT(name && name[0]);
        GMLib.RemoveMaterial(name);
    }
    res = true;
}

void CSHGameMtlTools::SetCurrentItem(LPCSTR name, bool bView)
{
    if (m_bLockUpdate) return;

	SGameMtl* S = FindItem(name);
    // load material
	if (m_Mtl!=S){
        m_Mtl = S;
	    ExecCommand(COMMAND_UPDATE_PROPERTIES);
	 	if (bView) ViewSetCurrentItem(name);
   	}
}

void CSHGameMtlTools::ResetCurrentItem()
{
	m_Mtl=0;
}
//---------------------------------------------------------------------------

void CSHGameMtlTools::RealUpdateList()
{
	FillItemList			();
}
//------------------------------------------------------------------------------

void CSHGameMtlTools::RealUpdateProperties()
{
	PropItemVec items;
    if (m_Mtl)
    	m_Mtl->FillProp	(items,m_CurrentItem);
    Ext.m_ItemProps->AssignItems		(items);
    Ext.m_ItemProps->SetModifiedEvent	(fastdelegate::bind<TOnModifiedEvent>(this,&CSHGameMtlTools::Modified));
}
//---------------------------------------------------------------------------

void CSHGameMtlTools::ApplyChanges(bool bForced)
{
}
//---------------------------------------------------------------------------


