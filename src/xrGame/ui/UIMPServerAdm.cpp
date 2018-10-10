#include "StdAfx.h"
#include "UIMPServerAdm.h"
#include "UIXmlInit.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/SpinBox/UISpinNum.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrEngine/XR_IOConsole.h"
#include "UIDialogWnd.h"

CUIMpServerAdm::CUIMpServerAdm()
{
    m_pBackBtn = new CUI3tButton();
    m_pBackBtn->SetAutoDelete(true);
    AttachChild(m_pBackBtn);
    m_pBackBtn->Show(false);

    m_pMainSelectionWnd = new CUIWindow();
    m_pMainSelectionWnd->SetAutoDelete(true);
    AttachChild(m_pMainSelectionWnd);

    m_pRestartBtn = new CUI3tButton();
    m_pRestartBtn->SetAutoDelete(true);
    m_pMainSelectionWnd->AttachChild(m_pRestartBtn);
    m_pRestartBtn->SetMessageTarget(this);

    m_pFastRestartBtn = new CUI3tButton();
    m_pFastRestartBtn->SetAutoDelete(true);
    m_pMainSelectionWnd->AttachChild(m_pFastRestartBtn);
    m_pFastRestartBtn->SetMessageTarget(this);

    m_pChangeWeatherBtn = new CUI3tButton();
    m_pChangeWeatherBtn->SetAutoDelete(true);
    m_pMainSelectionWnd->AttachChild(m_pChangeWeatherBtn);
    m_pChangeWeatherBtn->SetMessageTarget(this);

    m_pChangeGameTypeBtn = new CUI3tButton();
    m_pChangeGameTypeBtn->SetAutoDelete(true);
    m_pMainSelectionWnd->AttachChild(m_pChangeGameTypeBtn);
    m_pChangeGameTypeBtn->SetMessageTarget(this);

    m_pChangeGameLimitsBtn = new CUI3tButton();
    m_pChangeGameLimitsBtn->SetAutoDelete(true);
    m_pMainSelectionWnd->AttachChild(m_pChangeGameLimitsBtn);
    m_pChangeGameLimitsBtn->SetMessageTarget(this);

    m_pVoteStopBtn = new CUI3tButton();
    m_pVoteStopBtn->SetAutoDelete(true);
    m_pMainSelectionWnd->AttachChild(m_pVoteStopBtn);
    m_pVoteStopBtn->SetMessageTarget(this);

    m_pWeatherSelectionWnd = new CUIWindow();
    m_pWeatherSelectionWnd->SetAutoDelete(true);
    AttachChild(m_pWeatherSelectionWnd);

    m_pClearWeatherBtn = new CUI3tButton();
    m_pClearWeatherBtn->SetAutoDelete(true);
    m_pWeatherSelectionWnd->AttachChild(m_pClearWeatherBtn);
    m_pClearWeatherBtn->SetMessageTarget(this);

    m_pCloudyWeatherBtn = new CUI3tButton();
    m_pCloudyWeatherBtn->SetAutoDelete(true);
    m_pWeatherSelectionWnd->AttachChild(m_pCloudyWeatherBtn);
    m_pCloudyWeatherBtn->SetMessageTarget(this);

    m_pRainWeatherBtn = new CUI3tButton();
    m_pRainWeatherBtn->SetAutoDelete(true);
    m_pWeatherSelectionWnd->AttachChild(m_pRainWeatherBtn);
    m_pRainWeatherBtn->SetMessageTarget(this);

    m_pNightWeatherBtn = new CUI3tButton();
    m_pNightWeatherBtn->SetAutoDelete(true);
    m_pWeatherSelectionWnd->AttachChild(m_pNightWeatherBtn);
    m_pNightWeatherBtn->SetMessageTarget(this);

    m_pWeatherChangeRateSpin = new CUISpinNum();
    m_pWeatherChangeRateSpin->SetAutoDelete(true);
    m_pWeatherSelectionWnd->AttachChild(m_pWeatherChangeRateSpin);

    m_pWeatherChangeRateBtn = new CUI3tButton();
    m_pWeatherChangeRateBtn->SetAutoDelete(true);
    m_pWeatherSelectionWnd->AttachChild(m_pWeatherChangeRateBtn);
    m_pWeatherChangeRateBtn->SetMessageTarget(this);
    m_pWeatherSelectionWnd->Show(false);

    m_pGameTypeSelectionWnd = new CUIWindow();
    m_pGameTypeSelectionWnd->SetAutoDelete(true);
    AttachChild(m_pGameTypeSelectionWnd);

    m_pDMBtn = new CUI3tButton();
    m_pDMBtn->SetAutoDelete(true);
    m_pGameTypeSelectionWnd->AttachChild(m_pDMBtn);
    m_pDMBtn->SetMessageTarget(this);

    m_pTDMBtn = new CUI3tButton();
    m_pTDMBtn->SetAutoDelete(true);
    m_pGameTypeSelectionWnd->AttachChild(m_pTDMBtn);
    m_pTDMBtn->SetMessageTarget(this);

    m_pCTABtn = new CUI3tButton();
    m_pCTABtn->SetAutoDelete(true);
    m_pGameTypeSelectionWnd->AttachChild(m_pCTABtn);
    m_pCTABtn->SetMessageTarget(this);

    m_pAHBtn = new CUI3tButton();
    m_pAHBtn->SetAutoDelete(true);
    m_pGameTypeSelectionWnd->AttachChild(m_pAHBtn);
    m_pAHBtn->SetMessageTarget(this);
    m_pGameTypeSelectionWnd->Show(false);

    m_pGameLimitsSelectionWnd = new CUIWindow();
    m_pGameLimitsSelectionWnd->SetAutoDelete(true);
    AttachChild(m_pGameLimitsSelectionWnd);

    m_pSetTimeLimitBtn = new CUI3tButton();
    m_pSetTimeLimitBtn->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSetTimeLimitBtn);
    m_pSetTimeLimitBtn->SetMessageTarget(this);
    m_pTimeLimitEdit = new CUIEditBox();
    m_pTimeLimitEdit->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pTimeLimitEdit);

    m_pSetFragLimitBtn = new CUI3tButton();
    m_pSetFragLimitBtn->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSetFragLimitBtn);
    m_pSetFragLimitBtn->SetMessageTarget(this);
    m_pFragLimitEdit = new CUIEditBox();
    m_pFragLimitEdit->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pFragLimitEdit);

    m_pSetArtLimitBtn = new CUI3tButton();
    m_pSetArtLimitBtn->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSetArtLimitBtn);
    m_pSetArtLimitBtn->SetMessageTarget(this);
    m_pArtLimitEdit = new CUIEditBox();
    m_pArtLimitEdit->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pArtLimitEdit);

    m_pSetWarmUpBtn = new CUI3tButton();
    m_pSetWarmUpBtn->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSetWarmUpBtn);
    m_pSetWarmUpBtn->SetMessageTarget(this);
    m_pWarmUpEdit = new CUIEditBox();
    m_pWarmUpEdit->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pWarmUpEdit);

    m_pSpectatorFECheck = new CUICheckButton();
    m_pSpectatorFECheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSpectatorFECheck);
    m_pSpectatorFECheck->SetMessageTarget(this);

    m_pSpectatorFFCheck = new CUICheckButton();
    m_pSpectatorFFCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSpectatorFFCheck);
    m_pSpectatorFFCheck->SetMessageTarget(this);

    m_pSpectatorFLCheck = new CUICheckButton();
    m_pSpectatorFLCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSpectatorFLCheck);
    m_pSpectatorFLCheck->SetMessageTarget(this);

    m_pSpectatorLACheck = new CUICheckButton();
    m_pSpectatorLACheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSpectatorLACheck);
    m_pSpectatorLACheck->SetMessageTarget(this);

    m_pSpectatorTCCheck = new CUICheckButton();
    m_pSpectatorTCCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSpectatorTCCheck);
    m_pSpectatorTCCheck->SetMessageTarget(this);

    m_pInvincibleTimeEdit = new CUIEditBox();
    m_pInvincibleTimeEdit->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pInvincibleTimeEdit);
    m_pSetInvincibleTimeBtn = new CUI3tButton();
    m_pSetInvincibleTimeBtn->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSetInvincibleTimeBtn);
    m_pSetInvincibleTimeBtn->SetMessageTarget(this);

    m_pDamageBlockTimeEdit = new CUIEditBox();
    m_pDamageBlockTimeEdit->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pDamageBlockTimeEdit);
    m_pSetDamageBlockTimeBtn = new CUI3tButton();
    m_pSetDamageBlockTimeBtn->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSetDamageBlockTimeBtn);
    m_pSetDamageBlockTimeBtn->SetMessageTarget(this);

    m_pReinforcementTimeEdit = new CUIEditBox();
    m_pReinforcementTimeEdit->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pReinforcementTimeEdit);
    m_pSetReinforcementTimeBtn = new CUI3tButton();
    m_pSetReinforcementTimeBtn->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pSetReinforcementTimeBtn);
    m_pSetReinforcementTimeBtn->SetMessageTarget(this);

    m_pVoteEnabledCheck = new CUICheckButton();
    m_pVoteEnabledCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pVoteEnabledCheck);
    m_pVoteEnabledCheck->SetMessageTarget(this);

    m_pDamBlockIndicCheck = new CUICheckButton();
    m_pDamBlockIndicCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pDamBlockIndicCheck);
    m_pDamBlockIndicCheck->SetMessageTarget(this);

    m_pFriendlyNamesCheck = new CUICheckButton();
    m_pFriendlyNamesCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pFriendlyNamesCheck);
    m_pFriendlyNamesCheck->SetMessageTarget(this);

    m_pFriendlyIndicCheck = new CUICheckButton();
    m_pFriendlyIndicCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pFriendlyIndicCheck);
    m_pFriendlyIndicCheck->SetMessageTarget(this);

    m_pBearerCantSprintCheck = new CUICheckButton();
    m_pBearerCantSprintCheck->SetAutoDelete(true);
    m_pGameLimitsSelectionWnd->AttachChild(m_pBearerCantSprintCheck);
    m_pBearerCantSprintCheck->SetMessageTarget(this);
    m_pGameLimitsSelectionWnd->Show(false);
}

