#pragma once

#include "Level.h"

#define GOOD_DISTANCE_FOR_TELE 15.f
#define MAX_TIME_CHECK_FAILURE 6000

template <typename Object>
CStateBurerAttackTele<Object>::CStateBurerAttackTele(Object* obj)
    : inherited(obj), selected_object(nullptr), time_started(0),
      m_action(), m_end_tick(0), m_initial_health(0)
{
    m_anim_end_tick = 0;
    m_last_grenade_scan = 0;
}

template <typename Object>
void CStateBurerAttackTele<Object>::initialize()
{
    inherited::initialize();

    m_action = ACTION_TELE_STARTED;
    selected_object = 0;

    SelectObjects();

    time_started = 0;
    m_anim_end_tick = 0;
    m_last_grenade_scan = 0;
    m_initial_health = this->object->conditions().GetHealth();
    m_end_tick = current_time() + this->object->m_tele_max_time;

    // запретить взятие скриптом
    this->object->set_script_capture(false);
}

template <typename Object>
void CStateBurerAttackTele<Object>::execute()
{
    HandleGrenades();
    // 	if ( object->EnemyMan.see_enemy_now() )
    // 	{
    // 		m_last_saw_enemy_tick					=	current_time();
    // 	}

    switch (m_action)
    {
    case ACTION_TELE_STARTED:
        this->object->anim().set_override_animation(eAnimTelekinesis, 0);
        if (!time_started)
        {
            float const time = this->object->anim().get_animation_length(eAnimTelekinesis, 0);
            m_anim_end_tick = current_time() + TTime(time * 1000);
            time_started = Device.dwTimeGlobal;
        }
        else
        {
            if (current_time() > m_anim_end_tick)
            {
                m_action = ACTION_TELE_CONTINUE;
            }
        }
        break;

    case ACTION_TELE_CONTINUE:
        this->object->anim().set_override_animation(eAnimTelekinesis, 1);
        ExecuteTeleContinue();
        break;

    case ACTION_TELE_FIRE:
    {
        this->object->anim().set_override_animation(eAnimTeleFire, 0);
        ExecuteTeleFire();
        float const time = this->object->anim().get_animation_length(eAnimTeleFire, 0);
        m_anim_end_tick = current_time() + TTime(time * 1000);
        m_action = ACTION_WAIT_FIRE_END;
        break;
    }

    case ACTION_WAIT_FIRE_END:
        this->object->anim().set_override_animation(eAnimTeleFire, 0);
        if (current_time() > m_anim_end_tick)
        {
            if (IsActiveObjects())
            {
                m_action = ACTION_TELE_CONTINUE;
            }
            else
            {
                m_action = ACTION_COMPLETED;
            }
        }

    case ACTION_COMPLETED: break;
    }

    this->object->face_enemy();
}

template <typename Object>
void CStateBurerAttackTele<Object>::deactivate()
{
    tele_objects.clear();
    // clear particles on active objects
    if (this->object->CTelekinesis::is_active())
    {
        for (u32 i = 0; i < this->object->CTelekinesis::get_objects_total_count(); i++)
        {
            CPhysicsShellHolder* cur_object = this->object->CTelekinesis::get_object_by_index(i).get_object();
            if (!cur_object || !cur_object->m_pPhysicsShell || !cur_object->m_pPhysicsShell->isActive())
            {
                continue;
            }
            if (CGrenade* grenade = smart_cast<CGrenade*>(cur_object))
            {
                grenade->set_destroy_callback(NULL);
            }
        }
    }

    for (u32 i = 0; i <this->object->CTelekinesis::get_objects_total_count(); ++i)
    {
        CPhysicsShellHolder* const cur_object = this->object->CTelekinesis::get_object_by_index(i).object;
        if (!cur_object || !cur_object->m_pPhysicsShell || !cur_object->m_pPhysicsShell->isActive())
        {
            continue;
        }

        this->object->StopTeleObjectParticle(cur_object);
    }

    FireAllToEnemy();
    this->object->CTelekinesis::deactivate();
    this->object->set_script_capture(true);
}

