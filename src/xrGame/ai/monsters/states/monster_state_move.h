#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CMonsterStateMove : public CState<_Object>
{
protected:
    using inherited = CState<_Object>;

public:
    CMonsterStateMove(_Object* obj, void* data = nullptr) : inherited(obj, data) {}

    void initialize() override
    {
        inherited::initialize();
        this->object->path().prepare_builder();
    }
};
