#include "stdafx.h"

#include "LightThread.h"

#include "global_calculation_data.h"

void LightThread::Execute()
{
    //		DetailSlot::verify	();
    CDB::COLLIDER DB;
    base_lighting Selected;

    for (u32 _z = Nstart; _z < Nend; _z++)
    {
        for (u32 _x = 0; _x < gl_data.slots_data.size_x(); _x++)
        {
            DetailSlot& DS = gl_data.slots_data.get_slot(_x, _z);
            if (!detail_slot_process(_x, _z, DS))
                continue;
            if (!detail_slot_calculate(_x, _z, DS, box_result, DB, CDB::OPT_FULL_TEST, CDB::OPT_CULL, Selected))
                continue; //?
            gl_data.slots_data.set_slot_calculated(_x, _z);

            thProgress = float(_z - Nstart) / float(Nend - Nstart);

            const auto secs = std::chrono::duration_cast<std::chrono::seconds>(t_time).count();
            thPerformance = float(double(t_count) / double(secs)) / 1000.f;
        }
    }
}
