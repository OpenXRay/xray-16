#include "StdAfx.h"
#include "PHWorld.h"
#include "PHActorCharacter.h"
#ifdef XRAY_GAMEPLAY_BENCHMARK
#include "GameplayBenchmark.h"
#endif
#include "PhysicsCommon.h"
#include "ExtendedGeom.h"
#include "PHCollideValidator.h"
#include "PHContactBodyEffector.h"
#include "params.h"
#ifdef DEBUG
#include "debug_output.h"
#endif

#include "xrServerEntities/PHSynchronize.h"
#include "xrServerEntities/PHNetState.h"
#include "GeometryBits.h"
#include "console_vars.h"
#include "PHCommander.h"
#include "PHSimpleCalls.h"
#include "xrCore/FS_internal.h"
#include "xrCDB/xr_area.h"
#include "xrEngine/defines.h"
#include "xrEngine/device.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"
#include "IPhysicsShellHolder.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"
#include "PhysicsShell.h"
#include "PHShell.h"
#include "PHElement.h"
#include "PHStaticGeomShell.h"
#include "PHCharacter.h"

CPHWorld* ph_world = 0;
static xr_map<BodyHandle, CPHContactBodyEffector> native_contact_effectors;
static void ApplyNativeContactEffectors()
{
    for (auto& [body, effector] : native_contact_effectors)
        if (GetPhysicsCore()->GetBodyMass(body) > 0.f) effector.Apply();
    native_contact_effectors.clear();
}

static void OnJoltBodyActivation(BodyHandle body, void* user_data, bool activated)
{
    if (!user_data) return;
    CPHElement* element = static_cast<CPHElement*>(user_data);
    CPHShell* shell = element->ph_shell();
    if (!shell || !shell->isActive()) return;

    IPhysicsShellHolder* holder = element->PhysicsRefObject();
    if (!holder || holder->ObjectGetDestroy() || holder->has_parent_object())
        return;

    if (activated)
    {
        if (!shell->isEnabled())
        {
            shell->EnableObject(nullptr);
        }
    }
    else
    {
        bool sleeping = true;
        for (u16 index = 0; index < shell->get_ElementsNumber(); ++index)
            if (GetPhysicsCore()->IsBodyActive(static_cast<CPHElement*>(shell->get_ElementByStoreOrder(index))->get_body())) sleeping = false;
        if (sleeping && shell->isEnabled())
        {
            shell->InterpolateGlobalTransform(&holder->ObjectXFORM());
            shell->DisableObject();
            holder->ObjectSpatialMove();
        }
    }
}

static u16 GetDefaultCreatureMaterial()
{
    static u16 s_creature_mtl = GAMEMTL_NONE_IDX;
    if (s_creature_mtl == GAMEMTL_NONE_IDX)
    {
        s_creature_mtl = GMLib.GetMaterialIdx("objects\\dead_body");
        if (s_creature_mtl == GAMEMTL_NONE_IDX)
            s_creature_mtl = GMLib.GetMaterialIdx("objects\\monster_body");
        if (s_creature_mtl == GAMEMTL_NONE_IDX)
            s_creature_mtl = GMLib.GetMaterialIdx("objects\\clothes");
        if (s_creature_mtl == GAMEMTL_NONE_IDX)
            s_creature_mtl = GMLib.GetMaterialIdx("creatures\\human");
        if (s_creature_mtl == GAMEMTL_NONE_IDX)
            s_creature_mtl = GMLib.GetMaterialIdx("materials\\cloth");
        if (s_creature_mtl == GAMEMTL_NONE_IDX)
            s_creature_mtl = GMLib.GetMaterialIdx("default");
    }
    return s_creature_mtl;
}

static PhysicsShapeHandle NativeCharacterPairShape(void* character, void* other)
{
    if (!character || !other) return nullptr;
    auto* actor = static_cast<CPHCharacter*>(character)->CastActorCharacter();
    if (!actor) return nullptr;
    auto* geometry = actor->NativeRestrictionGeometry(static_cast<CPHCharacter*>(other));
    return geometry ? geometry->geometry() : nullptr;
}

