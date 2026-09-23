#pragma once
#include "TestSupport.h"

#include "xrCore/_fbox.h"
#include "xrEngine/Engine.h"

#include "xrEngine/xr_collide_form.h"
#include "xrEngine/xr_object.h"

class TestObject : public IGameObject
{
    SpatialData spatial{};
    ICollisionForm* form = nullptr;

public:
    TestObject(ISpatial_DB& space, const Fvector& center)
    {
        spatial.type = STYPE_COLLIDEABLE;
        spatial.space = &space;
        spatial.sphere.set(center, 0.1f);
        space.insert(this);
    }

    ~TestObject() override
    {
        spatial.space->remove(this);
    }
#ifdef DEBUG
    u32 GetDbgUpdateFrame() const override
    {
        throw std::runtime_error("Unexpected TestObject::GetDbgUpdateFrame");
    }
#endif
#ifdef DEBUG
    void SetDbgUpdateFrame(u32 value) override
    {
        throw std::runtime_error("Unexpected TestObject::SetDbgUpdateFrame");
    }
#endif
    u32 GetUpdateFrame() const override
    {
        throw std::runtime_error("Unexpected TestObject::GetUpdateFrame");
    }

    void SetUpdateFrame(u32 value) override
    {
        throw std::runtime_error("Unexpected TestObject::SetUpdateFrame");
    }

    u32 GetCrowUpdateFrame() const override
    {
        throw std::runtime_error("Unexpected TestObject::GetCrowUpdateFrame");
    }

    void SetCrowUpdateFrame(u32 value) override
    {
        throw std::runtime_error("Unexpected TestObject::SetCrowUpdateFrame");
    }
#ifdef DEBUG
    void DBGGetProps(GameObjectProperties& p) const override
    {
        throw std::runtime_error("Unexpected TestObject::DBGGetProps");
    }
#endif
    void MakeMeCrow() override
    {
        throw std::runtime_error("Unexpected TestObject::MakeMeCrow");
    }

    void IAmNotACrowAnyMore() override
    {
        throw std::runtime_error("Unexpected TestObject::IAmNotACrowAnyMore");
    }

    bool AlwaysTheCrow() override
    {
        throw std::runtime_error("Unexpected TestObject::AlwaysTheCrow");
    }

    bool AmICrow() const override
    {
        throw std::runtime_error("Unexpected TestObject::AmICrow");
    }

    bool Local() const override
    {
        throw std::runtime_error("Unexpected TestObject::Local");
    }

    bool Remote() const override
    {
        throw std::runtime_error("Unexpected TestObject::Remote");
    }

    u16 ID() const override
    {
        throw std::runtime_error("Unexpected TestObject::ID");
    }

    void setID(u16 id) override
    {
        throw std::runtime_error("Unexpected TestObject::setID");
    }

    bool Ready() override
    {
        throw std::runtime_error("Unexpected TestObject::Ready");
    }

    bool GetTmpPreDestroy() const override
    {
        throw std::runtime_error("Unexpected TestObject::GetTmpPreDestroy");
    }

    void SetTmpPreDestroy(bool b) override
    {
        throw std::runtime_error("Unexpected TestObject::SetTmpPreDestroy");
    }

    IGameObject* H_Parent() override
    {
        throw std::runtime_error("Unexpected TestObject::H_Parent");
    }

    const IGameObject* H_Parent() const override
    {
        throw std::runtime_error("Unexpected TestObject::H_Parent");
    }

    IGameObject* H_Root() override
    {
        throw std::runtime_error("Unexpected TestObject::H_Root");
    }

    const IGameObject* H_Root() const override
    {
        throw std::runtime_error("Unexpected TestObject::H_Root");
    }

    IGameObject* H_SetParent(IGameObject* obj, bool justBeforeDestroy = false) override
    {
        throw std::runtime_error("Unexpected TestObject::H_SetParent");
    }

    void Center(Fvector& center) const override
    {
        throw std::runtime_error("Unexpected TestObject::Center");
    }

