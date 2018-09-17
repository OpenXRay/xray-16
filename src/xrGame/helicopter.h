#pragma once

#include "ShootingObject.h"
#include "WeaponAmmo.h"
#include "RocketLauncher.h"
#include "Entity.h"
#include "PHSkeleton.h"
#include "hit_immunity.h"
#include "memory_manager.h"
#include "HudSound.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "PHDestroyable.h"
#include "Explosive.h"

class CScriptGameObject;
class CLAItem;
class CHelicopterMovManager;
class CHelicopter;

enum EHeliHuntState
{
    eEnemyNone,
    eEnemyPoint,
    eEnemyEntity
};
struct SHeliEnemy
{
    EHeliHuntState type;
    Fvector destEnemyPos;
    u16 destEnemyID;
    float fire_trail_length_curr;
    float fire_trail_length_des;
    bool bUseFireTrail;
    float fStartFireTime;
    void reinit();
    void Update();
    void save(NET_Packet& output_packet);
    void load(IReader& input_packet);
    void Load(LPCSTR section);
};

enum EHeliBodyState
{
    eBodyByPath,
    eBodyToPoint
};
struct SHeliBodyState
{
    CHelicopter* parent;
    EHeliBodyState type;
    // settings, constants
    float model_pitch_k;
    float model_bank_k;
    float model_angSpeedBank;
    float model_angSpeedPitch;

    // runtime params
    Fvector currBodyHPB;

    bool b_looking_at_point;
    Fvector looking_point;
    void reinit();
    void LookAtPoint(Fvector point, bool do_it);

    void save(NET_Packet& output_packet);
    void load(IReader& input_packet);
    void Load(LPCSTR section);
};

enum EHeilMovementState
{
    eMovNone,
    eMovToPoint,
    eMovPatrolPath,
    eMovRoundPath,
    eMovLanding,
    eMovTakeOff
};
struct SHeliMovementState
{
    struct STmpPt
    {
        Fvector point;
        float dir_h;
        STmpPt(const Fvector& p, const float h) : point(p), dir_h(h){};
    };
    ~SHeliMovementState();
    CHelicopter* parent;
    EHeilMovementState type;
    // specified path
    const CPatrolPath* currPatrolPath;
    const CPatrolPath::CVertex* currPatrolVertex;

    int patrol_begin_idx;
    shared_str patrol_path_name;
    bool need_to_del_path;
    float safe_altitude_add;
    float maxLinearSpeed;
    float LinearAcc_fw;
    float LinearAcc_bk;
    float isAdnAcc;

protected:
    float HeadingSpK, HeadingSpB;
    float PitchSpK, PitchSpB, AngSP, AngSH;
    float speedInDestPoint;
    void SetPointFlags(u32 idx, u32 new_flags);

public:
    float min_altitude;
    // runtime values
    Fvector desiredPoint;

    float curLinearSpeed;
    float curLinearAcc;

    Fvector currP;
    float currPathH;
    float currPathP;

    Fvector round_center;
    float round_radius;
    bool round_reverse;

    float onPointRangeDist;
    float GetSpeedInDestPoint();
    void SetSpeedInDestPoint(float);
    float GetAngSpeedPitch(float speed);
    float GetAngSpeedHeading(float speed);

    float GetSafeAltitude();
    void reinit();
    void Update();
    void UpdateMovToPoint();
    void UpdatePatrolPath();
    bool AlreadyOnPoint();
    void goByRoundPath(Fvector center, float radius, bool clockwise);
    float GetDistanceToDestPosition();
    void getPathAltitude(Fvector& point, float base_altitude);
    void SetDestPosition(Fvector* pos);
    void goPatrolByPatrolPath(LPCSTR path_name, int start_idx);
    void CreateRoundPoints(Fvector center, float radius, float start_h, float end_h, xr_vector<STmpPt>& round_points);
    void save(NET_Packet& output_packet);
    void load(IReader& input_packet);
    void Load(LPCSTR section);
    void net_Destroy();
};

