#ifndef _NET_EXECUTION_IMPLICIT_LIGHT_H_
#define _NET_EXECUTION_IMPLICIT_LIGHT_H_
#include "net_execution.h"
#include "xrlight_implicit.h"

namespace lc_net
{
typedef class tnet_execution_base<et_implicit_light>::net_execution_impl execution_implicit_light;
template <>
class tnet_execution_base<et_implicit_light>::net_execution_impl
{
    ImplicitExecute exec;

public:
    void construct(const ImplicitExecute& _e) { exec = _e; }
    void send_task(IGenericStream* outStream);
    void receive_result(IGenericStream* inStream);
    bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream);
    void send_result(IGenericStream* outStream);
    bool execute(net_task_callback& net_callback);
};
// template<> struct add_global<et_implicit_light, gl_cl_data>{};
template <>
struct add_global<et_implicit_light, gl_implicit_cl_data>
{
};
// execution_lightmaps
};
#endif