static bool OnJoltRBContact(const NativePhysicsContact& contact)
{
    if (!ph_world) return false;
    void* user_data_1 = contact.object1;
    void* user_data_2 = contact.object2;
    const auto layer_1 = contact.kind1, layer_2 = contact.kind2;
    const auto& pos = contact.position;
    const auto& norm = contact.normal;
    u16 mtl_1 = contact.material1, mtl_2 = contact.material2;

    IPhysicsShellHolder* holder_1 = nullptr;
    IPhysicsShellHolder* holder_2 = nullptr;
    CPhysicsGeom* geom_1 = nullptr;
    CPhysicsGeom* geom_2 = nullptr;

    if (user_data_1)
    {
        if (layer_1 == 0 || layer_1 == 1 || layer_1 == 3) // 1 = Layers::MOVING (Props / Boxes / Barrels)
        {
            CPHElement* elem1 = reinterpret_cast<CPHElement*>(user_data_1);
            holder_1 = elem1->PhysicsRefObject();
            if (contact.geometry1 < elem1->numberOfGeoms()) geom_1 = elem1->geometry(contact.geometry1);
            if (mtl_1 == GAMEMTL_NONE_IDX) {
                if (geom_1 && geom_1->material != GAMEMTL_NONE_IDX) mtl_1 = geom_1->material;
                else if (elem1->Material() != GAMEMTL_NONE_IDX) mtl_1 = elem1->Material();
            }
        }
        else if (layer_1 == 5) {
            auto* owner = static_cast<CPHStaticGeomShell*>(user_data_1);
            holder_1 = owner->PhysicsRefObject();
            if (contact.geometry1 < owner->numberOfGeoms()) geom_1 = owner->Geom(contact.geometry1);
            if (mtl_1 == GAMEMTL_NONE_IDX && geom_1) mtl_1 = geom_1->material;
        }
        else if (layer_1 == 6) {
            auto* character = static_cast<CPHCharacter*>(user_data_1);
            holder_1 = character->PhysicsRefObject();
            mtl_1 = character->GetMaterial();
        }
        else if (layer_1 == 2) // 2 = Layers::RAGDOLL (Stalkers / Monsters)
        {
            holder_1 = reinterpret_cast<IPhysicsShellHolder*>(user_data_1);
            mtl_1 = GetDefaultCreatureMaterial();
        }
    }

    if (user_data_2)
    {
        if (layer_2 == 0 || layer_2 == 1 || layer_2 == 3) // 1 = Layers::MOVING (Props / Boxes / Barrels)
        {
            CPHElement* elem2 = reinterpret_cast<CPHElement*>(user_data_2);
            holder_2 = elem2->PhysicsRefObject();
            if (contact.geometry2 < elem2->numberOfGeoms()) geom_2 = elem2->geometry(contact.geometry2);
            if (mtl_2 == GAMEMTL_NONE_IDX) {
                if (geom_2 && geom_2->material != GAMEMTL_NONE_IDX) mtl_2 = geom_2->material;
                else if (elem2->Material() != GAMEMTL_NONE_IDX) mtl_2 = elem2->Material();
            }
        }
        else if (layer_2 == 5) {
            auto* owner = static_cast<CPHStaticGeomShell*>(user_data_2);
            holder_2 = owner->PhysicsRefObject();
            if (contact.geometry2 < owner->numberOfGeoms()) geom_2 = owner->Geom(contact.geometry2);
            if (mtl_2 == GAMEMTL_NONE_IDX && geom_2) mtl_2 = geom_2->material;
        }
        else if (layer_2 == 6) {
            auto* character = static_cast<CPHCharacter*>(user_data_2);
            holder_2 = character->PhysicsRefObject();
            mtl_2 = character->GetMaterial();
        }
        else if (layer_2 == 2) // 2 = Layers::RAGDOLL (Stalkers / Monsters)
        {
            holder_2 = reinterpret_cast<IPhysicsShellHolder*>(user_data_2);
            mtl_2 = GetDefaultCreatureMaterial();
        }
    }

    if (holder_1 && holder_1->ObjectGetDestroy()) return false;
    if (holder_2 && holder_2->ObjectGetDestroy()) return false;

    Fsphere sph1, sph2;
    sph1.set(pos, 0.2f);
    sph2.set(pos, 0.2f);
    CSphereGeom dummy_g1(sph1);
    CSphereGeom dummy_g2(sph2);
    dummy_g1.contact_triangle = contact.triangle1;
    dummy_g2.contact_triangle = contact.triangle2;
    if (layer_1 == 6 && layer_2 == 6 && user_data_1 && user_data_2) {
        auto* first = static_cast<CPHCharacter*>(user_data_1);
        auto* second = static_cast<CPHCharacter*>(user_data_2);
        if (auto* actor = first->CastActorCharacter()) {
            geom_1 = actor->NativeRestrictionGeometry(second);
        }
        if (auto* actor = second->CastActorCharacter()) {
            geom_2 = actor->NativeRestrictionGeometry(first);
        }
    }

    if (!geom_1) {
        dummy_g1.ph_ref_object = holder_1;
        if (layer_1 == 6) dummy_g1.ph_object = static_cast<CPHCharacter*>(user_data_1);
        geom_1 = &dummy_g1;
    }
    if (!geom_2) {
        dummy_g2.ph_ref_object = holder_2;
        if (layer_2 == 6) dummy_g2.ph_object = static_cast<CPHCharacter*>(user_data_2);
        geom_2 = &dummy_g2;
    }


    if (mtl_1 == GAMEMTL_NONE_IDX || mtl_1 >= GMLib.CountMaterial())
        mtl_1 = GMLib.GetMaterialIdx("default_object");
    if (mtl_2 == GAMEMTL_NONE_IDX || mtl_2 >= GMLib.CountMaterial())
        mtl_2 = GMLib.GetMaterialIdx("default");

    SGameMtl* g_mtl1 = GMLib.GetMaterialByIdx(mtl_1);
    SGameMtl* g_mtl2 = GMLib.GetMaterialByIdx(mtl_2);
    struct ContactScope {
        CPhysicsGeom* geometry;
        const NativePhysicsContact* previous;
        ContactScope(CPhysicsGeom* value, const NativePhysicsContact& contact)
            : geometry(value), previous(value->contact_response) { geometry->contact_response = &contact; }
        ~ContactScope() { geometry->contact_response = previous; }
    } firstResponse(geom_1, contact), secondResponse(geom_2, contact);

    if (!g_mtl1 || !g_mtl2) return true;
    auto collectEffector = [&norm, &contact](CPhysicsGeom* geometry, SGameMtl* material, bool first) {
        const auto body = first ? contact.body1 : contact.body2;
        if (body == INVALID_BODY_HANDLE || !geometry->collide_fluids() ||
            !material->Flags.test(SGameMtl::flSlowDown)) return;
        const auto [entry, added] = native_contact_effectors.try_emplace(body);
        Fvector normal = norm;
        if (!first) normal.invert();
        if (added) entry->second.Init(body, normal, contact.depth, material);
        else entry->second.Merge(normal, contact.depth, material);
    };
    if (layer_1 == 1 || layer_1 == 3) collectEffector(geom_1, g_mtl2, true);
    if (layer_2 == 1 || layer_2 == 3) collectEffector(geom_2, g_mtl1, false);

    bool bo1 = (layer_1 == 1 || layer_1 == 2 || (holder_1 != nullptr && holder_2 == nullptr));

    bool do_collide = true;
    if (layer_1 == 6 && user_data_1) {
        auto* character = static_cast<CPHCharacter*>(user_data_1);
        if (layer_2 == 5 && user_data_2)
            static_cast<CPHStaticGeomShell*>(user_data_2)->near_callback(character);
        character->DispatchNativeContacts(do_collide, true, geom_1, geom_2, norm, pos, g_mtl1, g_mtl2);
        if (do_collide || g_mtl2->Flags.test(SGameMtl::flPassable)) {
            auto resolved = contact;
            resolved.material1 = mtl_1;
            resolved.material2 = mtl_2;
            character->ProcessNativeContact(resolved);
        }
    }
    if (layer_2 == 6 && user_data_2) {
        auto* character = static_cast<CPHCharacter*>(user_data_2);
        character->DispatchNativeContacts(do_collide, false, geom_1, geom_2, norm, pos, g_mtl1, g_mtl2);
        auto resolved = contact;
        std::swap(resolved.body1, resolved.body2);
        std::swap(resolved.object1, resolved.object2);
        std::swap(resolved.kind1, resolved.kind2);
        std::swap(resolved.geometry1, resolved.geometry2);
        std::swap(resolved.triangle1, resolved.triangle2);
        resolved.material1 = mtl_2;
        resolved.material2 = mtl_1;
        resolved.normal.invert();
        if (do_collide || g_mtl1->Flags.test(SGameMtl::flPassable))
            character->ProcessNativeContact(resolved);
    }
    if (geom_1 && geom_1->object_callbacks)
        geom_1->object_callbacks->Call(do_collide, true, geom_1, geom_2, norm, pos, g_mtl1, g_mtl2);
    if (geom_2 && geom_2->object_callbacks)
        geom_2->object_callbacks->Call(do_collide, false, geom_1, geom_2, norm, pos, g_mtl1, g_mtl2);
    if (geom_1 && geom_1->contact_callback)
        geom_1->contact_callback(do_collide, true, geom_1, geom_2, norm, pos, g_mtl1, g_mtl2);
    else if (geom_2 && geom_2->contact_callback)
        geom_2->contact_callback(do_collide, false, geom_1, geom_2, norm, pos, g_mtl1, g_mtl2);
    else if (layer_1 != 6 && layer_2 != 6 && ph_world->default_contact_shotmark())
        ph_world->default_contact_shotmark()(do_collide, bo1, geom_1, geom_2, norm, pos, g_mtl1, g_mtl2);
    return do_collide;
}

