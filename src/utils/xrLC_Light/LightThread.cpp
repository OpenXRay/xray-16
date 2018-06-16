#include "stdafx.h"

#include "LightThread.h"

#include "global_calculation_data.h"

void LightThread::Execute()
{
    //		DetailSlot::verify	();
    CDB::COLLIDER DB;
    DB.ray_options(CDB::OPT_CULL);
    DB.box_options(CDB::OPT_FULL_TEST);
    base_lighting Selected;

    for (u32 _z = Nstart; _z < Nend; _z++)
    {
        for (u32 _x = 0; _x < gl_data.slots_data.size_x(); _x++)
        {
            DetailSlot& DS = gl_data.slots_data.get_slot(_x, _z);
            if (!detail_slot_process(_x, _z, DS))
                continue;
            if (!detail_slot_calculate(_x, _z, DS, box_result, DB, Selected))
                continue; //?
            gl_data.slots_data.set_slot_calculated(_x, _z);

            thProgress = float(_z - Nstart) / float(Nend - Nstart);
            thPerformance = float(double(t_count) / double(t_time * CPU::clk_to_seconds)) / 1000.f;
        }
    }
}
