#include "stdafx.h"

#include "net_global_data_cleanup.h"
#include "serialize.h"
namespace lc_net
{
global_data_cleanup _cleanup;
global_data_cleanup& cleanup() { return _cleanup; }
global_data_cleanup::global_data_cleanup() : id_state(0) { vec_cleanup.resize(gl_last, 0); }
void global_data_cleanup::on_net_send(IGenericStream* outStream) { outStream->Write(&id_state, sizeof(id_state)); }
static const u32 data_cleanup_callback_read_write_buff_size = 512;

void __cdecl data_cleanup_callback(const char* dataDesc, IGenericStream** stream)
{
    *stream = CreateGenericStream();
    u8 buff[data_cleanup_callback_read_write_buff_size];
    w_pod_vector(INetMemoryBuffWriter(*stream, sizeof(buff), buff), _cleanup.vec_cleanup);
    // w_pod_vector( INetIWriterGenStream( *stream, 512 ), _cleanup.vec_cleanup );
}

#pragma warning(push)
#pragma warning(disable : 4995)
void global_data_cleanup::on_net_receive(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    u32 state = u32(-1);
    inStream->Read(&state, sizeof(id_state));
    lock.Enter();
    if (state == id_state)
    {
        lock.Leave();
        return;
    }
    IGenericStream* globalDataStream = 0;
    HRESULT rz = agent->GetData(sessionId, "data_cleanup", &globalDataStream);
    if (rz != S_OK)
    {
        lock.Leave();
        return;
    }
    R_ASSERT(globalDataStream);
    u8 buff[data_cleanup_callback_read_write_buff_size];
    r_pod_vector(INetBlockReader(globalDataStream, buff, sizeof(buff)), vec_cleanup);
    //  r_pod_vector( INetReaderGenStream( globalDataStream ), vec_cleanup );

    free(globalDataStream->GetCurPointer());
    id_state = state;
#ifdef CL_NET_LOG
    Msg("cleanup call");
#endif
    globals().cleanup();
    lock.Leave();
    return;
}
#pragma warning(pop)
}
