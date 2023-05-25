//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "SHCompilerTools.h"
#include "ui_shadermain.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
CSHCompilerTools::CSHCompilerTools(const ISHInit &init) : ISHTools(init)
{
    m_Shader = 0;
}

CSHCompilerTools::~CSHCompilerTools()
{
}
//---------------------------------------------------------------------------

void CSHCompilerTools::OnActivate()
{
    // fill items
    FillItemList();

    // Ext.m_Items->SetOnModifiedEvent		(TOnModifiedEvent(this,&CSHCompilerTools::Modified));
    Ext.m_Items->SetOnItemCreaetEvent(TOnItemCreate(this, &CSHCompilerTools::OnCreateItem));
    Ext.m_Items->SetOnItemCloneEvent(TOnItemClone(this, &CSHCompilerTools::OnCloneItem));
    Ext.m_Items->SetOnItemRenameEvent(TOnItemRename(this, &CSHCompilerTools::OnRenameItem));
    Ext.m_Items->SetOnItemRemoveEvent(TOnItemRemove(this, &CSHCompilerTools::OnRemoveItem));

    inherited::OnActivate();
}
void CSHCompilerTools::OnDeactivate()
{
    inherited::OnDeactivate();
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
    m_bModified = FALSE;
}

void CSHCompilerTools::ApplyChanges(bool bForced)
{
}

void CSHCompilerTools::Reload()
{
    ResetCurrentItem();
    Load();
    FillItemList();
}

void CSHCompilerTools::Load()
{
    string_path fn;
    FS.update_path(fn, _game_data_, "shaders_xrlc.xr");

    if (FS.exist(fn))
    {
        m_Library.Load(fn);
    }
    else
    {
        ELog.DlgMsg(mtInformation, "Can't find file '%s'", fn);
    }
}

bool CSHCompilerTools::Save()
{
    ApplyChanges();

    // save
    string_path fn;
    FS.update_path(fn, _game_data_, "shaders_xrlc.xr");

    EFS.MarkFile(fn, false);
    bool bRes = m_Library.Save(fn);

    if (bRes)
        m_bModified = FALSE;
    return bRes;
}

Shader_xrLC *CSHCompilerTools::FindItem(LPCSTR name)
{
    if (name && name[0])
    {
        return m_Library.Get(name);
    }
    else
        return 0;
}

void CSHCompilerTools::AppendItem(LPCSTR path, LPCSTR parent_name)
{
    Shader_xrLC *parent = FindItem(parent_name);
    xr_string pref = path;
    Shader_xrLC *S = m_Library.Append(parent);
    m_LastSelection = pref;
    strcpy(S->Name, m_LastSelection.c_str());
    ExecCommand(COMMAND_UPDATE_LIST);
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
    Modified();
}

void CSHCompilerTools::OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type)
{
    if (type == TYPE_OBJECT)
    {
        ApplyChanges();
        Shader_xrLC *S = FindItem(old_full_name);
        R_ASSERT(S);
        strcpy(S->Name, new_full_name);
        if (S == m_Shader)
        {
            ExecCommand(COMMAND_UPDATE_PROPERTIES);
            ExecCommand(COMMAND_UPDATE_LIST);

            m_LastSelection = m_Shader->Name;

            m_Shader = 0;
        }
    }
}

void CSHCompilerTools::OnRemoveItem(LPCSTR name, EItemType type)
{
    if (type == TYPE_OBJECT)
    {
        R_ASSERT(name && name[0]);
        if (m_Shader && stricmp(m_Shader->Name, name) == 0)
        {
            m_Shader = 0;
            Tools->UpdateProperties(true);
        }
        m_Library.Remove(name);
    }
}

void CSHCompilerTools::SetCurrentItem(LPCSTR name, bool bView)
{
    if (m_bLockUpdate)
        return;

    Shader_xrLC *S = FindItem(name);
    // load shader
    if (m_Shader != S)
    {
        m_Shader = S;
        ExecCommand(COMMAND_UPDATE_PROPERTIES);
        if (bView)
            ViewSetCurrentItem(name);
    }
}

void CSHCompilerTools::ResetCurrentItem()
{
    m_Shader = 0;
}
