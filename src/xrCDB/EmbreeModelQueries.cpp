#include "stdafx.h"
#include "EmbreeModelQueries.h"
#include "Intersect.hpp"

#include <embree4/rtcore.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace CDB
{
namespace
{
struct MeshData
{
    std::vector<Fvector> vertices;
    std::vector<TRI> triangles;
};

struct QueryContext
{
    RTCRayQueryContext rtc;
    const MeshData* mesh = nullptr;
    COLLIDER* collider = nullptr;
    u32 rayMode = 0;
    float range = 0.f;
};

class EmbreeModel
{
public:
    MeshData mesh;
    RTCScene scene = nullptr;

    explicit EmbreeModel(const MODEL& model)
    {
        mesh.vertices.assign(model.get_verts(), model.get_verts() + model.get_verts_count());
        mesh.triangles.assign(model.get_tris(), model.get_tris() + model.get_tris_count());
        scene = rtcNewScene(Device());
        if (!scene) throw std::runtime_error("Embree scene allocation failed");
        rtcSetSceneFlags(scene, RTC_SCENE_FLAG_ROBUST);
        rtcSetSceneBuildQuality(scene, RTC_BUILD_QUALITY_MEDIUM);
        RTCGeometry geometry = rtcNewGeometry(Device(), RTC_GEOMETRY_TYPE_TRIANGLE);
        if (!geometry) { rtcReleaseScene(scene); scene = nullptr; throw std::runtime_error("Embree geometry allocation failed"); }
        try
        {
            auto* positions = static_cast<float*>(rtcSetNewGeometryBuffer(geometry, RTC_BUFFER_TYPE_VERTEX, 0,
                RTC_FORMAT_FLOAT3, 16, mesh.vertices.size()));
            if (!positions) throw std::runtime_error("Embree vertex allocation failed");
            for (size_t i = 0; i < mesh.vertices.size(); ++i)
            {
                positions[4 * i] = mesh.vertices[i].x;
                positions[4 * i + 1] = mesh.vertices[i].y;
                positions[4 * i + 2] = mesh.vertices[i].z;
                positions[4 * i + 3] = 0.f;
            }
            rtcSetSharedGeometryBuffer(geometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
                mesh.triangles.data(), 0, sizeof(TRI), mesh.triangles.size());
            rtcSetGeometryIntersectFilterFunction(geometry, Filter);
            rtcSetGeometryOccludedFilterFunction(geometry, Filter);
            rtcCommitGeometry(geometry);
            rtcAttachGeometry(scene, geometry);
            rtcReleaseGeometry(geometry);
            geometry = nullptr;
            rtcCommitScene(scene);
            if (rtcGetDeviceError(Device()) != RTC_ERROR_NONE) throw std::runtime_error("Embree scene build failed");
        }
        catch (...)
        {
            if (geometry) rtcReleaseGeometry(geometry);
            rtcReleaseScene(scene);
            scene = nullptr;
            throw;
        }
    }

    ~EmbreeModel() { if (scene) rtcReleaseScene(scene); }

    static RTCDevice Device()
    {
        static RTCDevice device = []
        {
            RTCDevice value = rtcNewDevice("threads=4");
            if (!value) throw std::runtime_error("Embree device initialization failed");
            return value;
        }();
        return device;
    }

    static void Filter(const RTCFilterFunctionNArguments* args)
    {
        auto& context = *reinterpret_cast<QueryContext*>(args->context);
        for (u32 lane = 0; lane < args->N; ++lane)
        {
            if (!args->valid[lane]) continue;
            const u32 id = RTCHitN_primID(args->hit, args->N, lane);
            const auto& triangle = context.mesh->triangles[id];
            Fvector triangleVertices[3] = {context.mesh->vertices[triangle.verts[0]],
                context.mesh->vertices[triangle.verts[1]], context.mesh->vertices[triangle.verts[2]]};
            Fvector* points[3] = {&triangleVertices[0], &triangleVertices[1], &triangleVertices[2]};
            const float origin[3] = {RTCRayN_org_x(args->ray, args->N, lane),
                RTCRayN_org_y(args->ray, args->N, lane), RTCRayN_org_z(args->ray, args->N, lane)};
            const float direction[3] = {RTCRayN_dir_x(args->ray, args->N, lane),
                RTCRayN_dir_y(args->ray, args->N, lane), RTCRayN_dir_z(args->ray, args->N, lane)};
            Fvector start, dir;
            start.set(origin[0], origin[1], origin[2]);
            dir.set(direction[0], direction[1], direction[2]);
            float u = 0.f, v = 0.f, distance = 0.f;
            if (!TestRayTri(start, dir, points, u, v, distance, (context.rayMode & OPT_CULL) != 0)
                || distance <= 0.f || distance > context.range)
            {
                args->valid[lane] = 0;
                continue;
            }
            RESULT result{};
            result.id = static_cast<int>(id);
            result.range = distance;
            result.u = u;
            result.v = v;
            result.dummy = triangle.dummy;
            result.verts[0] = triangleVertices[0]; result.verts[1] = triangleVertices[1]; result.verts[2] = triangleVertices[2];
            if (context.rayMode & OPT_ONLYNEAREST)
            {
                if (!context.collider->r_count()) context.collider->r_add() = result;
                else if (distance < context.collider->r_begin()->range) *context.collider->r_begin() = result;
                context.range = std::min(context.range, distance);
                RTCRayN_tfar(args->ray, args->N, lane) = std::nextafter(context.range * 1.000001f, INFINITY);
            }
            else context.collider->r_add() = result;

            if (!(context.rayMode & (OPT_ONLYFIRST | OPT_ONLYNEAREST)))
                args->valid[lane] = 0;
        }
    }
};

struct Registry
{
    std::mutex mutex;
    std::unordered_map<const MODEL*, std::shared_ptr<EmbreeModel>> scenes;
};

Registry& Scenes() { static Registry registry; return registry; }

enum class RayPolicy
{
    Opcode,
    Embree,
    Hybrid
};

RayPolicy GetRayPolicy()
{
    static const RayPolicy policy = []
    {
        const char* value = std::getenv("XRAY_COLLISION_QUERIES");
        if (value && std::strcmp(value, "embree") == 0) return RayPolicy::Embree;
        if (value && std::strcmp(value, "hybrid") == 0) return RayPolicy::Hybrid;
        return RayPolicy::Opcode;
    }();
    return policy;
}

std::shared_ptr<EmbreeModel> GetScene(const MODEL* model)
{
    thread_local const MODEL* cachedModel = nullptr;
    thread_local std::weak_ptr<EmbreeModel> cachedScene;
    if (cachedModel == model)
        if (auto scene = cachedScene.lock()) return scene;

    auto& registry = Scenes();
    std::lock_guard guard(registry.mutex);
    auto found = registry.scenes.find(model);
    if (found == registry.scenes.end())
        found = registry.scenes.emplace(model, std::make_shared<EmbreeModel>(*model)).first;
    cachedModel = model;
    cachedScene = found->second;
    return found->second;
}
}

bool ShouldUseEmbreeRayQueries(u32 rayMode)
{
    const RayPolicy policy = GetRayPolicy();
    return policy == RayPolicy::Embree || (policy == RayPolicy::Hybrid
        && (rayMode & OPT_ONLYNEAREST) != 0
        && (rayMode & OPT_ONLYFIRST) == 0);
}

void ReleaseEmbreeModel(const MODEL* model)
{
    auto& registry = Scenes();
    std::lock_guard guard(registry.mutex);
    registry.scenes.erase(model);
}

void QueryEmbreeModel(const MODEL* model, COLLIDER& collider, u32 rayMode,
    const Fvector& start, const Fvector& direction, float range)
{
    collider.r_clear();
    auto scene = GetScene(model);
    QueryContext context{};
    rtcInitRayQueryContext(&context.rtc);
    context.mesh = &scene->mesh;
    context.collider = &collider;
    context.rayMode = rayMode;
    context.range = range;
    RTCRay ray{};
    ray.org_x = start.x; ray.org_y = start.y; ray.org_z = start.z;
    ray.dir_x = direction.x; ray.dir_y = direction.y; ray.dir_z = direction.z;
    ray.tnear = 0.f; ray.tfar = range; ray.time = 0.f; ray.mask = 0xFFFFFFFFu; ray.id = 0; ray.flags = 0;
    if (rayMode & OPT_ONLYFIRST)
    {
        RTCOccludedArguments args;
        rtcInitOccludedArguments(&args);
        args.context = &context.rtc;
        rtcOccluded1(scene->scene, &ray, &args);
    }
    else
    {
        RTCRayHit rayHit{};
        rayHit.ray = ray;
        rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        RTCIntersectArguments args;
        rtcInitIntersectArguments(&args);
        args.context = &context.rtc;
        rtcIntersect1(scene->scene, &rayHit, &args);
    }
}
}
