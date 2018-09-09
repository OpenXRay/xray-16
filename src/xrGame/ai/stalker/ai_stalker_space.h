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
};
