#include "StdAfx.h"
#include "monster_hit_memory.h"
#include "basemonster/base_monster.h"

CMonsterHitMemory::CMonsterHitMemory()
{
    monster = 0;
    time_memory = 10000;
}

CMonsterHitMemory::~CMonsterHitMemory() {}
void CMonsterHitMemory::init_external(CBaseMonster* M, TTime mem_time)
{
    monster = M;
    time_memory = mem_time;
}

void CMonsterHitMemory::update()
{
    // удалить устаревшие hits
    remove_non_actual();
}

bool CMonsterHitMemory::is_hit(IGameObject* pO)
{
    return (std::find(m_hits.begin(), m_hits.end(), pO) != m_hits.end());
}

void CMonsterHitMemory::add_hit(IGameObject* who, EHitSide side)
{
    SMonsterHit new_hit_info;
    new_hit_info.object = who;
    new_hit_info.time = Device.dwTimeGlobal;
    new_hit_info.side = side;
    new_hit_info.position = monster->Position();

    auto it = std::find(m_hits.begin(), m_hits.end(), who);

    if (it == m_hits.end())
        m_hits.push_back(new_hit_info);
    else
        *it = new_hit_info;
}

struct predicate_old_hit
{
    TTime cur_time;
    TTime mem_time;

    predicate_old_hit(TTime mem_time, TTime cur_time)
    {
        this->cur_time = cur_time;
        this->mem_time = mem_time;
    }

    IC bool operator()(const SMonsterHit& hit_info)
    {
        if ((mem_time + hit_info.time) < cur_time)
            return true;
        if (hit_info.object)
        {
            CEntityAlive* entity = smart_cast<CEntityAlive*>(hit_info.object);
            if (entity && !entity->g_Alive())
                return true;
        }
        return false;
    }
};

void CMonsterHitMemory::remove_non_actual()
{
    m_hits.erase(std::remove_if(m_hits.begin(), m_hits.end(), predicate_old_hit(time_memory, Device.dwTimeGlobal)),
        m_hits.end());
}

Fvector CMonsterHitMemory::get_last_hit_dir()
{
    Fvector dir = monster->Direction();

    // найти последний по времени хит
    SMonsterHit last_hit;
    last_hit.time = 0;
    last_hit.side = eSideFront;

    for (u32 i = 0; i < m_hits.size(); i++)
    {
        if (m_hits[i].time > last_hit.time)
            last_hit = m_hits[i];
    }

    // если есть хит, вычислить направление
    if (last_hit.time != 0)
    {
        float h, p;
        dir.getHP(h, p);

        switch (last_hit.side)
        {
        case eSideBack: h += PI; break;
        case eSideLeft: h += PI_DIV_2; break;
        case eSideRight: h -= PI_DIV_2; break;
        }

        dir.setHP(h, p);
        dir.normalize();
    }

    return dir;
}

TTime CMonsterHitMemory::get_last_hit_time()
{
    SMonsterHit last_hit;
    last_hit.time = 0;

    for (u32 i = 0; i < m_hits.size(); i++)
    {
        if (m_hits[i].time > last_hit.time)
            last_hit = m_hits[i];
    }

    return last_hit.time;
}

IGameObject* CMonsterHitMemory::get_last_hit_object()
{
    SMonsterHit last_hit;
    last_hit.object = 0;
    last_hit.time = 0;

    for (u32 i = 0; i < m_hits.size(); i++)
    {
        if (m_hits[i].time > last_hit.time)
            last_hit = m_hits[i];
    }

    return last_hit.object;
}

Fvector CMonsterHitMemory::get_last_hit_position()
{
    SMonsterHit last_hit;
    last_hit.time = 0;
    last_hit.position.set(0.f, 0.f, 0.f);

    for (u32 i = 0; i < m_hits.size(); i++)
    {
        if (m_hits[i].time > last_hit.time)
            last_hit = m_hits[i];
    }

    return last_hit.position;
}

struct predicate_old_info
{
    const IGameObject* object;

    predicate_old_info(const IGameObject* obj) : object(obj) {}
    IC bool operator()(const SMonsterHit& hit_info) { return (object == hit_info.object); }
};

void CMonsterHitMemory::remove_hit_info(const IGameObject* obj)
{
    m_hits.erase(std::remove_if(m_hits.begin(), m_hits.end(), predicate_old_info(obj)), m_hits.end());
}
