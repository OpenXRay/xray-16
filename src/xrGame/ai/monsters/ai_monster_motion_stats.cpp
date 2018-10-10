#include "StdAfx.h"
#include "ai_monster_motion_stats.h"
#include "basemonster/base_monster.h"

void CMotionStats::update()
{
    elem _new;
    _new.speed = pMonster->control().movement().velocity_current();
    _new.position = pMonster->Position();
    _new.time = pMonster->m_dwCurrentTime;
    _data[index] = _new;

    // обновить значение index
    if ((index + 1) >= MAX_ELEMS)
    {
        for (u32 i = 0; i < (MAX_ELEMS - 1); i++)
        {
            _data[i] = _data[i + 1];
        }
    }
    else
        index++;
}

bool CMotionStats::is_good_motion(u32 elems_checked)
{
    u32 from_index;
    u32 to_index;

    if (index == 0)
        return true;
    else
        from_index = index - 1;

    if (s32(index - elems_checked) < 0)
        return true;
    else
        to_index = index - elems_checked;

    bool bGood = true;
    float test_speed = _data[from_index].speed;

    for (u32 i = from_index; i > to_index; i--)
    {
        // считать только, если все элементы содержат одинаковые скорости
        if (!fsimilar(test_speed, _data[i].speed))
            break;

        float cur_dist = _data[i].position.distance_to(_data[i - 1].position);
        TTime delta_t = _data[i].time - _data[i - 1].time;
        float speed = cur_dist * 1000.f / float(delta_t);

        if (fsimilar(_data[i - 1].speed, 0.0f))
            continue;

        if (speed * 5.f < _data[i].speed)
        {
            bGood = false;
            break;
        }
    }
    return bGood;
}
