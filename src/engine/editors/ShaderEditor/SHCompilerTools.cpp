//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "SHCompilerTools.h"
#include "ui_shadermain.h"
#include "../xrEProps/folderlib.h"
#include "leftbar.h"
#include "../xrEProps/ItemList.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
CSHCompilerTools::CSHCompilerTools(ISHInit& init):ISHTools(init)
{
	m_Shader		= 0;
}

CSHCompilerTools::~CSHCompilerTools(){
}
//---------------------------------------------------------------------------

void CSHCompilerTools::OnActivate()
{
    // fill items
    FillItemList   				();
    Ext.m_Items->SetOnModifiedEvent		(fastdelegate::bind<TOnModifiedEvent>(this,&CSHCompilerTools::Modified));
    Ext.m_Items->SetOnItemRenameEvent	(fastdelegate::bind<TOnItemRename>(this,&CSHCompilerTools::OnRenameItem));
    Ext.m_Items->SetOnItemRemoveEvent	(fastdelegate::bind<TOnItemRemove>(this,&CSHCompilerTools::OnRemoveItem));

    inherited::OnActivate		();
}
void CSHCompilerTools::OnDeactivate()
{
    inherited::OnDeactivate		();
}

void CSHCompilerTools::OnFrame()
{
	inherited::OnFrame();
}
//---------------------------------------------------------------------------

bool CSHCompilerTools::OnCreate()
{
    Load();
    return true;
}

void CSHCompilerTools::OnDestroy()
{
    m_bModified 		= FALSE;
}

void CSHCompilerTools::ApplyChanges(bool bForced)
{
}

void CSHCompilerTools::Reload()
{
    ResetCurrentItem	();
    Load				();
    FillItemList		();
}

void CSHCompilerTools::Load()
{
	string_path fn;
    FS.update_path(fn,_game_data_,"shaders_xrlc.xr");

    if (FS.exist(fn))
    {
    	m_Library.Load(fn);
    }else{
    	ELog.DlgMsg(mtInformation,"Can't find file '%s'",fn);
    }
}

bool CSHCompilerTools::Save()
{
    ApplyChanges			();

    // save
	string_path fn;
    FS.update_path			(fn,_game_data_,"shaders_xrlc.xr");

    EFS.MarkFile			(fn,false);
    bool bRes				= m_Library.Save(fn);

    if (bRes)				m_bModified	= FALSE;
    return bRes;
}

Shader_xrLC* CSHCompilerTools::FindItem(LPCSTR name)
{
	if (name && name[0]){
    	return m_Library.Get(name);
    }else return 0;
}

LPCSTR CSHCompilerTools::AppendItem(LPCSTR folder_name, LPCSTR parent_name) 
{
	Shader_xrLC* parent 	= FindItem(parent_name);
    AnsiString pref			= parent_name?AnsiString(parent_name):AnsiString(folder_name)+"shader";
    m_LastSelection			= FHelper.GenerateName(pref.c_str(),2,fastdelegate::bind<TFindObjectByName>(this,&CSHCompilerTools::ItemExist),false,true);
    Shader_xrLC* S 			= m_Library.Append(parent);
    strcpy					(S->Name,m_LastSelection.c_str());
    ExecCommand				(COMMAND_UPDATE_LIST);
    ExecCommand				(COMMAND_UPDATE_PROPERTIES);
	Modified				();
    return S->Name;
}

void CSHCompilerTools::OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type)
{
	if (type==TYPE_OBJECT){
        ApplyChanges();
        Shader_xrLC* S = FindItem(old_full_name); R_ASSERT(S);
        strcpy(S->Name,new_full_name);
        if (S==m_Shader){
            ExecCommand	(COMMAND_UPDATE_PROPERTIES);
            ExecCommand	(COMMAND_UPDATE_LIST);
        }
    }
}

void CSHCompilerTools::OnRemoveItem(LPCSTR name, EItemType type, bool& res)
{
	if (type==TYPE_OBJECT){
        R_ASSERT(name && name[0]);
        m_Library.Remove(name);
    }
    res = TRUE;
}

void CSHCompilerTools::SetCurrentItem(LPCSTR name, bool bView)
{
    if (m_bLockUpdate) return;

	Shader_xrLC* S = FindItem(name);
    // load shader
	if (m_Shader!=S){
        m_Shader = S;
	    ExecCommand(COMMAND_UPDATE_PROPERTIES);
	    if (bView)	ViewSetCurrentItem(name);
    }
}

void CSHCompilerTools::ResetCurrentItem()
{
	m_Shader=0;
}