    const Fmatrix& XFORM() const override
    {
        throw std::runtime_error("Unexpected TestObject::XFORM");
    }

    Fmatrix& XFORM() override
    {
        throw std::runtime_error("Unexpected TestObject::XFORM");
    }

    void spatial_update(float eps_P, float eps_R) override
    {
        throw std::runtime_error("Unexpected TestObject::spatial_update");
    }

    Fvector& Direction() override
    {
        throw std::runtime_error("Unexpected TestObject::Direction");
    }

    const Fvector& Direction() const override
    {
        throw std::runtime_error("Unexpected TestObject::Direction");
    }

    Fvector& Position() override
    {
        throw std::runtime_error("Unexpected TestObject::Position");
    }

    const Fvector& Position() const override
    {
        throw std::runtime_error("Unexpected TestObject::Position");
    }

    float Radius() const override
    {
        throw std::runtime_error("Unexpected TestObject::Radius");
    }

    const Fbox& BoundingBox() const override
    {
        throw std::runtime_error("Unexpected TestObject::BoundingBox");
    }

    IRender_Sector::sector_id_t Sector() override
    {
        throw std::runtime_error("Unexpected TestObject::Sector");
    }

    IRender_ObjectSpecific* ROS() override
    {
        throw std::runtime_error("Unexpected TestObject::ROS");
    }

    IRenderVisual* Visual() const override
    {
        throw std::runtime_error("Unexpected TestObject::Visual");
    }

    void OnChangeVisual() override
    {
        throw std::runtime_error("Unexpected TestObject::OnChangeVisual");
    }

    IPhysicsShell* physics_shell() override
    {
        throw std::runtime_error("Unexpected TestObject::physics_shell");
    }

    const IObjectPhysicsCollision* physics_collision() override
    {
        throw std::runtime_error("Unexpected TestObject::physics_collision");
    }

    shared_str cName() const override
    {
        throw std::runtime_error("Unexpected TestObject::cName");
    }

    void cName_set(shared_str N) override
    {
        throw std::runtime_error("Unexpected TestObject::cName_set");
    }

    shared_str cNameSect() const override
    {
        throw std::runtime_error("Unexpected TestObject::cNameSect");
    }

    pcstr cNameSect_str() const override
    {
        throw std::runtime_error("Unexpected TestObject::cNameSect_str");
    }

    void cNameSect_set(shared_str N) override
    {
        throw std::runtime_error("Unexpected TestObject::cNameSect_set");
    }

    shared_str cNameVisual() const override
    {
        throw std::runtime_error("Unexpected TestObject::cNameVisual");
    }

    void cNameVisual_set(shared_str N) override
    {
        throw std::runtime_error("Unexpected TestObject::cNameVisual_set");
    }

    void processing_activate() override
    {
        throw std::runtime_error("Unexpected TestObject::processing_activate");
    }

    void processing_deactivate() override
    {
        throw std::runtime_error("Unexpected TestObject::processing_deactivate");
    }

    bool processing_enabled() override
    {
        throw std::runtime_error("Unexpected TestObject::processing_enabled");
    }

    void setVisible(bool _visible) override
    {
        throw std::runtime_error("Unexpected TestObject::setVisible");
    }

    bool getVisible() const override
    {
        throw std::runtime_error("Unexpected TestObject::getVisible");
    }

    void setEnabled(bool _enabled) override
    {
        throw std::runtime_error("Unexpected TestObject::setEnabled");
    }

    bool getEnabled() const override
    {
        throw std::runtime_error("Unexpected TestObject::getEnabled");
    }

    void setDestroy(bool _destroy) override
    {
        throw std::runtime_error("Unexpected TestObject::setDestroy");
    }

    bool getDestroy() const override
    {
        throw std::runtime_error("Unexpected TestObject::getDestroy");
    }

    void setLocal(bool _local) override
    {
        throw std::runtime_error("Unexpected TestObject::setLocal");
    }

