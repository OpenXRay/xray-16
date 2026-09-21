#include "StdAfx.h"
#include "DeveloperLevel.h"
#include "DeveloperRoom.h"
#include "Actor.h"
#include "PhysicObject.h"
#include "GamePersistent.h"
#include "game_cl_base.h"
#include "UIGameCustom.h"
#include "xrEngine/CameraBase.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Render.h"
#include "xrEngine/xr_level_controller.h"
#include "xrEngine/xr_object.h"
#include "xrCDB/xr_area.h"
#include "xrCDB/xrCDB.h"
#include "xrCore/_std_extensions.h"
#include "xrCore/xr_ini.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrPhysics/IPHWorld.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrScriptEngine/script_process.hpp"
#include "xrServer.h"
#include "xrServer_Objects_ALife.h"
#include "xrServer_Space.h"
#include "xrMessages.h"
#include "Common/LevelStructure.hpp"

extern bool g_bDisableAllInput;

bool CDeveloperLevel::DeveloperModelExists(pcstr visual, string_path& fileName)
{
    if (!visual || !visual[0])
        return false;

    string_path name;
    if (nullptr == strext(visual))
        strconcat(sizeof(name), name, visual, ".ogf");
    else
        xr_strcpy(name, sizeof(name), visual);

    return FS.exist(fileName, "$game_meshes$", name);
}

CDeveloperLevel::~CDeveloperLevel()
{
    Msg("[dev_level] event=shutdown");
}

bool CDeveloperLevel::IsDeveloperLevel() const
{
    return true;
}

ALife::_TIME_ID CDeveloperLevel::GetEnvironmentGameTime() const
{
    return ALife::_TIME_ID(developer_day_time_seconds * 1000.0f);
}

bool CDeveloperLevel::Load(u32)
{
    Msg("[dev_level] event=load stage=begin");

    CDeveloperRoom::Build(m_scene);
    Msg("[dev_level] event=load stage=scene meshes=%u materials=%u", u32(m_scene.meshes.size()),
        u32(m_scene.materials.size()));

    pLevel = xr_new<CInifile>(nullptr, false, false, false);
    pLevel->w_string("level_map", "bound_rect", "-14,-14,14,14");
    pLevel->w_string("level_map", "texture", "ui\\ui_nomap2");

    CreateCollision();

    g_pGamePersistent->SpatialSpace.initialize(ObjectSpace.GetBoundingVolume());
    g_pGamePersistent->SpatialSpacePhysic.initialize(ObjectSpace.GetBoundingVolume());

    Sound->set_geometry_occ(ObjectSpace.GetStaticModel(), ObjectSpace.GetBoundingVolume());
    Sound->set_handler([](const ref_sound& S, float range)
    {
        if (g_pGameLevel && S && S->feedback)
            g_pGameLevel->SoundEvent_Register(S, range);
    });

    if (!GEnv.Render->level_LoadDeveloper(m_scene))
    {
        Msg("! [dev_level] event=load result=failed reason=renderer_rejected_scene");
        return false;
    }
    Msg("[dev_level] event=load stage=render result=ok");

    CEnvironment& environment = g_pGamePersistent->Environment();
    environment.mods_unload();
    environment.SetGameTime(developer_day_time_seconds, 1.0f);

    shared_str weather;
    u32 weatherFallback = 0;
    if (environment.WeatherCycles.find("clear") != environment.WeatherCycles.end())
        weather = "clear";
    else if (environment.WeatherCycles.find("default") != environment.WeatherCycles.end())
        weather = "default";
    else
    {
        weather = environment.GetWeather();
        weatherFallback = 1;
    }
    environment.SetWeather(weather, true);
    Msg("[dev_level] event=load stage=environment time=%.0f weather=\"%s\" fallback=%u", developer_day_time_seconds,
        weather.c_str(), weatherFallback);

    R_ASSERT(Load_GameSpecific_Before());
    Objects.Load();

    bReady = true;

    if (!GEnv.isDedicatedServer)
    {
        IR_Capture();
        Device.seqRender.Add(this);
    }
    Device.seqFrame.Add(this);

    Msg("[dev_level] event=load result=ok");
    return true;
}

