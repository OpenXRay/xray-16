//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SHToolsInterface.h"
#include "../XrECore/Editor/ui_main.h"

ISHTools::ISHTools(const ISHInit &init)
{
    m_bModified = FALSE;
    m_bLockUpdate = FALSE;
    Ext = init;
}
//---------------------------------------------------------------------------

void ISHTools::ViewSetCurrentItem(LPCSTR full_name)
{
    if (m_bLockUpdate)
        return;

    m_bLockUpdate = TRUE;
    Ext.m_Items->SelectItem(full_name);
    m_bLockUpdate = FALSE;
}
//---------------------------------------------------------------------------

void ISHTools::Modified()
{
    m_bModified = TRUE;

    ExecCommand(COMMAND_UPDATE_CAPTION);
    ApplyChanges();
}
//---------------------------------------------------------------------------

bool ISHTools::IfModified()
{
    if (m_bModified)
    {
        int mr = ELog.DlgMsg(mtConfirmation, "The '%s' has been modified.\nDo you want to save your changes?", ToolsName());
        switch (mr)
        {
        case mrYes:
            Save();
            m_bModified = FALSE;
            break;
        case mrNo:
            m_bModified = FALSE;
            break;
        case mrCancel:
            return false;
        }
    }
    return true;
}
//---------------------------------------------------------------------------

void ISHTools::ZoomObject(bool bOnlySel)
{
    Fbox BB;
    BB.set(-5, -5, -5, 5, 5, 5);
    EDevice.m_Camera.ZoomExtents(BB);
}
//---------------------------------------------------------------------------

xr_string ISHTools::ViewGetCurrentItem(bool bFolderOnly)
{
    xr_string name;
    RStringVec lst;
    Ext.m_Items->GetSelected(lst);
    if (lst.size() == 1)
    {
        name = lst[0].c_str();
    }
    return name;
}
//---------------------------------------------------------------------------
/*
TElTreeItem* ISHTools::ViewGetCurrentItem()
{
    return Ext.m_Items->GetSelected();
}*/
//---------------------------------------------------------------------------

void ISHTools::RemoveCurrent()
{
    R_ASSERT(0);
    //	Ext.m_Items->RemoveSelItems();
}
//---------------------------------------------------------------------------

void ISHTools::RenameCurrent()
{
    R_ASSERT(0);
    //	Ext.m_Items->RenameSelItem();
}
//---------------------------------------------------------------------------

void ISHTools::OnFrame()
{
    if (m_LastSelection.size())
    {
        SetCurrentItem(m_LastSelection.c_str(), true);
        m_LastSelection = "";
    }
}
void ISHTools::OnActivate()
{
    SetCurrentItem(m_LastSelection.c_str(), true);
    UI->RedrawScene();
}
void ISHTools::OnDeactivate()
{
    Ext.m_PreviewProps->ClearProperties();
    m_LastSelection = ViewGetCurrentItem(false);
    ResetCurrentItem();
    Ext.m_Items->ClearList();
}
//---------------------------------------------------------------------------
void ISHTools::OnCloneItem(LPCSTR parent_path, LPCSTR new_full_name)
{
    AppendItem(new_full_name, parent_path);
}

void ISHTools::OnCreateItem(LPCSTR path)
{
    AppendItem(path);
}