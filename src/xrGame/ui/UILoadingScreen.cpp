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

#include "UIHelper.h"
#include "xrUICore/XML/UITextureMaster.h"
#include "xrEngine/device.h"
#include "Include/xrRender/UIShader.h"

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
#if defined(XR_PLATFORM_ANDROID)
    if (strstr(Core.Params, "-renderer-vulkan") && loadingProgress)
    {
        auto& foreground = loadingProgress->m_UIProgressItem;
        auto& background = loadingProgress->m_UIBackgroundItem;
        Fvector2 foregroundSize{}, backgroundSize{};
        const bool foregroundReady = foreground.GetShader() &&
            foreground.GetShader()->GetBaseTextureResolution(foregroundSize);
        const bool backgroundReady = background.GetShader() &&
            background.GetShader()->GetBaseTextureResolution(backgroundSize);
        const Frect& foregroundRect = foreground.GetTextureRect();
        const Frect& backgroundRect = background.GetTextureRect();
        Msg("[renderer-vulkan] loading.progress xml=%d ui=(%.1f,%.1f) size=(%.1f,%.1f) front=%d atlas=(%.0f,%.0f) rect=(%.0f,%.0f)-(%.0f,%.0f) back=%d atlas=(%.0f,%.0f) rect=(%.0f,%.0f)-(%.0f,%.0f)",
            loaded ? 1 : 0, loadingProgress->GetWndPos().x, loadingProgress->GetWndPos().y,
            loadingProgress->GetWidth(), loadingProgress->GetHeight(), foregroundReady ? 1 : 0,
            foregroundSize.x, foregroundSize.y, foregroundRect.x1, foregroundRect.y1,
            foregroundRect.x2, foregroundRect.y2, backgroundReady ? 1 : 0,
            backgroundSize.x, backgroundSize.y, backgroundRect.x1, backgroundRect.y1,
            backgroundRect.x2, backgroundRect.y2);
    }
#endif
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
    if (Device.dwPrecacheFrame && Device.dwPrecacheFrame % 10 == 0 && loadingProgress)
        Msg("[load-trace] ui.loading.draw frame=%u precache=%u progress=%.1f shown=%d logo=%d",
            Device.dwFrame, Device.dwPrecacheFrame, loadingProgress->GetProgressPos(),
            IsShown() ? 1 : 0, loadingLogo && loadingLogo->IsShown() ? 1 : 0);
    CUIWindow::Draw();
#if defined(XR_PLATFORM_ANDROID)
    if (strstr(Core.Params, "-renderer-vulkan") && loadingProgress)
    {
        // The full-screen level artwork may cover progress bars declared
        // under the background in a game's UI XML. Draw progress last.
        loadingProgress->Draw();
    }
#endif
}

void UILoadingScreen::SetLevelLogo(const char* name)
{
    ScopeLock scope(&loadingLock);

    const bool describedTexture = loadingLogo->InitTexture(name);
#if defined(XR_PLATFORM_ANDROID)
    if (strstr(Core.Params, "-renderer-vulkan") && Device.dwWidth && Device.dwHeight)
    {
        Frect source = loadingLogo->GetUIStaticItem().GetTextureRect();
        // Raw game textures do not update UIStaticItem's saved texture rect.
        // Fetch the new texture's extent instead of cropping the previous logo again.
        if (!describedTexture && loadingLogo->GetShader() && loadingLogo->GetShader()->inited())
        {
            Fvector2 extent{};
            if (loadingLogo->GetShader()->GetBaseTextureResolution(extent))
                source.set(0.f, 0.f, extent.x, extent.y);
        }
        if (source.width() > 0.f && source.height() > 0.f)
        {
            const float imageAspect = source.width() / source.height();
            const float screenAspect = float(Device.dwWidth) / float(Device.dwHeight);
            Frect crop = source;
            if (imageAspect > screenAspect)
            {
                const float width = source.width() * screenAspect / imageAspect;
                crop.x1 += (source.width() - width) * .5f;
                crop.x2 = crop.x1 + width;
            }
            else
            {
                const float height = source.height() * imageAspect / screenAspect;
                crop.y1 += (source.height() - height) * .5f;
                crop.y2 = crop.y1 + height;
            }
            loadingLogo->SetTextureRect(crop);
            loadingLogo->SetWndPos({0.f, 0.f});
            loadingLogo->SetAlignment(waNone);
            loadingLogo->SetWndSize({UI_BASE_WIDTH, UI_BASE_HEIGHT});
            loadingLogo->SetStretchTexture(true);
            Frect rect;
            loadingLogo->GetAbsoluteRect(rect);
            Fvector2 pixelLT, pixelRB;
            UI().ClientToScreenScaled(pixelLT, rect.x1, rect.y1);
            UI().ClientToScreenScaled(pixelRB, rect.x2, rect.y2);
            Msg("[renderer-vulkan] loading.logo '%s' described=%d source=%.0fx%.0f crop=(%.1f,%.1f)-(%.1f,%.1f) screen=%ux%u",
                name ? name : "<null>", describedTexture ? 1 : 0, source.width(), source.height(), crop.x1, crop.y1, crop.x2, crop.y2,
                Device.dwWidth, Device.dwHeight);
            Msg("[renderer-vulkan] loading.bounds ui=(%.1f,%.1f)-(%.1f,%.1f) pixels=(%.1f,%.1f)-(%.1f,%.1f) window=%dx%d",
                rect.x1, rect.y1, rect.x2, rect.y2, pixelLT.x, pixelLT.y, pixelRB.x, pixelRB.y,
                Device.m_rcWindowClient.w, Device.m_rcWindowClient.h);
        }
    }
#endif
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
