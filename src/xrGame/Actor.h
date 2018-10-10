#pragma once

#include "xrEngine/Feel_Touch.h"
#include "xrEngine/Feel_Sound.h"
#include "xrEngine/IInputReceiver.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Actor_Flags.h"
#include "actor_defs.h"
#include "fire_disp_controller.h"
#include "entity_alive.h"
#include "PHMovementControl.h"
#include "xrPhysics/PhysicsShell.h"
#include "InventoryOwner.h"
#include "xrEngine/StatGraph.h"
#include "PhraseDialogManager.h"
#include "xrUICore/ui_defs.h"

#include "step_manager.h"

using namespace ACTOR_DEFS;

class CInfoPortion;
struct GAME_NEWS_DATA;
class CActorCondition;
class CCustomOutfit;
class CGameTaskRegistryWrapper;
class CGameNewsRegistryWrapper;
class CCharacterPhysicsSupport;
class CActorCameraManager;
// refs
class ENGINE_API CCameraBase;
class ENGINE_API CBoneInstance;
class ENGINE_API CBlend;
class CWeaponList;
class CEffectorBobbing;
class CHolderCustom;
struct SShootingEffector;
struct SSleepEffector;
class CSleepEffectorPP;
class CInventoryBox;

class CHudItem;
class CArtefact;

struct SActorMotions;
struct SActorVehicleAnims;
class CActorCondition;
class SndShockEffector;
class CActorFollowerMngr;

struct CameraRecoil;
class CCameraShotEffector;
class CActorInputHandler;

class CActorMemory;
class CActorStatisticMgr;

class CLocationManager;

class CActor : public CEntityAlive,
               public IInputReceiver,
               public Feel::Touch,
               public CInventoryOwner,
               public CPhraseDialogManager,
               public CStepManager,
               public Feel::Sound
#ifdef DEBUG
               ,
               public pureRender
