#pragma once

#include "Include/xrRender/KinematicsAnimated.h"
#include "xrEngine/CameraManager.h"

typedef u32 TTime;

#define COLOR color_xrgb
#define COLOR_RED COLOR(255, 0, 0)
#define COLOR_GREEN COLOR(0, 255, 0)
#define COLOR_BLUE COLOR(0, 0, 255)

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

#define AA_FLAG_ATTACK_RAT (1 << 0) // аттака крыс?
#define AA_FLAG_FIRE_ANYWAY (1 << 1) // трассировка не нужна

#define CRITICAL_STAND_TIME 1400
#define TIME_STAND_RECHECK 2000

#define HIT_SIDE_COUNT 2
#define HIT_BACK 0
#define HIT_FRONT 1

#define HIT_HEIGHT_COUNT 2
#define HIT_LOW 0
#define HIT_HIGH 1

// Enemy flags
#define FLAG_ENEMY_DIE (1 << 0)
#define FLAG_ENEMY_LOST_SIGHT (1 << 1)
#define FLAG_ENEMY_GO_CLOSER (1 << 2)
#define FLAG_ENEMY_GO_FARTHER (1 << 3)
#define FLAG_ENEMY_GO_CLOSER_FAST (1 << 4)
#define FLAG_ENEMY_GO_FARTHER_FAST (1 << 5)
#define FLAG_ENEMY_STANDING (1 << 6)
#define FLAG_ENEMY_HIDING (1 << 7)
#define FLAG_ENEMY_RUN_AWAY (1 << 8)
#define FLAG_ENEMY_DOESNT_KNOW_ABOUT_ME (1 << 9)
#define FLAG_ENEMY_GO_OFFLINE (1 << 10)
#define FLAG_ENEMY_DOESNT_SEE_ME (1 << 11)
#define FLAG_ENEMY_STATS_NOT_READY (1 << 12)

#define SOUND_ATTACK_HIT_MIN_DELAY 1000
#define MORALE_NORMAL 0.5f

#define STANDART_ATTACK -PI_DIV_6, PI_DIV_6, -PI_DIV_6, PI_DIV_6, 3.5f
#define SIMPLE_ENEMY_HIT_TEST

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
        float linear;
        float angular_path;
        float angular_real;
    } velocity;
    float min_factor;
    float max_factor;

    SVelocityParam()
    {
        velocity.linear = 0.f;
        velocity.angular_real = 0.f;
        velocity.angular_path = 0.f;
        min_factor = 1.0f;
        max_factor = 1.0f;
    }

    void Load(LPCSTR section, LPCSTR line)
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
enum EMotionAnim
{
    eAnimStandIdle = u32(0),
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
    eAnimUndefined = u32(-1)
};

// Generic actions
enum EAction
{
    ACT_STAND_IDLE = u32(0),
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
    ACT_NONE = u32(-1)
};

enum EPState
{
    PS_STAND,
    PS_SIT,
    PS_LIE,
    PS_STAND_UPPER
};

typedef shared_str anim_string;
#define DEFAULT_ANIM eAnimStandIdle

// элемент анимации
struct SAnimItem
{
    anim_string target_name; // "stand_idle_"
    int spec_id; // (-1) - any,  (0 - ...) - идентификатор 3
    u8 count; // количество анимаций : "idle_0", "idle_1", "idle_2"

    SVelocityParam velocity;

    EPState pos_state;

    struct
    {
        anim_string front;
        anim_string back;
        anim_string left;
        anim_string right;
    } fxs;
};

#define SKIP_IF_AGGRESSIVE true

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
typedef struct
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

} SAttackAnimation;

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

using AA_VECTOR = xr_vector<SAAParam>;

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
        IC void _set_current(float v)
        {
            current = v;
            VERIFY2(_abs(v) < 1000, "_set_current(). monster speed is too big");
        }
        IC void _set_target(float v)
        {
            target = v;
            VERIFY2(_abs(v) < 1000, "_set_target(). monster speed is too big");
            
        }
        IC float _get_current() { return current; }
        IC float _get_target() { return target; }

    private:
        float current;
        float target;
    } speed;

    void set_motion(EMotionAnim new_motion);
    EMotionAnim get_motion() const { return motion; }
};

//////////////////////////////////////////////////////////////////////////

struct t_fx_index
{
    s8 front;
    s8 back;
};

enum EHitSide
{
    eSideFront = u32(0),
    eSideBack,
    eSideLeft,
    eSideRight
};

using ANIM_ITEM_VECTOR = xr_vector<SAnimItem*>;
using TRANSITION_ANIM_VECTOR = xr_vector<STransition>;
using MOTION_ITEM_MAP = xr_map<EAction, SMotionItem>;
using SEQ_VECTOR = xr_vector<EMotionAnim>;
using ATTACK_ANIM = xr_vector<SAttackAnimation>;
using REPLACED_ANIM = xr_vector<SReplacedAnim>;

using FX_MAP_U16 = xr_map<u16, t_fx_index>;
using FX_MAP_STRING = xr_map<shared_str, t_fx_index>;

using VELOCITY_CHAIN_VEC = xr_vector<SEQ_VECTOR>;

struct SVelocity
{
    float current;
    float target;

    void set(float c, float t)
    {
        current = c;
        target = t;
    }
};

struct SMotionVel
{
    float linear;
    float angular;
    void set(float l, float a)
    {
        linear = l;
        angular = a;
    }
};

enum EAccelType
{
    eAT_Calm,
    eAT_Aggressive
};

enum EAccelValue
{
    eAV_Accel,
    eAV_Braking
};

#define deg(x) (x * PI / 180)

///////////////////////////////////////////////////////////////////////////////
// State Management
#define DO_ONCE_BEGIN(flag) \
    if (!flag)              \
    {                       \
        flag = true;
#define DO_ONCE_END() }

#define TIME_OUT(a, b) a + b < m_dwCurrentTime

#define DO_IN_TIME_INTERVAL_BEGIN(varLastTime, varTimeInterval) \
    if (TIME_OUT(varLastTime, varTimeInterval))                 \
    {                                                           \
        varLastTime = m_dwCurrentTime;
#define DO_IN_TIME_INTERVAL_END() }
///////////////////////////////////////////////////////////////////////////////

#define PATH_NEED_REBUILD() m_object->IsPathEnd(2, 0.5f)

// тип монстра (по количеству ног)
#define QUADRUPEDAL 4
#define BIPEDAL 2

struct SMonsterEnemy
{
    Fvector position;
    u32 vertex;
    TTime time;
    float danger;
};

class CEntityAlive;

using ENEMIES_MAP = xr_map<const CEntityAlive *, SMonsterEnemy>;
using ENEMIES_MAP_IT = ENEMIES_MAP::iterator;

struct SMonsterCorpse
{
    Fvector position;
    u32 vertex;
    TTime time;
};

using CORPSE_MAP = xr_map<const CEntityAlive *, SMonsterCorpse>;
using CORPSE_MAP_IT = CORPSE_MAP::iterator;

struct SMonsterHit
{
    IGameObject* object;
    TTime time;
    EHitSide side;
    Fvector position;

    bool operator==(const IGameObject* obj) { return (object == obj); }
};

using MONSTER_HIT_VECTOR = xr_vector<SMonsterHit>;

enum EDangerType
{
    eWeak,
    eNormal,
    eStrong,
    eVeryStrong,
    eNone
};

using ANIM_TO_MOTION_MAP = xr_map<MotionID, shared_str>;
