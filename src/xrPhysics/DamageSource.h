#pragma once
#include "xrCore/_types.h"

// XXX: tamlin: Can we declare IDamageSource __declspec(novtable), or is its RTTI info required somewhere?
class IDamageSource
{
public:
    virtual ~IDamageSource(){};
    virtual void SetInitiator(u16 id) = 0;
    virtual u16 Initiator() = 0;
    virtual IDamageSource* cast_IDamageSource() = 0; //{ return this; }
};