#endif
{
    friend class CActorCondition;

private:
    typedef CEntityAlive inherited;

public:
    CActor();
    virtual ~CActor();

public:
    virtual BOOL AlwaysTheCrow() { return TRUE; }
    virtual CAttachmentOwner* cast_attachment_owner() { return this; }
    virtual CInventoryOwner* cast_inventory_owner() { return this; }
    virtual CActor* cast_actor() { return this; }
    virtual CGameObject* cast_game_object() { return this; }
    virtual IInputReceiver* cast_input_receiver() { return this; }
    virtual CCharacterPhysicsSupport* character_physics_support() { return m_pPhysics_support; }
    virtual CCharacterPhysicsSupport* character_physics_support() const { return m_pPhysics_support; }
    virtual CPHDestroyable* ph_destroyable();
    CHolderCustom* Holder() { return m_holder; }
public:
    virtual void Load(LPCSTR section);

    virtual void shedule_Update(u32 T);
    virtual void UpdateCL();

    virtual void OnEvent(NET_Packet& P, u16 type);

    // Render
    virtual void renderable_Render();
    virtual BOOL renderable_ShadowGenerate();
    virtual void feel_sound_new(
        IGameObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power);
    virtual Feel::Sound* dcast_FeelSound() { return this; }
    float m_snd_noise;
#ifdef DEBUG
    virtual void OnRender();

#endif

public:
    virtual bool OnReceiveInfo(shared_str info_id) const;
    virtual void OnDisableInfo(shared_str info_id) const;

    virtual void NewPdaContact(CInventoryOwner*);
    virtual void LostPdaContact(CInventoryOwner*);

#ifdef DEBUG
    void DumpTasks();
#endif

    struct SDefNewsMsg
    {
        GAME_NEWS_DATA* news_data;
        u32 time;
        bool operator<(const SDefNewsMsg& other) const { return time > other.time; }
    };
    xr_vector<SDefNewsMsg> m_defferedMessages;
    void UpdateDefferedMessages();

public:
    void AddGameNews_deffered(GAME_NEWS_DATA& news_data, u32 delay);
    virtual void AddGameNews(GAME_NEWS_DATA& news_data);

protected:
    CActorStatisticMgr* m_statistic_manager;

public:
    virtual void StartTalk(CInventoryOwner* talk_partner);
    void RunTalkDialog(CInventoryOwner* talk_partner, bool disable_break);
    CActorStatisticMgr& StatisticMgr() { return *m_statistic_manager; }
    CGameNewsRegistryWrapper* game_news_registry;
    CCharacterPhysicsSupport* m_pPhysics_support;

    virtual LPCSTR Name() const { return CInventoryOwner::Name(); }
public:
    // PhraseDialogManager
    virtual void ReceivePhrase(DIALOG_SHARED_PTR& phrase_dialog);
    virtual void UpdateAvailableDialogs(CPhraseDialogManager* partner);
    virtual void TryToTalk();
    bool OnDialogSoundHandlerStart(CInventoryOwner* inv_owner, LPCSTR phrase);
    bool OnDialogSoundHandlerStop(CInventoryOwner* inv_owner);

    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual bool use_bolts() const;

    virtual void OnItemTake(CInventoryItem* inventory_item);

    virtual void OnItemRuck(CInventoryItem* inventory_item, const SInvItemPlace& previous_place);
    virtual void OnItemBelt(CInventoryItem* inventory_item, const SInvItemPlace& previous_place);

    virtual void OnItemDrop(CInventoryItem* inventory_item, bool just_before_destroy);
    virtual void OnItemDropUpdate();

    virtual void OnPlayHeadShotParticle(NET_Packet P);

    virtual void Die(IGameObject* who);
    virtual void Hit(SHit* pHDS);
    virtual void PHHit(SHit& H);
    virtual void HitSignal(float P, Fvector& vLocalDir, IGameObject* who, s16 element);
    void HitSector(IGameObject* who, IGameObject* weapon);
    void HitMark(float P, Fvector dir, IGameObject* who, s16 element, Fvector position_in_bone_space, float impulse,
        ALife::EHitType hit_type);

    void Feel_Grenade_Update(float rad);

    virtual float GetMass();
    virtual float Radius() const;
    virtual void g_PerformDrop();

    virtual bool use_default_throw_force();
    virtual float missile_throw_force();

    virtual bool unlimited_ammo();

    virtual bool NeedToDestroyObject() const;
    virtual ALife::_TIME_ID TimePassedAfterDeath() const;

public:
    //свойства артефактов
    virtual void UpdateArtefactsOnBeltAndOutfit();
    float HitArtefactsOnBelt(float hit_power, ALife::EHitType hit_type);
    float GetProtection_ArtefactsOnBelt(ALife::EHitType hit_type);

protected:
    //звук тяжелого дыхания
    ref_sound m_HeavyBreathSnd;
    ref_sound m_BloodSnd;
    ref_sound m_DangerSnd;

protected:
    // Death
    float m_hit_slowmo;
    float m_hit_probability;
    s8 m_block_sprint_counter;

    // media
    SndShockEffector* m_sndShockEffector;
    xr_vector<ref_sound> sndHit[ALife::eHitTypeMax];
    ref_sound sndDie[SND_DIE_COUNT];

    float m_fLandingTime;
    float m_fJumpTime;
    float m_fFallTime;
    float m_fCamHeightFactor;

    // Dropping
    BOOL b_DropActivated;
    float f_DropPower;

    // random seed для Zoom mode
    s32 m_ZoomRndSeed;
    // random seed для Weapon Effector Shot
    s32 m_ShotRndSeed;

    bool m_bOutBorder;
    //сохраняет счетчик объектов в feel_touch, для которых необходимо обновлять размер колижена с актером
    u32 m_feel_touch_characters;

private:
    void SwitchOutBorder(bool new_border_state);

public:
    bool m_bAllowDeathRemove;

    void SetZoomRndSeed(s32 Seed = 0);
    s32 GetZoomRndSeed() { return m_ZoomRndSeed; };
    void SetShotRndSeed(s32 Seed = 0);
    s32 GetShotRndSeed() { return m_ShotRndSeed; };
public:
    void detach_Vehicle();
    void steer_Vehicle(float angle);
    void attach_Vehicle(CHolderCustom* vehicle);
    bool use_MountedWeapon(CHolderCustom* object);
    virtual bool can_attach(const CInventoryItem* inventory_item) const;

protected:
    CHolderCustom* m_holder;
    u16 m_holderID;
    bool use_Holder(CHolderCustom* holder);

    bool use_Vehicle(CHolderCustom* object);
    void ActorUse();

protected:
    BOOL m_bAnimTorsoPlayed;
    static void AnimTorsoPlayCallBack(CBlend* B);

    // Rotation
    SRotation r_torso;
    float r_torso_tgt_roll;
    //положение торса без воздействия эффекта отдачи оружия
    SRotation unaffected_r_torso;

    //ориентация модели
    float r_model_yaw_dest;
    float r_model_yaw; // orientation of model
    float r_model_yaw_delta; // effect on multiple "strafe"+"something"

public:
    SActorMotions* m_anims;
    SActorVehicleAnims* m_vehicle_anims;

    CBlend* m_current_legs_blend;
    CBlend* m_current_torso_blend;
    CBlend* m_current_jump_blend;
    MotionID m_current_legs;
    MotionID m_current_torso;
    MotionID m_current_head;

    // callback на анимации модели актера
    void SetCallbacks();
    void ResetCallbacks();
    static void Spin0Callback(CBoneInstance*);
    static void Spin1Callback(CBoneInstance*);
    static void ShoulderCallback(CBoneInstance*);
    static void HeadCallback(CBoneInstance*);
    static void VehicleHeadCallback(CBoneInstance*);

    virtual const SRotation Orientation() const { return r_torso; };
    SRotation& Orientation() { return r_torso; };
    void g_SetAnimation(u32 mstate_rl);
    void g_SetSprintAnimation(u32 mstate_rl, MotionID& head, MotionID& torso, MotionID& legs);

public:
    virtual void OnHUDDraw(CCustomHUD* hud);
    BOOL HUDview() const;

    // visiblity
    virtual float ffGetFov() const { return 90.f; }
    virtual float ffGetRange() const { return 500.f; }
public:
    CActorCameraManager& Cameras()
    {
        VERIFY(m_pActorEffector);
        return *m_pActorEffector;
    }
    IC CCameraBase* cam_Active() { return cameras[cam_active]; }
    IC CCameraBase* cam_FirstEye() { return cameras[eacFirstEye]; }

    EActorCameras active_cam() const { return cam_active; } // KD: we need to know which cam is active outside actor methods
    virtual void cam_Set(EActorCameras style); //Alundaio: made public

protected:
    //virtual void cam_Set(EActorCameras style);
    void cam_Update(float dt, float fFOV);
    void cam_Lookout(const Fmatrix& xform, float camera_height);
    void camUpdateLadder(float dt);
    void cam_SetLadder();
    void cam_UnsetLadder();
    float currentFOV();

    // Cameras
    CCameraBase* cameras[eacMaxCam];
    EActorCameras cam_active;
    float fPrevCamPos;
    float current_ik_cam_shift;
    Fvector vPrevCamDir;
    float fCurAVelocity;
    CEffectorBobbing* pCamBobbing;

    //менеджер эффекторов, есть у каждого актрера
    CActorCameraManager* m_pActorEffector;
    static float f_Ladder_cam_limit;

public:
    virtual void feel_touch_new(IGameObject* O);
    virtual void feel_touch_delete(IGameObject* O);
    virtual bool feel_touch_contact(IGameObject* O);
    virtual bool feel_touch_on_contact(IGameObject* O);

    CGameObject* ObjectWeLookingAt() { return m_pObjectWeLookingAt; }
    CInventoryOwner* PersonWeLookingAt() { return m_pPersonWeLookingAt; }
    LPCSTR GetDefaultActionForObject() { return *m_sDefaultObjAction; }
protected:
    CGameObject* m_pUsableObject;
    // Person we're looking at
    CInventoryOwner* m_pPersonWeLookingAt;
    CHolderCustom* m_pVehicleWeLookingAt;
    CGameObject* m_pObjectWeLookingAt;
    CInventoryBox* m_pInvBoxWeLookingAt;

    // Tip for action for object we're looking at
    shared_str m_sDefaultObjAction;
    shared_str m_sCharacterUseAction;
    shared_str m_sDeadCharacterUseAction;
    shared_str m_sDeadCharacterUseOrDragAction;
    shared_str m_sDeadCharacterDontUseAction;
    shared_str m_sCarCharacterUseAction;
    shared_str m_sInventoryItemUseAction;
    shared_str m_sInventoryBoxUseAction;

    //	shared_str				m_quick_use_slots[4];
    //режим подбирания предметов
    bool m_bPickupMode;
    //расстояние (в метрах) на котором актер чувствует гранату (любую)
    float m_fFeelGrenadeRadius;
    float m_fFeelGrenadeTime; //время гранаты (сек) после которого актер чувствует гранату
    //расстояние подсветки предметов
    float m_fPickupInfoRadius;

    void PickupModeUpdate();
    void PickupInfoDraw(IGameObject* object);
    void PickupModeUpdate_COD();

    //////////////////////////////////////////////////////////////////////////
    // Motions (передвижения актрера)
    //////////////////////////////////////////////////////////////////////////
public:
    void g_cl_CheckControls(u32 mstate_wf, Fvector& vControlAccel, float& Jump, float dt);
    void g_cl_ValidateMState(float dt, u32 mstate_wf);
    void g_cl_Orientate(u32 mstate_rl, float dt);
    void g_sv_Orientate(u32 mstate_rl, float dt);
    void g_Orientate(u32 mstate_rl, float dt);
    bool g_LadderOrient();
    //	void					UpdateMotionIcon		(u32 mstate_rl);

    bool CanAccelerate();
    bool CanJump();
    bool CanMove();
    float CameraHeight();
    // Alex ADD: for smooth crouch fix
    float CurrentHeight;
    bool CanSprint();
    bool CanRun();
    void StopAnyMove();

    bool AnyAction() { return (mstate_real & mcAnyAction) != 0; };
    bool AnyMove() { return (mstate_real & mcAnyMove) != 0; };
    bool is_jump();
    u32 MovingState() const { return mstate_real; }
protected:
    u32 mstate_wishful;
    u32 mstate_old;
    u32 mstate_real;

    BOOL m_bJumpKeyPressed;

public:
    float m_fWalkAccel;
    float m_fJumpSpeed;
    float m_fRunFactor;
    float m_fRunBackFactor;
    float m_fWalkBackFactor;
    float m_fCrouchFactor;
    float m_fClimbFactor;
    float m_fSprintFactor;

    float m_fWalk_StrafeFactor;
    float m_fRun_StrafeFactor;

public:
    Fvector GetMovementSpeed() { return NET_SavedAccel; };
    //////////////////////////////////////////////////////////////////////////
    // User input/output
    //////////////////////////////////////////////////////////////////////////
public:
    virtual void IR_OnMouseMove(int x, int y);
    virtual void IR_OnKeyboardPress(int dik);
    virtual void IR_OnKeyboardRelease(int dik);
    virtual void IR_OnKeyboardHold(int dik);
    virtual void IR_OnMouseWheel(int x, int y);
    virtual float GetLookFactor();

public:
    virtual void g_WeaponBones(int& L, int& R1, int& R2);
    virtual void g_fireParams(const CHudItem* pHudItem, Fvector& P, Fvector& D);
    virtual bool g_stateFire() { return !((mstate_wishful & mcLookout) && !IsGameTypeSingle()); }
    virtual BOOL g_State(SEntityState& state) const;
    virtual float GetWeaponAccuracy() const;
    float GetFireDispertion() const { return m_fdisp_controller.GetCurrentDispertion(); }
    bool IsZoomAimingMode() const { return m_bZoomAimingMode; }
    virtual float MaxCarryWeight() const;
    float MaxWalkWeight() const;
    float get_additional_weight() const;

protected:
    CFireDispertionController m_fdisp_controller;
    //если актер целится в прицел
    void SetZoomAimingMode(bool val) { m_bZoomAimingMode = val; }
    bool m_bZoomAimingMode;

    //настройки аккуратности стрельбы
    //базовая дисперсия (когда игрок стоит на месте)
    float m_fDispBase;
    float m_fDispAim;
    //коэффициенты на сколько процентов увеличится базовая дисперсия
    //учитывает скорость актера
    float m_fDispVelFactor;
    //если актер бежит
    float m_fDispAccelFactor;
    //если актер сидит
    float m_fDispCrouchFactor;
    // crouch+no acceleration
    float m_fDispCrouchNoAccelFactor;

    Fvector m_vMissileOffset;

public:
    // Получение, и запись смещения для гранат
    Fvector GetMissileOffset() const;
    void SetMissileOffset(const Fvector& vNewOffset);

protected:
    //косточки используемые при стрельбе
    int m_r_hand;
    int m_l_finger1;
    int m_r_finger2;
    int m_head;
    int m_eye_left;
    int m_eye_right;

    int m_l_clavicle;
    int m_r_clavicle;
    int m_spine2;
    int m_spine1;
    int m_spine;
    int m_neck;

    //////////////////////////////////////////////////////////////////////////
    // Network
    //////////////////////////////////////////////////////////////////////////
    void ConvState(u32 mstate_rl, string128* buf);

public:
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Export(NET_Packet& P); // export to server
    virtual void net_Import(NET_Packet& P); // import from server
    virtual void net_Destroy();
    virtual BOOL net_Relevant(); //	{ return getSVU() | getLocal(); };		// relevant for export to server
    virtual void net_Relcase(IGameObject* O); //
    virtual void xr_stdcall on_requested_spawn(IGameObject* object);
    // object serialization
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);
    virtual void net_Save(NET_Packet& P);
    virtual BOOL net_SaveRelevant();