    bool getLocal() const override
    {
        throw std::runtime_error("Unexpected TestObject::getLocal");
    }

    void setSVU(bool _svu) override
    {
        throw std::runtime_error("Unexpected TestObject::setSVU");
    }

    bool getSVU() const override
    {
        throw std::runtime_error("Unexpected TestObject::getSVU");
    }

    void setReady(bool _ready) override
    {
        throw std::runtime_error("Unexpected TestObject::setReady");
    }

    bool getReady() const override
    {
        throw std::runtime_error("Unexpected TestObject::getReady");
    }

    void Load(pcstr section) override
    {
        throw std::runtime_error("Unexpected TestObject::Load");
    }

    void PostLoad(pcstr section) override
    {
        throw std::runtime_error("Unexpected TestObject::PostLoad");
    }

    void PreUpdateCL() override
    {
        throw std::runtime_error("Unexpected TestObject::PreUpdateCL");
    }

    void UpdateCL() override
    {
        throw std::runtime_error("Unexpected TestObject::UpdateCL");
    }

    void PostUpdateCL(bool bUpdateCL_disabled) override
    {
        throw std::runtime_error("Unexpected TestObject::PostUpdateCL");
    }

    u32 ps_Size() const override
    {
        throw std::runtime_error("Unexpected TestObject::ps_Size");
    }

    GameObjectSavedPosition ps_Element(u32 id) const override
    {
        throw std::runtime_error("Unexpected TestObject::ps_Element");
    }

    void ForceTransform(const Fmatrix& m) override
    {
        throw std::runtime_error("Unexpected TestObject::ForceTransform");
    }

    void ForceTransformAndDirection(const Fmatrix& m) override
    {
        throw std::runtime_error("Unexpected TestObject::ForceTransformAndDirection");
    }

    void OnHUDDraw(u32 context_id, CCustomHUD* hud, IRenderable* root) override
    {
        throw std::runtime_error("Unexpected TestObject::OnHUDDraw");
    }

    void OnRenderHUD(IGameObject* pCurViewEntity) override
    {
        throw std::runtime_error("Unexpected TestObject::OnRenderHUD");
    }

    void OnOwnedCameraMove(CCameraBase* pCam, float fOldYaw, float fOldPitch) override
    {
        throw std::runtime_error("Unexpected TestObject::OnOwnedCameraMove");
    }

    void OnH_B_Chield() override
    {
        throw std::runtime_error("Unexpected TestObject::OnH_B_Chield");
    }

    void OnH_B_Independent(bool justBeforeDestroy) override
    {
        throw std::runtime_error("Unexpected TestObject::OnH_B_Independent");
    }

    void OnH_A_Chield() override
    {
        throw std::runtime_error("Unexpected TestObject::OnH_A_Chield");
    }

    void OnH_A_Independent() override
    {
        throw std::runtime_error("Unexpected TestObject::OnH_A_Independent");
    }

    void On_SetEntity() override
    {
        throw std::runtime_error("Unexpected TestObject::On_SetEntity");
    }

    void On_LostEntity() override
    {
        throw std::runtime_error("Unexpected TestObject::On_LostEntity");
    }

    bool register_schedule() const override
    {
        throw std::runtime_error("Unexpected TestObject::register_schedule");
    }

    Fvector get_new_local_point_on_mesh(u16& boneId) const override
    {
        throw std::runtime_error("Unexpected TestObject::get_new_local_point_on_mesh");
    }

    Fvector get_last_local_point_on_mesh(const Fvector& lastPoint, u16 boneId) const override
    {
        throw std::runtime_error("Unexpected TestObject::get_last_local_point_on_mesh");
    }