template <typename Object>
void CStateBurerAttackTele<Object>::finalize()
{
    deactivate();
    inherited::finalize();
}

template <typename Object>
void CStateBurerAttackTele<Object>::critical_finalize()
{
    deactivate();
    inherited::critical_finalize();
}

template <typename Object>
bool CStateBurerAttackTele<Object>::check_start_conditions()
{
    return CheckTeleStart();
}

template <typename Object>
bool CStateBurerAttackTele<Object>::check_completion()
{
    const float dist = this->object->EnemyMan.get_enemy()->Position().distance_to(this->object->Position());

    if (dist < this->object->m_tele_min_distance)
    {
        return true;
    }

    if (dist > this->object->m_tele_max_distance)
    {
        return true;
    }

    if (this->object->conditions().GetHealth() < m_initial_health)
    {
        return true;
    }

    if (current_time() > m_end_tick)
    {
        return true;
    }

    if (m_action == ACTION_COMPLETED)
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////

template <typename Object>
void CStateBurerAttackTele<Object>::FindFreeObjects(xr_vector<IGameObject*>& tpObjects, const Fvector& pos)
{
    Level().ObjectSpace.GetNearest(tpObjects, pos, this->object->m_tele_find_radius, nullptr);

    for (u32 i = 0; i < tpObjects.size(); i++)
    {
        CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(tpObjects[i]);
        CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(tpObjects[i]);
        CGrenade* grenade = smart_cast<CGrenade*>(tpObjects[i]);

        if (grenade || // grenades are handled by HandleGrenades function
            !obj || !obj->PPhysicsShell() || !obj->PPhysicsShell()->isActive() || custom_monster ||
            (obj->spawn_ini() && obj->spawn_ini()->section_exist("ph_heavy")) ||
            (obj->m_pPhysicsShell->getMass() < this->object->m_tele_object_min_mass) ||
            (obj->m_pPhysicsShell->getMass() >this->object->m_tele_object_max_mass) || (obj == this->object) ||
            this->object->CTelekinesis::is_active_object(obj) || !obj->m_pPhysicsShell->get_ApplyByGravity())
            continue;

        tele_objects.push_back(obj);
    }
}

template <typename Object>
void CStateBurerAttackTele<Object>::FindObjects()
{
    u32 res_size = tele_objects.size();
    tele_objects.clear();

    // получить список объектов вокруг врага
    m_nearest.clear();
    m_nearest.reserve(res_size);
    FindFreeObjects(m_nearest, this->object->EnemyMan.get_enemy()->Position());

    // получить список объектов вокруг монстра
    FindFreeObjects(m_nearest, this->object->Position());

    // получить список объектов между монстром и врагом
    const float dist = this->object->EnemyMan.get_enemy()->Position().distance_to(this->object->Position());
    Fvector dir;
    dir.sub(this->object->EnemyMan.get_enemy()->Position(), this->object->Position());
    dir.normalize();

    Fvector pos;
    pos.mad(this->object->Position(), dir, dist / 2.f);
    FindFreeObjects(m_nearest, pos);

    // оставить уникальные объекты
    tele_objects.erase(std::unique(tele_objects.begin(), tele_objects.end()), tele_objects.end());
}

template <typename Object>
void CStateBurerAttackTele<Object>::FireAllToEnemy()
{
    if (!this->object->CTelekinesis::is_active())
    {
        return;
    }

    if (!this->object->EnemyMan.get_enemy())
    {
        return;
    }

    Fvector enemy_pos = get_head_position(const_cast<CEntityAlive*>(this->object->EnemyMan.get_enemy()));

    for (u32 i = 0; i <this->object->CTelekinesis::get_objects_count(); ++i)
    {
        u32 const prev_num_objects = this->object->CTelekinesis::get_objects_count();

        CPhysicsShellHolder* const cur_object = this->object->CTelekinesis::get_object_by_index(i).object;
        if (!cur_object)
        {
            continue;
        }
        float const dist_to_enemy = cur_object->Position().distance_to(enemy_pos);
        float const fire_time = dist_to_enemy / this->object->m_tele_fly_velocity;

        this->object->CTelekinesis::fire_t(cur_object, enemy_pos, fire_time);

        u32 const new_num_objects = this->object->CTelekinesis::get_objects_count();
        if (new_num_objects < prev_num_objects)
        {
            VERIFY(new_num_objects == prev_num_objects - 1);
            --i;
        }
    }

    // object->CTelekinesis::fire_all(enemy_pos);

    this->object->sound().play(CBurer::eMonsterSoundTeleAttack);
}

template <typename Object>
void CStateBurerAttackTele<Object>::ExecuteTeleContinue()
{
    if (time_started + this->object->m_tele_time_to_hold > Device.dwTimeGlobal)
        return;

    if (!this->object->EnemyMan.see_enemy_now())
        return;

    // найти объект для атаки
    bool object_found = false;
    CTelekineticObject tele_object;

    u32 i = 0;
    while (i <this->object->CTelekinesis::get_objects_count())
    {
        tele_object = this->object->CTelekinesis::get_object_by_index(i);

        if ((tele_object.get_state() == TS_Keep) && (tele_object.time_keep_started + 1500 < Device.dwTimeGlobal))
        {
            object_found = true;
            break;
        }
        else
            i++;
    }

    if (object_found)
    {
        m_action = ACTION_TELE_FIRE;
        selected_object = tele_object.get_object();
    }
    else
    {
        if (!IsActiveObjects() || (time_started + MAX_TIME_CHECK_FAILURE < Device.dwTimeGlobal))
        {
            m_action = ACTION_COMPLETED;
        }
    }
}

#define HEAD_OFFSET_INDOOR 1.f
#define HEAD_OFFSET_OUTDOOR 5.f

template <typename Object>
void CStateBurerAttackTele<Object>::ExecuteTeleFire()
{
    Fvector enemy_pos = get_head_position(const_cast<CEntityAlive*>(this->object->EnemyMan.get_enemy()));

    float const dist_to_enemy = selected_object->Position().distance_to(enemy_pos);
    float const fire_time = dist_to_enemy / this->object->m_tele_fly_velocity;

    this->object->CTelekinesis::fire_t(selected_object, enemy_pos, fire_time);

    this->object->StopTeleObjectParticle(selected_object);
    this->object->sound().play(CBurer::eMonsterSoundTeleAttack);
}

template <typename Object>
bool CStateBurerAttackTele<Object>::IsActiveObjects()
{
    return (this->object->CTelekinesis::get_objects_count() > 0);
}

template <typename Object>
bool CStateBurerAttackTele<Object>::CheckTeleStart()
{
    // проверка на текущую активность
    if (IsActiveObjects())
        return false;

    // проверить дистанцию до врага
    const float dist = this->object->Position().distance_to(this->object->EnemyMan.get_enemy()->Position());
    if (dist < this->object->m_tele_min_distance)
        return false;
    if (dist > this->object->m_tele_max_distance)
        return false;

    // найти телекинетические объекты
    FindObjects();

    // если нет объектов
    if (tele_objects.empty())
        return false;

    // всё ок можно начинать телекинез
    return true;
}

//////////////////////////////////////////////////////////////////////////
// Выбор подходящих объектов для телекинеза
//////////////////////////////////////////////////////////////////////////
class best_object_predicate
{
    Fvector enemy_pos;
    Fvector monster_pos;

public:
    best_object_predicate(const Fvector& m_pos, const Fvector& pos)
    {
        monster_pos = m_pos;
        enemy_pos = pos;
    }

    bool operator()(const CGameObject* tpObject1, const CGameObject* tpObject2) const
    {
        const float dist1 = monster_pos.distance_to(tpObject1->Position());
        const float dist2 = enemy_pos.distance_to(tpObject2->Position());
        const float dist3 = enemy_pos.distance_to(monster_pos);

        return dist1 < dist3 && dist2 > dist3;
    };
};

class best_object_predicate2
{
    Fvector enemy_pos;
    Fvector monster_pos;

public:
    best_object_predicate2(const Fvector& m_pos, const Fvector& pos)
    {
        monster_pos = m_pos;
        enemy_pos = pos;
    }

    bool operator()(const CGameObject* tpObject1, const CGameObject* tpObject2) const
    {
        const float dist1 = enemy_pos.distance_to(tpObject1->Position());
        const float dist2 = enemy_pos.distance_to(tpObject2->Position());

        return dist1 < dist2;
    }
};

template <typename Object>
void CStateBurerAttackTele<Object>::SelectObjects()
{
    std::sort(tele_objects.begin(), tele_objects.end(),
        best_object_predicate2(this->object->Position(), this->object->EnemyMan.get_enemy()->Position()));

    // выбрать объект
    for (u32 i = 0; i < tele_objects.size(); ++i)
    {
        CPhysicsShellHolder* obj = tele_objects[i];

        // применить телекинез на объект

        float height = this->object->m_tele_object_height;

        if (this->object->m_monster_type == CBaseMonster::eMonsterTypeIndoor)
        {
            height *= 0.7f;
        }

        bool const rotate = this->object->m_monster_type != CBaseMonster::eMonsterTypeIndoor;

        CTelekineticObject* tele_obj =
            this->object->CTelekinesis::activate(obj, this->object->m_tele_raise_speed, height, 10000, rotate);

        tele_obj->set_sound(this->object->sound_tele_hold, this->object->sound_tele_throw);

        this->object->StartTeleObjectParticle(obj);

        // удалить из списка
        tele_objects[i] = tele_objects[tele_objects.size() - 1];
        tele_objects.pop_back();

        if (this->object->CTelekinesis::get_objects_count() >= this->object->m_tele_max_handled_objects)
        {
            break;
        }
    }
}

template <typename Object>
void xr_stdcall CStateBurerAttackTele<Object>::OnGrenadeDestroyed(CGrenade* const grenade)
{
    this->object->CTelekinesis::remove_links(grenade);
}

template <typename Object>
void CStateBurerAttackTele<Object>::HandleGrenades()
{
    if (current_time() < m_last_grenade_scan + 1000)
    {
        return;
    }

    m_nearest.clear();
    Level().ObjectSpace.GetNearest(m_nearest, this->object->Position(), this->object->m_tele_find_radius, nullptr);

    for (u32 i = 0; i < m_nearest.size(); ++i)
    {
        CGrenade* grenade = smart_cast<CGrenade*>(m_nearest[i]);

        if (!grenade || !grenade->PPhysicsShell() || !grenade->PPhysicsShell()->isActive() ||
            this->object->CTelekinesis::is_active_object(grenade) || !grenade->m_pPhysicsShell->get_ApplyByGravity())
        {
            continue;
        }

        grenade->set_destroy_callback(
            CGrenade::destroy_callback(this, &CStateBurerAttackTele<Object>::OnGrenadeDestroyed));

        float const height = 2.5f;
        bool const rotate = false;

        CTelekineticObject* tele_obj = this->object->CTelekinesis::activate(grenade, 3.f, height, 10000, rotate);
        tele_obj->set_sound(this->object->sound_tele_hold, this->object->sound_tele_throw);
        this->object->StartTeleObjectParticle(grenade);

        if (this->object->CTelekinesis::get_objects_count() >= this->object->m_tele_max_handled_objects + 1)
        {
            break;
        }
    }
}
