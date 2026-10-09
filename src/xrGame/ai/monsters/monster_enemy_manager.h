#pragma once
#include "ai_monster_defs.h"

class CBaseMonster;

class CMonsterEnemyManager
{
    CBaseMonster* monster{};

    const CEntityAlive* enemy{};
    const CEntityAlive* prev_enemy{};

    Fvector position;
    Fvector prev_enemy_position;

    u32 vertex{};
    u32 time_last_seen{};

    Flags16 flags;
    bool forced{};

    EDangerType danger_type{ eNone };

    // node, where monster saw enemy last time
    u32 my_vertex_enemy_last_seen{ u32(-1) };
    // node, of enemy (its always valid unlike vertex)
    u32 enemy_vertex_enemy_last_seen{ u32(-1) };

    u32 m_time_updated{};
    u32 m_time_start_see_enemy{};

public:
    CMonsterEnemyManager();

    void init_external(CBaseMonster* m);
    void reinit();

    void update();

    void force_enemy(const CEntityAlive* enemy);
    void unforce_enemy();

    [[nodiscard]] const CEntityAlive* get_enemy() const { return enemy; }

    [[nodiscard]] EDangerType get_danger_type() const { return danger_type; }

    [[nodiscard]] const Fvector& get_enemy_position() const { return position; }

    [[nodiscard]] u32 get_enemy_vertex() const { return vertex; }

    [[nodiscard]] TTime get_enemy_time_last_seen() const { return time_last_seen; }

    [[nodiscard]] Flags16& get_flags() { return flags; }

    [[nodiscard]] const Flags16& get_flags() const { return flags; }

    [[nodiscard]] bool see_enemy_now() const;
    [[nodiscard]] bool see_enemy_now(const CEntityAlive* enemy) const;
    [[nodiscard]] bool see_enemy_recently() const;
    [[nodiscard]] bool see_enemy_recently(const CEntityAlive* enemy) const;
    [[nodiscard]] bool enemy_see_me_now() const;

    // вернуть количество врагов
    [[nodiscard]] size_t get_enemies_count() const;

    void add_enemy(const CEntityAlive*);
    [[nodiscard]] bool is_faced(const CEntityAlive* object0, const CEntityAlive* object1);

    [[nodiscard]] bool is_enemy(const CEntityAlive* obj) const;

    // обновить врага в соответствии с врагом у monster
    void transfer_enemy(CBaseMonster* friend_monster);

    [[nodiscard]] u32 get_my_vertex_enemy_last_seen() const { return my_vertex_enemy_last_seen; }
    [[nodiscard]] u32 get_enemy_vertex_enemy_last_seen() const { return enemy_vertex_enemy_last_seen; }
    [[nodiscard]] u32 see_enemy_duration() const;

private:
    const CEntityAlive* m_script_enemy{};

public:
    const CEntityAlive* get_script_enemy() const { return m_script_enemy; }
    void script_enemy();
    void script_enemy(const CEntityAlive& enemy);

    void remove_links(const IGameObject* O);
};
