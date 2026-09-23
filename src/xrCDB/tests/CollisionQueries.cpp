#include "Common/Common.hpp"
#include "xrCore/xrCore.h"

#include "xrCDB/Frustum.h"
#include "xrCDB/Intersect.hpp"
#include "xrCDB/xrCDB.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <random>
#include <set>
#include <stdexcept>

static void Require(bool condition, pcstr message)
{
    if (!condition)
        throw std::runtime_error(message);
}

static Fvector Vector(float x, float y, float z)
{
    return Fvector{ x, y, z };
}

struct Mesh
{
    std::vector<Fvector> vertices;
    std::vector<CDB::TRI> triangles;

    void Add(Fvector a, Fvector b, Fvector c)
    {
        const auto index = static_cast<u32>(vertices.size());
        CDB::TRI triangle{};
        triangle.verts[0] = index;
        triangle.verts[1] = index + 1;
        triangle.verts[2] = index + 2;
        triangle.dummy = 0x81234000u + static_cast<u32>(triangles.size());
        triangles.push_back(triangle);
        vertices.insert(vertices.end(), { a, b, c });
    }

    void Build(CDB::MODEL& model)
    {
        model.build(vertices.data(), static_cast<u32>(vertices.size()), triangles.data(), static_cast<u32>(triangles.size()));
    }
};

static Mesh Fixture()
{
    Mesh mesh;
    mesh.Add(Vector(0, 0, 0), Vector(0, 1, 0), Vector(1, 0, 0));
    mesh.Add(Vector(0, 0, 0), Vector(0, 1, 0), Vector(1, 0, 0));
    mesh.Add(Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 1, 0));
    mesh.Add(Vector(0, 0, 3), Vector(0, 1, 3), Vector(1, 0, 3));
    mesh.Add(Vector(0, 0, 6), Vector(1, 0, 6), Vector(0, 1, 6));
    return mesh;
}

static std::set<int> Ids(CDB::COLLIDER& collider, const Mesh& mesh)
{
    std::set<int> ids;
    for (const auto& result : *collider.r_get())
    {
        Require(result.id >= 0 && static_cast<size_t>(result.id) < mesh.triangles.size(), "Invalid triangle ID");
        const auto& triangle = mesh.triangles[result.id];
        Require(result.dummy == triangle.dummy, "Triangle metadata changed");
        for (u32 i = 0; i < 3; i++)
        {
            const auto& expected = mesh.vertices[triangle.verts[i]];
            Require(result.verts[i].x == expected.x && result.verts[i].y == expected.y && result.verts[i].z == expected.z, "Triangle vertices changed");
        }
        Require(ids.insert(result.id).second, "Duplicate result ID");
    }
    return ids;
}

static void CheckRays(CDB::MODEL& model, const Mesh& mesh)
{
    CDB::COLLIDER collider;
    const auto start = Vector(0.25f, 0.25f, -1);
    const auto direction = Vector(0, 0, 1);
    collider.ray_query(0, &model, start, direction, 10);
    Require(Ids(collider, mesh) == std::set<int>{ 0, 1, 3, 4 }, "Ray must retain both coincident triangles");
    for (const auto& hit : *collider.r_get())
    {
        Require(std::abs(hit.u - 0.25f) < 1.e-6f && std::abs(hit.v - 0.25f) < 1.e-6f, "Incorrect barycentric coordinates");
        Require(std::abs(hit.range - (mesh.vertices[mesh.triangles[hit.id].verts[0]].z + 1.f)) < 1.e-6f, "Incorrect ray distance");
    }
    collider.ray_query(CDB::OPT_CULL, &model, start, direction, 10);
    Require(Ids(collider, mesh) == std::set<int>{ 0, 1, 3 }, "Backface culling changed");
    collider.ray_query(CDB::OPT_ONLYNEAREST, &model, start, direction, 10);
    Require(collider.r_count() == 1 && collider.r_begin()->range == 1.f, "Nearest ray hit is incorrect");
    collider.ray_query(CDB::OPT_ONLYFIRST, &model, start, direction, 10);
    Require(collider.r_count() == 1 && Ids(collider, mesh).count(2) == 0, "First-hit query is incorrect");
    collider.ray_query(CDB::OPT_ONLYFIRST | CDB::OPT_ONLYNEAREST, &model, start, direction, 10);
    Require(collider.r_count() == 1, "Combined ray flags returned multiple hits");
    collider.ray_query(0, &model, start, direction, 1);
    Require(Ids(collider, mesh) == std::set<int>{ 0, 1 }, "Hits at the ray endpoint must be included");
    collider.ray_query(0, &model, Vector(0.25f, 0.25f, 0), direction, 10);
    Require(Ids(collider, mesh) == std::set<int>{ 3, 4 }, "Hits at the ray origin must be excluded");
    collider.ray_query(0, &model, Vector(2, 2, -1), direction, 10);
    Require(collider.r_count() == 0, "Parallel ray outside the bounds must miss");
    collider.ray_query(0, &model, start, Vector(0, 0, 0), 10);
    Require(collider.r_count() == 0, "Zero direction must miss");
    collider.ray_query(0, &model, start, direction, 0);
    Require(collider.r_count() == 0, "Zero length must miss");
}

