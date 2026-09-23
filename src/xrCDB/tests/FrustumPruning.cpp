#include "Instrumentation.h"
#include "TestSupport.h"

static u32 boundsVisited, trianglesClipped;

static bool Clips(const Mesh& mesh, const CDB::TRI& triangle, const std::vector<Fplane>& planes)
{
    using Point = std::array<double, 3>;
    std::vector<Point> polygon;
    for (auto index : triangle.verts)
    {
        const auto& v = mesh.vertices[index];
        polygon.push_back({ v.x, v.y, v.z });
    }
    for (const auto& p : planes)
    {
        auto distance = [&](const Point& v)
        {
            return p.n.x * v[0] + p.n.y * v[1] + p.n.z * v[2] + p.d;
        };
        std::vector<Point> clipped;
        for (size_t i = 0; i < polygon.size(); i++)
        {
            const auto& a = polygon[i];
            const auto& b = polygon[(i + 1) % polygon.size()];
            auto da = distance(a), db = distance(b);
            if (da <= 0)
                clipped.push_back(a);
            if ((da <= 0) != (db <= 0))
            {
                Point point;
                for (u32 axis = 0; axis < 3; axis++)
                    point[axis] = a[axis] + da / (da - db) * (b[axis] - a[axis]);
                clipped.push_back(point);
            }
        }
        polygon = std::move(clipped);
        if (polygon.empty())
            return false;
    }
    return polygon.size() >= 3;
}

void CheckFrustumPruning()
{
    Mesh mesh;
    for (u32 x = 0; x < 32; x++)
        for (u32 y = 0; y < 32; y++)
            mesh.Add(Vector(x * 10.f, y * 10.f, 0), Vector(x * 10.f, y * 10.f + 1, .3f), Vector(x * 10.f + 1, y * 10.f, 1));
    CDB::MODEL model;
    mesh.Build(model);
    model.syncronize();
    CDB::SetTestObserver(
        [](CDB::TestEvent event, const CDB::MODEL*)
        {
            if (event == CDB::TestEvent::FrustumBounds)
                boundsVisited++;
            if (event == CDB::TestEvent::FrustumTriangle)
                trianglesClipped++;
        });

    struct Reset
    {
        ~Reset()
        {
            CDB::SetTestObserver(nullptr);
        }
    } reset;

    for (const auto center : { Vector(100, 100, 0), Vector(160.5f, 170.5f, 0), Vector(-100, -100, -100) })
        for (const u32 count : { 6u, 12u })
        {
            std::vector<Fplane> planes;
            for (u32 axis = 0; axis < count / 2; axis++)
            {
                const auto normal = axis == 2 ? Vector(0, 0, 1) : Vector(std::cos(.61f + axis * 1.1f), std::sin(.61f + axis * 1.1f), 0);
                for (float sign : { -1.f, 1.f })
                {
                    Fplane p;
                    p.n.mul(normal, sign);
                    p.d = -p.n.dotproduct(center) - 2.3f;
                    planes.push_back(p);
                }
            }
            CFrustum frustum;
            frustum.CreateFromPlanes(planes.data(), planes.size());
            std::set<int> expected;
            for (size_t i = 0; i < mesh.triangles.size(); i++)
                if (Clips(mesh, mesh.triangles[i], planes))
                    expected.insert(i);
            CDB::COLLIDER collider;
            boundsVisited = trianglesClipped = 0;
            collider.frustum_query(CDB::OPT_FULL_TEST, &model, frustum);
            Require(Ids(collider, mesh) == expected, "Rotated frustum disagrees with independent clipping");
#ifdef XRAY_USE_JOLT_CDB
            if (boundsVisited >= 256 || trianglesClipped >= 64)
                std::fprintf(stderr, "Frustum visits: bounds=%u triangles=%u planes=%u\n", boundsVisited, trianglesClipped, count);
            Require(boundsVisited < 256 && trianglesClipped < 64, "Frustum query scanned unrelated branches");
#endif
            collider.frustum_query(0, &model, frustum);
            const auto candidates = Ids(collider, mesh);
            Require(std::includes(candidates.begin(), candidates.end(), expected.begin(), expected.end()), "Broad frustum lost hits");
            if (center.x < 0)
                Require(candidates.empty(), "Disjoint frustum returned candidates");
            collider.frustum_query(CDB::OPT_ONLYFIRST, &model, frustum);
            Require(collider.r_count() == (candidates.empty() ? 0u : 1u), "Broad first-hit count changed");
        }
}
