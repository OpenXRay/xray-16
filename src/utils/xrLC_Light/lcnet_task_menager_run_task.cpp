#include "stdafx.h"

#include "lcnet_task_manager.h"
#include "net_execution.h"
#include "net_exec_pool.h"
namespace lc_net
{
bool task_manager::run_task(IAgent* agent, u32 sessionId, IGenericStream* inStream, IGenericStream* outStream)
{
    if (!initialize_session(sessionId))
        return false;

    u8 pool_id(u8(-1));

    net_execution* e = receive_task(pool_id, agent, sessionId, inStream);
    ///////////////////////////////////////////////////////
    // inStream->Clear( );
    //////////////////////////////////////////////////////
    R_ASSERT(pools[pool_id]);
    if (!e)
        return false;

    if (!e->execute(agent, sessionId))
    {
        pools[pool_id]->remove_task(e);
        return false;
    }
    send_result(pool_id, outStream, *e);
    pools[pool_id]->remove_task(e);
    return true;
}
};
