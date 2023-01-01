#include "pch.hpp"
#include "ui_styles.h"

#include "xrCore/XML/XMLDocument.hpp"
#include "xrEngine/IGame_Persistent.h"

UIStyleManager* ui_styles;
bool isUIStylesClosed;

UIStyleManager* UIStyles()
{
    if (!ui_styles && !isUIStylesClosed)
    {
        ui_styles = xr_new<UIStyleManager>();
    }

    return ui_styles;
}

void CloseUIStyles()
{
    if (ui_styles)
    {
        isUIStylesClosed = true;
        xr_delete(ui_styles);
    }
}

UIStyleManager::UIStyleManager()
{
    m_token.emplace_back("ui_style_default", 0);

    string_path path;
    strconcat(sizeof(path), path, UI_PATH, DELIMITER "styles" DELIMITER);
    FS.update_path(path, _game_config_, path);
    auto styles = FS.file_list_open(path, FS_ListFolders | FS_RootOnly);
    if (styles != nullptr)
    {
        int i = 1; // It's 1, because 0 is default style
        for (const auto& style : *styles)
        {
            const auto pos = strchr(style, _DELIMITER);
            *pos = '\0'; // we don't need that backslash in the end
            m_token.emplace_back(xr_strdup(style), i++); // It's important to have postfix increment!
        }
        FS.file_list_close(styles);
    }

    m_token.emplace_back(nullptr, -1);
}

UIStyleManager::~UIStyleManager()
{
    for (auto& token : m_token)
    {
        if (token.name && token.id != 0)
            xr_free(token.name);
    }
    m_token.clear();
    if (!DefaultStyleIsSet())
    {
        xr_free(UI_PATH);
        xr_free(UI_PATH_WITH_DELIMITER);
    }
}

void UIStyleManager::SetupStyle(u32 styleID)
{
    if (m_style_id == styleID)
        return;

    if (styleID == DEFAULT_STYLE)
    {
        if (!DefaultStyleIsSet())
        {
            xr_free(UI_PATH);
            xr_free(UI_PATH_WITH_DELIMITER);
        }
        UI_PATH = UI_PATH_DEFAULT;
        UI_PATH_WITH_DELIMITER = UI_PATH_DEFAULT_WITH_DELIMITER;
    }

    m_style_id = styleID;
    if (DefaultStyleIsSet())
        return;

    pcstr selectedStyle = nullptr;
    for (const auto& token : m_token)
    {
        if (token.id == m_style_id)
            selectedStyle = token.name;
    }

    string_path selectedStylePath;
    strconcat(selectedStylePath, UI_PATH_DEFAULT, DELIMITER "styles" DELIMITER, selectedStyle);
    UI_PATH = xr_strdup(selectedStylePath);

    xr_strcat(selectedStylePath, DELIMITER);
    UI_PATH_WITH_DELIMITER = xr_strdup(selectedStylePath);
}

void UIStyleManager::Reset()
{
    // Hack: activate main menu to prevent crash
    // I don't know why it crashes while in the game
    bool shouldHideMainMenu = false;
    if (g_pGamePersistent && g_pGamePersistent->m_pMainMenu)
    {
        shouldHideMainMenu = !g_pGamePersistent->m_pMainMenu->IsActive();
        g_pGamePersistent->m_pMainMenu->Activate(true);
    }

    Device.seqUIReset.Process();

    if (shouldHideMainMenu)
        g_pGamePersistent->m_pMainMenu->Activate(false);
}
