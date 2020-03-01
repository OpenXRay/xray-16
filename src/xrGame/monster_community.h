//////////////////////////////////////////////////////////////////////////
// monster_community.h:	структура представления группировки для монстров
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "ini_id_loader.h"
#include "ini_table_loader.h"

typedef int MONSTER_COMMUNITY_INDEX;
typedef shared_str MONSTER_COMMUNITY_ID;
#define NO_MONSTER_COMMUNITY_INDEX MONSTER_COMMUNITY_INDEX(-1)

struct MONSTER_COMMUNITY_DATA
{
    MONSTER_COMMUNITY_DATA(MONSTER_COMMUNITY_INDEX, MONSTER_COMMUNITY_ID, LPCSTR);

    MONSTER_COMMUNITY_ID id;
    MONSTER_COMMUNITY_INDEX index;
    u8 team;
};

class MONSTER_COMMUNITY;

class MONSTER_COMMUNITY
    : public CIni_IdToIndex<true, MONSTER_COMMUNITY_DATA, MONSTER_COMMUNITY_ID, MONSTER_COMMUNITY_INDEX, MONSTER_COMMUNITY>
{
private:
    typedef CIni_IdToIndex<true, MONSTER_COMMUNITY_DATA, MONSTER_COMMUNITY_ID, MONSTER_COMMUNITY_INDEX, MONSTER_COMMUNITY>
        inherited;
    friend inherited;

public:
    MONSTER_COMMUNITY();
    ~MONSTER_COMMUNITY();

    void set(MONSTER_COMMUNITY_ID);
    void set(MONSTER_COMMUNITY_INDEX);

    MONSTER_COMMUNITY_ID id() const;
    MONSTER_COMMUNITY_INDEX index() const;
    u8 team() const;

private:
    MONSTER_COMMUNITY_INDEX m_current_index;

    static void InitIdToIndex();

public:
    //отношение между группами монстров
    static int relation(MONSTER_COMMUNITY_INDEX from, MONSTER_COMMUNITY_INDEX to);
    int relation(MONSTER_COMMUNITY_INDEX to);

    static void DeleteIdToIndexData();

private:
    typedef CIni_Table<int, MONSTER_COMMUNITY> MONSTER_RELATION_TABLE;
    friend MONSTER_RELATION_TABLE;
    static MONSTER_RELATION_TABLE m_relation_table;
};