    CAttachmentOwner* cast_attachment_owner() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_attachment_owner");
    }

    CInventoryOwner* cast_inventory_owner() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_inventory_owner");
    }

    CInventoryItem* cast_inventory_item() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_inventory_item");
    }

    CEntity* cast_entity() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_entity");
    }

    CEntityAlive* cast_entity_alive() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_entity_alive");
    }

    CActor* cast_actor() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_actor");
    }

    CCustomZone* cast_custom_zone() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_custom_zone");
    }

    CPhysicsShellHolder* cast_physics_shell_holder() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_physics_shell_holder");
    }

    IInputReceiver* cast_input_receiver() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_input_receiver");
    }

    CParticlesPlayer* cast_particles_player() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_particles_player");
    }

    CArtefact* cast_artefact() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_artefact");
    }

    CCustomMonster* cast_custom_monster() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_custom_monster");
    }

    CAI_Stalker* cast_stalker() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_stalker");
    }

    CScriptEntity* cast_script_entity() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_script_entity");
    }

    CWeapon* cast_weapon() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_weapon");
    }

    CExplosive* cast_explosive() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_explosive");
    }

    CSpaceRestrictor* cast_restrictor() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_restrictor");
    }

    CAttachableItem* cast_attachable_item() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_attachable_item");
    }

    CHolderCustom* cast_holder_custom() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_holder_custom");
    }

    CBaseMonster* cast_base_monster() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_base_monster");
    }

    CShellLauncher* cast_shell_launcher() override
    {
        throw std::runtime_error("Unexpected TestObject::cast_shell_launcher");
    }

    bool feel_touch_on_contact(IGameObject* obj) override
    {
        throw std::runtime_error("Unexpected TestObject::feel_touch_on_contact");
    }

    void net_Save(NET_Packet& packet) override
    {
        throw std::runtime_error("Unexpected TestObject::net_Save");
    }

    void net_Load(IReader& reader) override
    {
        throw std::runtime_error("Unexpected TestObject::net_Load");
    }

    bool net_SaveRelevant() override
    {
        throw std::runtime_error("Unexpected TestObject::net_SaveRelevant");
    }

    void net_Export(NET_Packet& packet) override
    {
        throw std::runtime_error("Unexpected TestObject::net_Export");
    }

    void net_Import(NET_Packet& packet) override
    {
        throw std::runtime_error("Unexpected TestObject::net_Import");
    }

    bool net_Spawn(CSE_Abstract* entity) override
    {
        throw std::runtime_error("Unexpected TestObject::net_Spawn");
    }

    void net_Destroy() override
    {
        throw std::runtime_error("Unexpected TestObject::net_Destroy");
    }

    void net_ImportInput(NET_Packet& packet) override
    {
        throw std::runtime_error("Unexpected TestObject::net_ImportInput");
    }

    bool net_Relevant() override
    {
        throw std::runtime_error("Unexpected TestObject::net_Relevant");
    }

    void net_MigrateInactive(NET_Packet& packet) override
    {
        throw std::runtime_error("Unexpected TestObject::net_MigrateInactive");
    }

    void net_MigrateActive(NET_Packet& packet) override
    {
        throw std::runtime_error("Unexpected TestObject::net_MigrateActive");
    }

    void net_Relcase(IGameObject* obj) override
    {
        throw std::runtime_error("Unexpected TestObject::net_Relcase");
    }

    void save(NET_Packet& packet) override
    {
        throw std::runtime_error("Unexpected TestObject::save");
    }

    void load(IReader& reader) override
    {
        throw std::runtime_error("Unexpected TestObject::load");
    }

    void OnEvent(NET_Packet& packet, u16 type) override
    {
        throw std::runtime_error("Unexpected TestObject::OnEvent");
    }

    void Hit(SHit* hit) override
    {
        throw std::runtime_error("Unexpected TestObject::Hit");
    }

    void SetHitInfo(IGameObject* who, IGameObject* weapon, s16 element, Fvector pos, Fvector dir) override
    {
        throw std::runtime_error("Unexpected TestObject::SetHitInfo");
    }

    bool BonePassBullet(int boneId) override
    {
        throw std::runtime_error("Unexpected TestObject::BonePassBullet");
    }

    pcstr Name() const override
    {
        throw std::runtime_error("Unexpected TestObject::Name");
    }

    bool IsVisibleForZones() override
    {
        throw std::runtime_error("Unexpected TestObject::IsVisibleForZones");
    }

    bool NeedToDestroyObject() const override
    {
        throw std::runtime_error("Unexpected TestObject::NeedToDestroyObject");
    }

    void DestroyObject() override
    {
        throw std::runtime_error("Unexpected TestObject::DestroyObject");
    }

    void create_anim_mov_ctrl(CBlend* blend, Fmatrix* startPos, bool localAnimation) override
    {
        throw std::runtime_error("Unexpected TestObject::create_anim_mov_ctrl");
    }

    void destroy_anim_mov_ctrl() override
    {
        throw std::runtime_error("Unexpected TestObject::destroy_anim_mov_ctrl");
    }

    void update_animation_movement_controller() override
    {
        throw std::runtime_error("Unexpected TestObject::update_animation_movement_controller");
    }

    bool animation_movement_controlled() const override
    {
        throw std::runtime_error("Unexpected TestObject::animation_movement_controlled");
    }

    const animation_movement_controller* animation_movement() const override
    {
        throw std::runtime_error("Unexpected TestObject::animation_movement");
    }

    animation_movement_controller* animation_movement() override
    {
        throw std::runtime_error("Unexpected TestObject::animation_movement");
    }

    bool UsedAI_Locations() override
    {
        throw std::runtime_error("Unexpected TestObject::UsedAI_Locations");
    }

    bool TestServerFlag(u32 flag) const override
    {
        throw std::runtime_error("Unexpected TestObject::TestServerFlag");
    }

    bool can_validate_position_on_spawn() override
    {
        throw std::runtime_error("Unexpected TestObject::can_validate_position_on_spawn");
    }

    bool ShouldProcessOnRender() const override
    {
        throw std::runtime_error("Unexpected TestObject::ShouldProcessOnRender");
    }

    void ShouldProcessOnRender(bool should_process) override
    {
        throw std::runtime_error("Unexpected TestObject::ShouldProcessOnRender");
    }

    void OnRender() override
    {
        throw std::runtime_error("Unexpected TestObject::OnRender");
    }

    void reinit() override
    {
        throw std::runtime_error("Unexpected TestObject::reinit");
    }

    void reload(pcstr section) override
    {
        throw std::runtime_error("Unexpected TestObject::reload");
    }

    bool object_removed() const override
    {
        throw std::runtime_error("Unexpected TestObject::object_removed");
    }

    void make_Interpolation() override
    {
        throw std::runtime_error("Unexpected TestObject::make_Interpolation");
    }

    void PH_B_CrPr() override
    {
        throw std::runtime_error("Unexpected TestObject::PH_B_CrPr");
    }

    void PH_I_CrPr() override
    {
        throw std::runtime_error("Unexpected TestObject::PH_I_CrPr");
    }

    void PH_Ch_CrPr() override
    {
        throw std::runtime_error("Unexpected TestObject::PH_Ch_CrPr");
    }

    void dbg_DrawSkeleton() override
    {
        throw std::runtime_error("Unexpected TestObject::dbg_DrawSkeleton");
    }

    void PH_A_CrPr() override
    {
        throw std::runtime_error("Unexpected TestObject::PH_A_CrPr");
    }

    void CrPr_SetActivationStep(u32 step) override
    {
        throw std::runtime_error("Unexpected TestObject::CrPr_SetActivationStep");
    }

    u32 CrPr_GetActivationStep() override
    {
        throw std::runtime_error("Unexpected TestObject::CrPr_GetActivationStep");
    }

    void CrPr_SetActivated(bool activate) override
    {
        throw std::runtime_error("Unexpected TestObject::CrPr_SetActivated");
    }

    bool CrPr_IsActivated() override
    {
        throw std::runtime_error("Unexpected TestObject::CrPr_IsActivated");
    }

    const SRotation Orientation() const override
    {
        throw std::runtime_error("Unexpected TestObject::Orientation");
    }

    bool use_parent_ai_locations() const override
    {
        throw std::runtime_error("Unexpected TestObject::use_parent_ai_locations");
    }

    void add_visual_callback(visual_callback callback) override
    {
        throw std::runtime_error("Unexpected TestObject::add_visual_callback");
    }

    void remove_visual_callback(visual_callback callback) override
    {
        throw std::runtime_error("Unexpected TestObject::remove_visual_callback");
    }

    CALLBACK_VECTOR& visual_callbacks() override
    {
        throw std::runtime_error("Unexpected TestObject::visual_callbacks");
    }

    CScriptGameObject* lua_game_object() const override
    {
        throw std::runtime_error("Unexpected TestObject::lua_game_object");
    }

    int clsid() const override
    {
        throw std::runtime_error("Unexpected TestObject::clsid");
    }

    CInifile* spawn_ini() override
    {
        throw std::runtime_error("Unexpected TestObject::spawn_ini");
    }

    CAI_ObjectLocation& ai_location() const override
    {
        throw std::runtime_error("Unexpected TestObject::ai_location");
    }

    bool is_spawned() const override
    {
        throw std::runtime_error("Unexpected TestObject::is_spawned");
    }

    u32 spawn_time() const override
    {
        throw std::runtime_error("Unexpected TestObject::spawn_time");
    }

    const ALife::_STORY_ID& story_id() const override
    {
        throw std::runtime_error("Unexpected TestObject::story_id");
    }

    u32 ef_creature_type() const override
    {
        throw std::runtime_error("Unexpected TestObject::ef_creature_type");
    }

    u32 ef_equipment_type() const override
    {
        throw std::runtime_error("Unexpected TestObject::ef_equipment_type");
    }

    u32 ef_main_weapon_type() const override
    {
        throw std::runtime_error("Unexpected TestObject::ef_main_weapon_type");
    }

    u32 ef_anomaly_type() const override
    {
        throw std::runtime_error("Unexpected TestObject::ef_anomaly_type");
    }

    u32 ef_weapon_type() const override
    {
        throw std::runtime_error("Unexpected TestObject::ef_weapon_type");
    }

    u32 ef_detector_type() const override
    {
        throw std::runtime_error("Unexpected TestObject::ef_detector_type");
    }

    bool natural_weapon() const override
    {
        throw std::runtime_error("Unexpected TestObject::natural_weapon");
    }

    bool natural_detector() const override
    {
        throw std::runtime_error("Unexpected TestObject::natural_detector");
    }

    bool use_center_to_aim() const override
    {
        throw std::runtime_error("Unexpected TestObject::use_center_to_aim");
    }

    void MoveTo(const Fvector& pos) override
    {
        throw std::runtime_error("Unexpected TestObject::MoveTo");
    }

    CScriptCallbackExVoid& callback(GameObject::ECallbackType type) const override
    {
        throw std::runtime_error("Unexpected TestObject::callback");
    }

    pcstr visual_name(CSE_Abstract* entity) override
    {
        throw std::runtime_error("Unexpected TestObject::visual_name");
    }

    void On_B_NotCurrentEntity() override
    {
        throw std::runtime_error("Unexpected TestObject::On_B_NotCurrentEntity");
    }

    bool is_ai_obstacle() const override
    {
        throw std::runtime_error("Unexpected TestObject::is_ai_obstacle");
    }

    ai_obstacle& obstacle() const override
    {
        throw std::runtime_error("Unexpected TestObject::obstacle");
    }

    void on_matrix_change(const Fmatrix& prev) override
    {
        throw std::runtime_error("Unexpected TestObject::on_matrix_change");
    }

    bool use(IGameObject* obj) override
    {
        throw std::runtime_error("Unexpected TestObject::use");
    }

    pcstr tip_text() override
    {
        throw std::runtime_error("Unexpected TestObject::tip_text");
    }

    void set_tip_text(pcstr text) override
    {
        throw std::runtime_error("Unexpected TestObject::set_tip_text");
    }

    void set_tip_text_default() override
    {
        throw std::runtime_error("Unexpected TestObject::set_tip_text_default");
    }

    bool nonscript_usable() override
    {
        throw std::runtime_error("Unexpected TestObject::nonscript_usable");
    }

    void set_nonscript_usable(bool usable) override
    {
        throw std::runtime_error("Unexpected TestObject::set_nonscript_usable");
    }

    CScriptBinderObject* GetScriptBinderObject() override
    {
        throw std::runtime_error("Unexpected TestObject::GetScriptBinderObject");
    }

    void SetScriptBinderObject(CScriptBinderObject* obj) override
    {
        throw std::runtime_error("Unexpected TestObject::SetScriptBinderObject");
    }

    CLASS_ID& GetClassId() override
    {
        throw std::runtime_error("Unexpected TestObject::GetClassId");
    }

    IFactoryObject* _construct() override
    {
        throw std::runtime_error("Unexpected TestObject::_construct");
    }

    SpatialData& GetSpatialData() override
    {
        return spatial;
    }

    bool spatial_inside() override
    {
        return true;
    }

    void spatial_register() override
    {
        throw std::runtime_error("Unexpected TestObject::spatial_register");
    }

    void spatial_unregister() override
    {
        throw std::runtime_error("Unexpected TestObject::spatial_unregister");
    }

    void spatial_move() override
    {
        throw std::runtime_error("Unexpected TestObject::spatial_move");
    }

    Fvector spatial_sector_point() override
    {
        throw std::runtime_error("Unexpected TestObject::spatial_sector_point");
    }

    void spatial_updatesector(IRender_Sector::sector_id_t sector_id) override
    {
        throw std::runtime_error("Unexpected TestObject::spatial_updatesector");
    }

    IGameObject* dcast_GameObject() override
    {
        return this;
    }

    Feel::Sound* dcast_FeelSound() override
    {
        throw std::runtime_error("Unexpected TestObject::dcast_FeelSound");
    }

    IRenderable* dcast_Renderable() override
    {
        throw std::runtime_error("Unexpected TestObject::dcast_Renderable");
    }

    IRender_Light* dcast_Light() override
    {
        throw std::runtime_error("Unexpected TestObject::dcast_Light");
    }

    SchedulerData& GetSchedulerData() override
    {
        throw std::runtime_error("Unexpected TestObject::GetSchedulerData");
    }

    float shedule_Scale() const override
    {
        throw std::runtime_error("Unexpected TestObject::shedule_Scale");
    }

    void shedule_Update(u32 dt) override
    {
        throw std::runtime_error("Unexpected TestObject::shedule_Update");
    }

    shared_str shedule_Name() const override
    {
        throw std::runtime_error("Unexpected TestObject::shedule_Name");
    }

    bool shedule_Needed() override
    {
        throw std::runtime_error("Unexpected TestObject::shedule_Needed");
    }

    RenderData& GetRenderData() override
    {
        throw std::runtime_error("Unexpected TestObject::GetRenderData");
    }

    void renderable_Render(u32 context_id, IRenderable* root) override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_Render");
    }

    IRender_ObjectSpecific* renderable_ROS() override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_ROS");
    }

    bool renderable_ShadowGenerate() override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_ShadowGenerate");
    }

    bool renderable_ShadowReceive() override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_ShadowReceive");
    }

    bool renderable_Invisible() override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_Invisible");
    }

    void renderable_Invisible(bool value) override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_Invisible");
    }

    bool renderable_HUD() override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_HUD");
    }

    void renderable_HUD(bool value) override
    {
        throw std::runtime_error("Unexpected TestObject::renderable_HUD");
    }

    void SetCForm(ICollisionForm* cform) override
    {
        form = cform;
    }

    ICollisionForm* GetCForm() const override
    {
        return form;
    }
};