IPHWorld* physics_world() { return ph_world; }
void create_physics_world(bool mt, CObjectSpace* os, CObjectList* lo)
{
    ZoneScoped;
    ph_world = xr_new<CPHWorld>();
    VERIFY(os);
    ph_world->Create(mt, os, lo);
}

void destroy_physics_world()
{
    ZoneScoped;
    ph_world->Destroy();
    xr_delete(ph_world);
}

void destroy_object_space(CObjectSpace*& os) { xr_delete(os); }

void CPHMesh::Create(CObjectSpace& space)
{
    const auto* model = space.GetStaticModel();
    model->syncronize();
    m_mesh_handle = model->acquire_physics_shape();
    if (!m_mesh_handle)
        m_mesh_handle = GetPhysicsCore()->BuildCDBModel(model->get_verts(), model->get_verts_count(),
            model->get_tris(), model->get_tris_count());
    R_ASSERT2(m_mesh_handle, "Jolt level collision shape creation failed");
    m_body = GetPhysicsCore()->CreateStaticBody(m_mesh_handle, Fvector().set(0, 0, 0));
    R_ASSERT2(m_body != INVALID_BODY_HANDLE, "Jolt level collision body creation failed");
}

static bool NativeCollisionFilter(void* first, u16 firstKind, void* second, u16 secondKind)
{
    auto object = [](void* data, u16 kind) -> CPHObject* {
        if (!data) return nullptr;
        if (kind == 0 || kind == 1 || kind == 3)
            return static_cast<CPHElement*>(data)->ph_shell();
        if (kind == 5) return static_cast<CPHStaticGeomShell*>(data);
        if (kind == 6) return static_cast<CPHCharacter*>(data);
        return nullptr;
    };
    auto* object1 = object(first, firstKind);
    auto* object2 = object(second, secondKind);
    // ODE's object broad phase excludes an object paired with itself. Jolt
    // sees individual bodies, so apply the same rule to every shell element.
    if (object1 && object1 == object2) return false;
    if (object1 && object2) return CPHCollideValidator::DoCollide(*object1, *object2);
    if (object1 && secondKind == 0) return CPHCollideValidator::DoCollideStatic(*object1);
    if (object2 && firstKind == 0) return CPHCollideValidator::DoCollideStatic(*object2);
    return true;
}

