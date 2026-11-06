#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrPhysics/PHContactBodyEffector.h"
#include "xrPhysics/BlockAllocator.h"

#include <ode/ode.h>

#include <array>
#include <cstdio>

int main()
{
    Core.Initialize("ContactBodyEffectorLifetime", "", false);

    SGameMtl material;
    material.fFlotationFactor = 0.f;
    material.Flags.set(SGameMtl::flPassable, true);
    dContact contact{};

    // An ordinary live effector must still apply its force and clear body data.
    dBodyID live = dBodyCreate(nullptr);
    dMass mass{};
    dMassSetSphere(&mass, 1.f, 1.f);
    dBodySetMass(live, &mass);
    dBodySetLinearVel(live, 1.f, 0.f, 0.f);
    CPHContactBodyEffector live_effector;
    live_effector.Init(live, contact, &material);
    dBodySetData(live, &live_effector);
    live_effector.Apply();
    if (dBodyGetData(live) || dBodyGetForce(live)[0] >= 0.f)
        return 1;
    dBodyDestroy(live);

    // The issue's order: a temporary body dies before its effector is applied.
    dBodyID temporary = dBodyCreate(nullptr);
    CPHContactBodyEffector pending_effector;
    pending_effector.Init(temporary, contact, &material);
    dBodySetData(temporary, &pending_effector);
    CPHContactBodyEffector::InvalidateBody(temporary);
    if (dBodyGetData(temporary))
        return 2;
    dBodyDestroy(temporary);
    pending_effector.Apply();

    // The per-step allocator must visit every pending effector exactly once,
    // including the last partial block and the 128-item boundary.
    CBlockAllocator<CPHContactBodyEffector, 128> effectors;
    std::array<dBodyID, 130> bodies;
    for (auto& body : bodies)
    {
        body = dBodyCreate(nullptr);
        auto* effector = effectors.add();
        effector->Init(body, contact, &material);
        dBodySetData(body, effector);
    }
    int visited = 0;
    effectors.for_each([&visited](CPHContactBodyEffector* effector)
    {
        ++visited;
        effector->Detach();
    });
    if (visited != bodies.size())
        return 3;
    for (auto body : bodies)
    {
        if (dBodyGetData(body))
            return 4;
        dBodyDestroy(body);
    }
    effectors.empty();

    // The allocator reuses its first block on the next step.
    dBodyID reused = dBodyCreate(nullptr);
    auto* reused_effector = effectors.add();
    reused_effector->Init(reused, contact, &material);
    dBodySetData(reused, reused_effector);
    visited = 0;
    effectors.for_each([&visited](CPHContactBodyEffector* effector)
    {
        ++visited;
        effector->Detach();
    });
    if (visited != 1)
        return 5;
    if (dBodyGetData(reused))
        return 6;
    dBodyDestroy(reused);
    effectors.empty();

    Core._destroy();
    std::puts("Contact body effector lifetime passed");
    return 0;
}
