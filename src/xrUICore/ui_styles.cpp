#include "pch.hpp"
#include "ui_styles.h"

#include "xrCore/XML/XMLDocument.hpp"
#include "xrEngine/IGame_Persistent.h"

static constexpr cpcstr DEFAULT_UI_STYLE_NAME = "ui_style_default";

UIStyleManager* UIStyles = nullptr;

UIStyleManager::UIStyleManager()
{
    m_token.emplace_back(DEFAULT_UI_STYLE_NAME, DEFAULT_STYLE_ID);

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
        if (token.name && token.id != DEFAULT_STYLE_ID)
        {
            char* tokenName = const_cast<char*>(token.name);
            xr_free(tokenName);
        }
    }
    m_token.clear();
    if (!DefaultStyleIsSet())
    {
        char* mutable_UI_PATH = const_cast<char*>(UI_PATH);
        char* mutable_UI_PATH_WITH_DELIMITER = const_cast<char*>(UI_PATH_WITH_DELIMITER);

        xr_free(mutable_UI_PATH);
        xr_free(mutable_UI_PATH_WITH_DELIMITER);
    }
}

void UIStyleManager::SetupStyle(u32 styleID)
{
    if (m_style_id == styleID)
        return;

    if (styleID == DEFAULT_STYLE_ID)
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
        if (token.id == static_cast<int>(m_style_id))
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

bool UIStyleManager::SetStyle(pcstr name, bool reloadUI)
{
    for (const auto& token : m_token)
    {
        if (0 == xr_strcmp(token.name, name))
        {
            SetupStyle(token.id);
            if (reloadUI)
                Reset();
            return true;
        }
    }
    return false;
}

pcstr UIStyleManager::GetCurrentStyleName() const
{
    for (const auto& token : m_token)
    {
        if (token.id == m_style_id)
            return token.name;
    }
    VERIFY(!"Could retrieve current style name!");
    return nullptr;
}
