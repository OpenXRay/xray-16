#pragma once

#include "ai_monster_defs.h"

class CBaseMonster;

class CMonsterCorpseMemory
{
    CBaseMonster* monster{};
    TTime time_memory{ 10000 };

    using CORPSE_MAP = xr_map<const CEntityAlive*, SMonsterCorpse>;
    using CORPSE_MAP_IT = CORPSE_MAP::iterator;

    CORPSE_MAP m_objects;

public:
    CMonsterCorpseMemory() = default;

    void init_external(CBaseMonster* M, const TTime mem_time)
    {
        monster = M;
        time_memory = mem_time;
    }

    void update();

    // -----------------------------------------------------
    const CEntityAlive* get_corpse();

    SMonsterCorpse get_corpse_info();
    auto get_corpse_count() const { return m_objects.size(); }
    void clear() { m_objects.clear(); }
    void remove_links(const IGameObject* O);

    void add_corpse(const CEntityAlive* corpse);
    bool is_valid_corpse(const CEntityAlive* corpse);

private:
    void remove_non_actual();

    CORPSE_MAP_IT find_best_corpse();
};