static CFrustum BoxFrustum(const Fvector& center, const Fvector& extents)
{
    Fplane planes[6];
    for (u32 axis = 0; axis < 3; axis++)
    {
        planes[axis * 2].n.set(0, 0, 0);
        planes[axis * 2].n[axis] = 1;
        planes[axis * 2].d = -center[axis] - extents[axis];
        planes[axis * 2 + 1].n.set(0, 0, 0);
        planes[axis * 2 + 1].n[axis] = -1;
        planes[axis * 2 + 1].d = center[axis] - extents[axis];
    }
    CFrustum frustum;
    frustum.CreateFromPlanes(planes, 6);
    return frustum;
}

static bool ClipsToBox(const Mesh& mesh, const CDB::TRI& triangle, const Fvector& center, const Fvector& extents, bool requirePolygon = false)
{
    using Point = std::array<double, 3>;
    std::vector<Point> polygon;
    for (const auto index : triangle.verts)
    {
        const auto& vertex = mesh.vertices[index];
        polygon.push_back({ vertex.x, vertex.y, vertex.z });
    }
    for (u32 axis = 0; axis < 3; axis++)
    {
        for (const double sign : { -1., 1. })
        {
            std::vector<Point> clipped;
            const double bound = sign * center[axis] + extents[axis];
            for (size_t i = 0; i < polygon.size(); i++)
            {
                const auto& a = polygon[i];
                const auto& b = polygon[(i + 1) % polygon.size()];
                const double da = sign * a[axis] - bound;
                const double db = sign * b[axis] - bound;
                if (da <= 0)
                    clipped.push_back(a);
                if ((da <= 0) != (db <= 0))
                {
                    Point intersection;
                    for (u32 component = 0; component < 3; component++)
                        intersection[component] = a[component] + da / (da - db) * (b[component] - a[component]);
                    clipped.push_back(intersection);
                }
            }
            if (requirePolygon)
            {
                auto coincident = [](const Point& a, const Point& b)
                {
                    return std::abs(a[0] - b[0]) < EPS_S && std::abs(a[1] - b[1]) < EPS_S && std::abs(a[2] - b[2]) < EPS_S;
                };
                clipped.erase(std::unique(clipped.begin(), clipped.end(), coincident), clipped.end());
                if (clipped.size() > 1 && coincident(clipped.front(), clipped.back()))
                    clipped.pop_back();
                if (clipped.size() < 3)
                    return false;
            }
            polygon = std::move(clipped);
        }
    }
    return !polygon.empty();
}

