////////////////////////////////////////////////////////////////////////////
//	Created		: 23.03.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "net_execution_vertex_light.h"
#include "xrlc_globaldata.h"
#include "xrface.h"
#include "light_point.h"
#include "xrCDB/xrCDB.h"
extern XRLC_LIGHT_API void LightPoint(CDB::COLLIDER* DB, CDB::MODEL* MDL, base_color_c& C, Fvector& P, Fvector& N,
    base_lighting& lights, u32 flags, Face* skip);
void g_trans_register(Vertex* V);
namespace lc_net
{
static const u32 send_receive_task_buffer_size = 16;
void net_execution_vertex_light::send_task(IGenericStream* outStream)
{
    {
        R_ASSERT(start != u32(-1));
        R_ASSERT(end != u32(-1));
        R_ASSERT(start < end);
        u8 buff[send_receive_task_buffer_size];
        INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
        // INetIWriterGenStream w( outStream, send_receive_task_buffer_size );

        w.w_u32(start);
        w.w_u32(end);
    }
}
bool net_execution_vertex_light::receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    u8 buff[send_receive_task_buffer_size];
    INetBlockReader r(inStream, buff, sizeof(buff));
    // INetReaderGenStream r( inStream );
    start = r.r_u32();
    end = r.r_u32();
    R_ASSERT(start != u32(-1));
    R_ASSERT(end != u32(-1));
    R_ASSERT(start < end);

    return true;
}
static const u32 send_receive_result_buffer_size = 512 * 1024;
void net_execution_vertex_light::receive_result(IGenericStream* outStream)
{
    u8 buff[send_receive_result_buffer_size];
    INetBlockReader r(outStream, buff, sizeof(buff));
    // INetReaderGenStream r(outStream);
    u32 _start = r.r_u32();
    u32 _end = r.r_u32();
    VERIFY(_start == start);
    VERIFY(_end == end);
    for (u32 i = start; i < end; ++i)
    {
        Vertex* V = lc_global_data()->g_vertices()[i];
        V->read(r);
        g_trans_register(V);
    }

    // inlc_global_data()->g_deflectors()[ deflector_id ]->read( r );
}

void net_execution_vertex_light::send_result(IGenericStream* outStream)
{
    u8 buff[send_receive_result_buffer_size];
    INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
    // INetIWriterGenStream w( outStream, send_receive_result_buffer_size );
    VERIFY(start != u32(-1));
    VERIFY(end != u32(-1));
    w.w_u32(start);
    w.w_u32(end);
    for (u32 i = start; i < end; ++i)
    {
        Vertex* V = lc_global_data()->g_vertices()[i];
        V->write(w);
    }
    return;
}
bool net_execution_vertex_light::execute(net_task_callback& net_callback)
{
    for (u32 i = start; i < end; ++i)
    {
        Vertex* V = lc_global_data()->g_vertices()[i];

        base_color_c vC, old;
        V->C._get(old);

        CDB::COLLIDER DB;
        DB.ray_options(0);
        LightPoint(&DB, lc_global_data()->RCAST_Model(), vC, V->P, V->N, lc_global_data()->L_static(),
            (lc_global_data()->b_nosun() ? LP_dont_sun : 0) | LP_dont_hemi, 0);
        // vC._tmp_			= v_trans; //we olready have it in V->C.t
        vC.mul(.5f);
        vC.hemi = old.hemi; // preserve pre-calculated hemisphere
        V->C._set(vC.rgb, vC.hemi, vC.sun);
    }

    return true; //! net_callback.break_all();
}

void net_execution_vertex_light::construct(u32 _start, u32 _end)
{
    start = _start;
    end = _end;
}
};
