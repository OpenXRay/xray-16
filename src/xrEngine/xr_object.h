#pragma once

#include "xrCDB/ISpatial.h"
#include "ISheduled.h"

#include "IRenderable.h"
#include "ICollidable.h"
#include "EngineAPI.h"
#include "device.h"

#include "xrServerEntities/xrServer_Space.h"
#include "xrGame/game_object_space.h"
#include "xrServerEntities/alife_space.h"
#include "xrCommon/misc_math_types.h" // SRotation

// fwd. decl.
class IRender_Sector;
class IRender_ObjectSpecific;
class CCustomHUD;
class NET_Packet;
class CSE_Abstract;
class CInifile;

//-----------------------------------------------------------------------------------------------------------
#define CROW_RADIUS (30.f)
#define CROW_RADIUS2 (60.f)

class IPhysicsShell;
xr_pure_interface IObjectPhysicsCollision;

class CAttachmentOwner;
class CInventoryOwner;
class CInventoryItem;
class CEntity;
class CEntityAlive;
class CActor;
class CGameObject; // XXX: remove
class CCustomZone;
class CPhysicsShellHolder;
class IInputReceiver;
class CParticlesPlayer;
class CArtefact;
class CCustomMonster;
class CAI_Stalker;
class CScriptEntity;
class CWeapon;
class CExplosive;
class CSpaceRestrictor;
class CAttachableItem;
class CHolderCustom;
class CBaseMonster;
class CShellLauncher; //--#SM+#--
class CBlend;
struct SHit;
class CScriptGameObject;
class CAI_ObjectLocation;
class CScriptBinderObject;
class ai_obstacle;
class animation_movement_controller;
class CCameraBase; //--#SM+#--

template <typename TResult>
class CScriptCallbackEx;

struct GameObjectSavedPosition
{
    u32 dwTime;
    Fvector vPosition;
};

union GameObjectProperties
{
    struct
    {
        u32 net_ID : 16;
        u32 bActiveCounter : 8;
        u32 bEnabled : 1;
        u32 bVisible : 1;
        u32 bDestroy : 1;
        u32 net_Local : 1;
        u32 net_Ready : 1;
        u32 net_SV_Update : 1;
        u32 crow : 1;
        u32 bPreDestroy : 1;
    };
    u32 storage;
};