CUIMpServerAdm::~CUIMpServerAdm() {}
void CUIMpServerAdm::Init(CUIXml& xml_doc)
{
    CUIXmlInit::InitWindow(xml_doc, "server_adm", 0, this);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:back_button", 0, m_pBackBtn);

    CUIXmlInit::InitWindow(xml_doc, "server_adm:main_selection_wnd", 0, m_pMainSelectionWnd);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:main_selection_wnd:restart_button", 0, m_pRestartBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:main_selection_wnd:fast_restart_button", 0, m_pFastRestartBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:main_selection_wnd:change_weather_button", 0, m_pChangeWeatherBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:main_selection_wnd:change_game_type_button", 0, m_pChangeGameTypeBtn);
    CUIXmlInit::Init3tButton(
        xml_doc, "server_adm:main_selection_wnd:change_game_limits_button", 0, m_pChangeGameLimitsBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:main_selection_wnd:vote_stop_button", 0, m_pVoteStopBtn);

    CUIXmlInit::InitWindow(xml_doc, "server_adm:weather_selection_wnd", 0, m_pWeatherSelectionWnd);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:weather_selection_wnd:clear_weather_button", 0, m_pClearWeatherBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:weather_selection_wnd:cloudy_weather_button", 0, m_pCloudyWeatherBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:weather_selection_wnd:rain_weather_button", 0, m_pRainWeatherBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:weather_selection_wnd:night_weather_button", 0, m_pNightWeatherBtn);
    CUIXmlInit::InitSpin(
        xml_doc, "server_adm:weather_selection_wnd:weather_change_rate_spin", 0, m_pWeatherChangeRateSpin);
    CUIXmlInit::Init3tButton(
        xml_doc, "server_adm:weather_selection_wnd:set_weather_rate_button", 0, m_pWeatherChangeRateBtn);

    CUIXmlInit::InitWindow(xml_doc, "server_adm:game_type_selection_wnd", 0, m_pGameTypeSelectionWnd);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_type_selection_wnd:dm_button", 0, m_pDMBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_type_selection_wnd:tdm_button", 0, m_pTDMBtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_type_selection_wnd:cta_button", 0, m_pCTABtn);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_type_selection_wnd:ah_button", 0, m_pAHBtn);

    CUIXmlInit::InitWindow(xml_doc, "server_adm:game_limits_selection_wnd", 0, m_pGameLimitsSelectionWnd);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_limits_selection_wnd:time_limit_button", 0, m_pSetTimeLimitBtn);
    CUIXmlInit::InitEditBox(xml_doc, "server_adm:game_limits_selection_wnd:time_limit_edit", 0, m_pTimeLimitEdit);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_limits_selection_wnd:frag_limit_button", 0, m_pSetFragLimitBtn);
    CUIXmlInit::InitEditBox(xml_doc, "server_adm:game_limits_selection_wnd:frag_limit_edit", 0, m_pFragLimitEdit);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_limits_selection_wnd:art_limit_button", 0, m_pSetArtLimitBtn);
    CUIXmlInit::InitEditBox(xml_doc, "server_adm:game_limits_selection_wnd:art_limit_edit", 0, m_pArtLimitEdit);
    CUIXmlInit::Init3tButton(xml_doc, "server_adm:game_limits_selection_wnd:warm_up_button", 0, m_pSetWarmUpBtn);
    CUIXmlInit::InitEditBox(xml_doc, "server_adm:game_limits_selection_wnd:warm_up_edit", 0, m_pWarmUpEdit);
    CUIXmlInit::InitCheck(xml_doc, "server_adm:game_limits_selection_wnd:spectator_fe_check", 0, m_pSpectatorFECheck);
    CUIXmlInit::InitCheck(xml_doc, "server_adm:game_limits_selection_wnd:spectator_ff_check", 0, m_pSpectatorFFCheck);
    CUIXmlInit::InitCheck(xml_doc, "server_adm:game_limits_selection_wnd:spectator_fl_check", 0, m_pSpectatorFLCheck);
    CUIXmlInit::InitCheck(xml_doc, "server_adm:game_limits_selection_wnd:spectator_la_check", 0, m_pSpectatorLACheck);
    CUIXmlInit::InitCheck(xml_doc, "server_adm:game_limits_selection_wnd:spectator_tc_check", 0, m_pSpectatorTCCheck);
    CUIXmlInit::InitEditBox(
        xml_doc, "server_adm:game_limits_selection_wnd:invincible_time_edit", 0, m_pInvincibleTimeEdit);
    CUIXmlInit::Init3tButton(
        xml_doc, "server_adm:game_limits_selection_wnd:invincible_time_button", 0, m_pSetInvincibleTimeBtn);
    CUIXmlInit::InitEditBox(
        xml_doc, "server_adm:game_limits_selection_wnd:damage_block_time_edit", 0, m_pDamageBlockTimeEdit);
    CUIXmlInit::Init3tButton(
        xml_doc, "server_adm:game_limits_selection_wnd:damage_block_time_button", 0, m_pSetDamageBlockTimeBtn);
    CUIXmlInit::InitEditBox(
        xml_doc, "server_adm:game_limits_selection_wnd:reinforcement_time_edit", 0, m_pReinforcementTimeEdit);
    CUIXmlInit::Init3tButton(
        xml_doc, "server_adm:game_limits_selection_wnd:reinforcement_time_button", 0, m_pSetReinforcementTimeBtn);
    CUIXmlInit::InitCheck(xml_doc, "server_adm:game_limits_selection_wnd:vote_enabled_check", 0, m_pVoteEnabledCheck);
    CUIXmlInit::InitCheck(
        xml_doc, "server_adm:game_limits_selection_wnd:dam_block_indic_check", 0, m_pDamBlockIndicCheck);
    CUIXmlInit::InitCheck(
        xml_doc, "server_adm:game_limits_selection_wnd:friendly_names_check", 0, m_pFriendlyNamesCheck);
    CUIXmlInit::InitCheck(
        xml_doc, "server_adm:game_limits_selection_wnd:friendly_indic_check", 0, m_pFriendlyIndicCheck);
    CUIXmlInit::InitCheck(
        xml_doc, "server_adm:game_limits_selection_wnd:bearer_cant_sprint_check", 0, m_pBearerCantSprintCheck);

    m_pSpectatorFECheck->SetCurrentOptValue();
    m_pSpectatorFFCheck->SetCurrentOptValue();
    m_pSpectatorFLCheck->SetCurrentOptValue();
    m_pSpectatorLACheck->SetCurrentOptValue();
    m_pSpectatorTCCheck->SetCurrentOptValue();
    m_pVoteEnabledCheck->SetCurrentOptValue();
    m_pDamBlockIndicCheck->SetCurrentOptValue();
    m_pFriendlyNamesCheck->SetCurrentOptValue();
    m_pFriendlyIndicCheck->SetCurrentOptValue();
    m_pBearerCantSprintCheck->SetCurrentOptValue();
}