protected:
    xr_deque<net_update> NET;
    Fvector NET_SavedAccel;
    net_update NET_Last;
    BOOL NET_WasInterpolating; // previous update was by interpolation or by extrapolation
    u32 NET_Time; // server time of last update

    //---------------------------------------------
    void net_Import_Base(NET_Packet& P);
    void net_Import_Physic(NET_Packet& P);
    void net_Import_Base_proceed();
    void net_Import_Physic_proceed();
    //---------------------------------------------

    ////////////////////////////////////////////////////////////////////////////
    virtual bool can_validate_position_on_spawn() { return false; }
    ///////////////////////////////////////////////////////
    // апдайт с данными физики
    xr_deque<net_update_A> NET_A;

    //---------------------------------------------
    //	bool					m_bHasUpdate;
    /// spline coeff /////////////////////
    float SCoeff[3][4]; //коэффициэнты для сплайна Бизье
    float HCoeff[3][4]; //коэффициэнты для сплайна Эрмита
    Fvector IPosS, IPosH, IPosL; //положение актера после интерполяции Бизье, Эрмита, линейной

#ifdef DEBUG
    using VIS_POSITION = xr_deque<Fvector>;

    VIS_POSITION LastPosS;
    VIS_POSITION LastPosH;
    VIS_POSITION LastPosL;
