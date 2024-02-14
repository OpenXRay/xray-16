#include "stdafx.h"
#include "StringTable.h"

#include "xr_level_controller.h"

#include "xrCore/XML/XMLDocument.hpp"

constexpr pcstr OPENXRAY_XML = "openxray.xml";

CStringTable& StringTable()
{
    static CStringTable string_table;
    return string_table;
}

xr_unique_ptr<STRING_TABLE_DATA> CStringTable::pData{};
BOOL CStringTable::m_bWriteErrorsToLog = FALSE;
u32 CStringTable::LanguageID = std::numeric_limits<u32>::max();
xr_vector<xr_token> CStringTable::languagesToken;

void CStringTable::Destroy()
{
    pData.reset(nullptr);

    for (auto& token : languagesToken)
    {
        auto tokenName = const_cast<char*>(token.name);
        xr_free(tokenName);
    }

    languagesToken.clear();
}

void CStringTable::rescan()
{
    if (!pData)
    {
        Destroy();
        Init();
    }
}

void CStringTable::Init()
{
    if (pData)
        return;

    pData = xr_make_unique<STRING_TABLE_DATA>();

    FillLanguageToken();
    SetLanguage();

    FS_FileSet fset;
    string_path files_mask;
    xr_sprintf(files_mask, "text" DELIMITER "%s" DELIMITER "*.xml", pData->m_sLanguage.c_str());
    FS.file_list(fset, "$game_config$", FS_ListFiles, files_mask);
    auto fit = fset.begin();
    auto fit_e = fset.end();

    for (; fit != fit_e; ++fit)
    {
        string_path fn, ext;
        _splitpath((*fit).name.c_str(), 0, 0, fn, ext);
        xr_strcat(fn, ext);

        Load(fn);
    }
#ifdef DEBUG
    Msg("StringTable: loaded %d files", fset.size());
#endif
}

void CStringTable::FillLanguageToken()
{
    languagesToken.clear();

    string_path path;
    FS.update_path(path, _game_config_, "text" DELIMITER);
    auto languages = FS.file_list_open(path, FS_ListFolders | FS_RootOnly);

    const bool localizationPresent = languages != nullptr;

    // We must warn about lack of localization
    // However we can work without it
    VERIFY(localizationPresent);
    if (localizationPresent)
    {
        int i = 0;
        for (const auto& language : *languages)
        {
            const auto pos = strchr(language, _DELIMITER);
            *pos = '\0'; // we don't need that backslash in the end

            // Skip map_desc folder
            if (0 == xr_strcmp(language, "map_desc"))
                continue;

            bool shouldSkip = false;

            // Open current language folder
            string_path folder;
            strconcat(sizeof(folder), folder, path, language, DELIMITER);
            auto files = FS.file_list_open(folder, FS_ListFiles | FS_RootOnly);

            // Skip empty folder
            if (!files || files->empty())
                shouldSkip = true;

            // Skip folder with only openxray.xml file in it
            // It's important to have 'else if' instead of simple 'if'
            else if (files->size() == 1 && xr_strcmp(files->at(0), OPENXRAY_XML) == 0)
                shouldSkip = true;

            // Don't forget to close opened folder
            FS.file_list_close(files);

            if (shouldSkip)
                continue;

            // Finally, we can add language
            languagesToken.emplace_back(xr_strdup(language), i++); // It's important to have postfix increment!
        }
        FS.file_list_close(languages);
    }

    languagesToken.emplace_back(nullptr, -1);
}

void CStringTable::SetLanguage()
{
    cpcstr defined_language = pSettings->r_string("string_table", "language");
    cpcstr defined_prefix   = pSettings->r_string("string_table", "font_prefix");

    if (LanguageID != std::numeric_limits<u32>::max())
    {
        pData->m_sLanguage = languagesToken.at(LanguageID).name;

        if (0 == xr_strcmp(pData->m_sLanguage, defined_language))
            pData->m_fontPrefix = defined_prefix;
        else
        {
            pData->m_fontPrefix = nullptr;

            constexpr std::tuple<pcstr, pcstr> known_prefixes[] =
            {
                { "fra", "_west" },
                { "ger", "_west" },
                { "ita", "_west" },
                { "spa", "_west" },
                { "pol", "_cent" },
                { "cze", "_cent" },
            };
            for (const auto [language, prefix] : known_prefixes)
            {
                if (0 == xr_strcmp(pData->m_sLanguage, language))
                    pData->m_fontPrefix = prefix;
            }
        }
    }
    else
    {
        pData->m_sLanguage  = defined_language;
        pData->m_fontPrefix = defined_prefix;

        const auto it = std::find_if(languagesToken.begin(), languagesToken.end(), [](const xr_token& token)
        {
            return token.name && token.name == pData->m_sLanguage;
        });

        R_ASSERT3(it != languagesToken.end(), "Check localization.ltx! Current language: ", pData->m_sLanguage.c_str());
        if (it != languagesToken.end())
            LanguageID = it->id;
    }
}

