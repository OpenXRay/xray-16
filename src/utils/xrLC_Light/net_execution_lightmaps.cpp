#include "stdafx.h"

#include "net_execution_lightmaps.h"
#include "net_task.h"
#include "xrlc_globaldata.h"
#include "xrdeflector.h"
#include "light_execute.h"
#include "xrdeflector.h"

#include "execute_statistics.h"
#include "xrCore/ftimer.h"

namespace lc_net
{
static const u32 send_receive_task_buff_size = 8;
void execution_lightmaps::send_task(IGenericStream* outStream)
{
    {
        u8 buff[send_receive_task_buff_size];
        INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
        // INetIWriterGenStream w( outStream, send_receive_task_buff_size );

        R_ASSERT(from != u32(-1));
        R_ASSERT(to != u32(-1));
        R_ASSERT(from < to);
        w.w_u32(from);
        w.w_u32(to);
    }
}

bool execution_lightmaps::receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    u8 buff[send_receive_task_buff_size];

    INetBlockReader r(inStream, buff, send_receive_task_buff_size);
    // INetReaderGenStream r( inStream );
    from = r.r_u32();
    to = r.r_u32();
    R_ASSERT(from != u32(-1));
    R_ASSERT(to != u32(-1));
    R_ASSERT(from < to);
    return true;
}

static const u32 send_receive_result_buff_size = 512 * 1024;
void execution_lightmaps::send_result(IGenericStream* outStream)
{
    u8 buff[send_receive_result_buff_size];
    INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
    // INetIWriterGenStream w( outStream, send_receive_result_buff_size );
    // VERIFY(deflector_id!= u32(-1));
    // w.w_u32( deflector_id );
    R_ASSERT(from != u32(-1));
    R_ASSERT(to != u32(-1));
    R_ASSERT(from < to);
    w.w_u32(from);
    w.w_u32(to);
    for (u32 i = from; i < to; ++i)
    {
        CDeflector* D = inlc_global_data()->g_deflectors()[i];
        D->send_result(w);
        lm_layer& lm = D->layer;
        lm.destroy();
    }
#ifdef COLLECT_EXECUTION_STATS
    write_statistics(w);
#endif
    return;
}
void execution_lightmaps::receive_result(IGenericStream* outStream)
{
    u8 buff[send_receive_result_buff_size];
    INetBlockReader r(outStream, buff, sizeof(buff));
    // INetReaderGenStream r(outStream);
    // u32 id = r.r_u32();
    // VERIFY(id==deflector_id);

    u32 _from = r.r_u32();
    u32 _to = r.r_u32();

    R_ASSERT(_from != u32(-1));
    R_ASSERT(_to != u32(-1));
    R_ASSERT(_from < _to);
    R_ASSERT(_from == from);
    R_ASSERT(_to == to);
    for (u32 i = from; i < to; ++i)
        inlc_global_data()->g_deflectors()[i]->receive_result(r);
#ifdef COLLECT_EXECUTION_STATS
    read_statistics(r);
    statistic_log();
#endif
}
bool execution_lightmaps::execute(net_task_callback& net_callback)
{
#ifdef COLLECT_EXECUTION_STATS
    CTimer gtimer;
    gtimer.Start();
    // net_callback.agent().GetSessionCacheDirectory( net_callback.session(), statistics.dir );
    u32 sz = sizeof(statistics.dir);
    GetComputerName(statistics.dir, &sz);
#endif

    for (u32 i = from; i < to; ++i)
    {
#ifdef COLLECT_EXECUTION_STATS

        CTimer timer;
        timer.Start();
#endif
        CDeflector* D = inlc_global_data()->g_deflectors()[i];
        VERIFY(D);
        lm_layer& lm = D->layer;
        lm.create(lm.width, lm.height);
        D->_net_session = &net_callback;
        light_execute().run(*D);
        D->_net_session = 0;
#ifdef COLLECT_EXECUTION_STATS

        D->time_stat.m_time = timer.GetElapsed_sec();
#endif

        if (net_callback.break_all())
            break;
    }
#ifdef COLLECT_EXECUTION_STATS
    statistics.time_stats.m_time = gtimer.GetElapsed_sec();
#endif
    return !net_callback.break_all();
}

#ifdef COLLECT_EXECUTION_STATS
void execution_lightmaps::statistic_log()
{
    Msg("STATISTICS ");
    Msg("deflectors from: %d, to: %d ", from, to);
    Msg("deflectors number: %d", to - from);
    statistics.log();

    for (u32 i = from; i < to; ++i)
    {
        CDeflector* D = inlc_global_data()->g_deflectors()[i];
        D->statistic_log();
    }
}
void execution_lightmaps::read_statistics(INetReader& r)
{
    statistics.read(r);
    for (u32 i = from; i < to; ++i)
    {
        CDeflector* D = inlc_global_data()->g_deflectors()[i];
        D->time_stat.read(r);
    }
}
void execution_lightmaps::write_statistics(IWriter& w) const
{
    statistics.write(w);
    for (u32 i = from; i < to; ++i)
    {
        CDeflector* D = inlc_global_data()->g_deflectors()[i];
        D->time_stat.write(w);
    }
}
#endif
};
