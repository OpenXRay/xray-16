#pragma once

#include "xrPhysics/iphysics_scripted.h"
#include "xrPhysics/IPHWorld.h"

class CPHCondition;
class CPHAction;

class cphysics_world_scripted : public cphysics_game_scripted<IPHWorld>
{
public:
    cphysics_world_scripted(IPHWorld* imp) : cphysics_game_scripted<IPHWorld>(imp) {}
    float Gravity() { return physics_impl().Gravity(); }
    void SetGravity(float g) { return physics_impl().SetGravity(g); }
    void AddCall(CPHCondition* c, CPHAction* a);
};
