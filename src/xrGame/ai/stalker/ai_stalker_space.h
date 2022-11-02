////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_space.h
//	Created 	: 28.03.2003
//  Modified 	: 28.03.2003
//	Author		: Dmitriy Iassenev
//	Description : Stalker types and structures
////////////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_HEAD_TURN_ANGLE (1.f * PI_DIV_4)

namespace StalkerSpace
{
enum EStalkerSounds
{
    eStalkerSoundDie = u32(0),
    eStalkerSoundDieInAnomaly,
    eStalkerSoundInjuring,
    eStalkerSoundHumming,
    eStalkerSoundAlarm,
    eStalkerSoundAttackNoAllies,
    eStalkerSoundAttackAlliesSingleEnemy,
    eStalkerSoundAttackAlliesSeveralEnemies,
    eStalkerSoundBackup,
    eStalkerSoundDetour,
    eStalkerSoundSearch1WithAllies,
    eStalkerSoundSearch1NoAllies,
    eStalkerSoundEnemyLostNoAllies,
    eStalkerSoundEnemyLostWithAllies,
    eStalkerSoundInjuringByFriend,
    eStalkerSoundPanicHuman,
    eStalkerSoundPanicMonster,
    eStalkerSoundTolls,
    eStalkerSoundWounded,
    eStalkerSoundGrenadeAlarm,
    eStalkerSoundFriendlyGrenadeAlarm,
    eStalkerSoundNeedBackup,
    eStalkerSoundRunningInDanger,
    //		eStalkerSoundWalkingInDanger,
    eStalkerSoundKillWounded,
    eStalkerSoundEnemyCriticallyWounded,
    eStalkerSoundEnemyKilledOrWounded,
    eStalkerSoundThrowGrenade,

    eStalkerSoundScript,
    eStalkerSoundDummy = u32(-1),
};

enum EStalkerSoundMasks
{
    eStalkerSoundMaskAnySound = u32(0),
    eStalkerSoundMaskDie = u32(-1),
    eStalkerSoundMaskDieInAnomaly = u32(-1),
    eStalkerSoundMaskInjuring = u32(-1),
    eStalkerSoundMaskInjuringByFriend = u32(-1),
    eStalkerSoundMaskNonTriggered = u32(1 << 31) | (1 << 30),
    eStalkerSoundMaskNoHumming = (1 << 29),
    eStalkerSoundMaskFree = eStalkerSoundMaskNoHumming | eStalkerSoundMaskNonTriggered,
    eStalkerSoundMaskHumming = 1 | eStalkerSoundMaskFree,
    eStalkerSoundMaskNoDanger = (1 << 28),
    eStalkerSoundMaskDanger = eStalkerSoundMaskNoDanger | eStalkerSoundMaskNonTriggered,
    eStalkerSoundMaskAlarm = (1 << 0) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskAttackNoAllies = (1 << 1) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskAttackAlliesSingleEnemy = (1 << 2) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskAttackAlliesSeveralEnemies = (1 << 3) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskBackup = (1 << 4) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskDetour = (1 << 5) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskSearch1NoAllies = (1 << 6) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskSearch1WithAllies = (1 << 7) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskEnemyLostNoAllies = (1 << 8) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskEnemyLostWithAllies = (1 << 9) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskNeedBackup = (1 << 10) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskMovingInDanger = (1 << 11) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskKillWounded = (1 << 12) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskEnemyCriticallyWounded = (1 << 13) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskEnemyKilledOrWounded = (1 << 14) | eStalkerSoundMaskDanger,
    eStalkerSoundMaskPanicHuman = eStalkerSoundMaskDanger,
    eStalkerSoundMaskPanicMonster = eStalkerSoundMaskDanger,
    eStalkerSoundMaskTolls = eStalkerSoundMaskDanger,
    eStalkerSoundMaskWounded = eStalkerSoundMaskDanger,
    eStalkerSoundMaskGrenadeAlarm = eStalkerSoundMaskDanger,
    eStalkerSoundMaskFriendlyGrenadeAlarm = eStalkerSoundMaskDanger,
    eStalkerSoundMaskDummy = u32(-1),
};

enum EBodyAction : u32
{
    eBodyActionNone = u32(0),
    eBodyActionHello,
    eBodyActionDummy = u32(-1),
};

enum class WeaponTypes : u32
{
    Unknown,

    Item,
    Melee,

    // XXX: Better names, anyone...
    Mutant1,
    Mutant2,
    Mutant3,

    Pistol,
    SubmashineGun,
    Shotgun,
    MashineGun,
    SniperRifle,

    GrenadeLauncher,
    Grenade,

    AnomalyMine,
    AnomalyField,

    PsyStrike,     // Controller
    ThrowingItems, // Poltergeist, burer...

    Gravi,
    Mincer,
    BurningFuzz,
    RustyHair, // NoGravity anomaly also has this type, though..
};

constexpr WeaponTypes convert_weapon_type_cop(int type)
{
    switch (type)
    {
    case 0:  return WeaponTypes::Item;
    case 1:  return WeaponTypes::Melee;
    case 2:  return WeaponTypes::Mutant1;
    case 3:  return WeaponTypes::Mutant2;
    case 4:  return WeaponTypes::Grenade;
    case 5:  return WeaponTypes::Pistol;
    case 6:
    case 7:
    case 8:  return WeaponTypes::SubmashineGun;
    case 9:  return WeaponTypes::Shotgun;
    case 10: return WeaponTypes::MashineGun;
    case 11: return WeaponTypes::SniperRifle;
    case 12: return WeaponTypes::GrenadeLauncher;
    case 13: return WeaponTypes::AnomalyMine;
    case 14: return WeaponTypes::Mincer;
    case 15: return WeaponTypes::AnomalyField;
    //case 16: is not used in release gamedata
    case 17: return WeaponTypes::Gravi;
    case 18: return WeaponTypes::BurningFuzz;
    case 19: return WeaponTypes::RustyHair;
    }

    return WeaponTypes::Unknown;
}

constexpr WeaponTypes convert_weapon_type_soc_cs(int type)
{
    switch (type)
    {
    case 0:  return WeaponTypes::Item;
    case 1:  return WeaponTypes::Melee;
    case 2:  return WeaponTypes::Mutant1;
    case 3:  return WeaponTypes::Mutant2;
    case 4:  return WeaponTypes::Mutant3;
    case 5:  return WeaponTypes::Pistol;
    case 6:  return WeaponTypes::SubmashineGun;
    case 7:  return WeaponTypes::Shotgun;
    case 8:  return WeaponTypes::SniperRifle;
    case 9:  return WeaponTypes::GrenadeLauncher;
    case 10: return WeaponTypes::Grenade;
    case 11: return WeaponTypes::PsyStrike;
    case 12: return WeaponTypes::ThrowingItems;
    case 13: return WeaponTypes::AnomalyMine;
    case 14: return WeaponTypes::Mincer;
    case 15: return WeaponTypes::AnomalyField;
    //case 16: is not used in release gamedata
    case 17: return WeaponTypes::Gravi;
    case 18: return WeaponTypes::BurningFuzz;
    case 19: return WeaponTypes::RustyHair;
    }

    return WeaponTypes::Unknown;
}

inline WeaponTypes convert_weapon_type(int type)
{
    if (ShadowOfChernobylMode || ClearSkyMode)
        return convert_weapon_type_soc_cs(type);

    return convert_weapon_type_cop(type);
}
};
