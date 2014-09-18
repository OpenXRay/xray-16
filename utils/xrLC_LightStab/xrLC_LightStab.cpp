#include "xrlc_lightstab.h"
#include "../xrlc_light/net_light.h"

#include "../xrlc_light/lightstab_interface.h"

#pragma comment(lib,"xrLC_Light.lib")


extern "C" XRLC_LIGHT_STUB_API  bool __cdecl RunTask(IAgent* agent,
                 DWORD sessionId,
                 IGenericStream* inStream,
                 IGenericStream* outStream)
{

	if(lc_net::g_net_task_interface)
			return lc_net::g_net_task_interface->run_task( agent, sessionId, inStream, outStream );

  //if(g_net_task_interface)
	// return g_net_task_interface->RunTask( agent, sessionId, inStream, outStream );
	 return false;
}