static bool NativeQueryFilter(void* data, u16 kind, void* ignored, bool camera)
{
    IPhysicsShellHolder* holder = nullptr;
    if (data && (kind == 0 || kind == 1 || kind == 3))
        holder = static_cast<CPHElement*>(data)->PhysicsRefObject();
    else if (data && kind == 5)
        holder = static_cast<CPHStaticGeomShell*>(data)->PhysicsRefObject();
    else if (data && kind == 7)
        holder = static_cast<IPhysicsShellHolder*>(data);
    return !holder || (holder != ignored && (!camera || holder->IsCollideWithActorCamera()));
}

void CPHMesh::Destroy()
{
    if (m_body != INVALID_BODY_HANDLE) {
        GetPhysicsCore()->DestroyBody(m_body);
        m_body = INVALID_BODY_HANDLE;
    }
    if (m_mesh_handle) {
        GetPhysicsCore()->DestroyCDBModel(m_mesh_handle);
        m_mesh_handle = nullptr;
    }
}

#ifdef DEBUG
void CPHWorld::OnRender() { debug_output().PH_DBG_Render(); }
#endif

CPHWorld::CPHWorld()
    : m_object_space(0), m_level_objects(0), m_default_contact_shotmark(0),
      m_default_character_contact_shotmark(0), physics_step_time_callback(0)
{
    disable_count = 0;
    m_frame_time = 0.f;
    m_previous_frame_time = 0.f;
    b_frame_mark = false;
    m_steps_num = 0;
    m_steps_short_num = 0;
    m_frame_sum = 0.f;
    m_delay = 0;
    m_previous_delay = 0;
    m_reduce_delay = 0;
    m_update_delay_count = 0;
    b_world_freezed = false;
    b_processing = false;
    m_gravity = default_world_gravity;
    b_exist = false;
}

