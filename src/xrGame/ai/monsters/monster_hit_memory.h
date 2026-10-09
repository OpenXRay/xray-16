#pragma once
#include "ai_monster_defs.h"

class CBaseMonster;

class CMonsterHitMemory
{
    CBaseMonster* monster{};
    TTime time_memory{ 10000 };

    xr_vector<SMonsterHit> m_hits;

public:
    void init_external(CBaseMonster* M, const TTime mem_time)
    {
        monster = M;
        time_memory = mem_time;
    }

    void update();

    [[nodiscard]] bool is_hit() const { return !m_hits.empty(); }
    [[nodiscard]] bool is_hit(const IGameObject* pO);

    // Lain: added
    auto get_num_hits() const { return m_hits.size(); }
    void add_hit(IGameObject* who, EHitSide side);

    Fvector get_last_hit_dir() const;
    TTime get_last_hit_time() const;
    IGameObject* get_last_hit_object() const;
    Fvector get_last_hit_position() const;

    void clear() { m_hits.clear(); }
    void remove_hit_info(const IGameObject* obj);

private:
    void remove_non_actual();
};
