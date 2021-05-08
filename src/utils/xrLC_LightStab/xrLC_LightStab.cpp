#include "xrLC_LightStab.h"
#include "utils/xrLC_Light/net_light.h"

#include "utils/xrLC_Light/lightstab_interface.h"

extern "C" XRLC_LIGHT_STUB_API bool __cdecl RunTask(
    IAgent* agent, u32 sessionId, IGenericStream* inStream, IGenericStream* outStream)
{
    if (lc_net::g_net_task_interface)
        return lc_net::g_net_task_interface->run_task(agent, sessionId, inStream, outStream);

    // if(g_net_task_interface)
    // return g_net_task_interface->RunTask( agent, sessionId, inStream, outStream );
    return false;
}