class CHelicopter : public CEntity,
                    public CShootingObject,
                    public CRocketLauncher,
                    public CPHSkeleton,
                    public CPHDestroyable,
                    public CHitImmunity,
                    public CExplosive
#ifdef DEBUG
                    ,
                    public pureRender
#endif

{
    typedef CEntity inherited;

public:
    enum EHeliState
    {
        eAlive = u32(0),
        eDead,
        eForce = u32(-1)
    };

    // heli weapons
    bool m_use_rocket_on_attack;
    bool m_use_mgun_on_attack;
    float m_min_rocket_dist, m_max_rocket_dist;
    float m_min_mgun_dist, m_max_mgun_dist;
    u32 m_time_between_rocket_attack;
    bool m_syncronize_rocket;
    float m_barrel_dir_tolerance;
    HUD_SOUND_ITEM m_sndShot;
    HUD_SOUND_ITEM m_sndShotRocket;

    Fvector m_fire_dir, m_fire_pos;

    u16 m_left_rocket_bone, m_right_rocket_bone, m_fire_bone, m_rotate_x_bone, m_rotate_y_bone;

    Fmatrix m_fire_bone_xform;
    Fmatrix m_i_bind_x_xform, m_i_bind_y_xform;
    Fvector2 m_lim_x_rot, m_lim_y_rot;
    Fvector2 m_tgt_rot;
    Fvector2 m_cur_rot;
    Fvector2 m_bind_rot;
    Fvector m_bind_x, m_bind_y;
    bool m_allow_fire;
    u16 m_last_launched_rocket;
    u32 m_last_rocket_attack;

    shared_str m_sAmmoType, m_sRocketSection;
    CCartridge m_CurrentAmmo;
    float delta_t;
    float flag_by_fire;
    Fmatrix m_left_rocket_bone_xform, m_right_rocket_bone_xform;

    static void BoneMGunCallbackX(CBoneInstance* B);
    static void BoneMGunCallbackY(CBoneInstance* B);
    void startRocket(u16 idx);

    // CShootingObject
    virtual const Fmatrix& ParticlesXFORM() const { return m_fire_bone_xform; };
    virtual const Fvector& CurrentFirePoint() { return m_fire_pos; };
    void MGunFireStart();
    void MGunFireEnd();
    void MGunUpdateFire();
    virtual void OnShot();

    void UpdateMGunDir();
    void UpdateWeapons();

    bool m_flame_started;
    bool m_light_started;
    bool m_ready_explode;
    bool m_exploded;
    bool m_dead;

protected:
    SHeliEnemy m_enemy;
    SHeliBodyState m_body;
    SHeliMovementState m_movement;

    // on death...
    Fvector m_death_ang_vel;
    float m_death_lin_vel_k;
    shared_str m_death_bones_to_hide;

    virtual bool IsHudModeNow() { return false; };
    //////////////////////////////////////////////////

    // sound, light, particles...
    ref_sound m_engineSound;
    ref_sound m_brokenSound;
    //	ref_sound						m_explodeSound;
    ref_light m_light_render;
    CLAItem* m_lanim;
    u16 m_light_bone, m_smoke_bone;
    float m_light_range, m_light_brightness;
    Fcolor m_light_color;
    shared_str m_smoke_particle;
    CParticlesObject* m_pParticle;
    Fmatrix m_particleXFORM;

public:
    void StartFlame();

protected:
    void UpdateHeliParticles();

public:
    void DieHelicopter();
    void TurnLighting(bool bOn);
    void TurnEngineSound(bool bOn);

protected:
    // explosive
    virtual void OnAfterExplosion(){};
    virtual void GetRayExplosionSourcePos(Fvector& pos) { random_point_in_object_box(pos, this); }
    virtual void ActivateExplosionBox(const Fvector& size, Fvector& in_out_pos){};
    // general
    EHeliState m_curState;

    xr_map<s16, float> m_hitBones;
    typedef xr_map<s16, float>::iterator bonesIt;
    float m_stepRemains;

    void UpdateState();

public:
    void ExplodeHelicopter();

    CHelicopter();
    virtual ~CHelicopter();

    CHelicopter::EHeliState state() { return m_curState; };
    int state_script() { return m_curState; };
    void setState(CHelicopter::EHeliState s);
    void setState_script(u32 s) { setState((CHelicopter::EHeliState)s); };
    void init();
    virtual void reinit();

    virtual void Load(LPCSTR section);
    virtual void reload(LPCSTR section);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P){};
    virtual void net_Import(NET_Packet& P){};
    virtual void net_Relcase(IGameObject* O);
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    virtual void SpawnInitPhysics(CSE_Abstract* D);
    virtual CPhysicsShellHolder* PPhysicsShellHolder() { return PhysicsShellHolder(); }
    virtual void net_Save(NET_Packet& P);
    virtual BOOL net_SaveRelevant()
    {
        return (inherited::net_SaveRelevant() && BOOL(PPhysicsShell() != NULL)) || m_exploded;
    };

    virtual void renderable_Render() { inherited::renderable_Render(); };
    virtual BOOL renderable_ShadowGenerate() { return FALSE; }
    virtual BOOL renderable_ShadowReceive() { return TRUE; }
    virtual void OnEvent(NET_Packet& P, u16 type);
    virtual void UpdateCL();
    virtual void shedule_Update(u32 time_delta);
    void MoveStep();

    virtual void Hit(SHit* pHDS);
    virtual void PHHit(SHit& H);
    // CEntity
    virtual void HitSignal(float P, Fvector& local_dir, IGameObject* who, s16 element) { ; }
    virtual void HitImpulse(float P, Fvector& vWorldDir, Fvector& vLocalDir) { ; }
    virtual const Fmatrix& get_ParticlesXFORM();
    virtual const Fvector& get_CurrentFirePoint();

    virtual CGameObject* cast_game_object() { return this; }
    virtual CExplosive* cast_explosive() { return this; }
    virtual CPHSkeleton* PHSkeleton() { return this; }
