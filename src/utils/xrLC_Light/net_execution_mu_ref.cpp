#include "stdafx.h"
#include "net_execution_mu_ref.h"

#include "xrface.h"
#include "xrMU_Model.h"
#include "xrMU_Model_Reference.h"

#include "xrlc_globaldata.h"

namespace lc_net
{
void execution_mu_ref_light::send_task(IGenericStream* outStream)
{
    R_ASSERT(mu_ref_id != u32(-1));
    outStream->Write(&mu_ref_id, sizeof(mu_ref_id));
}
bool execution_mu_ref_light::receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    inStream->Read(&mu_ref_id, sizeof(mu_ref_id));
    R_ASSERT(mu_ref_id != u32(-1));
    return true;
}

static const u32 send_receive_result_buffer_size = 128 * 1024;
void execution_mu_ref_light::receive_result(IGenericStream* inStream)
{
    u8 buff[send_receive_result_buffer_size];
    INetBlockReader r(inStream, buff, sizeof(buff));
    // INetReaderGenStream r( inStream );
    mu_ref_id = r.r_u32();
    R_ASSERT(mu_ref_id != u32(-1));
    inlc_global_data()->mu_refs()[mu_ref_id]->receive_result(r);
}

void execution_mu_ref_light::send_result(IGenericStream* outStream)
{
    R_ASSERT(mu_ref_id != u32(-1));

    u8 buff[send_receive_result_buffer_size];
    INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
    // INetIWriterGenStream w( outStream, 100 );
    w.w_u32(mu_ref_id);
    inlc_global_data()->mu_refs()[mu_ref_id]->send_result(w);
}
bool execution_mu_ref_light::execute(net_task_callback& net_callback)
{
    R_ASSERT(mu_ref_id != u32(-1));
    inlc_global_data()->mu_refs()[mu_ref_id]->calc_lighting();
    return true;
}
};
