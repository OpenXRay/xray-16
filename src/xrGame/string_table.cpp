#include "stdafx.h"
#include "string_table.h"
#include "xrEngine/xr_ioconsole.h"
#include "ui/xrUIXmlParser.h"
#include "xr_level_controller.h"
#include "MainMenu.h"

STRING_TABLE_DATA* CStringTable::pData = NULL;
BOOL CStringTable::m_bWriteErrorsToLog = FALSE;
#ifdef COC_EDITION
u32 gLanguage = -1;
xr_vector<xr_token> gLanguagesToken;
#endif

CStringTable::CStringTable() { Init(); }
void CStringTable::Destroy() { xr_delete(pData); }
void CStringTable::rescan()
{
    if (NULL != pData)
        return;
    Destroy();
    Init();
}

void CStringTable::Init()
{
    if (NULL != pData)
        return;

    pData = new STRING_TABLE_DATA();

#ifdef COC_EDITION
    if (gLanguagesToken.size() == 0)
    {
        u32 lineCount = pSettings->line_count("Languages");
        LPCSTR lineName, lineVal;
        if (lineCount == 0)
            R_ASSERT2(false, "Section \"Languages\" is empty!");

        for (u16 i = 0; i < lineCount; i++)
        {
            pSettings->r_line("Languages", i, &lineName, &lineVal);
            gLanguagesToken.emplace_back(lineName, i);
        }
        gLanguagesToken.emplace_back(nullptr, -1);
    }
    
    //имя языка, если не задано (NULL), то первый <text> в <string> в XML
    if (gLanguage != -1)
        pData->m_sLanguage = gLanguagesToken.at(gLanguage).name;
    else
    {
        
        pData->m_sLanguage = pSettings->r_string("string_table", "language");
        for (const auto& it : gLanguagesToken)
        {
            if (it.name && it.name == pData->m_sLanguage)
            {
                /*string64 buf;
                xr_strcpy(buf, "g_language ");
                xr_strcat(buf, it.name);

                Console->Execute(buf);*/

                gLanguage = it.id;
                break;
            }
        }
    }
#else
    pData->m_sLanguage = pSettings->r_string("string_table", "language");
#endif
    
    //---
    FS_FileSet fset;
    string_path files_mask;
    xr_sprintf(files_mask, "text\\%s\\*.xml", pData->m_sLanguage.c_str());
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
    //---
    ReparseKeyBindings();
}

void CStringTable::Load(LPCSTR xml_file_full)
{
    CUIXml uiXml;
    string_path _s;
    strconcat(sizeof(_s), _s, "text\\", pData->m_sLanguage.c_str());

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

void CStringTable::ReloadLanguage()
{
#ifdef COC_EDITION
    if (0 == xr_strcmp(gLanguagesToken.at(gLanguage).name, pData->m_sLanguage.c_str()))
        return;
#else
    if (0 == xr_strcmp(pSettings->r_string("string_table", "language"), pData->m_sLanguage.c_str()))
        return;
#endif

    xr_delete(pData);

    Init();

    if (g_pGamePersistent && g_pGamePersistent->IsMainMenuActive())
        MainMenu()->setLanguageChanged(true);

    if (!g_pGameLevel)
        return;

    for (u16 id = 0; id < 0xffff; id++)
    {
        CGameObject* pGameObject = smart_cast<CGameObject*>(Level().Objects.net_Find(id));
        if (!pGameObject)
            continue;

        if (CInventoryItem* p = smart_cast<CInventoryItem*>(pGameObject))
            p->reloadNames();
    }
}

STRING_VALUE CStringTable::ParseLine(LPCSTR str, LPCSTR skey, bool bFirst)
{
    //	LPCSTR str = "1 $$action_left$$ 2 $$action_right$$ 3 $$action_left$$ 4";
    xr_string res;
    int k = 0;
    const char* b;
#define ACTION_STR "$$ACTION_"

//.	int LEN				= (int)xr_strlen(ACTION_STR);
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
