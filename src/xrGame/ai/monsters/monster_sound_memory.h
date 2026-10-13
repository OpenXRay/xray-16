#pragma once

#include "ai_monster_defs.h"
#include "xrServerEntities/ai_sounds.h"

class CBaseMonster;

typedef enum {
    WEAPON_SHOOTING = 0,
    MONSTER_ATTACKING,
    WEAPON_BULLET_RICOCHET,
    WEAPON_RECHARGING,

    WEAPON_TAKING,
    WEAPON_HIDING,
    WEAPON_CHANGING,
    WEAPON_EMPTY_CLICKING,

    MONSTER_DYING,
    MONSTER_INJURING,
    MONSTER_WALKING,
    MONSTER_JUMPING,
    MONSTER_FALLING,
    MONSTER_TALKING,

    DOOR_OPENING,
    DOOR_CLOSING,
    OBJECT_BREAKING,
    OBJECT_FALLING,
    NONE_DANGEROUS_SOUND
} TSoundDangerValue;

#define FACTOR_SOUND_TYPE int(8)
#define FACTOR_DISTANCE int(1)
#define FACTOR_DELTA_TIME int(2)
#define FACTOR_SOUND_POWER int(50)

struct SoundElem
{
    const IGameObject* who{};
    TSoundDangerValue type{ NONE_DANGEROUS_SOUND };
    Fvector position{}; // позиция звука, не объекта, издавшего звук
    float power{};
    TTime time{}; // время обнаружения звука

    int value{}; // оценочное значение данного звука

    SoundElem() = default;

    SoundElem(const IGameObject* who, const int eType, const Fvector& position, const float power, const TTime time)
        : who(who), type(ConvertSoundType((ESoundTypes)eType)), position(position), power(power), time(time){}

    void CalcValue(const TTime cur_time, const Fvector& cur_pos)
    {
        value = FACTOR_SOUND_TYPE * u32(NONE_DANGEROUS_SOUND - WEAPON_SHOOTING) -
            iFloor(FACTOR_DISTANCE * cur_pos.distance_to(position)) -
            FACTOR_DELTA_TIME * iFloor(float((cur_time - time) / 1000)) + FACTOR_SOUND_POWER * iFloor(power);
    }

    [[nodiscard]]
    bool operator<(const SoundElem& s) const { return value < s.value; }

    static TSoundDangerValue ConvertSoundType(ESoundTypes stype);
};

class CMonsterSoundMemory
{
    TTime time_memory{}; // время хранения звуков
    xr_vector<SoundElem> Sounds;

    CBaseMonster* monster{};

    u32 m_time_help_sound{};
    u32 m_help_node{ u32(-1) };

public:
    CMonsterSoundMemory();

    void init_external(CBaseMonster* m, const TTime mem_time)
    {
        monster = m;
        time_memory = mem_time;
    }

    void HearSound(const SoundElem& s);
    void HearSound(const IGameObject* who, int eType, const Fvector& Position, float power, TTime time);

    [[nodiscard]]
    bool IsRememberSound() const { return !Sounds.empty(); }

    // Lain: added
    [[nodiscard]]
    auto GetNumSounds() const { return Sounds.size(); }
    void GetFirstSound(SoundElem& s, bool& bDangerous) const;

    [[nodiscard]]
    SoundElem& GetSound();
    void GetSound(SoundElem& s, bool& bDangerous); // возвращает самый опасный звук
    bool get_sound_from_object(const IGameObject* obj, SoundElem& value) const;

    void UpdateHearing();

    [[nodiscard]]
    bool is_loud_sound(float val) const;

    void clear() { Sounds.clear(); }
    void remove_links(const IGameObject* O);

    // help sounds
    [[nodiscard]]
    bool hear_help_sound() const;

    [[nodiscard]]
    u32 hear_help_sound_node() const
    {
        VERIFY(m_help_node != u32(-1));
        return m_help_node;
    }

    void check_help_sound(int eType, u32 node);
};