static void CheckVolumes(CDB::MODEL& model, const Mesh& mesh, const Fvector& center, const Fvector& extents)
{
    std::set<int> expected;
    for (size_t i = 0; i < mesh.triangles.size(); i++)
    {
        if (ClipsToBox(mesh, mesh.triangles[i], center, extents))
            expected.insert(static_cast<int>(i));
    }
    CDB::COLLIDER collider;
    collider.box_query(CDB::OPT_FULL_TEST, &model, center, extents);
    Require(Ids(collider, mesh) == expected, "Box query disagrees with independent polygon clipping");
    collider.box_query(0, &model, center, extents);
    const auto boxCandidates = Ids(collider, mesh);
    Require(std::includes(boxCandidates.begin(), boxCandidates.end(), expected.begin(), expected.end()), "Broad box query missed a triangle");
    collider.box_query(CDB::OPT_FULL_TEST | CDB::OPT_ONLYFIRST, &model, center, extents);
    Require(collider.r_count() == (expected.empty() ? 0u : 1u), "First box hit count is incorrect");
    if (collider.r_count())
        Require(expected.count(collider.r_begin()->id) != 0, "First box hit is outside the box");

    const auto frustum = BoxFrustum(center, extents);
    expected.clear();
    for (size_t i = 0; i < mesh.triangles.size(); i++)
    {
        if (ClipsToBox(mesh, mesh.triangles[i], center, extents, true))
            expected.insert(static_cast<int>(i));
    }
    collider.frustum_query(CDB::OPT_FULL_TEST, &model, frustum);
    Require(Ids(collider, mesh) == expected, "Frustum query disagrees with independent polygon clipping");
    collider.frustum_query(0, &model, frustum);
    const auto candidates = Ids(collider, mesh);
    Require(std::includes(candidates.begin(), candidates.end(), expected.begin(), expected.end()), "Broad frustum query missed a triangle");
    collider.frustum_query(CDB::OPT_FULL_TEST | CDB::OPT_ONLYFIRST, &model, frustum);
    Require(collider.r_count() == (expected.empty() ? 0u : 1u), "First frustum hit count is incorrect");
    if (collider.r_count())
        Require(expected.count(collider.r_begin()->id) != 0, "First frustum hit is outside the frustum");
}

static void CheckRandomQueries()
{
    std::mt19937 random(2139);
    std::uniform_real_distribution<float> coordinate(-20, 20);
    auto point = [&]
    {
        return Vector(coordinate(random), coordinate(random), coordinate(random));
    };
    Mesh mesh;
    for (u32 i = 0; i < 128; i++)
        mesh.Add(point(), point(), point());
    CDB::MODEL model;
    mesh.Build(model);
    for (u32 query = 0; query < 200; query++)
    {
        const auto start = point();
        auto direction = point();
        direction.normalize();
        for (const u32 mode : { 0u, static_cast<u32>(CDB::OPT_CULL) })
        {
            std::set<int> expected;
            float nearest = 100;
            for (size_t i = 0; i < mesh.triangles.size(); i++)
            {
                Fvector vertices[3];
                for (u32 j = 0; j < 3; j++)
                    vertices[j] = mesh.vertices[mesh.triangles[i].verts[j]];
                float u, v, range;
                if (CDB::TestRayTri(start, direction, vertices, u, v, range, mode != 0) && range > 0 && range <= 100)
                {
                    expected.insert(static_cast<int>(i));
                    nearest = std::min(nearest, range);
                }
            }
            CDB::COLLIDER collider;
            collider.ray_query(mode, &model, start, direction, 100);
            Require(Ids(collider, mesh) == expected, "Ray tree traversal disagrees with exhaustive triangle queries");
            collider.ray_query(mode | CDB::OPT_ONLYNEAREST, &model, start, direction, 100);
            Require(collider.r_count() == (expected.empty() ? 0u : 1u), "Nearest ray hit count is incorrect");
            if (collider.r_count())
                Require(std::abs(collider.r_begin()->range - nearest) < 1.e-4f, "Nearest hit distance is incorrect");
        }
        CheckVolumes(model, mesh, point(), Vector(2, 3, 4));
    }
}

static void WriteCacheMetadata(IWriter& writer)
{
    writer.w_u32(0x2139);
}

