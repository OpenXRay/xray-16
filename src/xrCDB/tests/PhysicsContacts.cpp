#include "xrPhysics/StdAfx.h"

#include "Common/LevelStructure.hpp"
#include "TestSupport.h"
#include "xrCDB/xr_area.h"
#include "xrPhysics/ExtendedGeom.h"
#include "xrPhysics/PHWorld.h"

void CheckPhysicsContacts()
{
    Mesh floor;
    floor.Add(Vector(-5, 0, -5), Vector(-5, 0, 5), Vector(5, 0, -5));
    floor.Add(Vector(5, 0, 5), Vector(5, 0, -5), Vector(-5, 0, 5));
    for (auto& triangle : floor.triangles)
        triangle.material = 7;
    hdrCFORM header{};
    header.version = CFORM_CURRENT_VERSION;
    header.vertcount = floor.vertices.size();
    header.facecount = floor.triangles.size();
    header.aabb.set(-10, -10, -10, 10, 10, 10);
    CObjectSpace space(nullptr);
    space.GetStaticModel()->set_model_crc32(999);
    space.Create(floor.vertices.data(), floor.triangles.data(), header, nullptr, nullptr, nullptr, nullptr);
    space.GetStaticModel()->syncronize();
    Require(physics_world() == nullptr, "Physics fixture found an existing world");
    create_physics_world(false, &space, nullptr);

    struct World
    {
        ~World()
        {
            destroy_physics_world();
        }
    } world;

    auto* concrete = static_cast<CPHWorld*>(physics_world());
    auto dynamics = dWorldCreate();

    struct Dynamics
    {
        dWorldID world;

        ~Dynamics()
        {
            dWorldDestroy(world);
        }
    } dynamicsCleanup{ dynamics };

    auto body = dBodyCreate(dynamics);

    struct Body
    {
        dBodyID body;

        ~Body()
        {
            dBodyDestroy(body);
        }
    } bodyCleanup{ body };

    auto sphere = dCreateSphere(nullptr, .5f);
    dGeomSetBody(sphere, body);
    dGeomCreateUserData(sphere);

    struct Sphere
    {
        dGeomID geometry;

        ~Sphere()
        {
            dGeomDestroyUserData(geometry);
            dGeomDestroy(geometry);
        }
    } cleanup{ sphere };

    dGeomSetPosition(sphere, 1, .25f, 1);
    std::array<dContact, 16> contacts{};
    const auto count = dCollide(concrete->GetMeshGeom(), sphere, contacts.size(), &contacts[0].geom, sizeof(dContact));
    Require(count > 0, "ODE contact query missed the collision mesh");
    for (int i = 0; i < count; i++)
    {
        Require(std::abs(contacts[i].geom.depth - .25f) < 1.e-4f, "ODE contact depth changed");
        Require(std::abs(std::abs(contacts[i].geom.normal[1]) - 1.f) < 1.e-4f, "ODE contact normal changed");
        Require(contacts[i].surface.mode == 7, "ODE contact lost the triangle material");
    }
    dGeomSetPosition(sphere, 1, 2, 1);
    Require(dCollide(concrete->GetMeshGeom(), sphere, contacts.size(), &contacts[0].geom, sizeof(dContact)) == 0, "ODE reused stale collision candidates");
}
