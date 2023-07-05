#pragma once

#include "xrServerEntities/xrServer_Object_Base.h"
#include "utils/xrSE_Factory/xrSE_Factory_import_export.h"

IC CSE_Abstract* F_entity_Create(LPCSTR section) 
{ 
    IServerEntity* i = xrSE_Factory::create_entity(section);
    CSE_Abstract* j = smart_cast<CSE_Abstract*>(i);
    return (j);
}

IC void F_entity_Destroy(CSE_Abstract*& i)
{
    IServerEntity* j = i;
    xrSE_Factory::destroy_entity(j);
    i = 0;
}
