//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SHToolsInterface.h"
#include "../xrEProps/FolderLib.h"
#include "../ECore/Editor/ui_main.h"
#include "../xrEProps/itemlist.h"

ISHTools::ISHTools(ISHInit& init)
{
	m_bModified			= FALSE;
    m_bLockUpdate		= FALSE;
    Ext					= init;
}
//---------------------------------------------------------------------------

void ISHTools::ViewSetCurrentItem(LPCSTR full_name)
{
	if (m_bLockUpdate) 	return;

    m_bLockUpdate		= TRUE;
    Ext.m_Items->SelectItem(full_name,TRUE,false,true);
    m_bLockUpdate		= FALSE;
}
//---------------------------------------------------------------------------

void ISHTools::Modified()
{
	m_bModified=TRUE;
    ExecCommand(COMMAND_UPDATE_CAPTION);
    ApplyChanges();
}
//---------------------------------------------------------------------------

bool ISHTools::IfModified()
{
    if (m_bModified){
        int mr = ELog.DlgMsg(mtConfirmation, "The '%s' has been modified.\nDo you want to save your changes?",ToolsName());
        switch(mr){
        case mrYes: Save(); m_bModified = FALSE; break;
        case mrNo: m_bModified = FALSE; break;
        case mrCancel: return false;
        }
    }
    return true;
}
//---------------------------------------------------------------------------

void ISHTools::ZoomObject(bool bOnlySel)
{
    Fbox BB;
    BB.set(-5,-5,-5,5,5,5);
    EDevice.m_Camera.ZoomExtents(BB);
}
//---------------------------------------------------------------------------

AnsiString ISHTools::ViewGetCurrentItem(bool bFolderOnly)
{
    AnsiString name;
	TElTreeItem* item 	= Ext.m_Items->GetSelected();
    FHelper.MakeName	(item,0,name,bFolderOnly);
    return name;
}
//---------------------------------------------------------------------------

TElTreeItem* ISHTools::ViewGetCurrentItem()
{
	return Ext.m_Items->GetSelected();
}
//---------------------------------------------------------------------------

void ISHTools::RemoveCurrent()
{
	Ext.m_Items->RemoveSelItems();
}
//---------------------------------------------------------------------------

void ISHTools::RenameCurrent()
{
	Ext.m_Items->RenameSelItem();
}
//---------------------------------------------------------------------------

void ISHTools::OnFrame()
{
    if (m_LastSelection.Length()){
	    SetCurrentItem				(m_LastSelection.c_str(),true);
        m_LastSelection				= "";
    }
}                
void ISHTools::OnActivate()
{
    SetCurrentItem					(m_LastSelection.c_str(),true);
}
void ISHTools::OnDeactivate()
{
	Ext.m_PreviewProps->ClearProperties();
    m_LastSelection					= ViewGetCurrentItem(false);
    ResetCurrentItem				();
    Ext.m_Items->ClearList			();
}
//---------------------------------------------------------------------------