void CDeveloperLevel::CreateCollision()
{
    const u16 materialIndex = GMLib.GetMaterialIdx("default");

    xr_vector<Fvector> vertices;
    xr_vector<CDB::TRI> triangles;

    for (const xray::render::DeveloperSceneMesh& mesh : m_scene.meshes)
    {
        if (!mesh.collision || mesh.indices.size() < 3)
            continue;

        const u32 base = u32(vertices.size());
        for (const xray::render::DeveloperSceneVertex& vertex : mesh.vertices)
            vertices.push_back(vertex.position);

        for (size_t i = 0; i + 3 <= mesh.indices.size(); i += 3)
        {
            CDB::TRI triangle{};
            triangle.verts[0] = base + mesh.indices[i];
            triangle.verts[1] = base + mesh.indices[i + 1];
            triangle.verts[2] = base + mesh.indices[i + 2];
            triangle.material = materialIndex;
            triangle.sector = 0;
            triangle.suppress_shadows = 0;
            triangle.suppress_wm = 0;
            triangles.push_back(triangle);
        }
    }

    hdrCFORM header{};
    header.version = CFORM_CURRENT_VERSION;
    header.vertcount = u32(vertices.size());
    header.facecount = u32(triangles.size());
    header.aabb = m_scene.bounds;

    ObjectSpace.Create(vertices.data(), triangles.data(), header, nullptr);

    Msg("[dev_level] event=load stage=collision verts=%u tris=%u", header.vertcount, header.facecount);
}

bool CDeveloperLevel::Load_GameSpecific_Before()
{
    return true;
}

bool CDeveloperLevel::Load_GameSpecific_After()
{
    if (!GEnv.isDedicatedServer)
    {
        auto& scriptEngine = *GEnv.ScriptEngine;
        scriptEngine.remove_script_process(ScriptProcessor::Level);
        scriptEngine.add_script_process(ScriptProcessor::Level, scriptEngine.CreateScriptProcess("level", ""));
    }

    g_pGamePersistent->Environment().SetGameTime(GetEnvironmentGameDayTimeSec(), game->GetEnvironmentGameTimeFactor());
    Msg("[dev_level] event=load stage=game_specific_after");
    return true;
}

void CDeveloperLevel::OnFrame()
{
    inherited::OnFrame();

    if (!bReady)
        return;

    const float dt = Device.fTimeDeltaReal;
    if (dt > 0.0f)
    {
        if (m_dtCount == 0 || dt < m_dtMin)
            m_dtMin = dt;
        if (dt > m_dtMax)
            m_dtMax = dt;
        m_dtSum += dt;
        ++m_dtCount;
    }

    UpdateProgress();

    if (Device.dwTimeGlobal >= m_nextSummaryTime)
    {
        m_nextSummaryTime = Device.dwTimeGlobal + 1000;
        RunDiagnostics();
    }
}

void CDeveloperLevel::UpdateProgress()
{
    if (!m_actorLogged && g_actor)
    {
        m_actorLogged = true;
        Msg("[dev_level] event=player_spawn id=%u pos=%.2f,%.2f,%.2f", g_actor->ID(), g_actor->Position().x,
            g_actor->Position().y, g_actor->Position().z);
        Msg("[dev_level] event=controls source=defaults move=wasd jump=space crouch=ctrl use=F action=impulse_nearest_prop");
    }

    if (!m_propsRequested && g_actor && Server && physics_world())
    {
        m_propsRequested = true;
        SpawnProps();
    }

    if (m_readyLogged || !m_propsRequested || m_propSpawned != required_props || !g_actor || !physics_world())
        return;

    for (u32 i = 0; i < m_propSpawned; ++i)
    {
        IGameObject* object = Objects.net_Find(u16(m_propIds[i]));
        CPhysicObject* prop = object ? smart_cast<CPhysicObject*>(object) : nullptr;
        if (!prop)
            return;

        CPhysicsShell* shell = prop->PPhysicsShell();
        if (!shell || !shell->isActive())
            return;
    }

    m_readyLogged = true;
    Msg("[dev_level] event=ready actor=%u props=%u", g_actor->ID(), m_propSpawned);
}