void CPHWorld::SetStep(float s)
{
    fixed_step = s;
    if (ph_world && ph_world->Exist())
    {
        float frame_time = Device.fTimeDelta;
        u32 it_number = iFloor(frame_time / fixed_step);
        frame_time -= it_number * fixed_step;
        ph_world->m_previous_frame_time = frame_time;
        ph_world->m_frame_time = frame_time;
    }
}

void CPHWorld::Create(bool mt, CObjectSpace* os, CObjectList* lo)
{
    ZoneScoped;
    GetPhysicsCore()->Initialize();
    GetPhysicsCore()->SetBodyActivationCallback(OnJoltBodyActivation);
    GetPhysicsCore()->SetRigidBodyContactCallback(OnJoltRBContact);
    GetPhysicsCore()->SetCollisionFilter(NativeCollisionFilter);
    GetPhysicsCore()->SetPreIntegrationCallback(ApplyNativeContactEffectors);
    GetPhysicsCore()->SetQueryFilter(NativeQueryFilter);
    GetPhysicsCore()->SetCharacterPairShapeCallback(NativeCharacterPairShape);
    LoadParams();
    m_object_space = os;
    m_level_objects = lo;
    Device.AddSeqFrame(this, mt);
    m_commander = xr_new<CPHCommander>();

    Mesh.Create(*os);
    GetPhysicsCore()->SetSimulationParameters(m_gravity, phIterations);
#ifdef XRAY_GAMEPLAY_BENCHMARK
    m_benchmark = xr_new<GameplayBenchmark>();
#endif

    disable_count = 0;
    phBoundaries.set(inl_ph_world().ObjectSpace().GetBoundingVolume());
    phBoundaries.y1 -= 30.f;
    CPHCollideValidator::Init();
    b_exist = true;

    SetStep(ph_console::ph_step_time);
}

