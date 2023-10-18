//////////////////////////////////////////////////////////////////////////
// string_table.h: таблица строк, используемых в игре
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCommon/xr_map.h"
#include "xrCore/xrstring.h"
#include "xrCore/xr_token.h"

using STRING_ID = shared_str;
using STRING_VALUE = shared_str;

using STRING_TABLE_MAP = xr_map<STRING_ID, STRING_VALUE>;

struct STRING_TABLE_DATA
{
    shared_str m_sLanguage;
    STRING_TABLE_MAP m_StringTable;
};

class ENGINE_API CStringTable final
{
public:
    void Init();
    static void Destroy();

    STRING_VALUE translate(const STRING_ID& str_id) const;
    bool translate(const STRING_ID& str_id, STRING_VALUE& out) const;
    pcstr translate(const STRING_ID& str_id, pcstr default_value) const;
    void rescan();

    void ReloadLanguage();

    static BOOL m_bWriteErrorsToLog;

    shared_str GetCurrentLanguage() const;
    xr_token* GetLanguagesToken() const;
    static u32 LanguageID;

private:
    void Load(LPCSTR xml_file);
    void FillLanguageToken();
    void SetLanguage();
    static STRING_VALUE ParseLine(pcstr str);
    static xr_unique_ptr<STRING_TABLE_DATA> pData;
    static xr_vector<xr_token> languagesToken;
};

ENGINE_API CStringTable& StringTable();
