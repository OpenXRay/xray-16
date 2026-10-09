#include "StdAfx.h"
#include "monster_hit_memory.h"
#include "basemonster/base_monster.h"

void CMonsterHitMemory::update()
{
    // удалить устаревшие hits
    remove_non_actual();
}

bool CMonsterHitMemory::is_hit(const IGameObject* pO)
{
    return std::find(m_hits.begin(), m_hits.end(), pO) != m_hits.end();
}

void CMonsterHitMemory::add_hit(IGameObject* who, const EHitSide side)
{
    SMonsterHit new_hit_info
    {
        .object = who,
        .position = monster->Position(),
        .time = Device.dwTimeGlobal,
        .side = side,
    };

    if (const auto it = std::find(m_hits.begin(), m_hits.end(), who); it == m_hits.end())
        m_hits.emplace_back(new_hit_info);
    else
        *it = new_hit_info;
}

void CMonsterHitMemory::remove_non_actual()
{
    const auto it = std::remove_if(m_hits.begin(), m_hits.end(),
        [mem_time = time_memory, cur_time = Device.dwTimeGlobal](const SMonsterHit& hit_info)
        {
            if ((mem_time + hit_info.time) < cur_time)
                return true;
            if (hit_info.object)
            {
                const CEntityAlive* entity = smart_cast<CEntityAlive*>(hit_info.object);
                if (entity && !entity->g_Alive())
                    return true;
            }
            return false;
        }
    );
    m_hits.erase(it, m_hits.end());
}

Fvector CMonsterHitMemory::get_last_hit_dir() const
{
    Fvector dir = monster->Direction();

    // найти последний по времени хит
    SMonsterHit last_hit{};
    last_hit.side = eSideFront;

    for (const auto& hit : m_hits)
    {
        if (hit.time > last_hit.time)
            last_hit = hit;
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

TTime CMonsterHitMemory::get_last_hit_time() const
{
    SMonsterHit last_hit{};

    for (const auto& hit : m_hits)
    {
        if (hit.time > last_hit.time)
            last_hit = hit;
    }

    return last_hit.time;
}

IGameObject* CMonsterHitMemory::get_last_hit_object() const
{
    SMonsterHit last_hit{};

    for (const auto& hit : m_hits)
    {
        if (hit.time > last_hit.time)
            last_hit = hit;
    }

    return last_hit.object;
}

Fvector CMonsterHitMemory::get_last_hit_position() const
{
    SMonsterHit last_hit{};

    for (const auto& hit : m_hits)
    {
        if (hit.time > last_hit.time)
            last_hit = hit;
    }

    return last_hit.position;
}

void CMonsterHitMemory::remove_hit_info(const IGameObject* obj)
{
    const auto it = std::remove_if(m_hits.begin(), m_hits.end(),
        [obj](const SMonsterHit& hit_info)
        {
            return obj == hit_info.object;
        }
    );
    m_hits.erase(it, m_hits.end());
}