static void CheckBoundaryQueries()
{
    auto mesh = Fixture();
    CDB::MODEL model;
    mesh.Build(model);
    CDB::COLLIDER collider;
    collider.box_query(CDB::OPT_FULL_TEST, &model, Vector(0, 0, 0), Vector(0, 0, 0));
    Require(Ids(collider, mesh) == std::set<int>{ 0, 1, 2 }, "Point box must include touching triangles");
    collider.box_query(CDB::OPT_FULL_TEST, &model, Vector(0.25f, 0.25f, 0), Vector(0.1f, 0.1f, 0));
    Require(Ids(collider, mesh) == std::set<int>{ 0, 1 }, "Flat box must include coplanar triangles");

    Mesh coincident;
    for (u32 i = 0; i < 17; i++)
        coincident.Add(Vector(0, 0, 200000), Vector(0, 1, 200000), Vector(1, 0, 200000));
    for (u32 i = 0; i < 8; i++)
    {
        CDB::MODEL temporary;
        coincident.Build(temporary);
    }
    CDB::MODEL distant;
    coincident.Build(distant);
    collider.ray_query(0, &distant, Vector(0.25f, 0.25f, 0), Vector(0, 0, 1), 200000);
    Require(collider.r_count() == 17 && Ids(collider, coincident).size() == 17, "Long rays or unsplittable geometry lost triangles");
    collider.box_query(CDB::OPT_FULL_TEST, &distant, Vector(0, 0, 0), Vector(300000, 300000, 300000));
    Require(collider.r_count() == 17, "Large query boxes must not be clamped");
}

static bool ReadCacheMetadata(IReader& reader)
{
    return reader.elapsed() >= 4 && reader.r_u32() == 0x2139;
}

static void CheckCache(CDB::MODEL& model, const Mesh& mesh, pcstr fileName)
{
    model.set_model_crc32(2139);
    Require(model.serialize(fileName, WriteCacheMetadata), "Could not write cache");
    {
        CDB::MODEL loaded;
        loaded.set_model_crc32(2139);
        Require(loaded.deserialize(fileName, false, ReadCacheMetadata), "Could not reload cache");
        CheckRays(loaded, mesh);
        CheckVolumes(loaded, mesh, Vector(0.25f, 0.25f, 0), Vector(0.1f, 0.1f, 0.1f));
    }
    {
        CDB::MODEL wrongSource;
        wrongSource.set_model_crc32(1);
        Require(!wrongSource.deserialize(fileName, false, ReadCacheMetadata), "Cache accepted a different source checksum");
    }
    std::string storedName = fileName;
#ifdef XRAY_USE_JOLT_CDB
    storedName += ".jolt";
#endif
    IWriter* writer = FS.w_open(storedName.c_str());
    writer->w_u8(1);
    FS.w_close(writer);
    {
        CDB::MODEL truncated;
        truncated.set_model_crc32(2139);
        Require(!truncated.deserialize(fileName, false, ReadCacheMetadata), "Truncated cache was accepted");
    }
    FS.file_delete(storedName.c_str());
}

int main(int argc, char** argv)
{
    const pcstr commandLine = argc > 1 ? argv[1] : "";
    Core.Initialize("xrCDBTests", commandLine, false);
    int result = 0;
    try
    {
        auto mesh = Fixture();
        CDB::MODEL model;
        mesh.Build(model);
        CheckRays(model, mesh);
        CheckVolumes(model, mesh, Vector(0.25f, 0.25f, 0), Vector(0.1f, 0.1f, 0.1f));
        CheckVolumes(model, mesh, Vector(0, 0, 0), Vector(10, 10, 10));
        CheckVolumes(model, mesh, Vector(100, 100, 100), Vector(1, 1, 1));
        Require(model.memory() > sizeof(model), "Collision memory accounting is empty");
        const auto cache = (std::filesystem::current_path() / (argc > 1 ? "cdb-threaded.cache" : "cdb-sync.cache")).string();
        CheckCache(model, mesh, cache.c_str());
        CheckBoundaryQueries();
        CheckRandomQueries();
        std::puts("Collision queries, metadata, cache and randomized geometry checks passed");
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        result = 1;
    }
    Core._destroy();
    return result;
}
