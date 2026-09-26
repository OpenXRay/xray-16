#pragma once

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

inline void Require(bool condition, pcstr message)
{
    if (!condition)
        throw std::runtime_error(message);
}

inline Fvector Vector(float x, float y, float z)
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

inline Mesh Fixture()
{
    Mesh mesh;
    mesh.Add(Vector(0, 0, 0), Vector(0, 1, 0), Vector(1, 0, 0));
    mesh.Add(Vector(0, 0, 0), Vector(0, 1, 0), Vector(1, 0, 0));
    mesh.Add(Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 1, 0));
    mesh.Add(Vector(0, 0, 3), Vector(0, 1, 3), Vector(1, 0, 3));
    mesh.Add(Vector(0, 0, 6), Vector(1, 0, 6), Vector(0, 1, 6));
    return mesh;
}

inline std::set<int> Ids(CDB::COLLIDER& collider, const Mesh& mesh)
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

inline std::string TestPath(pcstr name)
{
    return (std::filesystem::current_path() / (std::string(name) + (strstr(Core.Params, "-mt_cdb") ? "-threaded" : "-sync"))).string();
}

void CheckCacheFailures();
void CheckConstructionLifecycle();
void CheckObjectSpace();
void CheckFrustumPruning();
