#pragma once

#include "xrCore/xr_resource.h"

namespace xray::render::fg
{
struct ECORE_API SInputSignature : public xr_resource_flagged
{
    ID3DBlob* signature;
    SInputSignature(ID3DBlob* pBlob);
    ~SInputSignature();
};
typedef resptr_core<SInputSignature, resptr_base<SInputSignature>> ref_input_sign;
}
