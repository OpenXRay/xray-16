#include "StdAfx.h"
#include "monster_sound_memory.h"
#include "basemonster/base_monster.h"

#define CHECK_SOUND_TYPE(a, b, c) \
    {                             \
        if ((a & b) == b)         \
            return c;             \
    }

const u32 time_help_sound_remember = 10000;

TSoundDangerValue SoundElem::ConvertSoundType(ESoundTypes stype)
{
    if (((stype & SOUND_TYPE_WEAPON) != SOUND_TYPE_WEAPON) && ((stype & SOUND_TYPE_MONSTER) != SOUND_TYPE_MONSTER) &&
        ((stype & SOUND_TYPE_WORLD) != SOUND_TYPE_WORLD))
        return NONE_DANGEROUS_SOUND;

    CHECK_SOUND_TYPE(stype, SOUND_TYPE_WEAPON_RECHARGING, WEAPON_RECHARGING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_WEAPON_SHOOTING, WEAPON_SHOOTING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_ITEM_TAKING, WEAPON_TAKING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_ITEM_HIDING, WEAPON_HIDING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_WEAPON_EMPTY_CLICKING, WEAPON_EMPTY_CLICKING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_WEAPON_BULLET_HIT, WEAPON_BULLET_RICOCHET);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_MONSTER_DYING, MONSTER_DYING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_MONSTER_INJURING, MONSTER_INJURING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_MONSTER_STEP, MONSTER_WALKING);

    CHECK_SOUND_TYPE(stype, SOUND_TYPE_MONSTER_TALKING, MONSTER_TALKING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_MONSTER_ATTACKING, MONSTER_ATTACKING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_WORLD_OBJECT_BREAKING, OBJECT_BREAKING);
    CHECK_SOUND_TYPE(stype, SOUND_TYPE_WORLD_OBJECT_COLLIDING, OBJECT_FALLING);

    return NONE_DANGEROUS_SOUND;
}

CMonsterSoundMemory::CMonsterSoundMemory()
{
    Sounds.reserve(20);
}

void CMonsterSoundMemory::HearSound(const SoundElem& s)
{
    if (NONE_DANGEROUS_SOUND == s.type)
        return;
    if (DOOR_OPENING <= s.type)
        return;
    if (s.type == MONSTER_WALKING && !s.who)
        return;

    // поиск в массиве звука
    bool b_sound_replaced = false;
    for (auto& sound : Sounds)
    {
        if (s.who == sound.who && sound.type == s.type)
        {
            if (s.time >= sound.time)
            {
                sound = s;
                b_sound_replaced = true;
            }
        }
    }

    if (!b_sound_replaced)
        Sounds.emplace_back(s);
}

void CMonsterSoundMemory::HearSound(const IGameObject* who, int eType, const Fvector& Position, float power, TTime time)
{
    SoundElem s{ who, eType, Position, power, time };
    s.CalcValue(time, monster->Position());
    HearSound(s);
}

// Lain: added
void CMonsterSoundMemory::GetFirstSound(SoundElem& s, bool& bDangerous) const
{
    VERIFY(!Sounds.empty());
    s = *Sounds.begin();
    if (s.type > WEAPON_EMPTY_CLICKING)
        bDangerous = false;
    else
        bDangerous = true;
}

void CMonsterSoundMemory::GetSound(SoundElem& s, bool& bDangerous)
{
    VERIFY(!Sounds.empty());

    // возврат самого опасного
    s = GetSound();

    if (s.type > WEAPON_EMPTY_CLICKING)
        bDangerous = false;
    else
        bDangerous = true;
}

SoundElem& CMonsterSoundMemory::GetSound()
{
    VERIFY(!Sounds.empty());

    const auto it = std::max_element(Sounds.begin(), Sounds.end());
    return *it;
}

struct pred_remove_nonactual_sounds
{
    TTime new_time;

    pred_remove_nonactual_sounds(TTime time) { new_time = time; }
    bool operator()(const SoundElem& x) const
    {
        // удалить звуки от объектов, перешедших в оффлайн
        if (x.who && x.who->getDestroy())
            return true;

        // удалить 'старые' звуки
        if (x.time < new_time)
            return true;

        // удалить звуки от неживых объектов
        if (x.who)
        {
            const CEntityAlive* pE = smart_cast<const CEntityAlive*>(x.who);
            if (pE && !pE->g_Alive())
                return true;
        }

        return false;
    }
};

void CMonsterSoundMemory::UpdateHearing()
{
    // удаление устаревших звуков
    Sounds.erase(
        std::remove_if(Sounds.begin(), Sounds.end(), pred_remove_nonactual_sounds(Device.dwTimeGlobal - time_memory)),
        Sounds.end());

    // пересчитать value
    for (auto& sound : Sounds)
        sound.CalcValue(Device.dwTimeGlobal, monster->Position());

    // update help sound
    if (m_time_help_sound + time_help_sound_remember < time())
        m_time_help_sound = 0;
}

bool CMonsterSoundMemory::is_loud_sound(const float val) const
{
    for (auto& sound : Sounds)
    {
        if (sound.power > val)
            return true;
    }
    return false;
}

bool CMonsterSoundMemory::get_sound_from_object(const IGameObject* obj, SoundElem& value) const
{
    for (const auto& sound : Sounds)
    {
        if (sound.who == obj)
        {
            value = sound;
            return true;
        }
    }
    return false;
}

void CMonsterSoundMemory::remove_links(const IGameObject* O)
{
    // удаление устаревших звуков
    const auto it = std::remove_if(Sounds.begin(), Sounds.end(), [O](const SoundElem& x)
    {
        if (x.who == O)
            return true;

        return false;
    });
    Sounds.erase(it, Sounds.end());
}

//////////////////////////////////////////////////////////////////////////
// Help Sounds
//////////////////////////////////////////////////////////////////////////
bool CMonsterSoundMemory::hear_help_sound() const
{
    if ((m_time_help_sound == 0) || (m_time_help_sound + time_help_sound_remember < time()))
        return false;
    return true;
}

void CMonsterSoundMemory::check_help_sound(int eType, u32 node)
{
    if ((eType & SOUND_TYPE_MONSTER_ATTACKING) != SOUND_TYPE_MONSTER_ATTACKING)
        return;
    if ((eType & SOUND_TYPE_MONSTER_INJURING) != SOUND_TYPE_MONSTER_INJURING)
        return;
    if ((eType & SOUND_TYPE_MONSTER_DYING) != SOUND_TYPE_MONSTER_DYING)
        return;

    if (m_time_help_sound)
        return;

    m_time_help_sound = time();
    m_help_node = node;
}
