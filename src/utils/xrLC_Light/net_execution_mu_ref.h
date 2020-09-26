#ifndef _MU_EXECUTION_MU_REF_H_
#define _MU_EXECUTION_MU_REF_H_
#include "net_execution.h"

namespace lc_net
{
typedef class tnet_execution_base<et_mu_ref_light>::net_execution_impl execution_mu_ref_light;
template <>
class tnet_execution_base<et_mu_ref_light>::net_execution_impl
{
    u32 mu_ref_id;

public:
    net_execution_impl() : mu_ref_id(u32(-1)) {}
    void construct(u32 id) { mu_ref_id = id; }
    void send_task(IGenericStream* outStream);
    void receive_result(IGenericStream* inStream);
    bool receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream);
    void send_result(IGenericStream* outStream);
    bool execute(net_task_callback& net_callback);
};
template <>
struct add_global<et_mu_ref_light, gl_ref_model_data>
{
};

// execution_lightmaps
};

#endif
