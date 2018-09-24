#pragma once
#include "limb.h"
#include "IKFoot.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ik_anim_state.h"
#include "ik_calculate_data.h"
#include "ik_limb_state.h"
#include "ik_collide_data.h"
#include "ik_limb_state_predict.h"

class IKinematics;
struct SCalculateData;
struct SIKCollideData;
class CGameObject;
class motion_marks;
class ik_goal_matrix;
namespace CDB
{
class TRI;
}
namespace extrapolation
{
class points;
};
class CIKLimb
{
private:
    //			friend		class	ik_limb_state;
public:
    CIKLimb();
    CIKLimb(const CIKLimb& l);

    CIKLimb& operator=(const CIKLimb& l);

public:
    void Create(u16 id, IKinematicsAnimated* K, bool collide_);
    void Destroy();

public:
    void SolveBones(SCalculateData& cd);
    void ApplyState(SCalculateData& cd);
    void SetGoal(SCalculateData& cd);
    void Update(CGameObject* O, const CBlend* b, const extrapolation::points& object_pose_extrapolation);

public:
    IC u16 get_id() const { return m_id; }
    float ObjShiftDown(float current_shift, const SCalculateData& cd) const;
    IC Fmatrix& ref_bone_to_foot(Fmatrix& ref_bone) const;
    IC IKinematics* Kinematics() const { return m_foot.Kinematics(); }
    IC IKinematicsAnimated* KinematicsAnimated() const { return m_K; }
    IC u16 ref_bone() const { return m_foot.ref_bone(); }
    Fmatrix& transform(Fmatrix& m, u16 bone0, u16 bone1) const;
    float time_to_footstep() const { return state_predict.time_to_footstep; }
    float footstep_shift() const { return state_predict.footstep_shift; }
    void step_predict(CGameObject* O, const CBlend* b, ik_limb_state_predict& state,
        const extrapolation::points& object_pose_extrapolation); // const;
    bool foot_step() const { return sv_state.foot_step(); }
    u16 foot_matrix_predict(Fmatrix& foot, Fmatrix& toe, float time, IKinematicsAnimated* K) const;
#ifdef DEBUG
    u16 dbg_ref_bone_id() const { return m_bones[m_foot.ref_bone()]; }
    const CIKFoot& dbg_ik_foot() const { return m_foot; }
#endif
private:
    float get_time_to_step_begin(const CBlend& B) const;
    void Invalidate();

private:
    void Solve(SCalculateData& cd);
    IC void AnimGoal(Fmatrix& gl);
    void SetAnimGoal(SCalculateData& cd);
    void SetNewGoal(const SIKCollideData& cld, SCalculateData& cd);
    void SetNewStepGoal(const SIKCollideData& cld, SCalculateData& cd);
    void Blending(SCalculateData& cd);
    bool blend_collide(
        ik_goal_matrix& m, const SCalculateData& cd, const ik_goal_matrix& m0, const ik_goal_matrix& m1) const;
    bool SetGoalToLimb(const SCalculateData& cd);
    void CalculateBones(SCalculateData& cd);
    Matrix& Goal(Matrix& gl, const Fmatrix& xm, const SCalculateData& cd);
    Fmatrix& GetHipInvert(Fmatrix& ihip, const SCalculateData& cd);

    float SwivelAngle(const Fmatrix& ihip, const SCalculateData& cd);
    void GetKnee(Fvector& knee, const SCalculateData& cd) const;
    void GetPickDir(Fvector& v, SCalculateData& cd) const;
    void ToeTimeDiff(Fvector& v, const SCalculateData& cd) const;
    void ToeTimeDiffPredict(Fvector& v) const;
    IC static void get_start(Fmatrix& start, SCalculateData& D, u16 bone);
#ifdef DEBUG
    void DBGDrawSetNewGoal(SCalculateData& cd, const SIKCollideData& cld);
#endif
private:
    static void BonesCallback0(CBoneInstance* B);
    static void BonesCallback1(CBoneInstance* B);
    static void BonesCallback2(CBoneInstance* B);

private:
    Limb m_limb;
    IKinematicsAnimated* m_K;
    CIKFoot m_foot;
    ik_foot_collider collider;
    u16 m_bones[4];
    u16 m_id;

    bool m_collide;
    SIKCollideData collide_data;

    ik_anim_state anim_state;
    ik_limb_state sv_state;
    ik_limb_state_predict state_predict;
#ifdef DEBUG
    bool dbg_disabled;
#endif
#ifdef IK_DBG_STATE_SEQUENCE
    friend struct dbg_matrises;
    dbg_matrises m_dbg_matrises;
#endif
};
