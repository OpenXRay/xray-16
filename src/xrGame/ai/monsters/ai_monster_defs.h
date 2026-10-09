#pragma once

#include "Include/xrRender/KinematicsAnimated.h"
#include "xrEngine/CameraManager.h"

using TTime = u32;

constexpr auto COLOR_RED   = color_xrgb(255, 0, 0);
constexpr auto COLOR_GREEN = color_xrgb(0, 255, 0);
constexpr auto COLOR_BLUE  = color_xrgb(0, 0, 255);

class CBlend;

// специальные параметры анимаций (animation spec params)
#define ASP_MOVE_BKWD (1 << 0)
#define ASP_DRAG_CORPSE (1 << 1)
#define ASP_CHECK_CORPSE (1 << 2)
#define ASP_ATTACK_RAT (1 << 3)
#define ASP_ATTACK_RAT_JUMP (1 << 4)
#define ASP_STAND_SCARED (1 << 5)
#define ASP_THREATEN (1 << 6)
#define ASP_BACK_ATTACK (1 << 7)
#define ASP_ROTATION_JUMP (1 << 8)
#define ASP_ROTATION_RUN_LEFT (1 << 9)
#define ASP_ROTATION_RUN_RIGHT (1 << 10)
#define ASP_ATTACK_RUN (1 << 11)
#define ASP_PSI_ATTACK (1 << 12)
#define ASP_UPPER_STATE (1 << 13)
#define ASP_MOVE_SMELLING (1 << 14)

enum EnemyFlags : u16
{
    FLAG_ENEMY_DIE                  = 1 << 0,
    FLAG_ENEMY_LOST_SIGHT           = 1 << 1,
    FLAG_ENEMY_GO_CLOSER            = 1 << 2,
    FLAG_ENEMY_GO_FARTHER           = 1 << 3,
    FLAG_ENEMY_GO_CLOSER_FAST       = 1 << 4,
    FLAG_ENEMY_GO_FARTHER_FAST      = 1 << 5,
    FLAG_ENEMY_STANDING             = 1 << 6,
    FLAG_ENEMY_HIDING               = 1 << 7,
    FLAG_ENEMY_RUN_AWAY             = 1 << 8,
    FLAG_ENEMY_DOESNT_KNOW_ABOUT_ME = 1 << 9,
    FLAG_ENEMY_GO_OFFLINE           = 1 << 10,
    FLAG_ENEMY_DOESNT_SEE_ME        = 1 << 11,
    FLAG_ENEMY_STATS_NOT_READY      = 1 << 12,
};

// StepSounds
struct SStepSound
{
    float vol;
    float freq;
};

struct SAttackEffector
{
    SPPInfo ppi;
    float time;
    float time_attack;
    float time_release;

    // camera effects
    float ce_time;
    float ce_amplitude;
    float ce_period_number;
    float ce_power;
};

struct SVelocityParam
{
    struct
    {
        float linear{};
        float angular_path{};
        float angular_real{};
    } velocity{};
    float min_factor{ 1.0f };
    float max_factor{ 1.0f };

    void Load(pcstr section, pcstr line)
    {
        string32 buffer;
        velocity.linear = float(atof(_GetItem(pSettings->r_string(section, line), 0, buffer)));
        velocity.angular_real = float(atof(_GetItem(pSettings->r_string(section, line), 1, buffer)));
        velocity.angular_path = float(atof(_GetItem(pSettings->r_string(section, line), 2, buffer)));
        min_factor = float(atof(_GetItem(pSettings->r_string(section, line), 3, buffer)));
        max_factor = float(atof(_GetItem(pSettings->r_string(section, line), 4, buffer)));
    }
};

// Activities
enum EMotionAnim : u8
{
    eAnimStandIdle,
    eAnimCapturePrepare,
    eAnimStandTurnLeft,
    eAnimStandTurnRight,

    eAnimSitIdle,
    eAnimLieIdle,

