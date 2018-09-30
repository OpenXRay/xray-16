#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "xrUICore/Buttons/UI3tButton.h"

class CUIXml;
class CUI3tButton;
class CUIWindow;
class CUIEditBox;
class CUISpinNum;
class CUICheckButton;

class CUIMpServerAdm : public CUIWindow, public CUIWndCallback
{
    typedef CUIWindow inherited;
    CUI3tButton* m_pBackBtn;

    CUIWindow* m_pMainSelectionWnd;
    CUI3tButton* m_pRestartBtn;
    CUI3tButton* m_pFastRestartBtn;
    CUI3tButton* m_pChangeWeatherBtn;
    CUI3tButton* m_pChangeGameTypeBtn;
    CUI3tButton* m_pChangeGameLimitsBtn;
    CUI3tButton* m_pVoteStopBtn;

    CUIWindow* m_pWeatherSelectionWnd;
    CUI3tButton* m_pClearWeatherBtn;
    CUI3tButton* m_pCloudyWeatherBtn;
    CUI3tButton* m_pRainWeatherBtn;
    CUI3tButton* m_pNightWeatherBtn;
    CUISpinNum* m_pWeatherChangeRateSpin;
    CUI3tButton* m_pWeatherChangeRateBtn;

    CUIWindow* m_pGameTypeSelectionWnd;
    CUI3tButton* m_pDMBtn;
    CUI3tButton* m_pTDMBtn;
    CUI3tButton* m_pCTABtn;
    CUI3tButton* m_pAHBtn;

    CUIWindow* m_pGameLimitsSelectionWnd;
    CUI3tButton* m_pSetTimeLimitBtn;
    CUIEditBox* m_pTimeLimitEdit;
    CUI3tButton* m_pSetFragLimitBtn;
    CUIEditBox* m_pFragLimitEdit;
    CUI3tButton* m_pSetArtLimitBtn;
    CUIEditBox* m_pArtLimitEdit;
    CUI3tButton* m_pSetWarmUpBtn;
    CUIEditBox* m_pWarmUpEdit;
    CUICheckButton* m_pSpectatorFECheck;
    CUICheckButton* m_pSpectatorFFCheck;
    CUICheckButton* m_pSpectatorFLCheck;
    CUICheckButton* m_pSpectatorLACheck;
    CUICheckButton* m_pSpectatorTCCheck;
    CUIEditBox* m_pInvincibleTimeEdit;
    CUI3tButton* m_pSetInvincibleTimeBtn;
    CUIEditBox* m_pDamageBlockTimeEdit;
    CUI3tButton* m_pSetDamageBlockTimeBtn;
    CUIEditBox* m_pReinforcementTimeEdit;
    CUI3tButton* m_pSetReinforcementTimeBtn;
    CUICheckButton* m_pVoteEnabledCheck;
    CUICheckButton* m_pDamBlockIndicCheck;
    CUICheckButton* m_pFriendlyNamesCheck;
    CUICheckButton* m_pFriendlyIndicCheck;
    CUICheckButton* m_pBearerCantSprintCheck;

public:
    CUIMpServerAdm();
    ~CUIMpServerAdm();
    void Init(CUIXml& xml_doc);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
    void ShowChangeWeatherBtns();
    void ShowChangeGameTypeBtns();
    void ShowChangeGameLimitsBtns();
    void OnBackBtn();
    bool IsBackBtnShown() { return m_pBackBtn->IsShown(); };
};
