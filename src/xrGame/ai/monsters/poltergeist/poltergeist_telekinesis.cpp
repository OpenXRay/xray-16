#include "StdAfx.h"
#include "poltergeist.h"
#include "PhysicsShellHolder.h"
#include "Level.h"
#include "Actor.h"
#include "xrPhysics/IColisiondamageInfo.h"
CPolterTele::CPolterTele(CPoltergeist* polter) : inherited(polter), m_pmt_object_collision_damage(0.5f) {}
CPolterTele::~CPolterTele() {}
void CPolterTele::load(LPCSTR section)
{
    inherited::load(section);

    m_pmt_radius = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Find_Radius", 10.f);
    m_pmt_object_min_mass = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Object_Min_Mass", 40.f);
    m_pmt_object_max_mass = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Object_Max_Mass", 500.f);
    m_pmt_object_count = READ_IF_EXISTS(pSettings, r_u32, section, "Tele_Object_Count", 10);
    m_pmt_time_to_hold = READ_IF_EXISTS(pSettings, r_u32, section, "Tele_Hold_Time", 3000);
    m_pmt_time_to_wait = READ_IF_EXISTS(pSettings, r_u32, section, "Tele_Wait_Time", 3000);
    m_pmt_time_to_wait_in_objects = READ_IF_EXISTS(pSettings, r_u32, section, "Tele_Delay_Between_Objects_Time", 500);
    m_pmt_distance = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Distance", 50.f);
    m_pmt_object_height = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Object_Height", 10.f);
    m_pmt_time_object_keep = READ_IF_EXISTS(pSettings, r_u32, section, "Tele_Time_Object_Keep", 10000);
    m_pmt_raise_speed = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Raise_Speed", 3.f);
    m_pmt_raise_time_to_wait_in_objects =
        READ_IF_EXISTS(pSettings, r_u32, section, "Tele_Delay_Between_Objects_Raise_Time", 500);
    m_pmt_fly_velocity = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Fly_Velocity", 30.f);
    m_pmt_object_collision_damage = READ_IF_EXISTS(pSettings, r_float, section, "Tele_Collision_Damage", 0.5f);
    GEnv.Sound->create(m_sound_tele_hold, pSettings->r_string(section, "sound_tele_hold"), st_Effect, SOUND_TYPE_WORLD);
    GEnv.Sound->create(m_sound_tele_throw, pSettings->r_string(section, "sound_tele_throw"), st_Effect, SOUND_TYPE_WORLD);

    m_state = eWait;
    m_time = 0;
    m_time_next = 0;
}

void CPolterTele::update_frame() { inherited::update_frame(); }
void CPolterTele::update_schedule()
{
    inherited::update_schedule();

    Fvector const actor_pos = Actor()->Position();
    float const dist2actor = actor_pos.distance_to(m_object->Position());

    if (dist2actor > m_pmt_distance)
        return;

    if (m_object->get_current_detection_level() < m_object->get_detection_success_level())
        return;

    if (m_object->get_actor_ignore())
        return;

    switch (m_state)
    {
    case eStartRaiseObjects:
        if (m_time + m_time_next < time())
        {
            if (!tele_raise_objects())
                m_state = eRaisingObjects;

            m_time = time();
            m_time_next =
                m_pmt_raise_time_to_wait_in_objects / 2 + Random.randI(m_pmt_raise_time_to_wait_in_objects / 2);
        }

        if (m_state == eStartRaiseObjects)
        {
            if (m_object->CTelekinesis::get_objects_count() >= m_pmt_object_count)
            {
                m_state = eRaisingObjects;
                m_time = time();
            }
        }

        break;
    case eRaisingObjects:
        if (m_time + m_pmt_time_to_hold > time())
            break;

        m_time = time();
        m_time_next = 0;
        m_state = eFireObjects;
    case eFireObjects:
        if (m_time + m_time_next < time())
        {
            tele_fire_objects();

            m_time = time();
            m_time_next = m_pmt_time_to_wait_in_objects / 2 + Random.randI(m_pmt_time_to_wait_in_objects / 2);
        }

        if (m_object->CTelekinesis::get_objects_count() == 0)
        {
            m_state = eWait;
            m_time = time();
        }
        break;
    case eWait:
        if (m_time + m_pmt_time_to_wait < time())
        {
            m_time_next = 0;
            m_state = eStartRaiseObjects;
        }
        break;
    }
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
        float dist1 = monster_pos.distance_to(tpObject1->Position());
        float dist2 = enemy_pos.distance_to(tpObject2->Position());
        float dist3 = enemy_pos.distance_to(monster_pos);

        return ((dist1 < dist3) && (dist2 > dist3));
    };
};

class best_object_predicate2
{
    Fvector enemy_pos;
    Fvector monster_pos;

public:
    typedef IGameObject* CObject_ptr;

    best_object_predicate2(const Fvector& m_pos, const Fvector& pos)
    {
        monster_pos = m_pos;
        enemy_pos = pos;
    }

    bool operator()(const CObject_ptr& tpObject1, const CObject_ptr& tpObject2) const
    {
        float dist1 = enemy_pos.distance_to(tpObject1->Position());
        float dist2 = enemy_pos.distance_to(tpObject2->Position());

        return (dist1 < dist2);
    };
};

//////////////////////////////////////////////////////////////////////////