#endif

    SPHNetState LastState;
    SPHNetState RecalculatedState;
    SPHNetState PredictedState;

    InterpData IStart;
    InterpData IRec;
    InterpData IEnd;

    bool m_bInInterpolation;
    bool m_bInterpolate;
    u32 m_dwIStartTime;
    u32 m_dwIEndTime;
    u32 m_dwILastUpdateTime;

    //---------------------------------------------
    using PH_STATES = xr_deque<SPHNetState>;
    PH_STATES m_States;
    u16 m_u16NumBones;
    void net_ExportDeadBody(NET_Packet& P);
    //---------------------------------------------
    void CalculateInterpolationParams();
    //---------------------------------------------
    virtual void make_Interpolation();
#ifdef DEBUG
    //---------------------------------------------
    virtual void OnRender_Network();
//---------------------------------------------
#endif

    // Igor	ref_geom 				hFriendlyIndicator;
    //////////////////////////////////////////////////////////////////////////
    // Actor physics
    //////////////////////////////////////////////////////////////////////////
public:
    void g_Physics(Fvector& accel, float jump, float dt);
    virtual void ForceTransform(const Fmatrix& m);
    void SetPhPosition(const Fmatrix& pos);
    virtual void PH_B_CrPr(); // actions & operations before physic correction-prediction steps
    virtual void PH_I_CrPr(); // actions & operations after correction before prediction steps
    virtual void PH_A_CrPr(); // actions & operations after phisic correction-prediction steps
    //	virtual void			UpdatePosStack	( u32 Time0, u32 Time1 );
    virtual void MoveActor(Fvector NewPos, Fvector NewDir);

    virtual void SpawnAmmoForWeapon(CInventoryItem* pIItem);
    virtual void RemoveAmmoForWeapon(CInventoryItem* pIItem);
    virtual void spawn_supplies();
    virtual bool human_being() const { return (true); }
    virtual shared_str GetDefaultVisualOutfit() const { return m_DefaultVisualOutfit; };
    virtual void SetDefaultVisualOutfit(shared_str DefaultOutfit) { m_DefaultVisualOutfit = DefaultOutfit; };
    virtual void UpdateAnimation() { g_SetAnimation(mstate_real); };
    virtual void ChangeVisual(shared_str NewVisual);
    virtual void OnChangeVisual();

    virtual void RenderIndicator(Fvector dpos, float r1, float r2, const ui_shader& IndShader);
    virtual void RenderText(LPCSTR Text, Fvector dpos, float* pdup, u32 color);

    //////////////////////////////////////////////////////////////////////////
    // Controlled Routines
    //////////////////////////////////////////////////////////////////////////

    void set_input_external_handler(CActorInputHandler* handler);
    bool input_external_handler_installed() const { return (m_input_external_handler != 0); }
    IC void lock_accel_for(u32 time) { m_time_lock_accel = Device.dwTimeGlobal + time; }