void CDeveloperLevel::SpawnProps()
{
    static DevPropSource const sources[] = {
        { "box_wood_01", nullptr },
        { "box_metall_01", nullptr },
        { "box_wood_02", nullptr },
        { "box_paper", nullptr },
        { "physic_object", "dynamics\\box\\box_wood_01.ogf" },
        { "physic_object", "dynamics\\box\\box_metall_01.ogf" },
    };

    m_propFailure.clear();

    for (DevPropSource const& source : sources)
    {
        if (m_propSpawned >= required_props)
            break;

        if (!pSettings->section_exist(source.section))
        {
            m_propFailure = "section_missing";
            Msg("[dev_level] event=prop_spawn result=skipped reason=section_missing section=%s", source.section);
            continue;
        }

        pcstr const visual = source.visual ? source.visual :
            (pSettings->line_exist(source.section, "visual") ? pSettings->r_string(source.section, "visual") : nullptr);

        string_path fileName;
        if (!DeveloperModelExists(visual, fileName))
        {
            m_propFailure = "visual_missing";
            Msg("[dev_level] event=prop_spawn result=skipped reason=visual_missing section=%s", source.section);
            continue;
        }

        CSE_Abstract* abstract = Server->entity_Create(source.section);
        if (!abstract)
        {
            m_propFailure = "entity_create_failed";
            Msg("[dev_level] event=prop_spawn result=skipped reason=entity_create_failed section=%s", source.section);
            continue;
        }

        CSE_ALifeObjectPhysic* physic = smart_cast<CSE_ALifeObjectPhysic*>(abstract);
        if (!physic)
        {
            m_propFailure = "not_physics_entity";
            Msg("[dev_level] event=prop_spawn result=skipped reason=not_physics_entity section=%s", source.section);
            F_entity_Destroy(abstract);
            continue;
        }

        if (source.visual)
            physic->set_visual(source.visual);

        const u32 index = m_propSpawned;
        const Fvector position{ -1.5f + 1.5f * float(index), 0.75f, -5.0f };

        physic->type = epotBox;
        physic->mass = 12.0f;
        physic->_flags.set(CSE_PHSkeleton::flActive, TRUE);
        physic->o_Position = position;
        physic->o_Angle.set(0.0f, 0.0f, 0.0f);
        physic->set_name_replace(source.section);
        physic->s_flags.assign(M_SPAWN_OBJECT_LOCAL);

        NET_Packet packet;
        packet.w_begin(M_SPAWN);
        physic->Spawn_Write(packet, TRUE);

        u16 id;
        packet.r_begin(id);
        R_ASSERT(id == M_SPAWN);

        ClientID clientID;
        clientID.set(0);

        CSE_Abstract* spawned = Server->Process_spawn(packet, clientID);
        F_entity_Destroy(abstract);

        if (!spawned)
        {
            m_propFailure = "spawn_rejected";
            Msg("[dev_level] event=prop_spawn result=failed reason=spawn_rejected section=%s", source.section);
            continue;
        }

        m_propIds[m_propSpawned] = spawned->ID;
        ++m_propSpawned;

        Msg("[dev_level] event=prop_spawn result=ok index=%u id=%u section=%s visual=%s pos=%.2f,%.2f,%.2f", index,
            u32(spawned->ID), source.section, visual, position.x, position.y, position.z);
    }

    if (m_propSpawned < required_props)
    {
        Msg("! [dev_level] event=prop_spawn result=incomplete spawned=%u required=%u reason=%s", m_propSpawned,
            required_props, m_propFailure.c_str());
    }
}

void CDeveloperLevel::IR_OnKeyboardPress(int key)
{
    inherited::IR_OnKeyboardPress(key);

    if (GetBindedAction(key) != kUSE)
        return;
    if (Device.dwPrecacheFrame != 0 || g_bDisableAllInput || Device.Paused())
        return;
    if (!bReady || !g_actor)
        return;

    CUIGameCustom* gameUI = CurrentGameUI();
    if (!gameUI || gameUI->TopInputReceiver())
        return;

    Interact();
}