void CPHWorld::Destroy()
{
    ZoneScoped;
    GetPhysicsCore()->SetBodyActivationCallback(nullptr);
    GetPhysicsCore()->SetRigidBodyContactCallback(nullptr);
    GetPhysicsCore()->SetCollisionFilter(nullptr);
    GetPhysicsCore()->SetPreIntegrationCallback(nullptr);
    native_contact_effectors.clear();
    GetPhysicsCore()->SetQueryFilter(nullptr);
    GetPhysicsCore()->SetCharacterPairShapeCallback(nullptr);
#ifdef XRAY_GAMEPLAY_BENCHMARK
    xr_delete(m_benchmark);
#endif
    r_spatial.clear();
    xr_delete(m_commander);
    Mesh.Destroy();

    // Очистка физического мира от объектов текущего уровня
    GetPhysicsCore()->Clear();

    Device.RemoveSeqFrame(this);
    b_exist = false;
}

void CPHWorld::SetGravity(float g)
{
    m_gravity = g;
    GetPhysicsCore()->SetSimulationParameters(g, phIterations);
}

void CPHWorld::OnFrame()
{
    ZoneScoped;
    stats.FrameStart();
#ifdef XRAY_GAMEPLAY_BENCHMARK
    m_benchmark->Frame();
#endif
    stats.MovCollision.Begin();
    FrameStep(Device.fTimeDelta);
    stats.MovCollision.End();
    stats.FrameEnd();
}

void CPHWorld::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    stats.FrameEnd();
    float engineTotal = Device.GetStats().EngineTotal.result;
    float percentage = 100.0f * stats.MovCollision.result / engineTotal;
    font.OutNext("Physics:      %2.2fms, %2.1f%%", stats.MovCollision.result, percentage);
    font.OutNext("- collider:   %2.2fms", stats.Collision.result);
    font.OutNext("- solver:     %2.2fms, %d", stats.Core.result, stats.Core.count);
    if (alert && stats.MovCollision.result > 5.0f)
        alert->Print(font, "Physics   > 5ms:  %3.1f", stats.MovCollision.result);
}

