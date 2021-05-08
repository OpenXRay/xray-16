////////////////////////////////////////////////////////////////////////////
//	Created		: 27.03.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef NET_EXECUTION_DETAIL_LIGHT_H_INCLUDED
#define NET_EXECUTION_DETAIL_LIGHT_H_INCLUDED

#include "net_execution.h"

class net_task_callback;

namespace lc_net
{
typedef class tnet_execution_base<et_detail_light>::net_execution_impl net_execution_detail_light;
template <>
class tnet_execution_base<et_detail_light>::net_execution_impl
{
    u32 start;
    u32 end;

public:
    net_execution_impl() : start(u32(-1)), end(-1) {}
    void construct(u32 _x, u32 _z); // { start = _start;end = _end; }
    void send_task(IGenericStream* outStream);
    void receive_result(IGenericStream* outStream);
    bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream);
    void send_result(IGenericStream* outStream);
    bool execute(net_task_callback& net_callback);
};
template <>
struct add_global<et_detail_light, gl_detail_cl_data>
{
};
// execution_lightmaps
};

#endif // #ifndef NET_EXECUTION_DETAIL_LIGHT_H_INCLUDED
