/*
  This source code is a part of IKAN.
  Copyright (c) 2000 University of Pennsylvania
  Center for Human Modeling and Simulation
  All Rights Reserved.

  IN NO EVENT SHALL THE UNIVERSITY OF PENNSYLVANIA BE LIABLE TO ANY
  PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
  DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS
  SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF PENNSYLVANIA
  HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

  Permission to use, copy, modify and distribute this software and its
  documentation for educational, research and non-profit purposes,
  without fee, and without a written agreement is hereby granted,
  provided that the above copyright notice and the following three
  paragraphs appear in all copies. For for-profit purposes, please
  contact University of Pennsylvania
 (http://hms.upenn.edu/software/ik/ik.html) for the software license
  agreement.


  THE UNIVERSITY OF PENNSYLVANIA SPECIFICALLY DISCLAIM ANY
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
  BASIS, AND THE UNIVERSITY OF PENNSYLVANIA HAS NO OBLIGATION
  TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
  MODIFICATIONS.

 */

#ifndef _LIMBH
#define _LIMBH

#include "aint.h"
#include "Dof7control.h"
#include "eulersolver.h"

class Limb
{
private:
    //    short check_limits;
    SRS solver;

    short check_limits;
    short solve;
    short num_singular;
    short euler1, euler2;

    float singular_pts[4];
    float x3;

    // Set by SetGoal or SetGoalPos
    // For SetGoal:
    //
    // PSI[0] = valid psi solns for (family1,family1) of R1 and R2
    // PSI[1] = valid psi solns for (family1,family2) of R1 and R2
    // PSI[2] = valid psi solns for (family2,family1) of R1 and R2
    // PSI[3] = valid psi solns for (family2,family2) of R1 and R2
    //
    // For SetGoalPos:
    // PSI[0] = valid psi solns for family1 of R1
    // PSI[1] = valid psi solns for family2 of R1

    AngleIntList PSI[4];

    // The euler convention fors the first and second S joints
public:
    AngleInt jt_limits[7];

private:
    float min[7], max[7];

    int check_r_joint(float& val);

    void extract_s1(const Matrix R1, float s1[3]);
    void extract_s1s2(const Matrix R1, const Matrix R2, float s1[3], float s2[3]);

    void extract_s1_family(const Matrix R1, int family, float s1[3]);
    void extract_s1s2_family(const Matrix R1, const Matrix R2, int family1, int family2, float s1[3], float s2[3]);
    int set_goal(const Matrix G);
    int set_goal_pos(const float g[3], const Matrix E);

    void solve_aux(float swivel_angle, float x[]);
    void solve_aux_family(int family_set, float swivel_angle, float x[]);

    void solve_pos_aux(float swivel_angle, float x[]);
    void solve_pos_aux_family(int family, float swivel_angle, float x[]);

    int try_swivel_angle(int solve, float swivel_angle, float x[]);
    int try_singularities(int solve, float& swivel_angle, float x[]);
    int try_closeby_singularity(int solve, float& swivel_angle, float x[]);

public:
    void get_R1R2psi(AngleIntList psi[]);
    void get_R1psi(AngleIntList psi[]);

    Limb() {}
    void init(const Matrix T, const Matrix S, int s1_euler, int s2_euler, const float proj_axis[3],
        const float pos_axis[3], const float min[7], const float max[7]);

    Limb(const Matrix T, const Matrix S, int s1_euler, int s2_euler, const float proj_axis[3], const float pos_axis[3],
        const float Min[7], const float Max[7])
    {
        init(T, S, s1_euler, s2_euler, proj_axis, pos_axis, Min, Max);
    }

    ~Limb() {}
    void SetTMatrix(const Matrix TT) { solver.SetTMatrix(TT); }
    void SetSMatrix(const Matrix SS) { solver.SetSMatrix(SS); }
    int SetGoalPos(const float g[3], const Matrix E, int limits_on);
    int SetGoal(const Matrix G, int limits_on);
    float Length() const { return solver.Length(); }
    void SetAimGoal(const float goal[3], const float axis[3], float flex_angle)
    {
        solver.SetAimGoal(goal, axis, flex_angle);
    }

    int SolveAim(float x[3], float psi_angle);

    float PosToAngle(const float p[3]);
    float KneeAngle(const float goal_pos[3], const float knee_pos[3]);

    int Solve(float x[7], float* new_psi = 0, float* new_pos = 0);

    int SolveByAngle(float psi, float x[7], float* new_psi = 0, float* new_pos = 0);

    int SolveByPos(const float pos[3], float x[7], float* new_psi = 0, float* new_pos = 0);

    int InLimits(const float x[7]) const;
    void Debug(char* file1, char* file2);

    // Must call SetGoal first with joint limits turned on
    void GetPsiIntervals(AngleIntList& f11, AngleIntList& f12, AngleIntList& f21, AngleIntList& f22)
    {
        PSI[0].Copy(f11);
        PSI[1].Copy(f12);
        PSI[2].Copy(f21);
        PSI[3].Copy(f22);
    }

    // Must call SetGoalPos first with joint limits turned on
    void GetPsiPosIntervals(AngleIntList& f1, AngleIntList& f2)
    {
        PSI[0].Copy(f1);
        PSI[1].Copy(f2);
    }

    void ForwardKinematics(float x[7], Matrix R);

    int GetJointIntervals(Matrix G, AngleIntList f1[6], AngleIntList f2[6]);
};

#endif