static u32 start_time = 0;
void CPHWorld::Step()
{
#ifdef XRAY_GAMEPLAY_BENCHMARK
    const bool capturing = m_benchmark->Active();
    if (capturing)
        m_benchmark->MaintainWorkload(fixed_step);
    GameplayBenchmark::StepSample sample{};
    const auto captureStart = GameplayBenchmark::Clock::now();
#endif
    VERIFY(b_processing || IsFreezed());

    PH_OBJECT_I i_object;
    PH_UPDATE_OBJECT_I i_update_object;

    if (disable_count == 0)
    {
        disable_count = worldDisablingParams.objects_params.L2frames;
        for (i_object = m_recently_disabled_objects.begin(); m_recently_disabled_objects.end() != i_object;)
        {
            CPHObject* obj = (*i_object);
            obj->check_recently_deactivated();
            ++i_object;
        }
    }
    if (!IsFreezed())
        --disable_count;

    ++m_steps_num;
    ++m_steps_short_num;
    stats.Collision.Begin();

    for (i_object = m_objects.begin(); m_objects.end() != i_object;)
    {
        CPHObject* obj = (*i_object);
        ++i_object;
        obj->Collide();
        obj->PhTune(fixed_step);
    }

    stats.Collision.End();

    for (i_update_object = m_update_objects.begin(); m_update_objects.end() != i_update_object;)
    {
        (*i_update_object)->PhTune(fixed_step);
        ++i_update_object;
    }

    stats.Core.Begin();
    m_commander->update_threadsafety();

    // --- ГЛОБАЛЬНЫЙ ШАГ JOLT ---
#ifdef XRAY_GAMEPLAY_BENCHMARK
    const auto captureCoreStart = GameplayBenchmark::Clock::now();
#endif
    GetPhysicsCore()->Step(fixed_step);
#ifdef XRAY_GAMEPLAY_BENCHMARK
    const auto captureCoreEnd = GameplayBenchmark::Clock::now();
#endif

    stats.Core.End();

    // Синхронизируем результаты обратно в движок
    for (i_object = m_objects.begin(); m_objects.end() != i_object;)
    {
        CPHObject* obj = (*i_object);
        ++i_object;
        obj->PhDataUpdate(fixed_step);
        obj->spatial_move();
    }

    for (i_update_object = m_update_objects.begin(); m_update_objects.end() != i_update_object;)
    {
        CPHUpdateObject* obj = *i_update_object;
        ++i_update_object;
        obj->PhDataUpdate(fixed_step);
    }

    if (physics_step_time_callback)
    {
        physics_step_time_callback(start_time, start_time + u32(fixed_step * 1000));
        start_time += u32(fixed_step * 1000);
    };
#ifdef XRAY_GAMEPLAY_BENCHMARK
    if (capturing) {
        sample.step = m_steps_num;
        sample.dt = fixed_step;
        sample.total = GameplayBenchmark::Milliseconds(GameplayBenchmark::Clock::now() - captureStart);
        // Native collision detection and constraint solving occur in the same
        // Jolt update. solver_ms reports their combined time, including callbacks.
        sample.solver = GameplayBenchmark::Milliseconds(captureCoreEnd - captureCoreStart);
        sample.collision = GameplayBenchmark::Milliseconds(captureCoreStart - captureStart);
        const auto nativeStats = GetPhysicsCore()->GetStatistics();
        sample.nativePreparation = nativeStats.contact_preparation_ms;
        sample.nativeIntegration = nativeStats.integration_ms;
        sample.nativeFeedback = nativeStats.feedback_ms;
        sample.bodies = nativeStats.active_bodies + nativeStats.characters;
        sample.joints = nativeStats.active_constraints;
        sample.contacts = nativeStats.contact_points;
        m_benchmark->Record(sample);
    }
#endif
}

void CPHWorld::StepTouch()
{
    PH_OBJECT_I i_object;
    for (i_object = m_objects.begin(); m_objects.end() != i_object;)
    {
        (*i_object)->Collide();
        ++i_object;
    }

    for (i_object = m_objects.begin(); m_objects.end() != i_object;)
    {
        CPHObject* obj = (*i_object);
        ++i_object;
        obj->spatial_move();
    }
}

u32 CPHWorld::CalcNumSteps(u32 dTime)
{
    if (dTime < m_frame_time * 1000)
        return 0;
    u32 res = iCeil((float(dTime) - m_frame_time * 1000) / (fixed_step * 1000));
    return res;
};

void CPHWorld::FrameStep(float step)
{
    if (IsFreezed())
        return;

    VERIFY(_valid(step));
    step *= phTimefactor;

    u32 it_number;
    float frame_time = m_frame_time;
    frame_time += step;

    if (!(frame_time < fixed_step))
    {
        it_number = iFloor(frame_time / fixed_step);
        frame_time -= it_number * fixed_step;
        m_previous_frame_time = m_frame_time;
        m_frame_time = frame_time;
        b_frame_mark = !b_frame_mark;
    }
    else
    {
        m_frame_time = frame_time;
        return;
    }

    b_processing = true;

    start_time = Device.dwTimeGlobal;
    if (ph_console::g_bDebugDumpPhysicsStep && it_number > 20)
        Msg("!!! TOO MANY PHYSICS STEPS PER FRAME = %d !!!", it_number);

    for (u32 i = 0; i < it_number; ++i)
        Step();

    b_processing = false;
}