    eAnimSitToSleep,
    eAnimLieToSleep,
    eAnimStandSitDown,
    eAnimStandLieDown,
    eAnimLieStandUp,
    eAnimSitStandUp,
    eAnimStandLieDownEat,
    eAnimSitLieDown,
    eAnimLieSitUp,
    eAnimSleepStandUp,

    eAnimWalkFwd,
    eAnimWalkBkwd,
    eAnimWalkTurnLeft,
    eAnimWalkTurnRight,

    eAnimRun,
    eAnimRunTurnLeft,
    eAnimRunTurnRight,
    eAnimFastTurn,

    eAnimAttack,
    eAnimAttackFromBack,
    eAnimAttackRun,

    eAnimEat,
    eAnimSleep,
    eAnimSleepStanding,
    eAnimDie,

    eAnimDragCorpse,
    eAnimCheckCorpse,
    eAnimScared,
    eAnimAttackJump,

    eAnimLookAround,

    eAnimPrepareAttack,
    eAnimJump,
    eAnimSteal,

    eAnimJumpStart,
    eAnimJumpGlide,
    eAnimJumpFinish,

    eAnimJumpLeft,
    eAnimJumpRight,

    eAnimStandDamaged,
    eAnimWalkDamaged,
    eAnimRunDamaged,

    eAnimSniff,
    eAnimHowling,
    eAnimThreaten,

    eAnimMiscAction_00,
    eAnimMiscAction_01,

    eAnimUpperStandIdle,
    eAnimUpperStandTurnLeft,
    eAnimUpperStandTurnRight,

    eAnimStandToUpperStand,
    eAnimUppperStandToStand,

    eAnimUpperWalkFwd,
    eAnimUpperThreaten,
    eAnimUpperAttack,

    eAnimAttackPsi,

    eAnimTeleRaise,
    eAnimTeleFire,
    eAnimGraviPrepare,
    eAnimGraviFire,
    eAnimShieldStart,
    eAnimShieldContinue,

    eAnimTelekinesis,

    // mob home animations
    /*eAnimHomeIdleDigGround,
    eAnimHomeIdleHowl,
    eAnimHomeIdleShake,
    eAnimHomeIdleSmellingUp,
    eAnimHomeIdleSmellingDown,
    eAnimHomeIdleSmellingLookAround,
    eAnimHomeIdleGrowl,*/
    eAnimHomeWalkGrowl,
    eAnimHomeWalkSmelling,

    // end mob home animations

    eAnimAttackOnRunLeft,
    eAnimAttackOnRunRight,

    eAnimAntiAimAbility,
    eAnimFastStandTurnLeft,
    eAnimFastStandTurnRight,

    eAnimCount,
    eAnimUndefined = u8(-1),
};

// Generic actions
enum EAction : u8
{
    ACT_STAND_IDLE,
    ACT_SIT_IDLE,
    ACT_LIE_IDLE,
    ACT_WALK_FWD,
    ACT_WALK_BKWD,
    ACT_RUN,
    ACT_CAPTURE_PREPARE,
    ACT_EAT,
    ACT_SLEEP,
    ACT_REST,
    ACT_DRAG,
    ACT_ATTACK,
    ACT_STEAL,
    ACT_LOOK_AROUND,
    ACT_HOME_WALK_GROWL,
    ACT_HOME_WALK_SMELLING,
    ACT_NONE = u8(-1),
};

enum EPState : u8
{
    PS_STAND,
    PS_SIT,
    PS_LIE,
    PS_STAND_UPPER
};

enum EHitSide : u8
{
    eSideFront,
    eSideBack,
    eSideLeft,
    eSideRight,
    eSideCount
};

using anim_string = shared_str;

constexpr auto DEFAULT_ANIM = eAnimStandIdle;

// элемент анимации
struct SAnimItem
{
    anim_string target_name; // "stand_idle_"