class IGameObject : public virtual IFactoryObject,
                    public virtual ISpatial,
                    public virtual ISheduled,
                    public virtual IRenderable,
                    public virtual ICollidable
{
public:
    using visual_callback = void(__stdcall*)(IKinematics*);
    using CALLBACK_VECTOR = svector<visual_callback, 6>;
    using CALLBACK_VECTOR_IT = CALLBACK_VECTOR::iterator;
    using CScriptCallbackExVoid = CScriptCallbackEx<void>;
    // typedef xr_map<GameObject::ECallbackType, CScriptCallbackExVoid> CALLBACK_MAP;
    // typedef CALLBACK_MAP::iterator CALLBACK_MAP_IT;

    virtual ~IGameObject() = 0;
// derived interfaces: some functions declared as final in base classes
// IFactoryObject
// virtual CLASS_ID &GetClassId() override = 0;
// virtual IFactoryObject *_construct() override = 0;
// ~IFactoryObject
// ISpatial
// virtual bool spatial_inside() override = 0;
// virtual void spatial_register() override = 0;
// virtual void spatial_unregister() override = 0;
// virtual void spatial_move() override = 0;
// virtual Fvector spatial_sector_point() override = 0;
// virtual void spatial_updatesector() override = 0;
// virtual IGameObject *dcast_GameObject() override = 0;
// virtual Feel::Sound *dcast_FeelSound() override = 0;
// virtual IRenderable *dcast_Renderable() override = 0;
// virtual IRender_Light *dcast_Light() override = 0;
// ~ISpatial
// IScheduled
// virtual SchedulerData &GetSchedulerData() override = 0;
// virtual float shedule_Scale() override = 0;
// virtual void shedule_Update(u32 dt) override = 0; // Called by sheduler
// virtual shared_str shedule_Name() const override = 0;
// virtual bool shedule_Needed() override = 0;
// ~ISheduled
// IRenderable
// virtual RenderData &GetRenderData() override = 0;
// virtual void renderable_Render(IRenderable* root) override = 0;
// virtual IRender_ObjectSpecific *renderable_ROS() override = 0;
// virtual bool renderable_ShadowGenerate() override = 0;
// virtual bool renderable_ShadowReceive() override = 0;
// ~IRenderable
// ICollidable
// virtual void SetCForm(ICollisionForm *cform) override = 0;
// virtual ICollisionForm *GetCForm() const override = 0;
// ~ICollidable
#ifdef DEBUG
    virtual u32 GetDbgUpdateFrame() const = 0;
    virtual void SetDbgUpdateFrame(u32 value) = 0;
#endif
    virtual u32 GetUpdateFrame() const = 0;
    virtual void SetUpdateFrame(u32 value) = 0;
    virtual u32 GetCrowUpdateFrame() const = 0;
    virtual void SetCrowUpdateFrame(u32 value) = 0;
// Crow-MODE
// if (object_is_visible)
// if (object_is_near)
// if (object_is_crow_always)
#ifdef DEBUG
    virtual void DBGGetProps(GameObjectProperties& p) const = 0;
#endif
    virtual void MakeMeCrow() = 0;
    virtual void IAmNotACrowAnyMore() = 0;
    virtual bool AlwaysTheCrow() = 0;
    virtual bool AmICrow() const = 0;
    // Network
    virtual bool Local() const = 0;
    virtual bool Remote() const = 0;
    virtual u16 ID() const = 0;
    virtual void setID(u16 id) = 0;
    virtual bool Ready() = 0;
    virtual bool GetTmpPreDestroy() const = 0;
    virtual void SetTmpPreDestroy(bool b) = 0;
    // Parentness
    virtual IGameObject* H_Parent() = 0;
    virtual const IGameObject* H_Parent() const = 0;
    virtual IGameObject* H_Root() = 0;
    virtual const IGameObject* H_Root() const = 0;
    virtual IGameObject* H_SetParent(IGameObject* obj, bool justBeforeDestroy = false) = 0;
    // Geometry xform
    virtual void Center(Fvector& center) const = 0;
    virtual const Fmatrix& XFORM() const = 0;
    virtual Fmatrix& XFORM() = 0;
    virtual void spatial_update(float eps_P, float eps_R) = 0;
    virtual Fvector& Direction() = 0;
    virtual const Fvector& Direction() const = 0;
    virtual Fvector& Position() = 0;
    virtual const Fvector& Position() const = 0;
    virtual float Radius() const = 0;
    virtual const Fbox& BoundingBox() const = 0;
    virtual IRender_Sector* Sector() = 0;
    virtual IRender_ObjectSpecific* ROS() = 0;
    // Accessors and converters
    virtual IRenderVisual* Visual() const = 0;
    virtual void OnChangeVisual() = 0;
    virtual IPhysicsShell* physics_shell() = 0;
    virtual const IObjectPhysicsCollision* physics_collision() = 0;
    // Name management
    virtual shared_str cName() const = 0;
    virtual void cName_set(shared_str N) = 0;
    virtual shared_str cNameSect() const = 0;
    virtual pcstr cNameSect_str() const = 0;
    virtual void cNameSect_set(shared_str N) = 0;
    virtual shared_str cNameVisual() const = 0;
    virtual void cNameVisual_set(shared_str N) = 0;
    // Properties
    virtual void processing_activate() = 0; // request to enable UpdateCL
    virtual void processing_deactivate() = 0; // request to disable UpdateCL
    virtual bool processing_enabled() = 0;
    virtual void setVisible(bool _visible) = 0;
    virtual bool getVisible() const = 0;
    virtual void setEnabled(bool _enabled) = 0;
    virtual bool getEnabled() const = 0;
    virtual void setDestroy(bool _destroy) = 0;
    virtual bool getDestroy() const = 0;
    virtual void setLocal(bool _local) = 0;
    virtual bool getLocal() const = 0;
    virtual void setSVU(bool _svu) = 0;
    virtual bool getSVU() const = 0;
    virtual void setReady(bool _ready) = 0;
    virtual bool getReady() const = 0;
    // ~Properties
    virtual void Load(pcstr section) = 0;
    virtual void PostLoad(pcstr section) = 0; //--#SM+#--
    // Update
    virtual void PreUpdateCL() = 0;
    virtual void UpdateCL() = 0; // Called each frame, so no need for dt
    virtual void PostUpdateCL(bool bUpdateCL_disabled) = 0; //--#SM+#-- Вызывается всегда, в отличии от UpdateCL [called always for object regardless of it being active\sleep]
    // Position stack
    virtual u32 ps_Size() const = 0;
    virtual GameObjectSavedPosition ps_Element(u32 id) const = 0;
    virtual void ForceTransform(const Fmatrix& m) = 0;
    virtual void ForceTransformAndDirection(const Fmatrix& m) = 0;
    // HUD
    virtual void OnHUDDraw(CCustomHUD* hud, IRenderable* root) = 0;
    virtual void OnRenderHUD(IGameObject* pCurViewEntity) = 0; //--#SM+#--
    virtual void OnOwnedCameraMove(CCameraBase* pCam, float fOldYaw, float fOldPitch) = 0; //--#SM+#--
    // Active/non active
    virtual void OnH_B_Chield() = 0; // before
    virtual void OnH_B_Independent(bool justBeforeDestroy) = 0;
    virtual void OnH_A_Chield() = 0; // after
    virtual void OnH_A_Independent() = 0;
    virtual void On_SetEntity() = 0;
    virtual void On_LostEntity() = 0;
    virtual bool register_schedule() const = 0;
    virtual Fvector get_new_local_point_on_mesh(u16& boneId) const = 0;
    virtual Fvector get_last_local_point_on_mesh(const Fvector& lastPoint, u16 boneId) const = 0;
    // CGameObject
    // functions used for avoiding most of the smart_cast
    virtual CAttachmentOwner* cast_attachment_owner() = 0;
    virtual CInventoryOwner* cast_inventory_owner() = 0;
    virtual CInventoryItem* cast_inventory_item() = 0;
    virtual CEntity* cast_entity() = 0;
    virtual CEntityAlive* cast_entity_alive() = 0;
    virtual CActor* cast_actor() = 0;
    virtual CGameObject* cast_game_object() = 0;
    virtual CCustomZone* cast_custom_zone() = 0;
    virtual CPhysicsShellHolder* cast_physics_shell_holder() = 0;
    virtual IInputReceiver* cast_input_receiver() = 0;
    virtual CParticlesPlayer* cast_particles_player() = 0;
    virtual CArtefact* cast_artefact() = 0;
    virtual CCustomMonster* cast_custom_monster() = 0;
    virtual CAI_Stalker* cast_stalker() = 0;
    virtual CScriptEntity* cast_script_entity() = 0;
    virtual CWeapon* cast_weapon() = 0;
    virtual CExplosive* cast_explosive() = 0;
    virtual CSpaceRestrictor* cast_restrictor() = 0;
    virtual CAttachableItem* cast_attachable_item() = 0;
    virtual CHolderCustom* cast_holder_custom() = 0;
    virtual CBaseMonster* cast_base_monster() = 0;
    virtual CShellLauncher* cast_shell_launcher() = 0; //--#SM+#--
    virtual bool feel_touch_on_contact(IGameObject* obj) = 0;
    // Utilities
    // XXX: move out
    // static void u_EventGen(NET_Packet& P, u32 type, u32 dest);
    // static void u_EventSend(NET_Packet& P, u32 dwFlags = DPNSEND_GUARANTEED);
    // Methods
    // object serialization
    virtual void net_Save(NET_Packet& packet) = 0;
    virtual void net_Load(IReader& reader) = 0;
    virtual bool net_SaveRelevant() = 0;
    virtual void net_Export(NET_Packet& packet) = 0; // export to server
    virtual void net_Import(NET_Packet& packet) = 0; // import from server
    virtual bool net_Spawn(CSE_Abstract* entity) = 0;
    virtual void net_Destroy() = 0;
    virtual void net_ImportInput(NET_Packet& packet) = 0;
    virtual bool net_Relevant() = 0; // relevant for export to server
    virtual void net_MigrateInactive(NET_Packet& packet) = 0;
    virtual void net_MigrateActive(NET_Packet& packet) = 0;
    virtual void net_Relcase(IGameObject* obj) = 0; // destroy all links to another objects
    virtual void save(NET_Packet& packet) = 0;
    virtual void load(IReader& reader) = 0;
    virtual void OnEvent(NET_Packet& packet, u16 type) = 0;
    virtual void Hit(SHit* hit) = 0;
    virtual void SetHitInfo(IGameObject* who, IGameObject* weapon, s16 element, Fvector pos, Fvector dir) = 0;
    virtual bool BonePassBullet(int boneId) = 0;
    // игровое имя объекта
    virtual pcstr Name() const = 0;
    virtual bool IsVisibleForZones() = 0;
    virtual bool NeedToDestroyObject() const = 0;
    virtual void DestroyObject() = 0;
    // animation_movement_controller
    virtual void create_anim_mov_ctrl(CBlend* blend, Fmatrix* startPos, bool localAnimation) = 0;
    virtual void destroy_anim_mov_ctrl() = 0;
    virtual void update_animation_movement_controller() = 0;
    virtual bool animation_movement_controlled() const = 0;
    virtual const animation_movement_controller* animation_movement() const = 0;
    virtual animation_movement_controller* animation_movement() = 0;
    // Game-specific events
    virtual bool UsedAI_Locations() = 0;
    virtual bool TestServerFlag(u32 flag) const = 0;
    virtual bool can_validate_position_on_spawn() = 0;
#ifdef DEBUG
    virtual bool ShouldProcessOnRender() const = 0;
    virtual void ShouldProcessOnRender(bool should_process) = 0;
    virtual void OnRender() = 0;
#endif
    virtual void reinit() = 0;
    virtual void reload(pcstr section) = 0;
    // network
    virtual bool object_removed() const = 0;
    virtual void make_Interpolation() = 0; // interpolation from last visible to corrected position/rotation
    virtual void PH_B_CrPr() = 0; // actions & operations before physic correction-prediction steps
    virtual void PH_I_CrPr() = 0; // actions & operations after correction before prediction steps
#ifdef DEBUG
    virtual void PH_Ch_CrPr() = 0;
    virtual void dbg_DrawSkeleton() = 0;
#endif
    virtual void PH_A_CrPr() = 0; // actions & operations after phisic correction-prediction steps
    virtual void CrPr_SetActivationStep(u32 step) = 0;
    virtual u32 CrPr_GetActivationStep() = 0;
    virtual void CrPr_SetActivated(bool activate) = 0;
    virtual bool CrPr_IsActivated() = 0;
    // ~network
    virtual const SRotation Orientation() const = 0;
    virtual bool use_parent_ai_locations() const = 0;
    virtual void add_visual_callback(visual_callback callback) = 0;
    virtual void remove_visual_callback(visual_callback callback) = 0;
    virtual CALLBACK_VECTOR& visual_callbacks() = 0;
    virtual CScriptGameObject* lua_game_object() const = 0;
    virtual int clsid() const = 0;
    virtual CInifile* spawn_ini() = 0;
    virtual CAI_ObjectLocation& ai_location() const = 0;
    virtual u32 spawn_time() const = 0;
    virtual const ALife::_STORY_ID& story_id() const = 0;
    virtual u32 ef_creature_type() const = 0;
    virtual u32 ef_equipment_type() const = 0;
    virtual u32 ef_main_weapon_type() const = 0;
    virtual u32 ef_anomaly_type() const = 0;
    virtual u32 ef_weapon_type() const = 0;
    virtual u32 ef_detector_type() const = 0;
    virtual bool natural_weapon() const = 0;
    virtual bool natural_detector() const = 0;
    virtual bool use_center_to_aim() const = 0;
    virtual void MoveTo(const Fvector& pos) = 0;
    virtual CScriptCallbackExVoid& callback(GameObject::ECallbackType type) const = 0;
    virtual pcstr visual_name(CSE_Abstract* entity) = 0;
    virtual void On_B_NotCurrentEntity() = 0;
    virtual bool is_ai_obstacle() const = 0;
    virtual ai_obstacle& obstacle() const = 0;
    virtual void on_matrix_change(const Fmatrix& prev) = 0;
    // UsableScriptObject functions
    virtual bool use(IGameObject* obj) = 0;
    // строчка появляющаяся при наведении на объект (если NULL, то нет)
    virtual pcstr tip_text() = 0;
    virtual void set_tip_text(pcstr text) = 0;
    virtual void set_tip_text_default() = 0;
    // можно ли использовать объект стандартным (не скриптовым) образом
    virtual bool nonscript_usable() = 0;
    virtual void set_nonscript_usable(bool usable) = 0;
    virtual CScriptBinderObject* GetScriptBinderObject() = 0;
    virtual void SetScriptBinderObject(CScriptBinderObject* obj) = 0;
};

inline IGameObject::~IGameObject() {}
