// GameObject.h: interface for the CGameObject class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#if !defined(AFX_GAMEOBJECT_H__3DA72D03_C759_4688_AEBB_89FA812AA873__INCLUDED_)
#define AFX_GAMEOBJECT_H__3DA72D03_C759_4688_AEBB_89FA812AA873__INCLUDED_

#include "xrEngine/xr_object.h"
#include "xrServer_Space.h"
#include "alife_space.h"
#include "xrScriptEngine/script_space_forward.hpp"
#include "xrScriptEngine/DebugMacros.hpp" // XXX: move debug macros to xrCore
#include "script_binder.h"
#include "Hit.h"
#include "game_object_space.h"

class CPhysicsShell;
class CSE_Abstract;
class CPHSynchronize;
class CScriptGameObject;
class CInventoryItem;
class CEntity;
class CEntityAlive;
class CInventoryOwner;
class CActor;
class CPhysicsShellHolder;
class CParticlesPlayer;
class CCustomZone;
class IInputReceiver;
class CArtefact;
class CCustomMonster;
class CAI_Stalker;
class CScriptEntity;
class CAI_ObjectLocation;
class CWeapon;
class CExplosive;
class CHolderCustom;
class CAttachmentOwner;
class CBaseMonster;
class CSpaceRestrictor;
class CAttachableItem;
class animation_movement_controller;
class CBlend;
class ai_obstacle;

class IKinematics;

template <typename _return_type>
class CScriptCallbackEx;