    s8 spec_id; // (-1) - any,  (0 - 127) - идентификатор 3
    u8 count; // количество анимаций : "idle_0", "idle_1", "idle_2"
    EPState pos_state;

    SVelocityParam velocity;

    struct Effects
    {
        anim_string front;
        anim_string back;
        anim_string left;
        anim_string right;
    } fxs;
};

constexpr bool SKIP_IF_AGGRESSIVE = true;

// описание перехода
struct STransition
{
    struct
    {
        bool state_used;
        EMotionAnim anim;
        EPState state;
    } from, target;

    EMotionAnim anim_transition;
    bool chain;
    bool skip_if_aggressive;
};

// элемент движения
struct SMotionItem
{
    EMotionAnim anim;
    bool is_turn_params;

    struct
    {
        EMotionAnim anim_left; // speed, r_speed got from turn_left member
        EMotionAnim anim_right;
        float min_angle;
    } turn;
};

// подмена анимаций (если *flag == true, то необходимо заменить анимацию)
struct SReplacedAnim
{
    EMotionAnim cur_anim;
    EMotionAnim new_anim;
    bool* flag;
};

// Определение времени аттаки по анимации
struct SAttackAnimation
{
    EMotionAnim anim; // параметры конкретной анимации
    u32 anim_i3;

    TTime time_from; // диапазон времени когда можно наносить hit (от)
    TTime time_to; // диапазон времени когда можно наносить hit (до)

    Fvector trace_from; // направление трассировки (относительно центра)
    Fvector trace_to;

    u32 flags; // специальные флаги

    float damage; // урон при данной атаке
    Fvector hit_dir; // угол направления приложения силы к объекту

    //-----------------------------------------
    // temp
    float yaw_from;
    float yaw_to;
    float pitch_from;
    float pitch_to;
    float dist;
};

struct SAAParam
{
    MotionID motion;
    float time;
    float hit_power; // damage
    float impulse;
    Fvector impulse_dir;

    // field of hit
    struct
    {
        float from_yaw;
        float to_yaw;
        float from_pitch;
        float to_pitch;
    } foh;

    float dist;
};

struct SCurrentAnimationInfo
{
    u8 index;
    TTime time_started;
    float speed_change_vel;

private:
    EMotionAnim motion;

public:
    shared_str name;
    CBlend* blend;

    struct
    {
        void _set_current(float v)
        {
            current = v;
            VERIFY2(_abs(v) < 1000, "_set_current(). monster speed is too big");
        }
        void _set_target(float v)
        {
            target = v;
            VERIFY2(_abs(v) < 1000, "_set_target(). monster speed is too big");

        }

        [[nodiscard]]
        float _get_current() const { return current; }

        [[nodiscard]]
        float _get_target() const { return target; }

    private:
        float current;
        float target;
    } speed;

    [[nodiscard]]
    auto get_motion() const { return motion; }
    void set_motion(const EMotionAnim new_motion) { motion = new_motion; }
};

//////////////////////////////////////////////////////////////////////////

struct t_fx_index
{
    s8 front;
    s8 back;
};

using SEQ_VECTOR = xr_vector<EMotionAnim>;

struct SMotionVel
{
    float linear;
    float angular;
};

enum EAccelType : u8
{
    eAT_Calm,
    eAT_Aggressive
};

enum EAccelValue : u8
{
    eAV_Accel,
    eAV_Braking
};

// XXX: Replace macro with constexpr function
#define deg(x) (x * PI / 180)

class IGameObject;

struct SMonsterEnemy
{
    Fvector position;
    u32 vertex;
    TTime time;
    float danger;
};

struct SMonsterCorpse
{
    Fvector position;
    u32 vertex;
    TTime time;
};

struct SMonsterHit
{
    IGameObject* object;
    Fvector position;
    TTime time;
    EHitSide side;

    bool operator==(const IGameObject* obj) const { return (object == obj); }
};

enum EDangerType : u8
{
    eWeak,
    eNormal,
    eStrong,
    eVeryStrong,
    eNone
};
