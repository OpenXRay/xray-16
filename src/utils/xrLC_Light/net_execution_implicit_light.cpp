#include "stdafx.h"
#include "net_execution_implicit_light.h"
#include "serialize.h"
namespace lc_net
{
static constexpr size_t implicit_light_task_buffer_size = 32;
static constexpr size_t implicit_light_result_buffer_size = 1024;

void execution_implicit_light::send_task(IGenericStream* outStream)
{
    {
        u8 buff[implicit_light_task_buffer_size];
        INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
        // INetIWriterGenStream w( outStream, send_receive_task_buffer_size );
        exec.write(w);
    }
}
bool execution_implicit_light::receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    u8 buff[implicit_light_task_buffer_size];
    INetBlockReader r(inStream, buff, sizeof(buff));
    // INetReaderGenStream r( inStream );
    exec.read(r);
    return true;
}

void execution_implicit_light::receive_result(IGenericStream* inStream)
{
    u8 buff[implicit_light_result_buffer_size];
    INetBlockReader r(inStream, buff, sizeof(buff));
    // INetReaderGenStream r(inStream);
    exec.receive_result(r);
}
void execution_implicit_light::send_result(IGenericStream* outStream)
{
    u8 buff[implicit_light_result_buffer_size];
    INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
    // INetIWriterGenStream w( outStream, implicit_light_result_buffer_size );
    exec.send_result(w);
}
bool execution_implicit_light::execute(net_task_callback& net_callback)
{
    exec.Execute(&net_callback);
    return true;
}
};
