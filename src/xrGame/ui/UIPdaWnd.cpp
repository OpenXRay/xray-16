#include "StdAfx.h"
#include "UIPdaWnd.h"
#include "PDA.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIInventoryUtilities.h"

#include "Level.h"
#include "UIGameCustom.h"

#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/TabControl/UITabControl.h"
#include "UIMapWnd.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "Common/object_broker.h"
#include "UIMessagesWindow.h"
#include "UIMainIngameWnd.h"
#include "xrUICore/TabControl/UITabButton.h"
#include "xrUICore/Static/UIAnimatedStatic.h"

#include "UIHelper.h"
#include "xrUICore/Hint/UIHint.h"
#include "xrUICore/Buttons/UIBtnHint.h"
#include "UITaskWnd.h"
#include "UIFactionWarWnd.h"
#include "UIActorInfo.h"
#include "UIRankingWnd.h"
#include "UILogsWnd.h"
#include "UIScriptWnd.h"

#define PDA_XML "pda.xml"

u32 g_pda_info_state = 0;

void RearrangeTabButtons(CUITabControl* pTab);
CDialogHolder* CurrentDialogHolder();

CUIPdaWnd::CUIPdaWnd() : CUIDialogWnd(CUIPdaWnd::GetDebugType())
{
    pUIMapWnd = nullptr;
    pUITaskWnd = nullptr;
    pUIFactionWarWnd = nullptr;
    pUIActorInfo = nullptr;
    pUIRankingWnd = nullptr;
    pUILogsWnd = nullptr;
    m_hint_wnd = nullptr;
    Init();
}

CUIPdaWnd::~CUIPdaWnd()
{
    if (pUIMapWnd)
        delete_data(pUIMapWnd);
    if (pUITaskWnd)
        delete_data(pUITaskWnd);
    if (pUIFactionWarWnd)
        delete_data(pUIFactionWarWnd);
    if (pUIActorInfo)
        delete_data(pUIActorInfo);
    if (pUIRankingWnd)
        delete_data(pUIRankingWnd);
    if (pUILogsWnd)
        delete_data(pUILogsWnd);
    delete_data(m_hint_wnd);
    if (UINoice)
        delete_data(UINoice);
}

void CUIPdaWnd::Init()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_XML);

    m_pActiveDialog = NULL;
    m_sActiveSection = "";

    CUIXmlInit::InitWindow(uiXml, "main", 0, this);

    UIMainPdaFrame = UIHelper::CreateStatic(uiXml, "background_static", this);
    m_caption = UIHelper::CreateStatic(uiXml, "caption_static", this);
    m_caption_const = (m_caption->GetText());
    m_clock = UIHelper::CreateStatic(uiXml, "clock_wnd", this, false);

    if (uiXml.NavigateToNode("anim_static")) // XXX: Replace with UIHelper
    {
        auto* anim_static = xr_new<CUIAnimatedStatic>();
        AttachChild(anim_static);
        anim_static->SetAutoDelete(true);
        CUIXmlInit::InitAnimatedStatic(uiXml, "anim_static", 0, anim_static);
    }

    m_btn_close = UIHelper::Create3tButton(uiXml, "close_button", this);
    m_hint_wnd = UIHelper::CreateHint(uiXml, "hint_wnd");

    if (IsGameTypeSingle())
    {
        pUIMapWnd = xr_new<CUIMapWnd>(m_hint_wnd);
        if (!pUIMapWnd->Init("pda_map.xml", "map_wnd", false))
            xr_delete(pUIMapWnd);

        pUITaskWnd = xr_new<CUITaskWnd>(m_hint_wnd);
        if (!pUITaskWnd->Init())
            xr_delete(pUITaskWnd);

        pUIFactionWarWnd = xr_new<CUIFactionWarWnd>(m_hint_wnd);
        if (!pUIFactionWarWnd->Init())
            xr_delete(pUIFactionWarWnd);

        pUIActorInfo = xr_new<CUIActorInfoWnd>();
        if (!pUIActorInfo->Init())
            xr_delete(pUIActorInfo);

        pUIRankingWnd = xr_new<CUIRankingWnd>();
        if (!pUIRankingWnd->Init())
            xr_delete(pUIRankingWnd);

        pUILogsWnd = xr_new<CUILogsWnd>();
        if (!pUILogsWnd->Init())
            xr_delete(pUILogsWnd);
    }

    UITabControl = xr_new<CUITabControl>();
    UITabControl->SetAutoDelete(true);
    AttachChild(UITabControl);
    CUIXmlInit::InitTabControl(uiXml, "tab", 0, UITabControl);
    UITabControl->SetMessageTarget(this);

    UINoice = xr_new<CUIStatic>("Noise");
    UINoice->SetAutoDelete(true);
    if (!CUIXmlInit::InitStatic(uiXml, "noice_static", 0, UINoice, false))
        xr_delete(UINoice);

    // XXX: dynamically determine if we need to rearrange the tabs
    if (ClearSkyMode)
        RearrangeTabButtons(UITabControl);
}

void CUIPdaWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    switch (msg)
    {
    case TAB_CHANGED:
    {
        if (pWnd == UITabControl)
        {
            const auto& id = UITabControl->GetActiveId();
            SetActiveSubdialog(id);
            if (pInput->IsCurrentInputTypeController())
                UI().GetUICursor().WarpToWindow(UITabControl->GetButtonById(id));
        }
        break;
    }
    case BUTTON_CLICKED:
    {
        if (pWnd == m_btn_close)
        {
            HideDialog();
        }
        break;
    }
    default:
    {
        if (m_pActiveDialog)
            m_pActiveDialog->SendMessage(pWnd, msg, pData);
    }
    };
}

void CUIPdaWnd::Show(bool status)
{
    inherited::Show(status);
    if (status)
    {
        InventoryUtilities::SendInfoToActor("ui_pda");

        if (!m_sActiveSection.empty())
            SetActiveSubdialog(m_sActiveSection);
        else
        {
            cpcstr subdialog = pUIMapWnd && !pUITaskWnd ? "eptMap" : "eptTasks";
            SetActiveSubdialog(subdialog);
            UITabControl->SetActiveTab(subdialog);
        }
    }
    else
    {
        InventoryUtilities::SendInfoToActor("ui_pda_hide");
        CurrentGameUI()->UIMainIngameWnd->SetFlashIconState_(CUIMainIngameWnd::efiPdaTask, false);
        if (m_pActiveDialog)
        {
            m_pActiveDialog->Show(false);
            m_pActiveDialog = pUITaskWnd; //hack for script window
        }
        g_btnHint->Discard();
        g_statHint->Discard();
    }
}

void CUIPdaWnd::Update()
{
    inherited::Update();

    if (m_pActiveDialog)
        m_pActiveDialog->Update();

    if (m_clock)
        m_clock->SetText(GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());

    if (pUILogsWnd)
        Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(pUILogsWnd, &CUILogsWnd::PerformWork));
}

void CUIPdaWnd::SetActiveSubdialog(const shared_str& section)
{
    //if (m_sActiveSection == section)
    //    return;

    if (m_pActiveDialog)
    {
        if (UIMainPdaFrame->IsChild(m_pActiveDialog))
            UIMainPdaFrame->DetachChild(m_pActiveDialog);
        m_pActiveDialog->Show(false);
    }

    const std::tuple<shared_str, CUIWindow*> availableWindowsList[] =
    {
        { "eptMap",         pUIMapWnd },
        { "eptTasks",       pUITaskWnd },
        { "eptFractionWar", pUIFactionWarWnd },
        { "eptStatistics",  pUIActorInfo },
        { "eptRanking",     pUIRankingWnd },
        { "eptLogs",        pUILogsWnd },
    };

    for (const auto& [id, wnd] : availableWindowsList)
    {
        if (section == id && wnd)
        {
            m_pActiveDialog = wnd;
            break;
        }
    }

    luabind::functor<CUIDialogWndEx*> functor;
    if (GEnv.ScriptEngine->functor("pda.set_active_subdialog", functor))
    {
        if (CUIDialogWndEx* scriptWnd = functor(section.c_str()))
        {
            scriptWnd->SetHolder(CurrentDialogHolder());
            m_pActiveDialog = scriptWnd;
        }
    }

    if (m_pActiveDialog == pUIActorInfo && pUIActorInfo)
    {
        InventoryUtilities::SendInfoToActor("ui_pda_actor_info");
    }

    if (m_pActiveDialog)
    {
        if (!UIMainPdaFrame->IsChild(m_pActiveDialog))
            UIMainPdaFrame->AttachChild(m_pActiveDialog);
        m_pActiveDialog->Show(true);
        m_sActiveSection = section;
        SetActiveCaption();
    }
    else
    {
        m_sActiveSection = "";
    }
}

