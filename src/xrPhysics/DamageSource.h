#pragma once
#include "xrCore/xr_types.h"

class XR_NOVTABLE IDamageSource
{
public:
    virtual ~IDamageSource() = default;
    virtual void SetInitiator(u16 id) = 0;
    virtual u16 Initiator() = 0;
    virtual IDamageSource* cast_IDamageSource() = 0; //{ return this; }
};
