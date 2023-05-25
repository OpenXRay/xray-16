//---------------------------------------------------------------------------
#include "stdafx.h"

#include "SHGameMtlTools.h"
#include "ui_shadermain.h"
#include "UI_ShaderTools.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
CSHGameMtlTools::CSHGameMtlTools(const ISHInit &init) : ISHTools(init)
{
    m_CreatingMtl = FALSE;
    m_Mtl = 0;
    m_GameMtlPairTools = 0;
}

CSHGameMtlTools::~CSHGameMtlTools()
{
}
//---------------------------------------------------------------------------

void CSHGameMtlTools::OnActivate()
{
    // fill items
    FillItemList();

    /*  Ext.m_Items->SetOnModifiedEvent		(fastdelegate::bind<TOnModifiedEvent>(this,&CSHGameMtlTools::Modified));*/
    Ext.m_Items->SetOnItemCloneEvent(TOnItemClone(this, &CSHGameMtlTools::OnCloneItem));
    Ext.m_Items->SetOnItemCreaetEvent(TOnItemCreate(this, &CSHGameMtlTools::OnCreateItem));
    Ext.m_Items->SetOnItemRenameEvent(TOnItemRename(this, &CSHGameMtlTools::OnRenameItem));
    Ext.m_Items->SetOnItemRemoveEvent(TOnItemRemove(this, &CSHGameMtlTools::OnRemoveItem));
    inherited::OnActivate();
}

void CSHGameMtlTools::OnDeactivate()
{
    inherited::OnDeactivate();
}

void CSHGameMtlTools::OnFrame()
{
    inherited::OnFrame();
    if (m_CreatingMtl)
    {
        bool change = true;
        shared_str result;
        if (UIChooseForm::GetResult(change, result))
        {
            if (change)
            {
                AppendItem(m_CreatingMtlPath.c_str(), stricmp(result.c_str(), "dynamic") == 0);
            }
            m_CreatingMtl = FALSE;
        }
    }
}
void CSHGameMtlTools::OnDrawUI()
{
    if (m_CreatingMtl)
    {
        UIChooseForm::Update();
    }
}
//---------------------------------------------------------------------------

bool CSHGameMtlTools::OnCreate()
{
    m_GameMtlPairTools = STools->FindTools(aeMtlPair);
    R_ASSERT(m_GameMtlPairTools);
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
    FillItemList();
}

void CSHGameMtlTools::FillItemList()
{

    ListItemsVec items;
    for (GameMtlIt m_it = GMLib.FirstMaterial(); m_it != GMLib.LastMaterial(); m_it++)
        LHelper().CreateItem(items, *(*m_it)->m_Name, 0);
    // assign items
    Ext.m_Items->AssignItems(items, 0, false);
}

void CSHGameMtlTools::Load()
{
    m_bLockUpdate = TRUE;

    GMLib.Unload();
    GMLib.Load();
    ResetCurrentItem();

    m_bLockUpdate = FALSE;
}

bool CSHGameMtlTools::Save()
{
    ResetCurrentItem();
    m_bLockUpdate = TRUE;

    // save
    string_path fn;
    FS.update_path(fn, _game_data_, GAMEMTL_FILENAME);
    EFS.MarkFile(fn, false);
    bool bRes = GMLib.Save();

    m_bLockUpdate = FALSE;

    if (bRes)
        m_bModified = FALSE;
    return bRes;
}

SGameMtl *CSHGameMtlTools::FindItem(LPCSTR name)
{
    if (name && name[0])
    {
        return GMLib.GetMaterial(name);
    }
    else
        return 0;
}

void CSHGameMtlTools::FillChooseMtlType(ChooseItemVec &items, void *param)
{
    items.push_back(SChooseItem("Dynamic", "Dynamic material"));
    items.push_back(SChooseItem("Static", "Static material"));
}

void CSHGameMtlTools::AppendItem(LPCSTR path, LPCSTR parent_name)
{
    SGameMtl *parent = FindItem(parent_name);
    if (!parent)
    {
        UIChooseForm::SelectItem(smCustom, 1, 0, TOnChooseFillItems(this, &CSHGameMtlTools::FillChooseMtlType), 0, 0, 0, 0);
        m_CreatingMtl = TRUE;
        m_CreatingMtlPath = path;
    }
    else
    {
        AppendItem(path, false, parent);
    }
}

void CSHGameMtlTools::AppendItem(LPCSTR path, bool dynamic, SGameMtl *parent)
{
    SGameMtl *S = GMLib.AppendMaterial(parent);
    m_LastSelection = path;
    S->m_Name = m_LastSelection.c_str();
    if (!parent)
        S->Flags.set(SGameMtl::flDynamic, dynamic);
    ExecCommand(COMMAND_UPDATE_LIST);
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
    Modified();
}

void CSHGameMtlTools::OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type)
{
    if (type == TYPE_OBJECT)
    {
        SGameMtl *S = FindItem(old_full_name);
        R_ASSERT(S);
        S->m_Name = new_full_name;
        if (S == m_Mtl)
        {
            ExecCommand(COMMAND_UPDATE_PROPERTIES);
            ExecCommand(COMMAND_UPDATE_LIST);
            m_LastSelection = m_Mtl->m_Name.c_str();

            m_Mtl = 0;
        }
    }
}

void CSHGameMtlTools::OnRemoveItem(LPCSTR name, EItemType type)
{
    if (type == TYPE_OBJECT)
    {
        R_ASSERT(name && name[0]);
        if (m_Mtl && m_Mtl->m_Name == name)
        {
            m_Mtl = 0;
            Tools->UpdateProperties(true);
        }
        GMLib.RemoveMaterial(name);
    }
}

void CSHGameMtlTools::SetCurrentItem(LPCSTR name, bool bView)
{
    if (m_bLockUpdate)
        return;

    SGameMtl *S = FindItem(name);
    // load material
    if (m_Mtl != S)
    {
        m_Mtl = S;
        ExecCommand(COMMAND_UPDATE_PROPERTIES);
        if (bView)
            ViewSetCurrentItem(name);
    }
}

void CSHGameMtlTools::ResetCurrentItem()
{
    m_Mtl = 0;
}
//---------------------------------------------------------------------------

void CSHGameMtlTools::RealUpdateList()
{
    FillItemList();
}
//------------------------------------------------------------------------------

void CSHGameMtlTools::RealUpdateProperties()
{
    PropItemVec items;
    if (m_Mtl)
        m_Mtl->FillProp(items, m_CurrentItem);
    Ext.m_ItemProps->AssignItems(items);
    Ext.m_ItemProps->SetModifiedEvent(TOnModifiedEvent(this, &CSHGameMtlTools::Modified));
}
//---------------------------------------------------------------------------

void CSHGameMtlTools::ApplyChanges(bool bForced)
{
}
//---------------------------------------------------------------------------
