#include "StdAfx.h"
#include "ai_monster_squad.h"
#include "Entity.h"
#include "entity_alive.h"
#include "memory_manager.h"
#include "basemonster/base_monster.h"

CMonsterSquad::CMonsterSquad() : leader(0), m_home_danger_end_tick(0), m_home_danger_mode_time(8000)
{
    m_locked_covers.reserve(20);
    m_locked_corpses.reserve(10);
}

CMonsterSquad::~CMonsterSquad() {}
void CMonsterSquad::RegisterMember(CEntity* pE)
{
    // Добавить цель
    SMemberGoal G;
    m_goals.insert(std::make_pair(pE, G));

    // Добавить команду
    SSquadCommand C;
    C.type = SC_NONE;
    m_commands.insert(std::make_pair(pE, C));

    // установить лидера
    if (!leader)
        leader = pE;
}

void CMonsterSquad::RemoveMember(CEntity* pE)
{
    // удалить из целей
    auto it_goal = m_goals.find(pE);
    if (it_goal == m_goals.end())
        return;
    m_goals.erase(it_goal);

    // удалить из команд
    MEMBER_COMMAND_MAP_IT it_command = m_commands.find(pE);
    if (it_command == m_commands.end())
        return;
    m_commands.erase(it_command);

    // если удаляемый елемент является лидером - переназначить лидера
    if (leader == pE)
    {
        if (m_goals.empty())
            leader = 0;
        else
            leader = m_goals.begin()->first;
    }

    // усли последний элемент, очистить залоченные каверы
    if (m_goals.empty())
    {
        m_locked_covers.clear();
        m_locked_corpses.clear();
    }
}

bool CMonsterSquad::SquadActive()
{
    if (!leader)
        return false;

    // проверить количество живых объектов в группе
    u32 alive_num = 0;
    for (auto it = m_goals.begin(); it != m_goals.end(); it++)
        if (it->first->g_Alive())
            alive_num++;

    if (alive_num < 2)
        return false;

    return true;
}

u8 CMonsterSquad::squad_alife_count()
{
    if (!leader)
        return u8(0);

    // проверить количество живых объектов в группе
    u8 alive_num = 0;
    for (auto it = m_goals.begin(); it != m_goals.end(); it++)
        if (it->first->g_Alive())
            alive_num++;

    if (alive_num < 2)
        return u8(0);

    return alive_num;
}

void CMonsterSquad::UpdateGoal(CEntity* pE, const SMemberGoal& goal)
{
    auto it = m_goals.find(pE);
    VERIFY(it != m_goals.end());

    it->second = goal;
}

void CMonsterSquad::InformSquadAboutEnemy(CEntityAlive const* const enemy)
{
    for (auto it = m_goals.begin(); it != m_goals.end(); ++it)
    {
        CBaseMonster* monster = smart_cast<CBaseMonster*>(it->first);

        if (monster)
        {
            monster->EnemyMan.add_enemy(enemy);
            //			monster->memory().make_object_visible_somewhen(enemy);
        }
    }
}

void CMonsterSquad::UpdateCommand(const CEntity* pE, const SSquadCommand& com)
{
    MEMBER_COMMAND_MAP_IT it = m_commands.find(pE);
    VERIFY(it != m_commands.end());

    it->second = com;
}

SMemberGoal& CMonsterSquad::GetGoal(CEntity* pE)
{
    auto it = m_goals.find(pE);
    VERIFY(it != m_goals.end());

    return it->second;
}

SSquadCommand& CMonsterSquad::GetCommand(CEntity* pE)
{
    MEMBER_COMMAND_MAP_IT it = m_commands.find(pE);
    VERIFY(it != m_commands.end());
    return it->second;
}

void CMonsterSquad::GetGoal(CEntity* pE, SMemberGoal& goal) { goal = GetGoal(pE); }
void CMonsterSquad::GetCommand(CEntity* pE, SSquadCommand& com) { com = GetCommand(pE); }
void CMonsterSquad::UpdateSquadCommands()
{
    // Отменить все команды в группе
    for (MEMBER_COMMAND_MAP_IT it = m_commands.begin(); it != m_commands.end(); it++)
    {
        it->second.type = SC_NONE;
    }

    // Удалить все цели, объекты которых невалидны или ушли в оффлайн
    for (auto it_goal = m_goals.begin(); it_goal != m_goals.end(); ++it_goal)
    {
        SMemberGoal goal = it_goal->second;
        if (!goal.entity || goal.entity->getDestroy())
        {
            it_goal->second.type = MG_None;
        }
    }

    ProcessAttack();
    ProcessIdle();
}