shared_str CStringTable::GetCurrentLanguage() const
{
    return pData ? pData->m_sLanguage : nullptr;
}

shared_str CStringTable::GetCurrentFontPrefix() const
{
    return pData ? pData->m_fontPrefix : nullptr;
}

xr_token* CStringTable::GetLanguagesToken() const { return languagesToken.data(); }

void CStringTable::Load(LPCSTR xml_file_full)
{
    XMLDocument uiXml;
    string_path _s;
    strconcat(sizeof(_s), _s, "text" DELIMITER, pData->m_sLanguage.c_str());

    uiXml.Load(CONFIG_PATH, _s, xml_file_full);

    //общий список всех записей таблицы в файле
    const int string_num = uiXml.GetNodesNum(uiXml.GetRoot(), "string");

    for (int i = 0; i < string_num; ++i)
    {
        LPCSTR string_name = uiXml.ReadAttrib(uiXml.GetRoot(), "string", i, "id", NULL);

#ifndef MASTER_GOLD
        if (pData->m_StringTable.find(string_name) != pData->m_StringTable.end())
            Msg("~ duplicate string table id [%s]", string_name);
#endif

        LPCSTR string_text = uiXml.Read(uiXml.GetRoot(), "string:text", i, NULL);

        if (m_bWriteErrorsToLog && string_text)
            Msg("[string table] '%s' no translation in '%s'", string_name, pData->m_sLanguage.c_str());

        VERIFY3(string_text, "string table entry does not has a text", string_name);

        const STRING_VALUE str_val = ParseLine(string_text);

        pData->m_StringTable[string_name] = str_val;
    }
}

void CStringTable::ReloadLanguage()
{
    if (0 == xr_strcmp(languagesToken.at(LanguageID).name, pData->m_sLanguage.c_str()))
        return;

    Destroy();
    Init();
}

STRING_VALUE CStringTable::ParseLine(pcstr str)
{
    constexpr char   ACTION_STR[]     = "$$ACTION_";
    constexpr size_t ACTION_STR_LEN   = std::size(ACTION_STR) - 1;

    constexpr char   ACTION_STR_END[] = "$$";
    constexpr size_t ACTION_STR_END_LEN = std::size(ACTION_STR_END) - 1;

    xr_string string{ str };
    string.erase(std::remove_if(string.begin(), string.end(), [](char ch)
    {
        VERIFY2(ch != GAME_ACTION_MARK,"Using of escape symbol is not allowed in localization.");
        return ch == GAME_ACTION_MARK;
    }), string.end());

    size_t actionStartPos = 0;

    while ((actionStartPos = string.find(ACTION_STR, actionStartPos)) != xr_string::npos)
    {
        const size_t actionEndPos = string.find(ACTION_STR_END, actionStartPos + ACTION_STR_LEN);
        const xr_string actionName = string.substr(actionStartPos + ACTION_STR_LEN, actionEndPos - (actionStartPos + ACTION_STR_LEN));

        if (const auto action = ActionNameToPtr(actionName.c_str())) // if exist, get bindings
        {
            static_assert(kLASTACTION < type_max<u8>, "Modify the code to have more than 255 actions.");
            const char actionCode[] = { GAME_ACTION_MARK, (char)action->id, '\0' };
            string.replace(actionStartPos, actionEndPos + ACTION_STR_END_LEN - actionStartPos, actionCode);
            actionStartPos += std::size(actionCode) - 1;
        }
        else // skip
        {
            actionStartPos = actionEndPos + ACTION_STR_END_LEN;
        }
    }

    return STRING_VALUE(string.c_str());
}

STRING_VALUE CStringTable::translate(const STRING_ID& str_id) const
{
    if (!pData)
        return str_id;

    if (pData->m_StringTable.find(str_id) != pData->m_StringTable.end())
        return pData->m_StringTable[str_id];
    return str_id;
}

bool CStringTable::translate(const STRING_ID& str_id, STRING_VALUE& out) const
{
    if (!pData)
        return false;

    if (pData->m_StringTable.find(str_id) != pData->m_StringTable.end())
    {
        out = pData->m_StringTable[str_id];
        return true;
    }
    return false;
}

pcstr CStringTable::translate(const STRING_ID& str_id, pcstr default_value) const
{
    STRING_VALUE out;
    if (translate(str_id, out))
        return out.c_str();

    return default_value;
}
