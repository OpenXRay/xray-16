#pragma once

#include "xr_collide_form.h"

class ENGINE_API CCF_DynamicMesh : public CCF_Skeleton
{
    typedef CCF_Skeleton inherited;

public:
    CCF_DynamicMesh(IGameObject* _owner) : CCF_Skeleton(_owner){};

    virtual bool _RayQuery(const collide::ray_defs& Q, collide::rq_results& R);
};
