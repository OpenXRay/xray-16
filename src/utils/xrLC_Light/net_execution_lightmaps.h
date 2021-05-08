#ifndef _NET_EXECUTION_LIGHTMAPS_H_
#define _NET_EXECUTION_LIGHTMAPS_H_
#include "net_execution.h"
#include "execute_statistics.h"

class net_task_callback;

namespace lc_net
{
typedef class tnet_execution_base<et_lightmaps>::net_execution_impl execution_lightmaps;
template <>
class tnet_execution_base<et_lightmaps>::net_execution_impl
{
    u32 from;
    u32 to;

public:
    net_execution_impl() : from(u32(-1)), to(u32(-1)) {}
    void construct(u32 _from, u32 _to)
    {
        from = _from;
        to = _to;
    }
    void send_task(IGenericStream* outStream);
    void receive_result(IGenericStream* outStream);
    bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream);
    void send_result(IGenericStream* outStream);
    bool execute(net_task_callback& net_callback);
#ifdef COLLECT_EXECUTION_STATS
    execute_statistics statistics;
    void statistic_log();
    void read_statistics(INetReader& r);
    void write_statistics(IWriter& w) const;
#endif
};
template <>
struct add_global<et_lightmaps, gl_lm_data>
{
};
// execution_lightmaps
};
#endif
