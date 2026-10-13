#pragma once

#include "ik_calculate_state.h"

class CIKLimb;
struct SCalculateData
{
    CIKLimb* m_limb{};
    Fmatrix const* m_obj{};

    bool do_collide{};

    calculate_state state{};
    Fvector cl_shift{ 0, 0, 0 };
    bool apply{};
    float l{};
    float a{};

public:
    SCalculateData() = default;
    SCalculateData(CIKLimb& l, const Fmatrix& o);

public:
    IC Fmatrix& goal(Fmatrix& g) const;
};