bool CPolterTele::trace_object(IGameObject* obj, const Fvector& target)
{
    Fvector trace_from;
    obj->Center(trace_from);

    Fvector dir;
    float range;
    dir.sub(target, trace_from);

    range = dir.magnitude();
    if (range < 0.0001f)
        return false;

    dir.normalize();

    collide::rq_result l_rq;
    if (Level().ObjectSpace.RayPick(trace_from, dir, range, collide::rqtBoth, l_rq, obj))
    {
        if (l_rq.O == Actor())
            return true;
    }

    return false;
}

void CPolterTele::tele_find_objects(xr_vector<IGameObject*>& objects, const Fvector& pos)
{
    m_nearest.clear();
    Level().ObjectSpace.GetNearest(m_nearest, pos, m_pmt_radius, NULL);

    for (u32 i = 0; i < m_nearest.size(); i++)
    {
        CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(m_nearest[i]);
        CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(m_nearest[i]);
        if (!obj || !obj->PPhysicsShell() || !obj->PPhysicsShell()->isActive() || custom_monster ||
            (obj->spawn_ini() && obj->spawn_ini()->section_exist("ph_heavy")) ||
            (obj->m_pPhysicsShell->getMass() < m_pmt_object_min_mass) ||
            (obj->m_pPhysicsShell->getMass() > m_pmt_object_max_mass) || (obj == m_object) ||
            m_object->CTelekinesis::is_active_object(obj) || !obj->m_pPhysicsShell->get_ApplyByGravity())
            continue;

        Fvector center;
        Actor()->Center(center);

        if (trace_object(obj, center) || trace_object(obj, get_head_position(Actor())))
            objects.push_back(obj);
    }
}

bool CPolterTele::tele_raise_objects()
{
    // find objects near actor
    xr_vector<IGameObject*> tele_objects;
    tele_objects.reserve(20);

    // получить список объектов вокруг врага
    tele_find_objects(tele_objects, Actor()->Position());

    // получить список объектов вокруг монстра
    tele_find_objects(tele_objects, m_object->Position());

    // получить список объектов между монстром и врагом
    float dist = Actor()->Position().distance_to(m_object->Position());
    Fvector dir;
    dir.sub(Actor()->Position(), m_object->Position());
    dir.normalize();

    Fvector pos;
    pos.mad(m_object->Position(), dir, dist / 2.f);
    tele_find_objects(tele_objects, pos);

    // сортировать и оставить только необходимое количество объектов
    std::sort(
        tele_objects.begin(), tele_objects.end(), best_object_predicate2(m_object->Position(), Actor()->Position()));

    // оставить уникальные объекты
    tele_objects.erase(std::unique(tele_objects.begin(), tele_objects.end()), tele_objects.end());

    // оставить необходимое количество объектов
    // if (tele_objects.size() > m_pmt_tele_object_count) tele_objects.resize	(m_pmt_tele_object_count);

    //// активировать
    // for (u32 i=0; i<tele_objects.size(); i++) {
    //	CPhysicsShellHolder *obj = smart_cast<CPhysicsShellHolder *>(tele_objects[i]);

    //	// применить телекинез на объект
    //	bool	rotate = false;

    //	CTelekinesis::activate		(obj, m_pmt_tele_raise_speed, m_pmt_tele_object_height, m_pmt_tele_time_object_keep,
    // rotate);
    //}

    if (!tele_objects.empty())
    {
        CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(tele_objects[0]);

        // применить телекинез на объект
        bool rotate = false;

        CTelekineticObject* tele_obj = m_object->CTelekinesis::activate(
            obj, m_pmt_raise_speed, m_pmt_object_height, m_pmt_time_object_keep, rotate);
        tele_obj->set_sound(m_sound_tele_hold, m_sound_tele_throw);

        return true;
    }

    return false;
}
struct SCollisionHitCallback : public ICollisionHitCallback

{
    //	CollisionHitCallbackFun				*m_collision_hit_callback
    //;
    CPhysicsShellHolder* m_object;
    float m_pmt_object_collision_damage;
    SCollisionHitCallback(CPhysicsShellHolder* object, float pmt_object_collision_damage)
        : m_object(object), m_pmt_object_collision_damage(pmt_object_collision_damage)
    {
        VERIFY(object);
    }
    void call(IPhysicsShellHolder* obj, float min_cs, float max_cs, float& cs, float& hl, ICollisionDamageInfo* di)
    {
        if (cs > min_cs * 0.5f)
            hl = m_pmt_object_collision_damage;
        VERIFY(m_object);
        di->SetInitiated();
        m_object->set_collision_hit_callback(0); // delete this!!
    }
};

void CPolterTele::tele_fire_objects()
{
    for (u32 i = 0; i < m_object->CTelekinesis::get_objects_total_count(); i++)
    {
        CTelekineticObject tele_object = m_object->CTelekinesis::get_object_by_index(i);
        // if (tele_object.get_state() != TS_Fire) {
        if ((tele_object.get_state() == TS_Raise) || (tele_object.get_state() == TS_Keep))
        {
            Fvector enemy_pos;
            enemy_pos = get_head_position(Actor());
            CPhysicsShellHolder* hobj = tele_object.get_object();

            VERIFY(hobj);
            hobj->set_collision_hit_callback(new SCollisionHitCallback(hobj, m_pmt_object_collision_damage));
            m_object->CTelekinesis::fire_t(tele_object.get_object(), enemy_pos,
                tele_object.get_object()->Position().distance_to(enemy_pos) / m_pmt_fly_velocity);
            return;
        }
    }
}
