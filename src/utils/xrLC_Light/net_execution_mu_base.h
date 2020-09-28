////////////////////////////////////////////////////////////////////////////
//	Created		: 03.06.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef NET_EXECUTION_MU_BASE_H_INCLUDED
#define NET_EXECUTION_MU_BASE_H_INCLUDED
#include "net_execution.h"

namespace lc_net
{
typedef class tnet_execution_base<et_mu_base_light>::net_execution_impl execution_mu_base_light;
template <>
class tnet_execution_base<et_mu_base_light>::net_execution_impl
{
    u32 mu_model_id;

public:
    net_execution_impl() : mu_model_id(u32(-1)) {}
    void construct(u32 id) { mu_model_id = id; }
    void send_task(IGenericStream* outStream);
    void receive_result(IGenericStream* inStream);
    bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream);
    void send_result(IGenericStream* outStream);
    bool execute(net_task_callback& net_callback);
};
template <>
struct add_global<et_mu_base_light, gl_base_cl_data>
{
};

// execution_lightmaps
};

#endif // #ifndef NET_EXECUTION_MU_BASE_H_INCLUDED
