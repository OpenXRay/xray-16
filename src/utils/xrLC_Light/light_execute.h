#pragma once

#include "base_lighting.h"
#include "xrCDB/xrCDB.h"
#include "xrdeflectordefs.h"

class light_execute
{
    HASH H;
    CDB::COLLIDER DB;
    base_lighting LightsSelected;

public:
    void run(CDeflector& D);
};
