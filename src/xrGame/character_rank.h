//////////////////////////////////////////////////////////////////////////
// character_rank.h:	структура представления рангов и отношений между
//						ними
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "ini_id_loader.h"
#include "ini_table_loader.h"

#include "character_info_defs.h"

struct RANK_DATA
{
    RANK_DATA(int, shared_str, LPCSTR);

    shared_str id;
    int index;
    CHARACTER_RANK_VALUE threshold;
};

class CHARACTER_RANK;

typedef CIni_IdToIndex<true, RANK_DATA, shared_str, int, CHARACTER_RANK> CHARACTER_RANK_base;

class CHARACTER_RANK : public CHARACTER_RANK_base
{
private:
    typedef CHARACTER_RANK_base inherited;
    friend inherited;

public:
    CHARACTER_RANK() : m_current_value(NO_RANK), m_current_index(0) {}
    ~CHARACTER_RANK(){};

    void set(CHARACTER_RANK_VALUE);

    shared_str id() const;
    int index() const { return m_current_index; };
    CHARACTER_RANK_VALUE value() const { return m_current_value; };
    static int ValueToIndex(CHARACTER_RANK_VALUE);

private:
    CHARACTER_RANK_VALUE m_current_value;
    int m_current_index;

    static void InitIdToIndex();

public:
    //отношение между рангами
    static CHARACTER_GOODWILL relation(int from, int to);
    CHARACTER_GOODWILL relation(int to);

    static CHARACTER_RANK_VALUE rank_kill_points(int);

    static void DeleteIdToIndexData();

private:
    typedef CIni_Table<CHARACTER_GOODWILL, CHARACTER_RANK> GOODWILL_TABLE;
    friend GOODWILL_TABLE;
    static GOODWILL_TABLE m_relation_table;

    //очки рейтинга которые прибавляются за убийство персонажа с определенным рангом
    typedef CIni_Table<CHARACTER_RANK_VALUE, CHARACTER_RANK> RANK_KILL_TABLE;
    friend RANK_KILL_TABLE;
    static RANK_KILL_TABLE m_rank_kill_table;
};
