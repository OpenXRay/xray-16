//////////////////////////////////////////////////////////////////////////
// string_table.h: таблица строк, используемых в игре
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrEngine/StringTable/IStringTable.h"
#include "xrCommon/xr_map.h"

using STRING_TABLE_MAP = xr_map<STRING_ID, STRING_VALUE>;

struct STRING_TABLE_DATA
{
    shared_str m_sLanguage;

    STRING_TABLE_MAP m_StringTable;

    STRING_TABLE_MAP m_string_key_binding;
};

class CStringTable : public IStringTable
{
public:
    CStringTable();
    virtual ~CStringTable();

    void Init();
    static void Destroy();

    STRING_VALUE translate(const STRING_ID& str_id) const;
    void rescan();

    void ReloadLanguage();

    static BOOL m_bWriteErrorsToLog;
    static void ReparseKeyBindings();

    xr_token* GetLanguagesToken() const;
    static u32 LanguageID;

private:
    void Load(LPCSTR xml_file);
    void FillLanguageToken();
    void SetLanguage();
    static STRING_VALUE ParseLine(LPCSTR str, LPCSTR key, bool bFirst);
    static xr_unique_ptr<STRING_TABLE_DATA> pData;
    static xr_vector<xr_token> languagesToken;
};

CStringTable& StringTable();
