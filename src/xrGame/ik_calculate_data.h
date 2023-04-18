#pragma once

#include "ik_calculate_state.h"

class CIKLimb;
struct SCalculateData
{
    float const* m_angles{};
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

//#define IK_DBG_STATE_SEQUENCE
#ifdef IK_DBG_STATE_SEQUENCE
extern u32 sdbg_state_sequence_number;
#include "ik_dbg_matrix.h"
#endif