private:
    CActorInputHandler* m_input_external_handler;
    u32 m_time_lock_accel;

    /////////////////////////////////////////
    // DEBUG INFO
protected:
    CStatGraph* pStatGraph;

    shared_str m_DefaultVisualOutfit;

    LPCSTR invincibility_fire_shield_3rd;
    LPCSTR invincibility_fire_shield_1st;
    shared_str m_sHeadShotParticle;
    u32 last_hit_frame;
#ifdef DEBUG
    friend class CLevelGraph;
#endif
    Fvector m_AutoPickUp_AABB;
    Fvector m_AutoPickUp_AABB_Offset;

    void Check_for_AutoPickUp();
    void SelectBestWeapon(IGameObject* O);

public:
    void SetWeaponHideState(u16 State, bool bSet);

private: // IPhysicsShellHolder
    virtual void HideAllWeapons(bool v) { SetWeaponHideState(INV_STATE_BLOCK_ALL, v); }
public:
    void SetCantRunState(bool bSet);

private:
    CActorCondition* m_entity_condition;

protected:
    virtual CEntityConditionSimple* create_entity_condition(CEntityConditionSimple* ec);

public:
    IC CActorCondition& conditions() const;
    virtual IFactoryObject* _construct();
    virtual bool natural_weapon() const { return false; }
    virtual bool natural_detector() const { return false; }
    virtual bool use_center_to_aim() const;

