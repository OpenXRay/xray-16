////////////////////////////////////////////////////////////////////////////
//	Created		: 27.03.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "net_execution_detail_light.h"
#include "serialize.h"
#include "global_calculation_data.h"
#include "detail_slot_calculate.h"

namespace lc_net
{
static const u32 send_receive_result_buff_size = 512 * 1024;
void net_execution_detail_light::receive_result(IGenericStream* outStream)
{
    u8 buff[send_receive_result_buff_size];
    INetBlockReader r(outStream, buff, sizeof(buff));
    // INetReaderGenStream r(outStream);
    u32 _start = r.r_u32();
    u32 _end = r.r_u32();
    VERIFY(_start == start);
    VERIFY(_end == end);

    for (u32 i = start; i < end; ++i)
    {
        int x, z;
        gl_data.slots_data.header().slot_x_z(i, x, z);

        if (gl_data.slots_data.calculate_ignore(x, z))
            continue;

        DetailSlot& DS = gl_data.slots_data.get_slot(x, z);
        r_pod(r, DS);
        gl_data.slots_data.set_slot_calculated(x, z);
    }
}

void net_execution_detail_light::send_result(IGenericStream* outStream)
{
    u8 buff[send_receive_result_buff_size];
    INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
    // INetIWriterGenStream w( outStream, 1024*1024 );
    VERIFY(start != u32(-1));
    VERIFY(end != u32(-1));
    w.w_u32(start);
    w.w_u32(end);
    for (u32 i = start; i < end; ++i)
    {
        int x, z;
        gl_data.slots_data.header().slot_x_z(i, x, z);
        if (gl_data.slots_data.calculate_ignore(x, z))
            continue;
        DetailSlot& DS = gl_data.slots_data.get_slot(x, z);
        w_pod(w, DS);
    }
}
static const u32 send_receive_task_buff_size = 8;
void net_execution_detail_light::send_task(IGenericStream* outStream)
{
    {
        R_ASSERT(start != u32(-1));
        R_ASSERT(end != u32(-1));
        u8 buff[send_receive_task_buff_size];
        INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
        // INetIWriterGenStream w( outStream, 100 );
        w.w_u32(start);
        w.w_u32(end);
    }
}
bool net_execution_detail_light::receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    u8 buff[send_receive_task_buff_size];
    INetBlockReader r(inStream, buff, sizeof(buff));
    // INetReaderGenStream r( inStream );
    start = r.r_u32();
    end = r.r_u32();
    R_ASSERT(start != u32(-1));
    R_ASSERT(end != u32(-1));

    return true;
}

bool net_execution_detail_light::execute(net_task_callback& net_callback)
{
    DWORDVec box_result;
    ////////////////////////////////////////////////////////
    CDB::COLLIDER DB;
    DB.ray_options(CDB::OPT_CULL);
    DB.box_options(CDB::OPT_FULL_TEST);
    base_lighting Selected;

    for (u32 i = start; i < end; ++i)
    {
        int x, z;
        gl_data.slots_data.header().slot_x_z(i, x, z);
        if (gl_data.slots_data.calculate_ignore(x, z))
            continue;
        DetailSlot& DS = gl_data.slots_data.get_slot(x, z);
        detail_slot_calculate(x, z, DS, box_result, DB, Selected);
        if (!net_callback.test_connection())
            break;
    }

    return !net_callback.break_all();
}

void net_execution_detail_light::construct(u32 _start, u32 _end)
{
    start = _start;
    end = _end;
}
};
