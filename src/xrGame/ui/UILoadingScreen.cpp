////////////////////////////////////////////////////////////////////////////
//  Created     : 19.06.2018
//  Authors     : Xottab_DUTY (OpenXRay project)
//                FozeSt
//                Unfainthful
//
//  Copyright (C) GSC Game World - 2018
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UILoadingScreen.h"
#include "UILoadingScreenHardcoded.h"

#include "xrEngine/x_ray.h"
#include "xrEngine/GameFont.h"
#include "UIHelper.h"

extern ENGINE_API int ps_rs_loading_stages;

UILoadingScreen::UILoadingScreen()
    : loadingProgressBackground(nullptr), loadingProgress(nullptr),
      loadingProgressPercent(nullptr), loadingLogo(nullptr),
      loadingStage(nullptr), loadingHeader(nullptr),
      loadingTipNumber(nullptr), loadingTip(nullptr)
{
    UILoadingScreen::Initialize();
}

void UILoadingScreen::Initialize()
{
    CUIXml uiXml;
    bool loaded = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_mm_loading_screen.xml");

    if (!loaded) // Robustness? Yes!
    {
        if (UI().is_widescreen())
            uiXml.Set(LoadingScreenXML16x9);
        else
            uiXml.Set(LoadingScreenXML);
    }

    const auto loadProgressBar = [&]()
    {
        loadingProgressBackground = UIHelper::CreateStatic(uiXml, "loading_progress_background", this, false);
        loadingProgress = UIHelper::CreateProgressBar(uiXml, "loading_progress", this);
    };

    const auto loadBackground = [&]
    {
        CUIXmlInit::InitWindow(uiXml, "background", 0, this);
    };

    const auto node = uiXml.NavigateToNodeWithAttribute("loading_progress", "under_background", "0");
    if (node)
    {
        loadBackground();
        loadProgressBar();
    }
    else
    {
        loadProgressBar();
        loadBackground();
    }

    loadingLogo = UIHelper::CreateStatic(uiXml, "loading_logo", this);
    loadingProgressPercent = UIHelper::CreateStatic(uiXml, "loading_progress_percent", this, false);
    loadingStage = UIHelper::CreateStatic(uiXml, "loading_stage", this, false);
    loadingHeader = UIHelper::CreateStatic(uiXml, "loading_header", this, false);
    loadingTipNumber = UIHelper::CreateStatic(uiXml, "loading_tip_number", this, false);
    loadingTip = UIHelper::CreateStatic(uiXml, "loading_tip", this, false);
}

void UILoadingScreen::Update(const int stagesCompleted, const int stagesTotal)
{
    ScopeLock scope(&loadingLock);

    const float progress = float(stagesCompleted) / stagesTotal * loadingProgress->GetRange_max();

    if (loadingProgress->GetProgressPos() < progress)
        loadingProgress->SetProgressPos(progress);

    if (loadingProgressPercent)
    {
        char buf[5];
        xr_sprintf(buf, "%.0f%%", loadingProgress->GetProgressPos());
        loadingProgressPercent->TextItemControl()->SetText(buf);
    }

    CUIWindow::Update();
    Draw();
}

void UILoadingScreen::ForceDrop()
{
    ScopeLock scope(&loadingLock);

    const float prev = loadingProgress->m_inertion;
    const float maximal = loadingProgress->GetRange_max();

    loadingProgress->m_inertion = 0.0f;
    loadingProgress->SetProgressPos(loadingProgress->GetRange_min());

    for (int i = 0; i < int(maximal); ++i)
    {
        loadingProgress->Update();
    }

    loadingProgress->m_inertion = prev;
}

void UILoadingScreen::ForceFinish()
{
    ScopeLock scope(&loadingLock);

    const float prev = loadingProgress->m_inertion;
    const float maximal = loadingProgress->GetRange_max();

    loadingProgress->m_inertion = 0.0f;
    loadingProgress->SetProgressPos(maximal);
    
    for (int i = 0; i < int(maximal); ++i)
    {
        loadingProgress->Update();
    }

    loadingProgress->m_inertion = prev;
}

void UILoadingScreen::SetLevelLogo(const char* name)
{
    ScopeLock scope(&loadingLock);

    loadingLogo->InitTexture(name);
}

void UILoadingScreen::SetStageTitle(const char* title)
{
    ScopeLock scope(&loadingLock);

    loadingStage->TextItemControl()->SetText(title);
}

void UILoadingScreen::SetStageTip(const char* header, const char* tipNumber, const char* tip)
{
    ScopeLock scope(&loadingLock);

    loadingHeader->TextItemControl()->SetText(header);
    loadingTipNumber->TextItemControl()->SetText(tipNumber);
    loadingTip->TextItemControl()->SetText(tip);
}

void UILoadingScreen::Show(bool status)
{
    CUIWindow::Show(status);
}

bool UILoadingScreen::IsShown()
{
    return CUIWindow::IsShown();
}