#pragma pack(push, 4)
class CGameObject : public IGameObject,
                    public FactoryObjectBase,
                    public SpatialBase,
                    public ScheduledBase,
                    public RenderableBase,
                    public CollidableBase
{
    BENCH_SEC_SCRAMBLEMEMBER1
    BENCH_SEC_SCRAMBLEVTBL2
    // Some property variables
    GameObjectProperties Props;
    shared_str NameObject;
    shared_str NameSection;
    shared_str NameVisual;

protected:
    // Parentness
    IGameObject* Parent;
    // Geometric (transformation)
    svector<GameObjectSavedPosition, 4> PositionStack;
#ifdef DEBUG
    u32 dbg_update_cl;
#endif
    u32 dwFrame_UpdateCL;
    u32 dwFrame_AsCrow;

private:
    shared_str m_sTipText;
    bool m_bNonscriptUsable;
    bool m_spawned;
    Flags32 m_server_flags;
    CAI_ObjectLocation* m_ai_location;
    ALife::_STORY_ID m_story_id;
    animation_movement_controller* m_anim_mov_ctrl;
    bool m_bCrPr_Activated;
    u32 m_dwCrPr_ActivationStep;
    mutable CScriptGameObject* m_lua_game_object;
    int m_script_clsid;
    u32 m_spawn_time;
    using CALLBACK_MAP = xr_map<GameObject::ECallbackType, CScriptCallbackExVoid>;
    CALLBACK_MAP* m_callbacks;
    ai_obstacle* m_ai_obstacle;
    Fmatrix m_previous_matrix;
    CALLBACK_VECTOR m_visual_callback;

protected:
    CScriptBinder scriptBinder;
    bool m_bObjectRemoved;
    CInifile* m_ini_file;
    bool m_client_updated{};
#ifdef DEBUG
    bool m_should_process_onrender{};
#endif

public:
    CGameObject();
    virtual ~CGameObject();
// XXX: review
#ifdef DEBUG
    virtual u32 GetDbgUpdateFrame() const override { return dbg_update_cl; }
    virtual void SetDbgUpdateFrame(u32 value) override { dbg_update_cl = value; }
#endif
    virtual u32 GetUpdateFrame() const override { return dwFrame_UpdateCL; }
    virtual void SetUpdateFrame(u32 value) override { dwFrame_UpdateCL = value; }
    virtual u32 GetCrowUpdateFrame() const override { return dwFrame_AsCrow; }
    virtual void SetCrowUpdateFrame(u32 value) override { dwFrame_AsCrow = value; }
#ifdef DEBUG
    virtual void DBGGetProps(GameObjectProperties& p) const override { p = Props; }
#endif
    virtual void MakeMeCrow() override;
    virtual void IAmNotACrowAnyMore() override { Props.crow = false; }
    virtual bool AlwaysTheCrow() override { return FALSE; }
    virtual bool AmICrow() const override { return !!Props.crow; }
    // Network
    virtual bool Local() const override { return Props.net_Local; }
    virtual bool Remote() const override { return !Props.net_Local; }
    virtual u16 ID() const override { return Props.net_ID; }
    virtual void setID(u16 _ID) override { Props.net_ID = _ID; }
    virtual bool GetTmpPreDestroy() const override { return Props.bPreDestroy; }
    virtual void SetTmpPreDestroy(bool b) override { Props.bPreDestroy = b; }
    virtual float shedule_Scale() override { return Device.vCameraPosition.distance_to(Position()) / 200.f; }
    virtual bool shedule_Needed() override;
    virtual void shedule_Update(u32 dt) override;
    // Parentness
    virtual IGameObject* H_Parent() override { return Parent; }
    virtual const IGameObject* H_Parent() const override { return Parent; }
    virtual IGameObject* H_Root() override { return Parent ? Parent->H_Root() : this; }
    virtual const IGameObject* H_Root() const override { return Parent ? Parent->H_Root() : this; }
    virtual IGameObject* H_SetParent(IGameObject* O, bool just_before_destroy = false) override;
    // Geometry xform
    virtual void Center(Fvector& C) const override;
    virtual const Fmatrix& XFORM() const override
    {
        VERIFY(_valid(renderable.xform));
        return renderable.xform;
    }
    virtual Fmatrix& XFORM() override { return renderable.xform; }
    virtual void spatial_register() override;
    virtual void spatial_unregister() override;
    virtual void spatial_move() override;
    virtual void spatial_update(float eps_P, float eps_R) override;
    virtual IGameObject* dcast_GameObject() override { return this; }
    virtual IRenderable* dcast_Renderable() override { return this; }
    virtual Fvector& Direction() override { return renderable.xform.k; }
    virtual const Fvector& Direction() const override { return renderable.xform.k; }
    virtual Fvector& Position() override { return renderable.xform.c; }
    virtual const Fvector& Position() const override { return renderable.xform.c; }
    virtual float Radius() const override;
    virtual const Fbox& BoundingBox() const override;
    virtual IRender_Sector* Sector() override { return H_Root()->GetSpatialData().sector; }
    virtual IRender_ObjectSpecific* ROS() override { return RenderableBase::renderable_ROS(); }
    virtual bool renderable_ShadowGenerate() override { return TRUE; }
    virtual bool renderable_ShadowReceive() override { return TRUE; }
    // Accessors and converters
    virtual IRenderVisual* Visual() const override { return renderable.visual; }
    virtual IPhysicsShell* physics_shell() override { return nullptr; }
    virtual const IObjectPhysicsCollision* physics_collision() override { return nullptr; }
    // Name management
    virtual shared_str cName() const override { return NameObject; }
    virtual void cName_set(shared_str N) override;
    virtual shared_str cNameSect() const override { return NameSection; }
    virtual LPCSTR cNameSect_str() const override { return NameSection.c_str(); }
    virtual void cNameSect_set(shared_str N) override;
    virtual shared_str cNameVisual() const override { return NameVisual; }
    virtual void cNameVisual_set(shared_str N) override;
    virtual shared_str shedule_Name() const override { return cName(); };
    // Properties
    virtual void processing_activate() override; // request to enable UpdateCL
    virtual void processing_deactivate() override; // request to disable UpdateCL
    virtual bool processing_enabled() override { return !!Props.bActiveCounter; }
    virtual void setVisible(bool _visible) override;
    virtual bool getVisible() const override { return Props.bVisible; }
    virtual void setEnabled(bool _enabled) override;
    virtual bool getEnabled() const override { return Props.bEnabled; }
    virtual void setDestroy(bool _destroy) override;
    virtual bool getDestroy() const override { return Props.bDestroy; }
    virtual void setLocal(bool _local) override { Props.net_Local = _local ? 1 : 0; }
    virtual bool getLocal() const override { return Props.net_Local; }
    virtual void setSVU(bool _svu) override { Props.net_SV_Update = _svu ? 1 : 0; }
    virtual bool getSVU() const override { return Props.net_SV_Update; }
    virtual void setReady(bool _ready) override { Props.net_Ready = _ready ? 1 : 0; }
    virtual bool getReady() const override { return Props.net_Ready; }
    // functions used for avoiding most of the smart_cast
    virtual CAttachmentOwner* cast_attachment_owner() override { return NULL; }
    virtual CInventoryOwner* cast_inventory_owner() override { return NULL; }
    virtual CInventoryItem* cast_inventory_item() override { return NULL; }
    virtual CEntity* cast_entity() override { return NULL; }
    virtual CEntityAlive* cast_entity_alive() override { return NULL; }
    virtual CActor* cast_actor() override { return NULL; }
    virtual CGameObject* cast_game_object() override { return this; }
    virtual CCustomZone* cast_custom_zone() override { return NULL; }
    virtual CPhysicsShellHolder* cast_physics_shell_holder() override { return NULL; }
    virtual IInputReceiver* cast_input_receiver() override { return NULL; }
    virtual CParticlesPlayer* cast_particles_player() override { return NULL; }
    virtual CArtefact* cast_artefact() override { return NULL; }
    virtual CCustomMonster* cast_custom_monster() override { return NULL; }
    virtual CAI_Stalker* cast_stalker() override { return NULL; }
    virtual CScriptEntity* cast_script_entity() override { return NULL; }
    virtual CWeapon* cast_weapon() override { return NULL; }
    virtual CExplosive* cast_explosive() override { return NULL; }
    virtual CSpaceRestrictor* cast_restrictor() override { return NULL; }
    virtual CAttachableItem* cast_attachable_item() override { return NULL; }
    virtual CHolderCustom* cast_holder_custom() override { return NULL; }
    virtual CBaseMonster* cast_base_monster() override { return NULL; }
    CShellLauncher* cast_shell_launcher() override { return nullptr; }
    virtual bool feel_touch_on_contact(IGameObject*) override { return TRUE; }
    // Utilities
    // XXX: move out
    static void u_EventGen(NET_Packet& P, u32 type, u32 dest);
    static void u_EventSend(NET_Packet& P, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/);
    // Methods
    virtual void Load(LPCSTR section) override;
    void PostLoad(LPCSTR section) override; //--#SM+#--
    void PreUpdateCL() override;
    virtual void UpdateCL() override; // Called each frame, so no need for dt
    void PostUpdateCL(bool bUpdateCL_disabled) override; //--#SM+#--
    virtual void OnChangeVisual() override;
    // object serialization
    virtual void net_Save(NET_Packet& packet) override;
    virtual void net_Load(IReader& reader) override;
    virtual bool net_SaveRelevant() override;
    virtual void net_Export(NET_Packet& packet) override {} // export to server
    virtual void net_Import(NET_Packet& packet) override {} // import from server
    virtual bool net_Spawn(CSE_Abstract* entity) override;
    virtual void net_Destroy() override;
    virtual void net_ImportInput(NET_Packet& packet) override {}
    virtual bool net_Relevant() override { return getLocal(); } // send messages only if active and local
    virtual void net_MigrateInactive(NET_Packet& packet) override { Props.net_Local = FALSE; }
    virtual void net_MigrateActive(NET_Packet& packet) override { Props.net_Local = TRUE; }
    virtual void net_Relcase(IGameObject* O) override; // destroy all links to another objects
    virtual void save(NET_Packet& output_packet) override;
    virtual void load(IReader& input_packet);
    // Position stack
    virtual u32 ps_Size() const override { return PositionStack.size(); }
    virtual GameObjectSavedPosition ps_Element(u32 ID) const override;

    virtual void ForceTransform(const Fmatrix& m) override {}
    void ForceTransformAndDirection(const Fmatrix& m) override { ForceTransform(m); }

    void OnHUDDraw(CCustomHUD* /*hud*/, IRenderable* /*root*/) override {}
    void OnRenderHUD(IGameObject* pCurViewEntity) override {} //--#SM+#--
    void OnOwnedCameraMove(CCameraBase* pCam, float fOldYaw, float fOldPitch) override  {} //--#SM+#--
    virtual bool Ready() override { return getReady(); } // update only if active and fully initialized by/for network
    void renderable_Render(IRenderable* root) override;
    virtual void OnEvent(NET_Packet& P, u16 type) override;
    virtual void Hit(SHit* pHDS) override {}
    virtual void SetHitInfo(IGameObject* who, IGameObject* weapon, s16 element, Fvector Pos, Fvector Dir) override {}
    virtual bool BonePassBullet(int boneID) override { return FALSE; }
    //игровое имя объекта
    virtual LPCSTR Name() const override;
    // Active/non active
    virtual void OnH_B_Chield() override; // before
    virtual void OnH_B_Independent(bool just_before_destroy) override;
    virtual void OnH_A_Chield() override; // after
    virtual void OnH_A_Independent() override;
    virtual void On_SetEntity() override {}
    virtual void On_LostEntity() override {}
    virtual bool register_schedule() const override { return true; }
    virtual Fvector get_new_local_point_on_mesh(u16& bone_id) const override;
    virtual Fvector get_last_local_point_on_mesh(const Fvector& last_point, u16 bone_id) const override;
    virtual bool IsVisibleForZones() override { return true; }
    virtual bool NeedToDestroyObject() const override;
    virtual void DestroyObject() override;
    // animation_movement_controller
    virtual void create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation) override;
    virtual void destroy_anim_mov_ctrl() override;
    virtual void update_animation_movement_controller();
    virtual bool animation_movement_controlled() const override;
    virtual const animation_movement_controller* animation_movement() const override { return m_anim_mov_ctrl; }
    virtual animation_movement_controller* animation_movement() override { return m_anim_mov_ctrl; }
    // Game-specific events
    virtual bool UsedAI_Locations() override;
    virtual bool TestServerFlag(u32 Flag) const override;
    virtual bool can_validate_position_on_spawn() override { return true; }
