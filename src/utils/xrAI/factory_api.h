#pragma once

#include "Common/Platform.hpp"

extern "C" {
    typedef XR_IMPORT  IServerEntity*   __stdcall Factory_Create    (LPCSTR section);
    typedef XR_IMPORT  void             __stdcall Factory_Destroy   (IServerEntity *&);
};

extern Factory_Create   *create_entity;
extern Factory_Destroy  *destroy_entity;

IC  CSE_Abstract *F_entity_Create(LPCSTR section)
{
    IServerEntity   *i = create_entity(section);
    CSE_Abstract    *j = smart_cast<CSE_Abstract*>(i);
    return          (j);
}

IC  void F_entity_Destroy(CSE_Abstract *&i)
{
    IServerEntity   *j = i;
    destroy_entity  (j);
    i               = 0;
}