public:
    // for scripting
    bool isVisible(CScriptGameObject* O);
    bool isObjectVisible(IGameObject* O);
    bool isOnAttack() { return m_enemy.type != eEnemyNone; }
    void goPatrolByPatrolPath(LPCSTR path_name, int start_idx);
    void goByRoundPath(Fvector center, float radius, bool clockwise);
    void LookAtPoint(Fvector point, bool do_it);
    void SetDestPosition(Fvector* pos);
    float GetDistanceToDestPosition();

    void SetSpeedInDestPoint(float sp);
    float GetSpeedInDestPoint(float sp);

    void SetOnPointRangeDist(float d);
    float GetOnPointRangeDist();

    float GetCurrVelocity();
    float GetMaxVelocity();
    void SetMaxVelocity(float v);
    //////////////////////Start By JoHnY///////////////////////
    void SetLinearAcc(float LAcc_fw, float LAcc_bw);
    //////////////////////End By JoHnY/////////////////////////
    Fvector GetCurrVelocityVec();
    void SetBarrelDirTolerance(float val) { m_barrel_dir_tolerance = val; };
    void SetEnemy(CScriptGameObject* e);
    void SetEnemy(Fvector* pos);
    void UnSetEnemy();
    void SetFireTrailLength(float val);
    bool UseFireTrail();
    void UseFireTrail(bool val);

    float GetRealAltitude();

    int GetMovementState();
    int GetHuntState();
    int GetBodyState();

    virtual IFactoryObject* _construct();
    float GetSafeAltitude() { return m_movement.GetSafeAltitude(); };
    float GetHeliHealth() const { return inherited::GetfHealth(); }
    float SetHeliHealth(float value) { return inherited::SetfHealth(value); }
#ifdef DEBUG
public:
    virtual void OnRender();
#endif
};