#ifdef DEBUG
    bool ShouldProcessOnRender() const override { return m_should_process_onrender; }
    void ShouldProcessOnRender(bool should_process) override { m_should_process_onrender = should_process; }
    virtual void OnRender() override;
#endif
    virtual void reinit() override;
    virtual void reload(LPCSTR section) override;
    ///////////////////// network /////////////////////////////////////////
    virtual bool object_removed() const override { return m_bObjectRemoved; }
    virtual void make_Interpolation() override {} // interpolation from last visible to corrected position/rotation
    virtual void PH_B_CrPr() override {} // actions & operations before physic correction-prediction steps
    virtual void PH_I_CrPr() override {} // actions & operations after correction before prediction steps
#ifdef DEBUG
    virtual void PH_Ch_CrPr() override {}
    virtual void dbg_DrawSkeleton() override;
#endif
    virtual void PH_A_CrPr() override {} // actions & operations after phisic correction-prediction steps
    virtual void CrPr_SetActivationStep(u32 Step) override { m_dwCrPr_ActivationStep = Step; }
    virtual u32 CrPr_GetActivationStep() override { return m_dwCrPr_ActivationStep; }
    virtual void CrPr_SetActivated(bool Activate) override { m_bCrPr_Activated = Activate; }
    virtual bool CrPr_IsActivated() override { return m_bCrPr_Activated; };
    ///////////////////////////////////////////////////////////////////////
    virtual const SRotation Orientation() const override
    {
        SRotation rotation;
        float h, p, b;
        XFORM().getHPB(h, p, b);
        rotation.yaw = h;
        rotation.pitch = p;
        return rotation;
    }
    virtual bool use_parent_ai_locations() const override { return true; }
    virtual void add_visual_callback(visual_callback callback) override;
    virtual void remove_visual_callback(visual_callback callback) override;
    virtual CALLBACK_VECTOR& visual_callbacks() override { return m_visual_callback; }
    virtual CScriptGameObject* lua_game_object() const override;
    virtual int clsid() const override
    {
        THROW(m_script_clsid >= 0);
        return m_script_clsid;
    }
    virtual CInifile* spawn_ini() override { return m_ini_file; }
    virtual CAI_ObjectLocation& ai_location() const override
    {
        VERIFY(m_ai_location);
        return *m_ai_location;
    }
    virtual u32 spawn_time() const override
    {
        VERIFY(m_spawned);
        return m_spawn_time;
    }
    virtual const ALife::_STORY_ID& story_id() const override { return m_story_id; }
    virtual u32 ef_creature_type() const override;
    virtual u32 ef_equipment_type() const override;
    virtual u32 ef_main_weapon_type() const override;
    virtual u32 ef_anomaly_type() const override;
    virtual u32 ef_weapon_type() const override;
    virtual u32 ef_detector_type() const override;
    virtual bool natural_weapon() const override { return true; }
    virtual bool natural_detector() const override { return true; }
    virtual bool use_center_to_aim() const override { return false; }
    // [12.11.07] Alexander Maniluk: added this method for moving object
    virtual void MoveTo(const Fvector& position) override {}
    // the only usage: aimers::base::fill_bones
    virtual CScriptCallbackExVoid& callback(GameObject::ECallbackType type) const override;
    virtual LPCSTR visual_name(CSE_Abstract* server_entity) override;
    virtual void On_B_NotCurrentEntity() override {}
    virtual bool is_ai_obstacle() const override;
    virtual ai_obstacle& obstacle() const override
    {
        VERIFY(m_ai_obstacle);
        return *m_ai_obstacle;
    }
    virtual void on_matrix_change(const Fmatrix& previous) override;

    // UsableScriptObject functions
    virtual bool use(IGameObject* obj) override;

    //строчка появляющаяся при наведении на объект (если NULL, то нет)
    virtual LPCSTR tip_text() override;
    virtual void set_tip_text(LPCSTR new_text) override;
    virtual void set_tip_text_default() override;

    //можно ли использовать объект стандартным (не скриптовым) образом
    virtual bool nonscript_usable() override;
    virtual void set_nonscript_usable(bool usable) override;
    virtual CScriptBinderObject* GetScriptBinderObject() override { return scriptBinder.object(); }
    virtual void SetScriptBinderObject(CScriptBinderObject* object) override { scriptBinder.set_object(object); }

protected:
    virtual void spawn_supplies();

private: // XXX: move to GameObjectBase
    void init();
    void setup_parent_ai_locations(bool assign_position = true);
    void validate_ai_locations(bool decrement_reference = true);
    u32 new_level_vertex_id() const;
    void update_ai_locations(bool decrement_reference);
    void SetKinematicsCallback(bool set);
};
#pragma pack(pop)

#endif // !defined(AFX_GAMEOBJECT_H__3DA72D03_C759_4688_AEBB_89FA812AA873__INCLUDED_)