void CUIMpServerAdm::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    switch (msg)
    {
    case BUTTON_CLICKED:
    {
        if (pWnd == m_pBackBtn)
            OnBackBtn();
        else if (pWnd == m_pChangeWeatherBtn)
            ShowChangeWeatherBtns();
        else if (pWnd == m_pChangeGameTypeBtn)
            ShowChangeGameTypeBtns();
        else if (pWnd == m_pChangeGameLimitsBtn)
            ShowChangeGameLimitsBtns();
        else if (pWnd == m_pVoteStopBtn)
            Console->Execute("ra sv_votestop");
        else if (pWnd == m_pRestartBtn)
        {
            Console->Execute("ra g_restart");
            smart_cast<CUIDialogWnd*>(GetParent())->HideDialog();
        }
        else if (pWnd == m_pFastRestartBtn)
        {
            Console->Execute("ra g_restart_fast");
            smart_cast<CUIDialogWnd*>(GetParent())->HideDialog();
        }
        else if (pWnd == m_pClearWeatherBtn)
            Console->Execute("ra sv_setenvtime 9:00");
        else if (pWnd == m_pCloudyWeatherBtn)
            Console->Execute("ra sv_setenvtime 13:00");
        else if (pWnd == m_pRainWeatherBtn)
            Console->Execute("ra sv_setenvtime 16:00");
        else if (pWnd == m_pNightWeatherBtn)
            Console->Execute("ra sv_setenvtime 01:00");
        else if (pWnd == m_pWeatherChangeRateBtn)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra mm_net_weather_rateofchange %d", m_pWeatherChangeRateSpin->Value());
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pDMBtn)
        {
            Console->Execute("ra sv_changegametype dm");
            smart_cast<CUIDialogWnd*>(GetParent())->HideDialog();
        }
        else if (pWnd == m_pTDMBtn)
        {
            Console->Execute("ra sv_changegametype tdm");
            smart_cast<CUIDialogWnd*>(GetParent())->HideDialog();
        }
        else if (pWnd == m_pCTABtn)
        {
            Console->Execute("ra sv_changegametype cta");
            smart_cast<CUIDialogWnd*>(GetParent())->HideDialog();
        }
        else if (pWnd == m_pAHBtn)
        {
            Console->Execute("ra sv_changegametype ah");
            smart_cast<CUIDialogWnd*>(GetParent())->HideDialog();
        }
        else if (pWnd == m_pSetTimeLimitBtn)
        {
            if (xr_strcmp(m_pTimeLimitEdit->GetText(), "") != 0)
            {
                string512 tmp_string;
                xr_sprintf(tmp_string, "ra sv_timelimit %s", m_pTimeLimitEdit->GetText());
                Console->Execute(tmp_string);
                m_pTimeLimitEdit->ClearText();
            }
        }
        else if (pWnd == m_pSetFragLimitBtn)
        {
            if (xr_strcmp(m_pFragLimitEdit->GetText(), "") != 0)
            {
                string512 tmp_string;
                xr_sprintf(tmp_string, "ra sv_fraglimit %s", m_pFragLimitEdit->GetText());
                Console->Execute(tmp_string);
                m_pFragLimitEdit->ClearText();
            }
        }
        else if (pWnd == m_pSetArtLimitBtn)
        {
            if (xr_strcmp(m_pArtLimitEdit->GetText(), "") != 0)
            {
                string512 tmp_string;
                xr_sprintf(tmp_string, "ra sv_artefacts_count %s", m_pArtLimitEdit->GetText());
                Console->Execute(tmp_string);
                m_pArtLimitEdit->ClearText();
            }
        }
        else if (pWnd == m_pSetWarmUpBtn)
        {
            if (xr_strcmp(m_pWarmUpEdit->GetText(), "") != 0)
            {
                string512 tmp_string;
                xr_sprintf(tmp_string, "ra sv_warm_up %s", m_pWarmUpEdit->GetText());
                Console->Execute(tmp_string);
                m_pWarmUpEdit->ClearText();
            }
        }
        else if (pWnd == m_pSpectatorFECheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_spectr_firsteye %d", m_pSpectatorFECheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pSpectatorFFCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_spectr_freefly %d", m_pSpectatorFFCheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pSpectatorFLCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_spectr_freelook %d", m_pSpectatorFLCheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pSpectatorLACheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_spectr_lookat %d", m_pSpectatorLACheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pSpectatorTCCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_spectr_teamcamera %d", m_pSpectatorTCCheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pSetInvincibleTimeBtn)
        {
            if (xr_strcmp(m_pInvincibleTimeEdit->GetText(), "") != 0)
            {
                string512 tmp_string;
                xr_sprintf(tmp_string, "ra sv_invincible_time %s", m_pInvincibleTimeEdit->GetText());
                Console->Execute(tmp_string);
                m_pInvincibleTimeEdit->ClearText();
            }
        }
        else if (pWnd == m_pSetDamageBlockTimeBtn)
        {
            if (xr_strcmp(m_pDamageBlockTimeEdit->GetText(), "") != 0)
            {
                string512 tmp_string;
                xr_sprintf(tmp_string, "ra sv_dmgblocktime %s", m_pDamageBlockTimeEdit->GetText());
                Console->Execute(tmp_string);
                m_pDamageBlockTimeEdit->ClearText();
            }
        }
        else if (pWnd == m_pSetReinforcementTimeBtn)
        {
            if (xr_strcmp(m_pReinforcementTimeEdit->GetText(), "") != 0)
            {
                string512 tmp_string;
                xr_sprintf(tmp_string, "ra sv_reinforcement_time %s", m_pReinforcementTimeEdit->GetText());
                Console->Execute(tmp_string);
                m_pReinforcementTimeEdit->ClearText();
            }
        }
        else if (pWnd == m_pVoteEnabledCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_vote_enabled %d", m_pVoteEnabledCheck->GetCheck() ? 0x00ff : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pDamBlockIndicCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_dmgblockindicator %d", m_pDamBlockIndicCheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pFriendlyNamesCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_friendly_names %d", m_pFriendlyNamesCheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pFriendlyIndicCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_friendly_indicators %d", m_pFriendlyIndicCheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        else if (pWnd == m_pBearerCantSprintCheck)
        {
            string512 tmp_string;
            xr_sprintf(tmp_string, "ra sv_bearercantsprint %d", m_pBearerCantSprintCheck->GetCheck() ? 1 : 0);
            Console->Execute(tmp_string);
        }
        break;
    }
    };
}

void CUIMpServerAdm::ShowChangeWeatherBtns()
{
    m_pMainSelectionWnd->Show(false);
    m_pBackBtn->Show(true);
    m_pWeatherSelectionWnd->Show(true);
}

void CUIMpServerAdm::ShowChangeGameTypeBtns()
{
    m_pMainSelectionWnd->Show(false);
    m_pBackBtn->Show(true);
    m_pGameTypeSelectionWnd->Show(true);
}

void CUIMpServerAdm::ShowChangeGameLimitsBtns()
{
    m_pMainSelectionWnd->Show(false);
    m_pBackBtn->Show(true);
    m_pGameLimitsSelectionWnd->Show(true);
}
void CUIMpServerAdm::OnBackBtn()
{
    m_pMainSelectionWnd->Show(true);
    m_pBackBtn->Show(false);
    m_pWeatherSelectionWnd->Show(false);
    m_pGameTypeSelectionWnd->Show(false);
    m_pGameLimitsSelectionWnd->Show(false);
}
