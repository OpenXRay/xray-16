#include "StdAfx.h"
#include "string_table.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "xr_level_controller.h"

CStringTable& StringTable() { return *((CStringTable*)gStringTable); }

xr_unique_ptr<STRING_TABLE_DATA> CStringTable::pData;
BOOL CStringTable::m_bWriteErrorsToLog = FALSE;
u32 CStringTable::LanguageID = std::numeric_limits<u32>::max();
xr_vector<xr_token> CStringTable::languagesToken;

CStringTable::CStringTable()
{
    pData = nullptr;
    FillLanguageToken();
}

CStringTable::~CStringTable() { Destroy(); }
void CStringTable::Destroy()
{
    pData.reset(nullptr);
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
    if (languagesToken.size() == 0)
    {
        u32 lineCount = pSettings->line_count("Languages");
        R_ASSERT2(lineCount > 0, "Section \"Languages\" is empty!");

        LPCSTR lineName, lineVal;
        for (u16 i = 0; i < lineCount; i++)
        {
            pSettings->r_line("Languages", i, &lineName, &lineVal);
            languagesToken.emplace_back(lineName, i);
        }
        languagesToken.emplace_back(nullptr, -1);
    }
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
    int string_num = uiXml.GetNodesNum(uiXml.GetRoot(), "string");

    for (int i = 0; i < string_num; ++i)
    {
        LPCSTR string_name = uiXml.ReadAttrib(uiXml.GetRoot(), "string", i, "id", NULL);

        VERIFY3(pData->m_StringTable.find(string_name) == pData->m_StringTable.end(), "duplicate string table id",
            string_name);

        LPCSTR string_text = uiXml.Read(uiXml.GetRoot(), "string:text", i, NULL);

        if (m_bWriteErrorsToLog && string_text)
            Msg("[string table] '%s' no translation in '%s'", string_name, pData->m_sLanguage.c_str());

        VERIFY3(string_text, "string table entry does not has a text", string_name);

        STRING_VALUE str_val = ParseLine(string_text, string_name, true);

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
    auto it_e = pData->m_string_key_binding.end();

    for (; it != it_e; ++it)
    {
        pData->m_StringTable[it->first] = ParseLine(*it->second, *it->first, false);
    }
}

STRING_VALUE CStringTable::ParseLine(LPCSTR str, LPCSTR skey, bool bFirst)
{
    xr_string res;
    int k = 0;
    const char* b;
#define ACTION_STR "$$ACTION_"

#define LEN 9

    string256 buff;
    string256 srcbuff;
    bool b_hit = false;

    while ((b = strstr(str + k, ACTION_STR)) != 0)
    {
        buff[0] = 0;
        srcbuff[0] = 0;
        res.append(str + k, b - str - k);
        const char* e = strstr(b + LEN, "$$");

        int len = (int)(e - b - LEN);

        strncpy_s(srcbuff, b + LEN, len);
        srcbuff[len] = 0;
        GetActionAllBinding(srcbuff, buff, sizeof(buff));
        res.append(buff, xr_strlen(buff));

        k = (int)(b - str);
        k += len;
        k += LEN;
        k += 2;
        b_hit = true;
    };

    if (k < (int)xr_strlen(str))
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
    else
        return str_id;
}
