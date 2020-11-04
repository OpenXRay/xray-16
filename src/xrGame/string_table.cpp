#include "StdAfx.h"
#include "string_table.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "xr_level_controller.h"

constexpr pcstr OPENXRAY_XML = "openxray.xml";

CStringTable& StringTable() { return *((CStringTable*)gStringTable); }

xr_unique_ptr<STRING_TABLE_DATA> CStringTable::pData;
BOOL CStringTable::m_bWriteErrorsToLog = FALSE;
u32 CStringTable::LanguageID = std::numeric_limits<u32>::max();
xr_vector<xr_token> CStringTable::languagesToken;

CStringTable::CStringTable()
{
    pData = nullptr;
}

CStringTable::~CStringTable() { Destroy(); }
void CStringTable::Destroy()
{
    pData.reset(nullptr);

    for (auto& token : languagesToken)
        xr_free(token.name);

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
#endif // #ifdef DEBUG

    ReparseKeyBindings();
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
    if (LanguageID != std::numeric_limits<u32>::max())
        pData->m_sLanguage = languagesToken.at(LanguageID).name;
    else
    {
        pData->m_sLanguage = pSettings->r_string("string_table", "language");
        auto it = std::find_if(languagesToken.begin(), languagesToken.end(), [](const xr_token& token) {
            return token.name && token.name == pData->m_sLanguage;
        });

        R_ASSERT3(it != languagesToken.end(), "Check localization.ltx! Current language: ", pData->m_sLanguage.c_str());
        if (it != languagesToken.end())
            LanguageID = (*it).id;
    }
}

xr_token* CStringTable::GetLanguagesToken() const { return languagesToken.data(); }

void CStringTable::Load(LPCSTR xml_file_full)
{
    CUIXml uiXml;
    string_path _s;
    strconcat(sizeof(_s), _s, "text" DELIMITER, pData->m_sLanguage.c_str());

    uiXml.Load(CONFIG_PATH, _s, xml_file_full);

    //общий список всех записей таблицы в файле
    const int string_num = uiXml.GetNodesNum(uiXml.GetRoot(), "string");

    for (int i = 0; i < string_num; ++i)
    {
        LPCSTR string_name = uiXml.ReadAttrib(uiXml.GetRoot(), "string", i, "id", NULL);

        VERIFY3(pData->m_StringTable.find(string_name) == pData->m_StringTable.end(), "duplicate string table id",
            string_name);

        LPCSTR string_text = uiXml.Read(uiXml.GetRoot(), "string:text", i, NULL);

        if (m_bWriteErrorsToLog && string_text)
            Msg("[string table] '%s' no translation in '%s'", string_name, pData->m_sLanguage.c_str());

        VERIFY3(string_text, "string table entry does not has a text", string_name);

        const STRING_VALUE str_val = ParseLine(string_text, string_name, true);

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

void CStringTable::ReparseKeyBindings()
{
    if (!pData)
        return;
    auto it = pData->m_string_key_binding.begin();
    const auto it_e = pData->m_string_key_binding.end();

    for (; it != it_e; ++it)
    {
        pData->m_StringTable[it->first] = ParseLine(*it->second, *it->first, false);
    }
}

STRING_VALUE CStringTable::ParseLine(LPCSTR str, LPCSTR skey, bool bFirst)
{
    constexpr char   ACTION_STR[]       = "$$ACTION_";
    constexpr char   ACTION_STR_END[]   = "$$";
    constexpr size_t ACTION_STR_LEN     = std::size(ACTION_STR) - 1;
    constexpr size_t ACTION_STR_END_LEN = std::size(ACTION_STR_END) - 1;

    xr_string res;
    size_t k = 0;
    const char* b;

    string256 buff;
    string256 srcbuff;
    bool b_hit = false;

    while ((b = strstr(str + k, ACTION_STR)) != 0)
    {
        buff[0] = 0;
        srcbuff[0] = 0;
        res.append(str + k, b - str - k);
        const char* e = strstr(b + ACTION_STR_LEN, ACTION_STR_END);

        const size_t len = (e - b - ACTION_STR_LEN);

        strncpy_s(srcbuff, b + ACTION_STR_LEN, len);
        srcbuff[len] = 0;
        if (ActionNameToPtr(srcbuff)) // if exist, get bindings
        {
            [[maybe_unused]] const bool result =
                GetActionAllBinding(srcbuff, buff, sizeof(buff));
            VERIFY(result);
            res.append(buff, xr_strlen(buff));
        }
        else // doesn't exist, insert as is
        {
            res.append(b, ACTION_STR_LEN + len + 2);
        }

        k = (b - str);
        k += len;
        k += ACTION_STR_LEN;
        k += ACTION_STR_END_LEN;
        b_hit = true;
    };

    if (k < xr_strlen(str))
    {
        res.append(str + k);
    }

    if (b_hit && bFirst)
        pData->m_string_key_binding[skey] = str;

    return STRING_VALUE(res.c_str());
}

STRING_VALUE CStringTable::translate(const STRING_ID& str_id) const
{
    VERIFY(pData);

    if (pData->m_StringTable.find(str_id) != pData->m_StringTable.end())
        return pData->m_StringTable[str_id];
    return str_id;
}
