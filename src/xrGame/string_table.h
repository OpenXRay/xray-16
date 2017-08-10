//////////////////////////////////////////////////////////////////////////
// string_table.h: таблица строк, используемых в игре
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "string_table_defs.h"

using STRING_TABLE_MAP = xr_map<STRING_ID, STRING_VALUE>;

struct STRING_TABLE_DATA
{
    shared_str m_sLanguage;

    STRING_TABLE_MAP m_StringTable;

    STRING_TABLE_MAP m_string_key_binding;
};

class CStringTable
{
public:
    CStringTable();

    static void Destroy();

    STRING_VALUE translate(const STRING_ID& str_id) const;
    void rescan();

    static BOOL m_bWriteErrorsToLog;
    static void ReparseKeyBindings();

private:
    void Init();
    void Load(LPCSTR xml_file);
    static STRING_VALUE ParseLine(LPCSTR str, LPCSTR key, bool bFirst);
    static STRING_TABLE_DATA* pData;
};