protected:
    u16 m_iLastHitterID;
    u16 m_iLastHittingWeaponID;
    s16 m_s16LastHittedElement;
    Fvector m_vLastHitDir;
    Fvector m_vLastHitPos;
    float m_fLastHealth;
    bool m_bWasHitted;
    bool m_bWasBackStabbed;

    virtual bool Check_for_BackStab_Bone(u16 element);

public:
    virtual void SetHitInfo(IGameObject* who, IGameObject* weapon, s16 element, Fvector Pos, Fvector Dir);

    virtual void OnHitHealthLoss(float NewHealth);
    virtual void OnCriticalHitHealthLoss();
    virtual void OnCriticalWoundHealthLoss();
    virtual void OnCriticalRadiationHealthLoss();

    virtual bool InventoryAllowSprint();
    virtual void OnNextWeaponSlot();
    virtual void OnPrevWeaponSlot();
    void SwitchNightVision();
    void SwitchTorch();
#ifdef DEBUG
    void NoClipFly(int cmd);
#endif // DEBUG

public:
    virtual void on_weapon_shot_start(CWeapon* weapon);
    virtual void on_weapon_shot_update();
    virtual void on_weapon_shot_stop();
    virtual void on_weapon_shot_remove(CWeapon* weapon);
    virtual void on_weapon_hide(CWeapon* weapon);
    Fvector weapon_recoil_delta_angle();
    Fvector weapon_recoil_last_delta();

