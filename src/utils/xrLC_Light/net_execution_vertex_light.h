////////////////////////////////////////////////////////////////////////////
//	Created		: 23.03.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef NET_EXECUTION_VERTEX_LIGHT_H_INCLUDED
#define NET_EXECUTION_VERTEX_LIGHT_H_INCLUDED

#include "net_execution.h"

class net_task_callback;

namespace lc_net
{
typedef class tnet_execution_base<et_vertex_light>::net_execution_impl net_execution_vertex_light;
template <>
class tnet_execution_base<et_vertex_light>::net_execution_impl
{
    u32 start;
    u32 end;

public:
    net_execution_impl() : start(u32(-1)), end(-1) {}
    void construct(u32 _start, u32 _end); // { start = _start;end = _end; }
    void send_task(IGenericStream* outStream);
    void receive_result(IGenericStream* outStream);
    bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream);
    void send_result(IGenericStream* outStream);
    bool execute(net_task_callback& net_callback);
};
template <>
struct add_global<et_vertex_light, gl_lm_data>
{
};
// execution_lightmaps
};

// class net_execution_vertex_light {
// public:
//
//
// private:
//
//}; // class net_execution_vertex_light

#endif // #ifndef NET_EXECUTION_VERTEX_LIGHT_H_INCLUDED
