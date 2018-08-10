#include "stdafx.h"
#include "UIPdaSpot.h"
#ifdef COC_USER_SPOT
#include <dinput.h>
#include "Level.h"
#include "map_manager.h"
#include "map_location.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/Static/UIStatic.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "string_table.h"

CUIPdaSpot::CUIPdaSpot()
{
    m_mainWnd = false;
    m_levelName = nullptr;
    m_position = Fvector();

    m_spotID = u16(-1);
    m_spotType = READ_IF_EXISTS(pSettings, r_string, "user_spots", "spot_type", "treasure");

    InitControls();
}

CUIPdaSpot::~CUIPdaSpot()
{
}

void CUIPdaSpot::Init(u16 spot_id, LPCSTR level_name, Fvector pos, bool main_wnd)
{
    m_mainWnd = main_wnd;
    m_levelName = level_name;
    m_position = pos;
    if (!m_mainWnd)
        m_spotID = spot_id;
    else
        m_spotID = u16(-1);

    if (!m_mainWnd)
    {
        CMapLocation* ml = Level().MapManager().GetMapLocation(m_spotType, m_spotID);
        if (!ml) return;
        m_editBox->SetText(ml->GetHint());
        ml->HighlightSpot(true, Fcolor().set(255.f, 36.f, 0.f, 255.f));
    }
}

void CUIPdaSpot::InitControls()
{
    this->SetWndRect(Frect().set(0.0f, 0.0f, 1024.f, 768.f));

    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, "pda_spot.xml");

    m_background = UIHelper::CreateStatic(uiXml, "background", this);
    m_editBox = UIHelper::CreateEditBox(uiXml, "spot_name_edit", this);

    m_btn_ok = UIHelper::Create3tButton(uiXml, "btn_apply", this);
    Register(m_btn_ok);
    AddCallback(m_btn_ok, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIPdaSpot::OnApply));

    m_btn_cancel = UIHelper::Create3tButton(uiXml, "btn_cancel", this);
    Register(m_btn_cancel);
    AddCallback(m_btn_cancel, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIPdaSpot::OnExit));
}

void CUIPdaSpot::OnAdd(CUIWindow* ui, void* d)
{
    CMapLocation* ml = Level().MapManager().AddUserLocation(m_spotType, m_levelName, m_position);
    ml->SetHint(m_editBox->GetText());
    ml->SetSerializable(true);

    OnExit(ui, d);
}

void CUIPdaSpot::OnApply(CUIWindow* ui, void* d)
{
    if (m_mainWnd)
    {
        OnAdd(ui, d);
        return;
    }

    CMapLocation* ml = Level().MapManager().GetMapLocation(m_spotType, m_spotID);
    if (!ml)
        return;

    if (m_editBox->GetText() != ml->GetHint())
        ml->SetHint(m_editBox->GetText());

    OnExit(ui, d);
}

void CUIPdaSpot::OnExit(CUIWindow* w, void* d)
{
    Exit();
}

void CUIPdaSpot::Exit()
{
    if (!m_mainWnd)
    {
        CMapLocation* ml = Level().MapManager().GetMapLocation(m_spotType, m_spotID);
        if (!ml) return;
        ml->HighlightSpot(false, Fcolor().set(0.f, 0.f, 0.f, 0.f));
    }

    m_mainWnd = false;
    m_levelName = nullptr;
    m_position = Fvector();
    m_spotID = u16(-1);

    m_editBox->ClearText();

    HideDialog();
}

bool CUIPdaSpot::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    switch (dik)
    {
    case DIK_ESCAPE:
    {
        if (IsShown())
        {
            Exit();
            return true;
        }
    }break;
    }

    return base_class::OnKeyboardAction(dik, keyboard_action);
}

void CUIPdaSpot::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    CUIWndCallback::OnEvent(pWnd, msg, pData);
}
#endif
