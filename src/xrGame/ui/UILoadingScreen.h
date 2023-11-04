////////////////////////////////////////////////////////////////////////////
//  Created     : 19.06.2018
//  Authors     : Xottab_DUTY (OpenXRay project)
//                FozeSt
//                Unfainthful
//
//  Copyright (C) GSC Game World - 2018
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "xrEngine/ILoadingScreen.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Windows/UIWindow.h"

class CApplication;

class UILoadingScreen final : public ILoadingScreen, public CUIWindow
{
    Lock loadingLock;
    bool alwaysShowStage;

    CUIProgressBar* loadingProgress;
    CUIStatic* loadingProgressPercent;
    CUIStatic* loadingLogo;

    CUIStatic* loadingStage;
    CUIStatic* loadingHeader;
    CUIStatic* loadingTipNumber;
    CUIStatic* loadingTip;

public:
    UILoadingScreen();

    void Initialize() override;

    void Show(bool show) override;

    [[nodiscard]]
    bool IsShown() const override;

    void Update(const int stagesCompleted, const int stagesTotal) override;
    void Draw() override;

    void SetLevelLogo(const char* name) override;
    void SetStageTitle(const char* title) override;
    void SetStageTip(const char* header, const char* tipNumber, const char* tip) override;

    pcstr GetDebugType() override { return "UILoadingScreen"; }
};