void CPHWorld::AddObject(CPHObject* object) { m_objects.push_back(object); }
void CPHWorld::AddRecentlyDisabled(CPHObject* object) { m_recently_disabled_objects.push_back(object); }
void CPHWorld::RemoveFromRecentlyDisabled(PH_OBJECT_I i) { m_recently_disabled_objects.erase(i); }
void CPHWorld::AddUpdateObject(CPHUpdateObject* object) { m_update_objects.push_back(object); }
void CPHWorld::RemoveUpdateObject(PH_UPDATE_OBJECT_I i) { m_update_objects.erase(i); }
void CPHWorld::RemoveObject(PH_OBJECT_I i) { m_objects.erase((i)); };
void CPHWorld::AddFreezedObject(CPHObject* obj) { m_freezed_objects.push_back(obj); }
void CPHWorld::RemoveFreezedObject(PH_OBJECT_I i) { m_freezed_objects.erase(i); }

void CPHWorld::Freeze()
{
    R_ASSERT2(!b_world_freezed, "Physics world is already frozen");
    m_freezed_objects.move_items(m_objects);
    for (auto* object : m_freezed_objects)
        object->FreezeContent();
    m_freezed_update_objects.move_items(m_update_objects);
    b_world_freezed = true;
}

void CPHWorld::UnFreeze()
{
    R_ASSERT2(b_world_freezed, "Physics world is not frozen");
    for (auto* object : m_freezed_objects)
        object->UnFreezeContent();
    m_objects.move_items(m_freezed_objects);
    m_update_objects.move_items(m_freezed_update_objects);
    b_world_freezed = false;
}

bool CPHWorld::IsFreezed() { return b_world_freezed; }

void CPHWorld::CutVelocity(float l_limit, float a_limit)
{
    PH_OBJECT_I i_object;
    for (i_object = m_objects.begin(); m_objects.end() != i_object;)
    {
        (*i_object)->CutVelocity(l_limit, a_limit);
        ++i_object;
    }
}

void CPHWorld::NetRelcase(CPhysicsShell* s)
{
    CPHReqComparerHasShell c(s);
    m_commander->remove_calls_threadsafety(&c);

    PH_UPDATE_OBJECT_I i_update_object;
    for (i_update_object = m_update_objects.begin(); m_update_objects.end() != i_update_object;)
    {
        CPHUpdateObject* obj = (*i_update_object);
        ++i_update_object;
        obj->NetRelcase(s);
    }
}

void CPHWorld::AddCall(CPHCondition* c,CPHAction* a)
{
    m_commander->add_call_threadsafety(c, a);
}

u16 CPHWorld::ObjectsNumber() { return m_objects.count(); }
u16 CPHWorld::UpdateObjectsNumber() { return m_update_objects.count(); }

void CPHWorld::GetState(V_PH_WORLD_STATE& state)
{
    state.clear();
    PH_OBJECT_I i_object;
    for (i_object = m_objects.begin(); m_objects.end() != i_object;)
    {
        CPHObject* obj = (*i_object);
        const u16 els = obj->get_elements_number();
        for (u16 i = 0; els > i; ++i)
        {
            std::pair<CPHSynchronize*, SPHNetState> s;
            s.first = obj->get_element_sync(i);
            s.first->get_State(s.second);
            state.push_back(s);
        }
        ++i_object;
    }
}

void CPHWorld::StepNumIterations(int num_it)
{
    R_ASSERT(num_it > 0);
    GetPhysicsCore()->SetSimulationParameters(m_gravity, num_it);
}