protected:
    virtual void update_camera(CCameraShotEffector* effector);
    // step manager
    virtual bool is_on_ground();

private:
    CActorMemory* m_memory;

public:
    IC CActorMemory& memory() const
    {
        VERIFY(m_memory);
        return (*m_memory);
    };

    void OnDifficultyChanged();

    IC float HitProbability() { return m_hit_probability; }
    virtual CVisualMemoryManager* visual_memory() const;

    virtual BOOL BonePassBullet(int boneID);
    virtual void On_B_NotCurrentEntity();

private:
    collide::rq_results RQR;
    BOOL CanPickItem(const CFrustum& frustum, const Fvector& from, IGameObject* item);
    xr_vector<ISpatial*> ISpatialResult;

private:
    CLocationManager* m_location_manager;

public:
    IC const CLocationManager& locations() const
    {
        VERIFY(m_location_manager);
        return (*m_location_manager);
    }

private:
    ALife::_OBJECT_ID m_holder_id;

public:
    virtual bool register_schedule() const { return false; }
    virtual bool is_ai_obstacle() const;

    float GetRestoreSpeed(ALife::EConditionRestoreType const& type);

public:
    virtual void On_SetEntity();
    virtual void On_LostEntity(){};

    void DisableHitMarks(bool disable) { m_disabled_hitmarks = disable; };
    bool DisableHitMarks() { return m_disabled_hitmarks; };
    void set_inventory_disabled(bool is_disabled) { m_inventory_disabled = is_disabled; }
    bool inventory_disabled() const { return m_inventory_disabled; }
private:
    void set_state_box(u32 mstate);

private:
    bool m_disabled_hitmarks;
    bool m_inventory_disabled;
    // static CPhysicsShell		*actor_camera_shell;

    IC u32 get_state() const
    {
        return this->mstate_real;
    }

    IC void set_state(u32 state)
    {
        mstate_real = state;
    }

    IC u32 get_state_wishful() const
    {
        return this->mstate_wishful;
    }

    IC void set_state_wishful(u32 state)
    {
        mstate_wishful = state;
    }
};

extern bool isActorAccelerated(u32 mstate, bool ZoomMode);

IC CActorCondition& CActor::conditions() const
{
    VERIFY(m_entity_condition);
    return (*m_entity_condition);
}

extern CActor* g_actor;
CActor* Actor();
extern const float s_fFallTime;
