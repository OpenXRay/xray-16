#pragma once

// Lain:
#include "steering_behaviour.h"

class CEntity;
class CEntityAlive;
class CBaseMonster;
//////////////////////////////////////////////////////////////////////////
// Member local goal notification
//////////////////////////////////////////////////////////////////////////
enum EMemberGoalType
{
    MG_AttackEnemy, // entity
    MG_PanicFromEnemy, // entity
    MG_InterestingSound, // position
    MG_DangerousSound, // position
    MG_WalkGraph, // node
    MG_Rest, // node, position
    MG_None,
};

struct SMemberGoal
{
    EMemberGoalType type;
    CEntity* entity;
    Fvector position;
    u32 node;

    SMemberGoal() : node(0)
    {
        type = MG_None;
        entity = 0;
    }
};

//////////////////////////////////////////////////////////////////////////
// Squad command
//////////////////////////////////////////////////////////////////////////
enum ESquadCommandType
{
    SC_EXPLORE,
    SC_ATTACK,
    SC_THREATEN,
    SC_COVER,
    SC_FOLLOW,
    SC_FEEL_DANGER,
    SC_EXPLICIT_ACTION,
    SC_REST,
    SC_NONE,
};

struct SSquadCommand
{
    ESquadCommandType type; // тип команды

    const CEntity* entity;
    Fvector position;
    u32 node;
    Fvector direction;
};

/////////////////////////////////////////////////////////////////////////////////////////
// MonsterSquad Class
class CMonsterSquad
{
public:
    using MEMBER_COMMAND_MAP = xr_map<const CEntity*, SSquadCommand>;
    using MEMBER_COMMAND_MAP_IT = MEMBER_COMMAND_MAP::iterator;

private:
    CEntity* leader;
    using MEMBER_GOAL_MAP = xr_map<CEntity*, SMemberGoal>;

    // карта целей членов группы (обновляется со стороны объекта)
    MEMBER_GOAL_MAP m_goals;

    // карта комманд членов группы (обновляется со стороны squad manager)
    MEMBER_COMMAND_MAP m_commands;

    using NODES_VECTOR = xr_vector<u32>;
    NODES_VECTOR m_locked_covers;

    using CORPSES_VECTOR = xr_vector<const CEntityAlive*>;
    CORPSES_VECTOR m_locked_corpses;

public:
    CMonsterSquad();
    ~CMonsterSquad();

    // -----------------------------------------------------------------

    void RegisterMember(CEntity* pE);
    void RemoveMember(CEntity* pE);

    bool SquadActive();
    u8 squad_alife_count();

    // -----------------------------------------------------------------

    void SetLeader(CEntity* pE) { leader = pE; }
    CEntity* GetLeader() { return leader; }
    // -----------------------------------------------------------------

    void UpdateGoal(CEntity* pE, const SMemberGoal& goal);
    void InformSquadAboutEnemy(CEntityAlive const* const enemy);
    void UpdateCommand(const CEntity* pE, const SSquadCommand& com);

    void GetGoal(CEntity* pE, SMemberGoal& goal);
    void GetCommand(CEntity* pE, SSquadCommand& com);
    SMemberGoal& GetGoal(CEntity* pE);
    SSquadCommand& GetCommand(CEntity* pE);

    // -----------------------------------------------------------------

    void UpdateSquadCommands();

    void remove_links(IGameObject* O);

    // return count of monsters in radius for object
    u8 get_count(const CEntity* object, float radius);
    void set_squad_index(const CEntity* m_enemy);
    void set_rat_squad_index(const CEntity* m_enemy);
    u8 get_index(CEntity* m_object) const;

    ///////////////////////////////////////////////////////////////////////////////////////
    //  Общие данные
    //////////////////////////////////////////////////////////////////////////////////////

    using ENTITY_VEC = xr_vector<CEntity*>;
    ENTITY_VEC m_temp_entities;

    ///////////////////////////////////////////////////////////////////////////////////////
    //  Атака группой монстров
    //////////////////////////////////////////////////////////////////////////////////////

    using ENEMY_MAP = xr_map<const CEntity*, ENTITY_VEC>;

    ENEMY_MAP m_enemy_map;

    void ProcessAttack();

    // -- Temp --
    struct _elem
    {
        CEntity* pE;
        Fvector p_from;
        float yaw;
    };
    xr_vector<_elem> lines;
    // ------------

    void Attack_AssignTargetDir(ENTITY_VEC& members, const CEntity* enemy);

    void get_index_in_squad(ENTITY_VEC& members, const CEntity* m_enemy) const;
    void get_index_in_rat_squad(ENTITY_VEC& members, const CEntity* m_enemy);

    ////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////
    //  групповой idle
    //////////////////////////////////////////////////////////////////////////////////////
    ENTITY_VEC front, back, left, right;

    void ProcessIdle();
    void Idle_AssignAction(ENTITY_VEC& members);

    ////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////
    //  Covers
    //////////////////////////////////////////////////////////////////////////////////////
    bool is_locked_cover(u32 node);
    void lock_cover(u32 node);
    void unlock_cover(u32 node);
    ////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////
    //  Corpses
    //////////////////////////////////////////////////////////////////////////////////////
    bool is_locked_corpse(const CEntityAlive*);
    void lock_corpse(const CEntityAlive*);
    void unlock_corpse(const CEntityAlive*);
    ////////////////////////////////////////////////////////////////////////////////////////

    // Lain: added
    MEMBER_COMMAND_MAP* get_commands() { return &m_commands; }
    bool home_in_danger() { return Device.dwTimeGlobal < m_home_danger_end_tick; }
    void set_home_in_danger() { m_home_danger_end_tick = Device.dwTimeGlobal + m_home_danger_mode_time; }
private:
    // danger mode is turns on when monsters hear dangerous sound or get a hit
    // danger mode turns off after m_danger_mode_time miliseconds
    u32 m_home_danger_mode_time;
    u32 m_home_danger_end_tick;

    void assign_monsters_target_dirs(ENTITY_VEC& members, const CEntity* enemy);
    Fvector calc_monster_target_dir(CBaseMonster* monster, const CEntity* enemy);
};

class squad_grouping_behaviour : public steering_behaviour::grouping::params
{
public:
    squad_grouping_behaviour(CEntity* self, Fvector cohesion_factor, Fvector separate_factor, float max_separate_range);

    void set_squad(CMonsterSquad* squad);
    virtual void first_nearest(Fvector& v);
    virtual bool nomore_nearest();
    virtual void next_nearest(Fvector& v);

    virtual bool update();

private:
    CMonsterSquad* squad;
    CMonsterSquad::MEMBER_COMMAND_MAP_IT it_cur;
    CEntity* self;
};
