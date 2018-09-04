#pragma once

//#if 0

#include "entity.h"

#include "xrPhysics/PhysicsShell.h"
#include "xrPhysics/phupdateobject.h"
#include "script_entity.h"
#include "CarLights.h"

#include "holder_custom.h"
#include "PHSkeleton.h"
#include "DamagableItem.h"
#include "phcollisiondamagereceiver.h"
#include "CarDamageParticles.h"
#include "xrserver_objects_alife.h"
#include "CarDamageParticles.h"
#include "hit_immunity.h"
#include "Explosive.h"
#include "PHDestroyable.h"
#include "DelayedActionFuse.h"
// refs
class ENGINE_API CBoneInstance;
class CActor;
class CInventory;
class CSE_PHSkeleton;
class CCarWeapon;
struct dxGeomUserData;
struct dSurfaceParameters;
// defs

#ifdef DEBUG
#include "xrEngine/StatGraph.h"
#include "PHDebug.h"
#endif

class CScriptEntityAction;
class car_memory;

class CCar : public CEntity,
             public CScriptEntity,
             public CPHUpdateObject,
             public CHolderCustom,
             public CPHSkeleton,
             public CDamagableItem,
             public CPHDestroyable,
             public CPHCollisionDamageReceiver,
             public CHitImmunity,
             public CExplosive,
             public CDelayedActionFuse
{
private:
    collide::rq_results RQR;

#ifdef DEBUG
    CFunctionGraph m_dbg_power_rpm;
    CFunctionGraph m_dbg_torque_rpm;
    CStatGraph* m_dbg_dynamic_plot;
    bool b_plots;
    float _stdcall TorqueRpmFun(float rpm) { return Parabola(rpm) / rpm; }
    void InitDebug();
    void DbgSheduleUpdate();
    void DbgUbdateCl();
    void DbgCreatePlots();
    void DBgClearPlots();
#endif
    ////////////////////////////////////////////////////////////////////
    Flags16 async_calls;
    static const u16 cAsCallsnum = 3;
    enum EAsyncCalls
    {
        ascSndTransmission = 1 << 0,
        ascSndStall = 1 << 1,
        ascExhoustStop = 1 << 2,
        ascLast = 1 << cAsCallsnum
    };
    void ASCUpdate();
    void ASCUpdate(EAsyncCalls c);
    void AscCall(EAsyncCalls c);
    ////////////////////////////////////////////////////////////////////////////////////////
    virtual bool CanRemoveObject();
    ////////////////////////////////////////////////////////////////////////
    static BONE_P_MAP bone_map; // interface for PhysicsShell
    static void ActorObstacleCallback(
        bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2);
    virtual void PhDataUpdate(float step);
    virtual void PhTune(float step);
    /////////////////////////////////////////////////////////////////////////
    virtual void ApplyDamage(u16 level);
    virtual float Health() { return GetfHealth(); }
    virtual void ChangeCondition(float fDeltaCondition);
    virtual void StartTimerEffects(){};
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CPhysicsShellHolder* PPhysicsShellHolder() { return static_cast<CPhysicsShellHolder*>(this); }
    virtual ICollisionDamageReceiver* PHCollisionDamageReceiver() { return this; }
    ////////////////////////////////////////////////////////////////////////
    CCarDamageParticles m_damage_particles;
    ///////////////////////////////////////////////////////////////////////
protected:
    enum ECarCamType
    {
        ectFirst = 0,
        ectChase,
        ectFree
    };

public:
    bool rsp, lsp, fwp, bkp, brp;
    Fmatrix m_root_transform;
    Fvector m_exit_position;

    enum eStateDrive
    {
        drive,
        neutral
    };

    eStateDrive e_state_drive;

    enum eStateSteer
    {
        right,
        idle,
        left
    };

    eStateSteer e_state_steer;

    bool b_wheels_limited;
    bool b_engine_on;
    bool b_clutch;
    bool b_starting;
    bool b_stalling;
    bool b_breaks;
    bool b_transmission_switching;

    u32 m_dwStartTime;
    float m_fuel;
    float m_fuel_tank;
    float m_fuel_consumption;
    u16 m_driver_anim_type;

    float m_break_start;
    float m_break_time;
    float m_breaks_to_back_rate;
    float m_power_neutral_factor; // multiplier for power when accelerator is not pressed (0-1,0.25)
    bool b_exploded;

    struct SWheel : public CDamagableHealthItem
    {
        typedef CDamagableHealthItem inherited;
        u16 bone_id;
        bool inited;
        float radius;
        CPhysicsJoint* joint;
        CCar* car;
        struct SWheelCollisionParams
        {
            float spring_factor;
            float damping_factor;
            float mu_factor;
            SWheelCollisionParams();
        } collision_params;

        IC static void applywheelCollisionParams(
            const dxGeomUserData* ud, bool& do_colide, dContact& c, SGameMtl* material_1, SGameMtl* material_2);
        static void WheellCollisionCallback(
            bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2);

        void Init(); // asumptions: bone_map is 1. ini parsed 2. filled in 3. bone_id is set
        void Load(LPCSTR section);
        void RestoreNetState(const CSE_ALifeCar::SWheelState& a_state);
        void SaveNetState(NET_Packet& P);
        void ApplyDriveAxisVel(float vel);
        void ApplyDriveAxisTorque(float torque);
        void ApplyDriveAxisVelTorque(float vel, float torque);
        void ApplySteerAxisVel(float vel);
        void ApplySteerAxisTorque(float torque);
        void ApplySteerAxisVelTorque(float vel, float torque);
        void SetSteerLoLimit(float lo);
        void SetSteerHiLimit(float hi);
        void SetSteerLimits(float hi, float lo);

        virtual void ApplyDamage(u16 level);
        SWheel(CCar* acar) : radius(0)
        {
            bone_id = BI_NONE;
            car = acar;
            joint = NULL;
            inited = false;
        }
    };
    struct SWheelDrive
    {
        SWheel* pwheel;
        float pos_fvd;
        float gear_factor;
        void Init();
        void Drive();
        void Neutral();
        void UpdatePower();
        float ASpeed();
        void Load(LPCSTR /*section*/){};
    };
    struct SWheelSteer
    {
        SWheel* pwheel;
        float pos_right;
        float lo_limit;
        float hi_limit;
        float steering_velocity;
        float steering_torque;
        bool limited; // zero limited for idle steering drive
        float GetSteerAngle();

        void Init();
        void SteerRight();
        void SteerLeft();
        void SteerIdle();
        void Limit();
        void Load(LPCSTR /*section*/){};
    };
    struct SWheelBreak
    {
        SWheel* pwheel;
        float break_torque;
        float hand_break_torque;
        void Init();
        void Break(float k);
        void HandBreak();
        void Neutral();
        void Load(LPCSTR section);
    };

    struct SExhaust
    {
        u16 bone_id;
        Fmatrix transform;
        // Fvector				velocity;
        CParticlesObject* p_pgobject;
        CPhysicsElement* pelement;
        CCar* pcar;
        void Init();
        void Play();
        void Stop();
        void Update();
        void Clear();
        SExhaust(CCar* acar)
        {
            bone_id = BI_NONE;
            pcar = acar;
            p_pgobject = NULL;
            pelement = NULL;
        }
        ~SExhaust();
    };

    struct SDoor;
    struct SDoor : public CDamagableHealthItem
    {
        typedef CDamagableHealthItem inherited;
        u16 bone_id;
        CCar* pcar;
        bool update;
        CPhysicsJoint* joint;
        float torque;
        float a_vel;
        float pos_open;
        float opened_angle;
        float closed_angle;
        u32 open_time;
        struct SDoorway
        {
            Fvector2 door_plane_ext;
            _vector2<int> door_plane_axes;
            SDoor* door;
            SDoorway();
            void SPass();
            void Init(SDoor* adoor);
            void Trace(const Fvector& point, const Fvector& dir);
        };
        Fvector2 door_plane_ext;
        _vector2<int> door_plane_axes;
        Fvector door_dir_in_door;
        Fmatrix closed_door_form_in_object;
        void Use();
        void Switch();
        void Init();
        void Open();
        void Close();
        void Break();
        virtual void ApplyDamage(u16 level);
        void Update();
        float GetAngle();
        bool CanEnter(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos);
        bool IsInArea(const Fvector& pos, const Fvector& dir);
        bool IsFront(const Fvector& pos, const Fvector& dir);
        bool CanExit(const Fvector& pos, const Fvector& dir);
        bool TestPass(const Fvector& pos, const Fvector& dir);
        // bool TestPass1(const Fvector& pos,const Fvector& dir);
        void GetExitPosition(Fvector& pos);
        void ApplyOpenTorque();
        void ApplyTorque(float atorque, float aa_vel);
        void ApplyCloseTorque();
        void NeutralTorque(float atorque);
        void ClosingToClosed();
        void ClosedToOpening();
        void PlaceInUpdate();
        void RemoveFromUpdate();
        void SaveNetState(NET_Packet& P);
        void RestoreNetState(const CSE_ALifeCar::SDoorState& a_state);
        void SetDefaultNetState();
        enum eState
        {
            opening,
            closing,
            opened,
            closed,
            broken
        };
        eState state;
        SDoor(CCar* acar)
            : update(false), pos_open(0),
              opened_angle(0), closed_angle(0),
              open_time(0)
        {
            bone_id = BI_NONE;
            pcar = acar;
            joint = NULL;
            state = closed;
            torque = 500.f;
            a_vel = M_PI;
        }
    };

    struct SCarSound
    {
        ref_sound snd_engine;
        ref_sound snd_engine_start;
        ref_sound snd_engine_stop;
        ref_sound snd_transmission;

        enum ESoundState
        {
            sndOff,
            sndStalling,
            sndStoping,
            sndStarting,
            sndDrive
        } eCarSound;
        void Update();
        void UpdateStarting();
        void UpdateStoping();
        void UpdateStalling();
        void UpdateDrive();
        void SwitchState(ESoundState new_state);
        void SetSoundPosition(ref_sound& snd);
        void SwitchOff();
        void SwitchOn();
        void Init();
        void Destroy();
        void Start();
        void Stop();
        void Stall();
        void Drive();
        void TransmissionSwitch();

        SCarSound(CCar* car);
        ~SCarSound();
        Fvector relative_pos;
        float volume;
        u32 engine_start_delay; // snd_engine starts after engine_start_delay ms by snd_engine_start
        u32 time_state_start;
        CCar* pcar;
    } * m_car_sound;

private:
    typedef CEntity inherited;

private:
    CCarWeapon* m_car_weapon;
    float m_steer_angle;
    bool m_repairing;
    u16 m_bone_steer;
    CCameraBase* camera[3];
    CCameraBase* active_camera;

    Fvector m_camera_position;

    ////////////////////////////////////////////////////
    friend struct SWheel;
    friend struct SDoor;

    xr_map<u16, SWheel> m_wheels_map;
    xr_vector<SWheelDrive> m_driving_wheels;
    xr_vector<SWheelSteer> m_steering_wheels;
    xr_vector<SWheelBreak> m_breaking_wheels;
    xr_vector<SExhaust> m_exhausts;
    shared_str m_exhaust_particles;
    xr_map<u16, SDoor> m_doors;
    xr_vector<SDoor*> m_doors_update;
    xr_vector<Fvector> m_gear_ratious;
    xr_vector<Fmatrix> m_sits_transforms; // m_sits_transforms[0] - driver_place
    float m_current_gear_ratio;

    /////////////////////////////////////////////////////////////
    bool b_auto_switch_transmission;

    /////////////////////////////////////////////////////////////
    float m_doors_torque_factor;
    /////////////////////////////////////////////////////////////

    float m_max_power; // best rpm
    float m_power_increment_factor;
    float m_power_decrement_factor;
    float m_rpm_increment_factor;
    float m_rpm_decrement_factor;
    /////////////////////porabola
    float m_a, m_b, m_c;

    float m_current_engine_power;
    float m_current_rpm;

    float m_axle_friction;

    float m_fSaveMaxRPM;
    float m_max_rpm;
    float m_min_rpm;
    float m_power_rpm; // max power
    float m_torque_rpm; // max torque

    float m_steering_speed;
    float m_ref_radius;
    size_t m_current_transmission_num;
    ///////////////////////////////////////////////////
    CCarLights m_lights;
    ////////////////////////////////////////////////////
    /////////////////////////////////////////////////

 public:
    void InitParabola();
    float _stdcall Parabola(float rpm);
    // float GetSteerAngle();
    void LimitWheels();
    void Drive();
    void Starter();
    void StartEngine();
    void StopEngine();
    void Stall();
    void Clutch();
    void Unclutch();
    void SwitchEngine();
    void NeutralDrive();
    void UpdatePower();
    void ReleasePedals();
    void ResetKeys();

    ////////////////////////////////////////////////////////////////////////////
    float RefWheelMaxSpeed() const;
    float EngineCurTorque() const;
    float RefWheelCurTorque();
    float EnginePower();
    float EngineDriveSpeed();
    float DriveWheelsMeanAngleRate();
    IC float EngineRpmFromWheels() { return _abs(DriveWheelsMeanAngleRate() * m_current_gear_ratio); }
    /////////////////////////////////////////////////////////////////////////
    void SteerRight();
    void SteerLeft();
    void SteerIdle();
    void Transmission(size_t num);
    void CircleSwitchTransmission();
    void TransmissionUp();
    void TransmissionDown();
    IC size_t CurrentTransmission() { return m_current_transmission_num; }
    void PressRight();
    void PressLeft();
    void PressForward();
    void PressBack();
    void PressBreaks();

    void ReleaseRight();
    void ReleaseLeft();
    void ReleaseForward();
    void ReleaseBack();
    void ReleaseBreaks();
    void Revert();
    float EffectiveGravity();
    float AntiGravityAccel();
    float GravityFactorImpulse();
    void StartBreaking();
    void StopBreaking();
    void UpdateBack();

    void HandBreak();
    void ReleaseHandBreak();
    void DriveForward();
    void DriveBack();
    void ParseDefinitions();
    void CreateSkeleton(CSE_Abstract* po); // creates m_pPhysicsShell
    void Init();

    void PlayExhausts();
    void StopExhausts();
    void UpdateExhausts();
    void ClearExhausts();
    void UpdateFuel(float time_delta);
    float AddFuel(float ammount); // ammount - fuel to load, ret - fuel loaded
public:
    void CarExplode();
    /***** added by Ray Twitty (aka Shadows) START *****/
    float GetfFuel();
    void SetfFuel(float fuel);

    float GetfFuelTank();
    void SetfFuelTank(float fuel_tank);

    float GetfFuelConsumption();
    void SetfFuelConsumption	(float fuel_consumption);

    void ChangefFuel	(float fuel);
    void ChangefHealth(float health);

    void PlayDamageParticles()
    {
        m_damage_particles.Play1(this);
        m_damage_particles.Play2(this);
    }

    void StopDamageParticles	()
    {
        m_damage_particles.Stop1(this);
        m_damage_particles.Stop2(this);
    }

    bool isActiveEngine	();
    /***** added by Ray Twitty (aka Shadows) END *****/
  

private:
    void OnCameraChange(int type);

    bool HUDview() { return IsFocused(); }
    static void cb_Steer(CBoneInstance* B);
    virtual void Hit(SHit* pHDS);
    virtual void Die(IGameObject* who);
    virtual void PHHit(SHit& H);
    bool WheelHit(float P, s16 element, ALife::EHitType hit_type);
    bool DoorHit(float P, s16 element, ALife::EHitType hit_type);

public:
    virtual bool allowWeapon() const { return true; };
    virtual bool HUDView() const;
    virtual Fvector ExitPosition() { return m_exit_position; }
    virtual Fvector ExitVelocity();
    void GetVelocity(Fvector& vel) { m_pPhysicsShell->get_LinearVel(vel); }
    void cam_Update(float dt, float fov);
    void detach_Actor();
    bool attach_Actor(CGameObject* actor);
    bool is_Door(u16 id, xr_map<u16, SDoor>::iterator& i);
    bool is_Door(u16 id);
    bool DoorOpen(u16 id);
    bool DoorClose(u16 id);
    bool DoorUse(u16 id);
    bool DoorSwitch(u16 id);
    bool Enter(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos);
    bool Exit(const Fvector& pos, const Fvector& dir);
    bool Use(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos);
    u16 DriverAnimationType();
    // Core events
    virtual IFactoryObject* _construct();
    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void UpdateCL();
    virtual void UpdateEx(float fov); // called by owner

    virtual void shedule_Update(u32 dt);
    virtual void renderable_Render();
    virtual bool bfAssignMovement(CScriptEntityAction* tpEntityAction);
    virtual bool bfAssignObject(CScriptEntityAction* tpEntityAction);

    // Network
    virtual void net_Export(NET_Packet& P); // export to server
    virtual void net_Import(NET_Packet& P); // import from server
    virtual BOOL net_Relevant() { return getLocal(); }; // relevant for export to server
    virtual BOOL UsedAI_Locations();
    virtual void net_Relcase(IGameObject* O);
    // Input
    virtual void OnMouseMove(int x, int y);
    virtual void OnKeyboardPress(int dik);
    virtual void OnKeyboardRelease(int dik);
    virtual void OnKeyboardHold(int dik);
    virtual void vfProcessInputKey(int iCommand, bool bPressed);
    virtual void OnEvent(NET_Packet& P, u16 type);
    virtual void OnAfterExplosion();
    virtual void OnBeforeExplosion();
    virtual void GetRayExplosionSourcePos(Fvector& pos);
    virtual void ActivateExplosionBox(const Fvector& size, Fvector& in_out_pos){};
    virtual void ResetScriptData(void* P = 0);

    virtual void Action(u16 id, u32 flags);
    virtual void SetParam(int id, Fvector2 val);
    virtual void SetParam(int id, Fvector val);
    bool HasWeapon();
    bool WpnCanHit();
    float FireDirDiff();
    bool isObjectVisible(CScriptGameObject* O);
    Fvector CurrentVel();
    virtual float GetfHealth() const { return CEntity::GetfHealth(); };
    virtual float SetfHealth(float value) { return CEntity::SetfHealth(value); };
    // Hits
    virtual void HitSignal(
        float /**HitAmount/**/, Fvector& /**local_dir/**/, IGameObject* /**who/**/, s16 /**element/**/){};
    virtual void HitImpulse(float /**amount/**/, Fvector& /**vWorldDir/**/, Fvector& /**vLocalDir/**/){};
    virtual void g_fireParams(const CHudItem* /**pHudItem/**/, Fvector& /**P/**/, Fvector& /**D/**/){};
    virtual u16 Initiator();
    // HUD
    virtual void OnHUDDraw(CCustomHUD* hud);

    CCameraBase* Camera() { return active_camera; }
    void SetExplodeTime(u32 et);
    u32 ExplodeTime();
    // Inventory for the car
    CInventory* GetInventory() { return inventory; }
    void VisualUpdate(float fov = 90.0f);

protected:
    virtual void SpawnInitPhysics(CSE_Abstract* D);
    virtual void net_Save(NET_Packet& P);
    virtual BOOL net_SaveRelevant();
    void SaveNetState(NET_Packet& P);
    virtual void RestoreNetState(CSE_PHSkeleton* po);
    void SetDefaultNetState(CSE_PHSkeleton* po);

    virtual bool IsHudModeNow() { return false; };
public:
    CCar(void);
    virtual ~CCar(void);
    virtual BOOL AlwaysTheCrow();

public:
    virtual CEntity* cast_entity() { return this; }
private:
    template <class T>
    IC void fill_wheel_vector(LPCSTR S, xr_vector<T>& type_wheels);
    IC void fill_exhaust_vector(LPCSTR S, xr_vector<SExhaust>& exhausts);
    IC void fill_doors_map(LPCSTR S, xr_map<u16, SDoor>& doors);

    // Inventory for the car
    CInventory* inventory;

    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual CGameObject* cast_game_object() { return this; }
    virtual CExplosive* cast_explosive() { return this; }
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return this; }
    virtual CParticlesPlayer* cast_particles_player() { return this; }
    virtual CScriptEntity* cast_script_entity() { return this; }
    virtual IDamageSource* cast_IDamageSource() { return this; }
    virtual CHolderCustom* cast_holder_custom() { return this; }
private:
    car_memory* m_memory;
};
