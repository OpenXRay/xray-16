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
#include "xrUICore/XML/UITextureMaster.h"

UILoadingScreen::UILoadingScreen()
    : CUIWindow("UILoadingScreen"),
      loadingProgress(nullptr), loadingProgressPercent(nullptr),
      loadingLogo(nullptr),     loadingStage(nullptr),
      loadingHeader(nullptr),   loadingTipNumber(nullptr), loadingTip(nullptr)
{
    alwaysShowStage = false;
    UILoadingScreen::Initialize();
}

void UILoadingScreen::Initialize()
{
    CUIXml uiXml;
    const bool loaded = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_mm_loading_screen.xml", false);

    // It was hardcoded even harder anyway. (search in history for deleted dxApplicationRender class)
    // Hardcoded XML is more flexible, so:
    if (!loaded) // then just use preset we have
    {
        // First, process textures description for loading screen (just in case)
        uiXml.Set(GetLoadingScreenTexturesDescr());
        CUITextureMaster::ParseShTexInfo(uiXml, false);
        uiXml.ClearInternal(); // cleanup

        // And then, set loading screen itself
        uiXml.Set(GetLoadingScreenXML());
    }

    pcstr xmlNode = "background";

    string64 temp;
    strconcat(temp, "background_", StringTable().GetCurrentLanguage().c_str());
    if (uiXml.NavigateToNode(temp))
        xmlNode = temp;

    if (uiXml.ReadAttribInt("loading_progress", 0, "under_background", 1))
    {
        loadingProgress = UIHelper::CreateProgressBar(uiXml, "loading_progress", this);
        CUIXmlInit::InitWindow(uiXml, xmlNode, 0, this);
    }
    else
    {
        CUIXmlInit::InitWindow(uiXml, xmlNode, 0, this);
        loadingProgress = UIHelper::CreateProgressBar(uiXml, "loading_progress", this);
    }

    alwaysShowStage = uiXml.ReadAttribInt("loading_stage", 0, "always_show");

    xmlNode = "loading_logo";
    strconcat(temp, "loading_logo_", StringTable().GetCurrentLanguage().c_str());
    if (uiXml.NavigateToNode(temp))
        xmlNode = temp;

    loadingLogo = UIHelper::CreateStatic(uiXml, xmlNode, this);

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
    loadingProgress->ForceSetProgressPos(progress); // XXX: use SetProgressPos() when CApplication rendering will be integrated into the normal rendering cycle

    if (loadingProgressPercent)
    {
        string16 buf;
        xr_sprintf(buf, "%.0f%%", loadingProgress->GetProgressPos());
        loadingProgressPercent->SetText(buf);
    }

    CUIWindow::Update();
}

void UILoadingScreen::Draw()
{
    ScopeLock scope(&loadingLock);
    CUIWindow::Draw();
}

void UILoadingScreen::SetLevelLogo(const char* name)
{
    ScopeLock scope(&loadingLock);

    loadingLogo->InitTexture(name);
}

void UILoadingScreen::SetStageTitle(const char* title)
{
    // Only if enabled by user or forced to be displayed by XML
    // And if exist at all
    if ((psActorFlags.test(AF_LOADING_STAGES) || alwaysShowStage) && loadingStage)
    {
        ScopeLock scope(&loadingLock);

        loadingStage->SetText(title);
    }
}

void UILoadingScreen::SetStageTip(const char* header, const char* tipNumber, const char* tip)
{
    ScopeLock scope(&loadingLock);

    if (loadingHeader)
        loadingHeader->SetText(header);
    if (loadingTipNumber)
        loadingTipNumber->SetText(tipNumber);
    if (loadingTip)
        loadingTip->SetText(tip);
}

void UILoadingScreen::Show(bool show)
{
    CUIWindow::Show(show);
    if (!show)
    {
        loadingLogo->GetStaticItem()->GetShader()->destroy();
        if (loadingStage)
            loadingStage->SetText(nullptr);
        SetStageTip(nullptr, nullptr, nullptr);
    }
}

bool UILoadingScreen::IsShown() const
{
    return CUIWindow::IsShown();
}