void CMonsterSquad::remove_links(IGameObject* O)
{
    // Удалить все цели, объекты которых невалидны или ушли в оффлайн
    for (auto it_goal = m_goals.begin(); it_goal != m_goals.end(); ++it_goal)
    {
        SMemberGoal goal = it_goal->second;
        if (goal.entity == O)
        {
            it_goal->second.entity = 0;
            it_goal->second.type = MG_None;
        }
    }

    // Удалить все цели, объекты которых невалидны или ушли в оффлайн
    for (MEMBER_COMMAND_MAP_IT it = m_commands.begin(); it != m_commands.end(); it++)
    {
        SSquadCommand com = it->second;
        if (com.entity == O)
        {
            it->second.entity = 0;
            it->second.type = SC_NONE;
        }
    }
}

bool CMonsterSquad::is_locked_cover(u32 node)
{
    return (std::find(m_locked_covers.begin(), m_locked_covers.end(), node) != m_locked_covers.end());
}

void CMonsterSquad::lock_cover(u32 node) { m_locked_covers.push_back(node); }
void CMonsterSquad::unlock_cover(u32 node)
{
    auto it = std::find(m_locked_covers.begin(), m_locked_covers.end(), node);
    if (it != m_locked_covers.end())
        m_locked_covers.erase(it);
}

u8 CMonsterSquad::get_index(CEntity* m_object) const { return m_object->cast_entity_alive()->m_squad_index; }
u8 CMonsterSquad::get_count(const CEntity* object, float radius)
{
    u8 count = 0;

    for (auto it_goal = m_goals.begin(); it_goal != m_goals.end(); ++it_goal)
    {
        SMemberGoal goal = it_goal->second;
        if ((goal.entity != 0) && (goal.entity != object) && (goal.entity->g_Alive()))
        {
            if (goal.entity->Position().distance_to(object->Position()) < radius)
                count++;
        }
    }

    return count;
}

//////////////////////////////////////////////////////////////////////////
// Corpses
//////////////////////////////////////////////////////////////////////////
bool CMonsterSquad::is_locked_corpse(const CEntityAlive* corpse)
{
    return (std::find(m_locked_corpses.begin(), m_locked_corpses.end(), corpse) != m_locked_corpses.end());
}

void CMonsterSquad::lock_corpse(const CEntityAlive* corpse) { m_locked_corpses.push_back(corpse); }
void CMonsterSquad::unlock_corpse(const CEntityAlive* corpse)
{
    auto it = std::find(m_locked_corpses.begin(), m_locked_corpses.end(), corpse);
    if (it != m_locked_corpses.end())
        m_locked_corpses.erase(it);
}
//////////////////////////////////////////////////////////////////////////

squad_grouping_behaviour::squad_grouping_behaviour(
    CEntity* self, Fvector cohesion_factor, Fvector separate_factor, float max_separate_range)
    : self(self), squad(NULL),
      steering_behaviour::grouping::params(cohesion_factor, separate_factor, max_separate_range)
{
    VERIFY(self);
}

void squad_grouping_behaviour::set_squad(CMonsterSquad* squad_) { squad = squad_; }
void squad_grouping_behaviour::first_nearest(Fvector& v)
{
    if (!squad)
    {
        return;
    }

    it_cur = squad->get_commands()->begin();
    if (it_cur->first == self)
    {
        ++it_cur;
    }

    if (it_cur != squad->get_commands()->end())
    {
        v = (*it_cur).first->Position();
    }
}

bool squad_grouping_behaviour::nomore_nearest()
{
    if (!squad)
    {
        return true;
    }

    return it_cur == squad->get_commands()->end();
}

void squad_grouping_behaviour::next_nearest(Fvector& v)
{
    if (!squad)
    {
        return;
    }

    if (it_cur != squad->get_commands()->end())
    {
        ++it_cur;
    }

    // if cur == self move onto next
    if (it_cur != squad->get_commands()->end())
    {
        if (it_cur->first == self)
        {
            ++it_cur;
        }
    }

    if (it_cur != squad->get_commands()->end())
    {
        v = (*it_cur).first->Position();
    }
}

float g_separate_factor = 1.f;
float g_separate_radius = 1.f;

bool squad_grouping_behaviour::update()
{
    // separation_factor = cr_fvector3(0, g_separate_factor, 0);
    // max_separate_range = g_separate_radius;
    pos = self->Position();

    return true;
}
