#include "TestSupport.h"

#include "Common/LevelStructure.hpp"
#include "xrCDB/ISpatial.h"
#include "xrCDB/xr_area.h"
#include "xrEngine/Engine.h"

#include <chrono>
#include <thread>

void CheckPhysicsContacts();

void CheckDynamicQueries(CObjectSpace& space, ISpatial_DB& spatial);
static u32 buildCalls, remapCalls, materialGeneration = 1;

static void BuildMaterials(Fvector*, u32, CDB::TRI* t, u32 count, void*)
{
    buildCalls++;
    for (u32 i = 0; i < count; i++)
        t[i].material = 10 + i;
}

static void SaveMaterials(IWriter& w)
{
    w.w_u32(materialGeneration);
}

static bool LoadMaterials(IReader& r)
{
    return r.elapsed() >= 4 && r.r_u32() == materialGeneration;
}

static void RemapMaterials(CDB::TRI* t, u32 count, xr_map<u16, shared_str>& materials)
{
    remapCalls++;
    for (u32 i = 0; i < count; i++)
    {
        Require(materials.count(t[i].material) == 1, "Embedded material table missing");
        Require(std::string(materials.at(t[i].material).c_str()) == "test-material", "Material name changed");
        t[i].material = 25 + i;
    }
}

static void CheckCallback()
{
    auto mesh = Fixture(), expected = mesh;

    struct State
    {
        u32 calls = 0;
        std::thread::id thread;
    } state;

    auto change = [](Fvector* v, u32 vc, CDB::TRI* t, u32 tc, void* data)
    {
        auto& s = *static_cast<State*>(data);
        s.calls++;
        s.thread = std::this_thread::get_id();
        for (u32 i = 0; i < vc; i++)
            v[i].x += 4;
        for (u32 i = 0; i < tc; i++)
            t[i].material = 100 + i;
    };
    CDB::MODEL model;
    model.build(mesh.vertices.data(), mesh.vertices.size(), mesh.triangles.data(), mesh.triangles.size(), change, &state);
    model.syncronize();
    Require(state.calls == 1, "Construction skipped or repeated its callback");
    Require((state.thread != std::this_thread::get_id()) == (strstr(Core.Params, "-mt_cdb") != nullptr), "Wrong construction thread");
    State ignored;
    change(expected.vertices.data(), expected.vertices.size(), expected.triangles.data(), expected.triangles.size(), &ignored);
    CDB::COLLIDER collider;
    collider.ray_query(0, &model, Vector(4.25f, .25f, -1), Vector(0, 0, 1), 10);
    Require(Ids(collider, expected) == std::set<int>{ 0, 1, 3, 4 }, "Tree or metadata ignored callback");
    Require(mesh.vertices[0].x == 0 && mesh.triangles[0].material == 0, "Callback modified caller-owned geometry");
}

static void CheckQueries(CObjectSpace& space, u32 firstMaterial)
{
    const auto start = Vector(.25f, .25f, -1), direction = Vector(0, 0, 1);
    collide::ray_cache cache;
    Require(space.RayTest(start, direction, 10, collide::rqtStatic, &cache, nullptr), "Occlusion missed");
    Require(space.RayTest(start, direction, 10, collide::rqtStatic, &cache, nullptr), "Cached occlusion missed");
    Require(!space.RayTest(Vector(10, 10, -1), direction, 10, collide::rqtStatic, &cache, nullptr), "Stale occlusion cache");
    collide::rq_result nearest;
    Require(space.RayPick(start, direction, 10, collide::rqtStatic, nearest, nullptr), "RayPick missed");
    Require(nearest.O == nullptr && nearest.range == 1.f, "RayPick selected wrong hit");
    collide::rq_results results;
    const collide::ray_defs ray(start, direction, 10, CDB::OPT_CULL, collide::rqtBoth);

    struct Hits
    {
        CObjectSpace* space;
        u32 material;
        float previous = 0, transmission = 1;
        u32 count = 0;
    } hits{ &space, firstMaterial };

    auto callback = [](collide::rq_result& hit, void* data)
    {
        auto& s = *static_cast<Hits*>(data);
        Require(hit.O == nullptr && hit.range >= s.previous, "Callbacks are not ordered");
        s.previous = hit.range;
        Require(s.space->GetStaticTris()[hit.element].material == s.material + hit.element, "Material index changed");
        s.transmission *= hit.element == 3 ? 0.f : .5f;
        s.count++;
        return s.transmission > .01f;
    };
    Require(space.RayQuery(results, ray, callback, &hits, nullptr, nullptr), "Material query missed");
    Require(hits.count == 3 && hits.transmission == 0 && results.r_count() == 3, "Material obstruction callback failed");
    u32 calls = 0;
    auto stop = [](collide::rq_result&, void* data)
    {
        (*static_cast<u32*>(data))++;
        return false;
    };
    Require(space.RayQuery(results, ray, stop, &calls, nullptr, nullptr), "Early stop missed");
    Require(calls == 1 && results.r_count() == 1, "Callback termination ignored");
    xr_vector<Fvector> triangles;
    Require(space.BoxQuery(Vector(.25f, .25f, 0), Vector(0, 0, 1), Vector(0, 1, 0), Vector(.2f, .2f, .2f), &triangles), "Volume query missed");
    Require(triangles.size() == 6, "Wrong volume geometry");
}

