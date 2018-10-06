//////////////////////////////////////////////////////////////////////////
// string_table.h: таблица строк, используемых в игре
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrEngine/StringTable/IStringTableDefs.h"
#include "xrCommon/xr_map.h"

class IStringTable
{
public:
    virtual ~IStringTable() {};

    virtual STRING_VALUE translate(const STRING_ID& str_id) const = 0;
    virtual void rescan() = 0;
};

extern ENGINE_API IStringTable* gStringTable;
