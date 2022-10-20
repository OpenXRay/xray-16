#include "StdAfx.h"
#include "UIStyle.h"

UIStyle* UIStyleManager;

void UIStyle::FillUIStyleToken()
{
    UIStyleToken.emplace_back("ui_style_default", 0);

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
            UIStyleToken.emplace_back(xr_strdup(style), i++); // It's important to have postfix increment!
        }
        FS.file_list_close(styles);
    }

    UIStyleToken.emplace_back(nullptr, -1);
}

void UIStyle::SetupUIStyle()
{
    if (UIStyleID == 0)
    {
        if (!defaultUIStyle)
        {
            xr_free(UI_PATH);
            xr_free(UI_PATH_WITH_DELIMITER);
        }
        UI_PATH = UI_PATH_DEFAULT;
        UI_PATH_WITH_DELIMITER = UI_PATH_DEFAULT_WITH_DELIMITER;
        defaultUIStyle = true;
        return;
    }

    pcstr selectedStyle = nullptr;
    for (const auto& token : UIStyleToken)
        if (token.id == UIStyleID)
            selectedStyle = token.name;

    string_path selectedStylePath;
    strconcat(selectedStylePath, UI_PATH_DEFAULT, DELIMITER "styles" DELIMITER, selectedStyle);
    UI_PATH = xr_strdup(selectedStylePath);

    xr_strcat(selectedStylePath, DELIMITER);
    UI_PATH_WITH_DELIMITER = xr_strdup(selectedStylePath);

    defaultUIStyle = false;
}

void UIStyle::CleanupUIStyleToken()
{
    for (auto& token : UIStyleToken)
    {
        if (token.name && token.id != 0)
            xr_free(token.name);
    }
    UIStyleToken.clear();
    if (!defaultUIStyle)
    {
        xr_free(UI_PATH);
        xr_free(UI_PATH_WITH_DELIMITER);
    }
}
