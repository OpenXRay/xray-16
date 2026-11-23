#include "TestSupport.h"
#include "PhysicsContacts.h"
#include "xrPhysicsCore/IPhysicsCore.h"

namespace {
#ifdef XRAY_USE_JOLT_CDB
unsigned contactCount;
bool contactValid;
bool Contact(const NativePhysicsContact& contact) {
    ++contactCount;
    contactValid = contactValid && (contact.material1 == 4099 || contact.material2 == 4099) &&
        (contact.triangle1 < 2 || contact.triangle2 < 2) &&
        std::abs(contact.depth - .25f) < .001f && std::abs(std::abs(contact.normal.y) - 1) < .001f;
    return true;
}
#endif
}
void CheckPhysicsContacts()
{
#ifdef XRAY_USE_JOLT_CDB
    auto* core = GetPhysicsCore();
    core->Clear();
    core->SetSimulationParameters(0, 18);
    BodyHandle floorBody;
    {
        Mesh floor;
        floor.Add(Vector(-5, 0, -5), Vector(-5, 0, 5), Vector(5, 0, -5));
        floor.Add(Vector(5, 0, 5), Vector(5, 0, -5), Vector(-5, 0, 5));
        for (auto& triangle : floor.triangles) triangle.material = 4099;
        CDB::MODEL model;
        floor.Build(model);
        auto first = model.acquire_physics_shape(), second = model.acquire_physics_shape();
        Require(first && first == second, "Level queries and physics did not share one native mesh");
        floorBody = core->CreateStaticBody(first, Vector(0, 0, 0));
        std::vector<CDBRaycastHit> hits;
        core->RaycastCDBModel(first, Vector(1, 2, 1), Vector(0, -1, 0), 4, CDBRayMode::Nearest, false, hits);
        Require(hits.size() == 1 && hits[0].tri_index < 2 && std::abs(hits[0].range - 2) < .001f,
            "Native shared-mesh ray lost its original triangle ID");
        core->DestroyCDBModel(first);
        core->DestroyCDBModel(second);
    } // The body retains the mesh and its materials after MODEL destruction.
    const auto body = core->CreateBox(Vector(.5f, .5f, .5f), Vector(1, .25f, 1), 2);
    core->SetBodyUserData(body, core);
    contactCount = 0;
    contactValid = true;
    core->SetRigidBodyContactCallback(Contact);
    core->Step(.01f);
    Require(contactCount > 0 && contactValid, "Shared level mesh lost depth, normal, or 14-bit material metadata");
    core->SetBodyPosition(body, Vector(1, 2, 1));
    core->SetBodyLinearVelocity(body, Vector(0, 0, 0));
    contactCount = 0;
    core->Step(.01f);
    Require(contactCount == 0, "Native contacts reused stale level collision candidates");
    core->SetRigidBodyContactCallback(nullptr);
    core->DestroyBody(body);
    core->DestroyBody(floorBody);
    core->Clear();
#endif
}
