//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "UI_ShaderMain.h"
#include "UI_ShaderTools.h"
#include "xr_input.h"

//---------------------------------------------------------------------------

CShaderMain::CShaderMain()
{
    EPrefs = xr_new<CCustomPreferences>();
}
//---------------------------------------------------------------------------

CShaderMain::~CShaderMain()
{
    xr_delete(EPrefs);
}
//---------------------------------------------------------------------------

CCommandVar CShaderTool::CommandSave(CCommandVar p1, CCommandVar p2)
{
    Save(0, 0);
    ExecCommand(COMMAND_UPDATE_CAPTION);
    return TRUE;
}
CCommandVar CShaderTool::CommandSaveBackup(CCommandVar p1, CCommandVar p2)
{
    ExecCommand(COMMAND_SAVE);
    return TRUE;
}
CCommandVar CShaderTool::CommandReload(CCommandVar p1, CCommandVar p2)
{
    Reload();
    ExecCommand(COMMAND_UPDATE_CAPTION);
    return TRUE;
}
CCommandVar CShaderTool::CommandClear(CCommandVar p1, CCommandVar p2)
{
    EDevice.m_Camera.Reset();
    ExecCommand(COMMAND_UPDATE_CAPTION);
    return TRUE;
}
CCommandVar CShaderTool::CommandUpdateList(CCommandVar p1, CCommandVar p2)
{
    UpdateList();
    return TRUE;
}
CCommandVar CommandRefreshUIBar(CCommandVar p1, CCommandVar p2)
{
    /* fraTopBar->RefreshBar	();
     fraLeftBar->RefreshBar	();
     fraBottomBar->RefreshBar();*/
    return TRUE;
}
CCommandVar CommandRestoreUIBar(CCommandVar p1, CCommandVar p2)
{
    /*fraTopBar->fsStorage->RestoreFormPlacement();
    fraLeftBar->fsStorage->RestoreFormPlacement();
    fraBottomBar->fsStorage->RestoreFormPlacement();*/
    return TRUE;
}
CCommandVar CommandSaveUIBar(CCommandVar p1, CCommandVar p2)
{
    /* fraTopBar->fsStorage->SaveFormPlacement();
     fraLeftBar->fsStorage->SaveFormPlacement();
     fraBottomBar->fsStorage->SaveFormPlacement();*/
    return TRUE;
}
CCommandVar CommandUpdateToolBar(CCommandVar p1, CCommandVar p2)
{
    // fraLeftBar->UpdateBar();
    return TRUE;
}
CCommandVar CommandUpdateCaption(CCommandVar p1, CCommandVar p2)
{
    // frmMain->UpdateCaption();
    return TRUE;
}

void CShaderMain::RegisterCommands()
{
    inherited::RegisterCommands();
    // tools
    REGISTER_CMD_CE(COMMAND_SAVE, "File\\Save", STools, CShaderTool::CommandSave, true);
    REGISTER_CMD_C(COMMAND_SAVE_BACKUP, STools, CShaderTool::CommandSaveBackup);
    REGISTER_CMD_CE(COMMAND_LOAD, "File\\Reload", STools, CShaderTool::CommandReload, true);
    REGISTER_CMD_CE(COMMAND_CLEAR, "File\\Clear", STools, CShaderTool::CommandClear, true);
    REGISTER_CMD_CE(COMMAND_UPDATE_LIST, "Update List", STools, CShaderTool::CommandUpdateList, true);
    REGISTER_CMD_S(COMMAND_REFRESH_UI_BAR, CommandRefreshUIBar);
    REGISTER_CMD_S(COMMAND_RESTORE_UI_BAR, CommandRestoreUIBar);
    REGISTER_CMD_S(COMMAND_SAVE_UI_BAR, CommandSaveUIBar);
    REGISTER_CMD_S(COMMAND_UPDATE_TOOLBAR, CommandUpdateToolBar);
    REGISTER_CMD_S(COMMAND_UPDATE_CAPTION, CommandUpdateCaption);
}

void CShaderMain::OnDrawUI()
{
    TUI::OnDrawUI();
    for (auto &tool : STools->m_Tools)
    {
        tool.second->OnDrawUI();
    }
}

char *CShaderMain::GetCaption()
{
    return (LPSTR)STools->CurrentToolsName(); // "shaders&materials";
}

bool CShaderMain::ApplyShortCut(DWORD Key, TShiftState Shift)
{
    return inherited::ApplyShortCut(Key, Shift);
}
//---------------------------------------------------------------------------

bool CShaderMain::ApplyGlobalShortCut(DWORD Key, TShiftState Shift)
{
    return inherited::ApplyGlobalShortCut(Key, Shift);
}
//---------------------------------------------------------------------------

void CShaderMain::RealUpdateScene()
{
    inherited::RealUpdateScene();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Common
//---------------------------------------------------------------------------
void CShaderMain::ResetStatus()
{
    /*VERIFY(m_bReady);
    if (fraBottomBar->paStatus->Caption!=""){
        fraBottomBar->paStatus->Caption=""; fraBottomBar->paStatus->Repaint();
    }*/
}
void CShaderMain::SetStatus(LPCSTR s, bool bOutLog)
{
    /*	VERIFY(m_bReady);
        if (fraBottomBar->paStatus->Caption!=s){
            fraBottomBar->paStatus->Caption=s; fraBottomBar->paStatus->Repaint();
            if (bOutLog&&s&&s[0]) ELog.Msg(mtInformation,s);
        }*/
}
void CShaderMain::ProgressDraw()
{
    inherited::ProgressDraw();
    // fraBottomBar->RedrawBar();
}
//---------------------------------------------------------------------------
void CShaderMain::OutCameraPos()
{
    /*	VERIFY(m_bReady);
        xr_string s;
        const Fvector& c 	= EDevice.m_Camera.GetPosition();
        s.sprintf("C: %3.1f, %3.1f, %3.1f",c.x,c.y,c.z);
    //	const Fvector& hpb 	= EDevice.m_Camera.GetHPB();
    //	s.sprintf(" Cam: %3.1f�, %3.1f�, %3.1f�",rad2deg(hpb.y),rad2deg(hpb.x),rad2deg(hpb.z));
        fraBottomBar->paCamera->Caption=s; fraBottomBar->paCamera->Repaint();*/
}
//---------------------------------------------------------------------------
void CShaderMain::OutUICursorPos()
{
    /*	VERIFY(fraBottomBar);
        xr_string s; POINT pt;
        GetCursorPos(&pt);
        s.sprintf("Cur: %d, %d",pt.x,pt.y);
        fraBottomBar->paUICursor->Caption=s; fraBottomBar->paUICursor->Repaint();*/
}

void CShaderMain::OutGridSize()
{
    /*VERIFY(fraBottomBar);
    xr_string s;
    s.sprintf("Grid: %1.1f",EPrefs->grid_cell_size);
    fraBottomBar->paGridSquareSize->Caption=s; fraBottomBar->paGridSquareSize->Repaint();*/
}

void CShaderMain::OutInfo()
{
    // fraBottomBar->paSel->Caption = Tools->GetInfo();
}

void CShaderMain::RealQuit()
{
    UI->Quit();
}

#include "Resources\resource.h"

HICON CShaderMain::EditorIcon()
{
    return LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
}