void CUIPdaWnd::SetActiveCaption()
{
    TABS_VECTOR* btn_vec = UITabControl->GetButtonsVector();
    TABS_VECTOR::iterator it_b = btn_vec->begin();
    TABS_VECTOR::iterator it_e = btn_vec->end();
    for (; it_b != it_e; ++it_b)
    {
        if ((*it_b)->m_btn_id == m_sActiveSection)
        {
            LPCSTR cur = (*it_b)->TextItemControl()->GetText();
            string256 buf;
            strconcat(sizeof(buf), buf, m_caption_const.c_str(), cur);
            SetCaption(buf);
            return;
        }
    }
}

void CUIPdaWnd::Show_SecondTaskWnd(bool status)
{
    if (pUITaskWnd)
    {
        if (status)
        {
            SetActiveSubdialog("eptTasks");
        }
        pUITaskWnd->Show_TaskListWnd(status);
    }
}

void CUIPdaWnd::Show_MapLegendWnd(bool status)
{
    if (pUITaskWnd)
    {
        if (status)
        {
            SetActiveSubdialog("eptTasks");
        }
        pUITaskWnd->ShowMapLegend(status);
    }
}

void CUIPdaWnd::Draw()
{
    inherited::Draw();
    //.	DrawUpdatedSections();
    DrawHint();
    if (UINoice)
        UINoice->Draw(); // over all
}

void CUIPdaWnd::DrawHint()
{
    if (m_sActiveSection == "eptTasks" && pUITaskWnd)
        pUITaskWnd->DrawHint();
    else if (m_sActiveSection == "eptMap" && pUIMapWnd)
        pUIMapWnd->DrawHint();
    else if (m_sActiveSection == "eptRanking" && pUIRankingWnd)
        pUIRankingWnd->DrawHint();

    m_hint_wnd->Draw();
}

void CUIPdaWnd::UpdatePda()
{
    if (pUILogsWnd)
        pUILogsWnd->UpdateNews();

    if (m_sActiveSection == "eptTasks" && pUITaskWnd)
    {
        pUITaskWnd->ReloadTaskInfo();
    }
}

void CUIPdaWnd::UpdateRankingWnd()
{
    if (pUIRankingWnd && pUIRankingWnd->IsShown())
        pUIRankingWnd->Update();
}

void CUIPdaWnd::Reset()
{
    inherited::ResetAll();

    if (pUIMapWnd)
        pUIMapWnd->Reset();
    if (pUITaskWnd)
        pUITaskWnd->ResetAll();
    if (pUIFactionWarWnd)
        pUIFactionWarWnd->ResetAll();
    if (pUIActorInfo)
        pUIActorInfo->ResetAll();
    if (pUIRankingWnd)
        pUIRankingWnd->ResetAll();
    if (pUILogsWnd)
        pUILogsWnd->ResetAll();
}

void CUIPdaWnd::SetCaption(pcstr text) { m_caption->SetText(text); }
void RearrangeTabButtons(CUITabControl* pTab)
{
    const auto& buttons = *pTab->GetButtonsVector();

    Fvector2 pos;
    pos.set(buttons.front()->GetWndPos());

    for (const auto& btn : buttons)
    {
        btn->SetWndPos(pos);
        btn->AdjustWidthToText();
        const float size_x = btn->GetWidth() + 30.0f;
        btn->SetWidth(size_x);
        pos.x += size_x - 6.0f;
    }

    pTab->SetWidth(pos.x + 5.0f);
    pos.x = pTab->GetWndPos().x - pos.x;
    pos.y = pTab->GetWndPos().y;
    pTab->SetWndPos(pos);
}

bool CUIPdaWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (inherited::OnKeyboardAction(dik, keyboard_action))
        return true;

    switch (GetBindedAction(dik, EKeyContext::UI))
    {
    case kUI_TAB_PREV:
        if (WINDOW_KEY_PRESSED == keyboard_action)
            UITabControl->SetNextActiveTab(false, true);
        return true;

    case kUI_TAB_NEXT:
        if (WINDOW_KEY_PRESSED == keyboard_action)
            UITabControl->SetNextActiveTab(true, true);
        return true;

    hide_pda:
    case kUI_BACK:
        if (WINDOW_KEY_PRESSED == keyboard_action)
            HideDialog();
        return true;
    }

    if (IsBinded(kACTIVE_JOBS, dik))
        goto hide_pda;

    return false;
}
