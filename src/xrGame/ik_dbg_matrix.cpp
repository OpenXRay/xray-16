#include "StdAfx.h"

#include "ik_dbg_matrix.h"

#include "ik/IKLimb.h"

u32 sdbg_state_sequence_number = 130;

IC Fmatrix& cvm(Matrix& IM) { return *((Fmatrix*)(&IM)); }
#ifdef IK_DBG_STATE_SEQUENCE
void dbg_matrises::next_state(SCalculateData& cd)
{
    if (old_dbg_m.size() > sdbg_state_sequence_number)
        old_dbg_m.erase(old_dbg_m.begin());
    old_dbg_m.push_back(dbg_m);
    Fmatrix m_dbg;
    dbg_m.OBJ = *cd.m_obj;
    dbg_m.ref_bone = cd.m_limb->ref_bone();
    if (cd.m_limb->ref_bone() == 2)
    {
        dbg_m.b2goal_gl = cd.state.goal;
        dbg_m.b3goal_gl.mul_43(dbg_m.b2goal_gl, cd.m_limb->transform(m_dbg, 2, 3));

        dbg_m.b2goal_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b2goal_gl);
        dbg_m.b3goal_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b3goal_gl);
        dbg_m.b3tob3_goal.mul(Fmatrix().invert(dbg_m.b2goal_gl), dbg_m.b3goal_gl);

        dbg_m.b2start_gl = cd.m_limb->sv_state.goal(m_dbg);
        dbg_m.b3start_gl.mulB_43(cd.m_limb->transform(m_dbg, 2, 3));

        dbg_m.b2start_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b2start_gl);
        dbg_m.b3start_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b3start_gl);
        dbg_m.b3tob3_start.mul(Fmatrix().invert(dbg_m.b2goal_gl), dbg_m.b3goal_gl);
    }
    else
    {
        dbg_m.b3goal_gl = cd.state.goal;
        dbg_m.b2goal_gl.mul_43(dbg_m.b3goal_gl, cd.m_limb->transform(m_dbg, 3, 2));

        dbg_m.b2goal_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b2goal_gl);
        dbg_m.b3goal_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b3goal_gl);
        dbg_m.b3tob3_goal.mul(Fmatrix().invert(dbg_m.b2goal_gl), dbg_m.b3goal_gl);

        dbg_m.b3start_gl = cd.m_limb->sv_state.goal(m_dbg);
        dbg_m.b2start_gl.mul_43(dbg_m.b3start_gl, cd.m_limb->transform(m_dbg, 3, 2));

        dbg_m.b2start_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b2start_gl);
        dbg_m.b3start_lc.mul_43(Fmatrix().invert(*cd.m_obj), dbg_m.b3start_gl);
        dbg_m.b3tob3_start.mul(Fmatrix().invert(dbg_m.b2goal_gl), dbg_m.b3goal_gl);
    }
}

void dbg_matrises::next_goal(const SCalculateData& cd)
{
    Fmatrix goal;
    Matrix gl;
    dbg_m.dbg_goal = cd.goal(goal);
    dbg_m.GOAL = cvm(cd.m_limb->Goal(gl, goal, cd));
    dbg_m.OBJ_END = *cd.m_obj;
}
#endif
