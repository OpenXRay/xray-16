#ifndef _LIGHTSTAB_INTERFACE_H_
#define _LIGHTSTAB_INTERFACE_H_
namespace lc_net
{
class net_task_interface
{
public:
    virtual bool run_task(IAgent* agent, u32 sessionId, IGenericStream* inStream, IGenericStream* outStream) = 0;
};
extern XRLC_LIGHT_API net_task_interface* g_net_task_interface;
}
#endif
