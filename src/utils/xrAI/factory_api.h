#pragma once

#include "xrServerEntities/xrServer_Object_Base.h"

extern "C" {
using CreateEntity = XR_IMPORT IServerEntity*(LPCSTR section);
using DestroyEntity = XR_IMPORT void(IServerEntity*&);
};

extern CreateEntity* create_entity;
extern DestroyEntity* destroy_entity;

IC CSE_Abstract* F_entity_Create(LPCSTR section)
{
    IServerEntity* i = create_entity(section);
    CSE_Abstract* j = smart_cast<CSE_Abstract*>(i);
    return (j);
}

IC void F_entity_Destroy(CSE_Abstract*& i)
{
    IServerEntity* j = i;
    destroy_entity(j);
    i = 0;
}