void CDeveloperLevel::Interact()
{
    CActor* actor = g_actor;
    if (!actor)
        return;

    const Fvector origin = actor->Position();

    xr_vector<IGameObject*> nearest;
    ObjectSpace.GetNearest(nearest, origin, interaction_range, actor);

    CPhysicObject* target = nullptr;
    float bestDistance = interaction_range;

    for (IGameObject* object : nearest)
    {
        CPhysicObject* physicsObject = smart_cast<CPhysicObject*>(object);
        if (!physicsObject)
            continue;

        const float distance = origin.distance_to(physicsObject->Position());
        if (distance <= bestDistance)
        {
            bestDistance = distance;
            target = physicsObject;
        }
    }

    if (!target)
    {
        Msg("[dev_level] event=interaction action=impulse result=no_target range=%.1f", interaction_range);
        return;
    }

    CPhysicsShell* shell = target->PPhysicsShell();
    if (!shell)
    {
        Msg("[dev_level] event=interaction action=impulse result=no_shell id=%u", target->ID());
        return;
    }

    if (!shell->isActive())
        shell->Activate();

    Fvector direction = actor->cam_Active()->Direction();
    direction.normalize();
    shell->applyImpulse(direction, interaction_impulse);
    Msg("[dev_level] event=interaction action=impulse result=ok id=%u distance=%.2f power=%.1f", target->ID(),
        bestDistance, interaction_impulse);
}

void CDeveloperLevel::RunDiagnostics()
{
    const float average_ms = m_dtCount ? (m_dtSum / float(m_dtCount)) * 1000.0f : 0.0f;
    Msg("[dev_level] event=frame interval=1s frame=%u ready=%d frames=%u dt_min_ms=%.3f dt_avg_ms=%.3f dt_max_ms=%.3f",
        Device.dwFrame, m_readyLogged ? 1 : 0, m_dtCount, m_dtMin * 1000.0f, average_ms, m_dtMax * 1000.0f);

    if (g_actor)
    {
        const Fvector direction = g_actor->cam_Active()->Direction();
        Msg("[dev_level] event=actor frame=%u id=%u pos=%.2f,%.2f,%.2f look=%.3f,%.3f,%.3f",
            Device.dwFrame, g_actor->ID(), g_actor->Position().x, g_actor->Position().y, g_actor->Position().z,
            direction.x, direction.y, direction.z);
    }
    else
    {
        Msg("[dev_level] event=actor state=missing");
    }

    if (IPHWorld* world = physics_world())
    {
        Msg("[dev_level] event=physics frame=%u gravity=%.3f steps=%llu paused=%d",
            Device.dwFrame, world->Gravity(), (unsigned long long)world->StepsNum(), Device.Paused() ? 1 : 0);
    }

    for (u32 i = 0; i < m_propSpawned; ++i)
    {
        IGameObject* object = Objects.net_Find(u16(m_propIds[i]));
        if (!object)
        {
            Msg("[dev_level] event=prop index=%u id=%u state=missing", i, m_propIds[i]);
            continue;
        }

        const Fvector position = object->Position();
        CPhysicObject* prop = smart_cast<CPhysicObject*>(object);
        CPhysicsShell* shell = prop ? prop->PPhysicsShell() : nullptr;
        Msg("[dev_level] event=prop frame=%u index=%u id=%u pos=%.2f,%.2f,%.2f present=1 shell_active=%d shell_enabled=%d",
            Device.dwFrame, i, m_propIds[i], position.x, position.y, position.z,
            shell && shell->isActive() ? 1 : 0, shell && shell->isEnabled() ? 1 : 0);
    }

    if (m_propSpawned < required_props)
    {
        Msg("[dev_level] event=prop state=incomplete spawned=%u required=%u reason=%s", m_propSpawned, required_props,
            m_propFailure.c_str());
    }

    GEnv.Render->DumpDeveloperSceneDiagnostics();

    m_dtCount = 0;
    m_dtSum = 0.0f;
    m_dtMin = 0.0f;
    m_dtMax = 0.0f;
}
