#pragma once
#include "PHSynchronize.h"
#include "xrServer_Space.h"
#include "xrCommon/misc_math_types.h"

#define ACTOR_HEIGHT 1.75f
#define ACTOR_LOOKOUT_SPEED 2.f

namespace ACTOR_DEFS
{
enum ESoundCcount
{
    //	SND_HIT_COUNT=8,
    SND_DIE_COUNT = 4
};

enum EActorCameras
{
    eacFirstEye = 0,
    eacLookAt,
    eacFreeLook,
    eacFixedLookAt,
    eacMaxCam
};
enum EDamages
{
    DAMAGE_FX_COUNT = 12
};

enum EMoveCommand
{
    mcFwd = (1ul << 0ul),
    mcBack = (1ul << 1ul),
    mcLStrafe = (1ul << 2ul),
    mcRStrafe = (1ul << 3ul),
    mcCrouch = (1ul << 4ul),
    mcAccel = (1ul << 5ul),
    mcTurn = (1ul << 6ul),
    mcJump = (1ul << 7ul),
    mcFall = (1ul << 8ul),
    mcLanding = (1ul << 9ul),
    mcLanding2 = (1ul << 10ul),
    mcClimb = (1ul << 11ul),
    mcSprint = (1ul << 12ul),
    mcLLookout = (1ul << 13ul),
    mcRLookout = (1ul << 14ul),
    mcAnyMove = (mcFwd | mcBack | mcLStrafe | mcRStrafe),
    mcAnyAction = (mcAnyMove | mcJump | mcFall | mcLanding | mcLanding2), // mcTurn|
    mcAnyState = (mcCrouch | mcAccel | mcClimb | mcSprint),
    mcLookout = (mcLLookout | mcRLookout),
};

// enum для определения действия над вещью на которую наведен в текущее время прицел.
// Используется для показа всплывающих динамических подсказок
enum EActorAction
{
    eaaNoAction = 0,
    eaaPickup,
    eaaTalk,
    eaaOpenDoor,
    eaaSearchCorpse,
};

typedef const char* EActorSleep;
extern EActorSleep easCanSleepResult;
extern string32 g_quick_use_slots[4];

/*
//результат функции GoSleep у актера
enum EActorSleep
{
    easCanSleep			= 0,
    easNotSolidGround,
    easEnemies
};
*/

//---------------------------------------------
// ввод с клавиатуры и мыши
struct net_input
{
    u32 m_dwTimeStamp;

    u32 mstate_wishful;

    u8 cam_mode;
    float cam_yaw;
    float cam_pitch;
    float cam_roll;

    bool operator<(const u32 Time) { return m_dwTimeStamp < Time; };
};

//------------------------------
struct net_update
{
    u32 dwTimeStamp; // server(game) timestamp
    float o_model; // model yaw
    SRotation o_torso; // torso in world coords
    Fvector p_pos; // in world coords
    Fvector p_accel; // in world coords
    Fvector p_velocity; // in world coords
    u32 mstate;
    int weapon;
    float fHealth;
    //	float				fArmor;

    net_update() : o_model(0), mstate(0), weapon(0), fHealth(0)
    {
        dwTimeStamp = 0;
        p_pos.set(0, 0, 0);
        p_accel.set(0, 0, 0);
        p_velocity.set(0, 0, 0);
    }

    void lerp(net_update& A, net_update& B, float f);
};

///////////////////////////////////////////////////////
// апдайт с данными физики
struct net_update_A
{
    u32 dwTimeStamp;
    //	u32					dwTime0;
    //	u32					dwTime1;
    SPHNetState State;
};

///////////////////////////////////////////////////////
// данные для интерполяции
struct InterpData
{
    Fvector Pos;
    Fvector Vel;
    float o_model; // model yaw
    SRotation o_torso; // torso in world coords
};

}; // namespace ACTOR_DEFS