void CheckObjectSpace()
{
    CheckCallback();
    const auto root = TestPath("cdb-object-space") + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

    struct Cleanup
    {
        std::string path;

        ~Cleanup()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{ root };

    std::filesystem::create_directories(std::filesystem::path(root) / "level");
    FS.append_path("$app_data_root$", root.c_str(), nullptr, false);
    FS.append_path("$level$", root.c_str(), "level" DELIMITER, false);
    auto mesh = Fixture();
    hdrCFORM header{};
    header.version = CFORM_CURRENT_VERSION;
    header.vertcount = mesh.vertices.size();
    header.facecount = mesh.triangles.size();
    header.aabb.set(-10, -10, -10, 10, 10, 10);
    ISpatial_DB spatial("test objects");
    spatial.initialize(header.aabb);
    buildCalls = remapCalls = 0;
    auto create = [&](u32 expected)
    {
        CObjectSpace space(&spatial);
        space.GetStaticModel()->set_model_crc32(2139);
        space.Create(mesh.vertices.data(), mesh.triangles.data(), header, BuildMaterials, SaveMaterials, LoadMaterials, RemapMaterials);
        Require(buildCalls == expected, "Wrong object-space cache path");
        CheckQueries(space, 10);
        CheckDynamicQueries(space, spatial);
    };
    create(1);
    create(1);
    materialGeneration++;
    create(2);
    create(2);
    auto cacheName = (std::filesystem::path(root) / "cdb_cache" / "level" / "objspace.bin").string();
#ifdef XRAY_USE_JOLT_CDB
    cacheName += ".jolt";
#endif
    std::vector<u8> tree;
#ifndef XRAY_USE_JOLT_CDB
    {
        auto* reader = FS.r_open(cacheName.c_str());
        Require(reader != nullptr, "Cache not written");
        reader->advance(5 * sizeof(u32) + mesh.vertices.size() * sizeof(Fvector) + mesh.triangles.size() * sizeof(CDB::TRI));
        tree.resize(reader->elapsed());
        reader->r(tree.data(), tree.size());
        FS.r_close(reader);
        tree.erase(tree.begin() + 12, tree.begin() + 16);
    }
#else
    tree = { 0xde, 0xad, 0xbe, 0xef };
#endif
    {
        auto* writer = FS.w_open(cacheName.c_str());
        Require(writer != nullptr, "Cannot truncate cache");
        writer->w_u8(1);
        FS.w_close(writer);
    }
    create(3);
    {
        CMemoryWriter embedded;
        embedded.w(&header, sizeof(header));
        embedded.w(mesh.vertices.data(), mesh.vertices.size() * sizeof(Fvector));
        embedded.w(mesh.triangles.data(), mesh.triangles.size() * sizeof(CDB::TRI));
        embedded.w_u32(CFORM_CACHE_CURRENT_VERSION);
        embedded.w_u32(mesh.triangles.size());
        for (const auto& t : mesh.triangles)
        {
            embedded.w_u16(t.material);
            embedded.w_stringZ("test-material");
        }
        embedded.w(tree.data(), tree.size());
        CObjectSpace space(&spatial);
        space.Load(xr_new<IReader>(embedded.pointer(), embedded.size()), BuildMaterials, SaveMaterials, LoadMaterials, RemapMaterials);
        Require(buildCalls == 3 && remapCalls == 1, "Embedded cache skipped remapping");
        CheckQueries(space, 25);
    }
    CheckPhysicsContacts();
    FS.file_delete(cacheName.c_str());
}
