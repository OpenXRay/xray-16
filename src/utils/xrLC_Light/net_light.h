#pragma once
#include <Unknwn.h>
#include "hxgrid/Interface/IAgent.h"
// interface IGenericStream;
__interface net_task_interface
{
public:
    virtual bool RunTask(IAgent * agent, u32 sessionId, IGenericStream * inStream, IGenericStream * outStream) = 0;
};

extern XRLC_LIGHT_API net_task_interface* g_net_task_interface;
