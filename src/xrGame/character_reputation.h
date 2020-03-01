//////////////////////////////////////////////////////////////////////////
// character_reputation.h:		структура представления репутаций и
//								отношений между ними
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "ini_id_loader.h"
#include "ini_table_loader.h"

#include "character_info_defs.h"

struct REPUTATION_DATA
{
    REPUTATION_DATA(int, shared_str, LPCSTR);

    shared_str id;
    int index;
    CHARACTER_REPUTATION_VALUE threshold;
};

class CHARACTER_REPUTATION;

class CHARACTER_REPUTATION : public CIni_IdToIndex<true, REPUTATION_DATA, shared_str, int, CHARACTER_REPUTATION>
{
private:
    typedef CIni_IdToIndex<true, REPUTATION_DATA, shared_str, int, CHARACTER_REPUTATION> inherited;
    friend inherited;

public:
    CHARACTER_REPUTATION() : m_current_value(NO_REPUTATION), m_current_index(0) {}
    ~CHARACTER_REPUTATION(){};

    void set(CHARACTER_REPUTATION_VALUE);

    shared_str id() const;
    int index() const { return m_current_index; };
    CHARACTER_REPUTATION_VALUE value() const { return m_current_value; };
    static int ValueToIndex(CHARACTER_REPUTATION_VALUE);

private:
    CHARACTER_REPUTATION_VALUE m_current_value;
    int m_current_index;

    static void InitIdToIndex();

public:
    //отношение между репутациями
    static CHARACTER_GOODWILL relation(int from, int to);
    CHARACTER_GOODWILL relation(int to);

    static void DeleteIdToIndexData();

private:
    typedef CIni_Table<CHARACTER_GOODWILL, CHARACTER_REPUTATION> GOODWILL_TABLE;
    friend GOODWILL_TABLE;
    static GOODWILL_TABLE m_relation_table;
};
