////////////////////////////////////////////////////////////////////////////
//	Created		: 03.06.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "net_execution_mu_base.h"

#include "xrface.h"
#include "xrMU_Model.h"
#include "xrlc_globaldata.h"

namespace lc_net
{
void execution_mu_base_light::send_task(IGenericStream* outStream)
{
    R_ASSERT(mu_model_id != u32(-1));
    outStream->Write(&mu_model_id, sizeof(mu_model_id));
}
bool execution_mu_base_light::receive_task(IAgent* agent, u32 sessionId, IGenericStream* inStream)
{
    inStream->Read(&mu_model_id, sizeof(mu_model_id));
    R_ASSERT(mu_model_id != u32(-1));
    return true;
}

static const u32 send_receive_result_buffer_size = 128 * 1024;
void execution_mu_base_light::receive_result(IGenericStream* inStream)
{
    u8 buff[send_receive_result_buffer_size];
    INetBlockReader r(inStream, buff, sizeof(buff));
    // INetReaderGenStream r( inStream );
    mu_model_id = r.r_u32();
    R_ASSERT(mu_model_id != u32(-1));
    xrMU_Model& model = *inlc_global_data()->mu_models()[mu_model_id];
    // model.read_subdivs( r );
    model.read_color(r);
}

void execution_mu_base_light::send_result(IGenericStream* outStream)
{
    R_ASSERT(mu_model_id != u32(-1));

    u8 buff[send_receive_result_buffer_size];
    INetMemoryBuffWriter w(outStream, sizeof(buff), buff);
    // INetIWriterGenStream w( outStream, 100 );
    w.w_u32(mu_model_id);
    xrMU_Model& model = *inlc_global_data()->mu_models()[mu_model_id];
    // model.write_subdivs( w );
    model.write_color(w);
}
bool execution_mu_base_light::execute(net_task_callback& net_callback)
{
    R_ASSERT(mu_model_id != u32(-1));
    xrMU_Model& model = *inlc_global_data()->mu_models()[mu_model_id];
    // model.calc_materials();
    model.calc_lighting();

    return true;
}
};
