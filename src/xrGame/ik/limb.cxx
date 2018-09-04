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

#include "stdafx.h"
#include "limb.h"

enum
{
    SolvePosOnly = 1,
    SolvePosAndOrientation = 2
};

//
// Initialize the limb ik object with the T and S matrices
// the Euler extraction routines for the two spherical joints,
// the projection axis, and the min and max joint limits
//
void Limb::init(const Matrix T, const Matrix S, int s1, int s2, const float* proj_axis, const float* pos_axis,
    const float* mmin, const float* mmax)
{
    euler1 = short(s1);
    euler2 = short(s2);
    check_limits = 0;
    solve = 0;
    x3 = 0.0;

    solver.init(T, S, proj_axis, pos_axis);

    if (mmin && mmax)
        for (int i = 0; i < 7; i++)
        {
            min[i] = mmin[i];
            max[i] = mmax[i];
            jt_limits[i].Set(mmin[i], mmax[i]);
        }
}

//
// Check to see whether the i-th joint value is within
// the joint limits
//

int Limb::check_r_joint(float& v)
{
    if (jt_limits[3].InRange(v))
    {
        // Put v in correct range
        if (v < min[3])
            v += 2 * M_PI;
        if (v > max[3])
            v -= 2 * M_PI;
        return 1;
    }

    return 0;
}

inline void swap(float& x, float& y)
{
    float t = x;
    x = y;
    y = t;
}

//
// Given two families of solutions
//	f1[0],..,f1[2]  and f2[0],..,f2[2]
// determine which one comes closest to satisfying the joint limits
// and copy the result into soln[0]...[2]
//
void select_best_family(const AngleInt jt_limits[], const float f1[], const float f2[], float soln[])
{
    float d1, d2, u;

    d1 = d2 = 0.0;
    for (int i = 0; i < 3; i++)
    {
        if ((u = jt_limits[i].Distance(f1[i])) > 0.0)
            d1 += u;

        if ((u = jt_limits[i].Distance(f2[i])) > 0.0)
            d2 += u;
    }

    // Select family with minimum displacement from joint limits
    if (d1 <= d2)
    {
        soln[0] = f1[0];
        soln[1] = f1[1];
        soln[2] = f1[2];
    }
    else
    {
        soln[0] = f2[0];
        soln[1] = f2[1];
        soln[2] = f2[2];
    }
}

inline float min(float x, float y) { return x < y ? x : y; }
//
// If possible put v (0 < v < 2*M_PI) in the range low < v < high
//

inline float put_angle_in_range(float low, float high, float v)
{
    float d1, d2, v2;

    if (low <= v && v <= high)
        return v;
    else
        d1 = min(_abs(v - low), _abs(v - high));

    v2 = v - 2 * M_PI;

    if (low <= v2 && v2 <= high)
        return v2;
    else
        d2 = min(_abs(v2 - low), _abs(v2 - high));

    return (_abs(d1) < _abs(d2)) ? v : v2;
}

//
// Extract the euler angles for the first spherical joint
// without checking if the joint limits are satisfied.
//
void Limb::extract_s1(const Matrix R1, float s[3])
{
    float f1[3], f2[3];

    EulerSolve2(euler1, R1, f1, f2);

    // Remember that jt convention is backwards from euler convention
    swap(f1[0], f1[2]);
    swap(f2[0], f2[2]);

    select_best_family(jt_limits, f1, f2, s);

    s[0] = put_angle_in_range(min[0], max[0], s[0]);
    s[1] = put_angle_in_range(min[1], max[1], s[1]);
    s[2] = put_angle_in_range(min[2], max[2], s[2]);
}

//
// Same as above but for a particular family
//
//
void Limb::extract_s1_family(const Matrix R1, int family, float s[3])
{
    EulerSolve(euler1, R1, s, family);

    // Remember that jt convention is backwards from euler convention
    swap(s[0], s[2]);

    s[0] = put_angle_in_range(min[0], max[0], s[0]);
    s[1] = put_angle_in_range(min[1], max[1], s[1]);
    s[2] = put_angle_in_range(min[2], max[2], s[2]);
}

//
// Extract the euler angles for both spherical
// joints without checking joint limits
//
void Limb::extract_s1s2(const Matrix R1, const Matrix R2, float s1[3], float s2[3])
{
    float f1[3], f2[3];

    extract_s1(R1, s1);

    EulerSolve2(euler2, R2, f1, f2);

    // Remember that jt convention is backwards from euler convention
    swap(f1[0], f1[2]);
    swap(f2[0], f2[2]);

    select_best_family(jt_limits + 4, f1, f2, s2);

    s2[0] = put_angle_in_range(min[4], max[4], s2[0]);
    s2[1] = put_angle_in_range(min[5], max[5], s2[1]);
    s2[2] = put_angle_in_range(min[6], max[6], s2[2]);
}

//
// Same as above but given which two families to use
//
void Limb::extract_s1s2_family(const Matrix R1, const Matrix R2, int f1, int f2, float s1[3], float s2[3])
{
    extract_s1_family(R1, f1, s1);

    EulerSolve(euler2, R2, s2, f2);

    // Remember that jt convention is backwards from euler convention
    swap(s2[0], s2[2]);

    s2[0] = put_angle_in_range(min[4], max[4], s2[0]);
    s2[1] = put_angle_in_range(min[5], max[5], s2[1]);
    s2[2] = put_angle_in_range(min[6], max[6], s2[2]);
}

//
// Sets the goal matrix and solves for the R angle
//
int Limb::set_goal(const Matrix G)
{
    if (!solver.SetGoal(G, x3))
        return 0;
    if (!check_r_joint(x3))
        return 0;

    return 1;
}

//
// Sets the goal position and calculates the R angle
//
int Limb::set_goal_pos(const float g[3], const Matrix E)
{
    if (!solver.SetGoalPos(g, E, x3))
        return 0;
    if (!check_r_joint(x3))
        return 0;
    return 1;
}

//
// Calculates the two families of valid psi for the
// first three joints for a specified position problem.
//
void Limb::get_R1psi(AngleIntList psi[])
{
    float low[3], high[3];
    Matrix c, s, o;
    AngleIntList f1[3], f2[3], temp;

    solver.R1Psi(c, s, o);
    psi[0].Clear();
    psi[1].Clear();

    low[0] = min[2];
    low[1] = min[1];
    low[2] = min[0];

    high[0] = max[2];
    high[1] = max[1];
    high[2] = max[0];

    EulerPsiSolver E(euler1, c, s, o, low, high);
    E.SolvePsiRanges(f1, f2);
    num_singular = (short)E.Singularities(singular_pts);

    Intersect(f1[0], f1[1], temp);
    Intersect(temp, f1[2], psi[0]);

    temp.Clear();

    Intersect(f2[0], f2[1], temp);
    Intersect(temp, f2[2], psi[1]);
}

//
// Calculates the four families of valid psi for the
// pair of spherical joints. PSI[0] corresponds to
// family1 in the first three joints, family1 in the
// last three joints. PSI[1] corresponds to family1
// in the first three joints, family2 in the last three,
// PSI[2] corresponds to family2 in the first three joints,
// family1 in the last three. PSI[3] corresponds to family2
// for both spherical joints
//
void Limb::get_R1R2psi(AngleIntList psi[])
{
    Matrix c, s, o, c2, s2, o2;
    float low[3], high[3];
    AngleIntList f1[3], f2[3], g1[3], g2[3];

    solver.R1R2Psi(c, s, o, c2, s2, o2);

    low[0] = min[2];
    low[1] = min[1];
    low[2] = min[0];

    high[0] = max[2];
    high[1] = max[1];
    high[2] = max[0];

    EulerPsiSolver E(euler1, c, s, o, low, high);
    E.SolvePsiRanges(f1, f2);
    num_singular = (short)E.Singularities(singular_pts);

    low[0] = min[6];
    low[1] = min[5];
    low[2] = min[4];

    high[0] = max[6];
    high[1] = max[5];
    high[2] = max[4];

    EulerPsiSolver E2(euler2, c2, s2, o2, low, high);
    E2.SolvePsiRanges(g1, g2);
    num_singular = num_singular + (short)E2.Singularities(singular_pts + num_singular);

    // There are four families of solutions (f1,g1),(f1,g2),(f2,g1),(f2,g2)

    AngleIntList t, ff1, ff2, gg1, gg2;

    Intersect(f1[0], f1[1], t);
    Intersect(f1[2], t, ff1);
    t.Clear();
    Intersect(f2[0], f2[1], t);
    Intersect(f2[2], t, ff2);
    t.Clear();

    Intersect(g1[0], g1[1], t);
    Intersect(g1[2], t, gg1);
    t.Clear();
    Intersect(g2[0], g2[1], t);
    Intersect(g2[2], t, gg2);

    psi[0].Clear();
    psi[1].Clear();
    psi[2].Clear();
    psi[3].Clear();
    if (ff1.IsEmpty() && ff2.IsEmpty())
        return;

    Intersect(ff1, gg1, psi[0]);
    Intersect(ff1, gg2, psi[1]);
    Intersect(ff2, gg1, psi[2]);
    Intersect(ff2, gg2, psi[3]);
}

int Limb::GetJointIntervals(Matrix G, AngleIntList f1[6], AngleIntList f2[6])
{
    float low[3], high[3];
    Matrix c, s, o, c2, s2, o2;

    if (!set_goal(G))
        return 0;

    solver.R1R2Psi(c, s, o, c2, s2, o2);

    low[0] = min[2];
    low[1] = min[1];
    low[2] = min[0];

    high[0] = max[2];
    high[1] = max[1];
    high[2] = max[0];

    EulerPsiSolver E(euler1, c, s, o, low, high);
    E.SolvePsiRanges(f1, f2);

    low[0] = min[6];
    low[1] = min[5];
    low[2] = min[4];

    high[0] = max[6];
    high[1] = max[5];
    high[2] = max[4];

    EulerPsiSolver E2(euler2, c2, s2, o2, low, high);
    E2.SolvePsiRanges(f1 + 3, f2 + 3);

    return 1;
}

int Limb::SetGoalPos(const float g[3], const Matrix E, int limits)
{
    solve = SolvePosOnly;
    check_limits = (short)limits;

    int success = set_goal_pos(g, E);

    if (limits && success)
        get_R1psi(PSI);

    return success;
}

//
// Sets the goal matrix. If limits is turned on then
// also compute the valid psi ranges and store them
//
int Limb::SetGoal(const Matrix G, int limits)
{
    int success = set_goal(G);
    check_limits = (short)(!!limits);

    solve = SolvePosAndOrientation;
    if (limits && success)
        get_R1R2psi(PSI);

    return success;
}

static void init_error(pcstr msg)
{
    fprintf(stderr, "You forgot to call SetGoal or SetGoalPos in %s\n", msg);
    exit(0);
}

//
// Calculates the swivel angle based on the elbow position.
//
float Limb::PosToAngle(const float elbow[3])
{
    if (!solve)
        init_error("Limb::PosToAngle");

    return solver.PosToAngle(elbow);
}

//
// Calculates the swivel angle based on the elbow position and goal position
//
float Limb::KneeAngle(const float goal_pos[3], const float knee_pos[3])
{
    solver.EvaluateCircle(goal_pos);
    short sv_solve = solve;
    solve = SolvePosOnly; // any !=0
    float swivel = PosToAngle(knee_pos);
    solve = sv_solve;
    return swivel;
}

//
// Returns the index of the smallest element in a float array
//
inline int find_min(int n, float d[])
{
    float min = d[0];
    int min_i = 0;

    for (int i = 1; i < n; i++)
        if (d[i] < min)
        {
            min = d[i];
            min_i = i;
        }
    return min_i;
}

//
// Given two or four disjoint intervals of valid psi, find the
// midpoint of the largest continous range of valid psi.
// Return an integer code corresponding to which of the two/four
// intervals contains this midpoint.
//
// 0 means that all intervals are empty.
//
int choose_largest_range(float& swivel_angle, const AngleIntList* f11, const AngleIntList* f12,
    const AngleIntList* f21 = 0, const AngleIntList* f22 = 0)
{
    const float unioneps = .05f;
    AngleIntList temp, all;

    // Take the union of all the intervals

    if (f21 && f22)
    {
        AngleIntList t1, t2;

        Union(*f11, *f12, t1);
        Union(*f21, *f22, t2);
        Union(t1, t2, temp);
    }
    else
        Union(*f11, *f12, temp);

    temp.AddList(all, unioneps);

    // find the largest continous interval and take its midpoint.
    AngleInt* a = all.Largest();

    if ((!a) || a->IsEmpty())
        return 0;

    swivel_angle = a->Mid();

    // One of the psi intervals should contain the swivel angle

    if (f11->InRange(swivel_angle))
        return 1;

    if (f12->InRange(swivel_angle))
        return 2;

    if (f21)
    {
        if (f21->InRange(swivel_angle))
            return 3;

        if (f22->InRange(swivel_angle))
            return 4;
    }

    //
    // Rarely, the swivel angle could be at the boundary and out of
    // range because of numerical rounding. In this case, return
    // the interval that is closest to the swivel angle
    //

    float d[4];

    d[0] = f11->Distance(swivel_angle);
    d[1] = f12->Distance(swivel_angle);

    if (f21)
    {
        d[2] = f21->Distance(swivel_angle);
        d[3] = f22->Distance(swivel_angle);

        return find_min(4, d) + 1;
    }

    return find_min(2, d) + 1;
}

int update_closest_boundary(AngleInt& a, float v, float& dist, float& boundary)
{
    float d1 = angle_distance(a.Low(), v);
    float d2 = angle_distance(a.High(), v);
    float angle;
    if (d1 < d2)
        angle = a.Low();
    else
    {
        angle = a.High();
        d1 = d2;
    }

    if (d1 < dist)
    {
        dist = d1;
        boundary = angle;
        return 1;
    }

    return 0;
}

int inspect_range(
    const AngleIntList& f, float swivel_angle, int index, float& new_angle, int& new_index, float& distance)
{
    AngleIntListIterator a;
    AngleInt* ap;

    for (a.Start(f), ap = a.Next(); ap; ap = a.Next())
    {
        if (ap->IsEmpty())
            continue;

        if (ap->InRange(swivel_angle))
            return (1);

        if (!update_closest_boundary(*ap, swivel_angle, distance, new_angle))
            continue;

        new_index = index;
    }

    return (0);
}

//
// Given two or four disjoint intervals of valid psi, find the
// interval that is closest to a desired value for the swivel angle.
// Return an integer code corresponding to which of the two/four
// intervals is closest. Also change the input swivel_angle to
// the closest angle that can be achieved.
//
// 0 means that all intervals are empty.
//

int choose_closest_range(float& swivel_angle, const AngleIntList* f11, const AngleIntList* f12,
    const AngleIntList* f21 = 0, const AngleIntList* f22 = 0)
{
    int i = 0;
    float d = 2 * M_PI;
    float angle;

    if (inspect_range(*f11, swivel_angle, 1, angle, i, d))
        return 1;

    if (inspect_range(*f12, swivel_angle, 2, angle, i, d))
        return 2;

    if (f21)
    {
        if (inspect_range(*f21, swivel_angle, 3, angle, i, d))
            return 3;

        if (inspect_range(*f22, swivel_angle, 4, angle, i, d))
            return 4;
    }

    if (i)
        swivel_angle = angle;

    return i;
}

//
// Solve for G and select the joint angles that are closest to satisfying
// the joint limits
//
void Limb::solve_aux(float swivel_angle, float x[])
{
    Matrix R1, R2;

    solver.SolveR1R2(swivel_angle, R1, R2);
    extract_s1s2(R1, R2, x, x + 4);
}

//
// Solve for G for a particular family for two spherical joints
//  family_set = 1 => family1,family1
//  family_set = 2 => family1,family2
//  family_set = 3 => family2,family1
//  family_set = 4 => family2,family2
//
// assumes that set_goal has already been called
//

void Limb::solve_aux_family(int family_set, float swivel_angle, float x[])
{
    Matrix R1, R2;

    solver.SolveR1R2(swivel_angle, R1, R2);
    switch (family_set)
    {
    case 1: extract_s1s2_family(R1, R2, 1, 1, x, x + 4); break;
    case 2: extract_s1s2_family(R1, R2, 1, 2, x, x + 4); break;
    case 3: extract_s1s2_family(R1, R2, 2, 1, x, x + 4); break;
    case 4: extract_s1s2_family(R1, R2, 2, 2, x, x + 4); break;
    }
}

//
// Solve for the first three joints for a given
// position, family, and swivel angle
//

void Limb::solve_pos_aux_family(int family, float swivel_angle, float x[])
{
    Matrix R1;

    solver.SolveR1(swivel_angle, R1);
    extract_s1_family(R1, family, x);
}

//
// Solve for the first three joints for a given
// position, and swivel angle. Choose the solution that
// is closest to satisfying the joint limits

void Limb::solve_pos_aux(float swivel_angle, float x[])
{
    Matrix R1;

    solver.SolveR1(swivel_angle, R1);
    extract_s1(R1, x);
}

//
// Try a swivel angle to see if it produces a solution within the
// joint limits
//
int Limb::try_swivel_angle(int solvea, float swivel_angle, float x[])
{
    if (solvea == SolvePosOnly)
    {
        solve_pos_aux(swivel_angle, x);
        if (jt_limits[0].InRange(x[0]) && jt_limits[1].InRange(x[1]) && jt_limits[2].InRange(x[2]))
            return 1;
    }
    else
    {
        solve_aux(swivel_angle, x);
        if (jt_limits[0].InRange(x[0]) && jt_limits[1].InRange(x[1]) && jt_limits[2].InRange(x[2]) &&
            jt_limits[4].InRange(x[4]) && jt_limits[5].InRange(x[5]) && jt_limits[6].InRange(x[6]))
            return 1;
    }

    return 0;
}

//
// If there are singularities see if any of them can be used
// to solve a problem with joint limits
//

int Limb::try_singularities(int solves, float& swivel_angle, float x[])
{
    for (int i = 0; i < num_singular; i++)
        if (try_swivel_angle(solves, singular_pts[i], x))
        {
            swivel_angle = singular_pts[i];
            return 1;
        }
    return 0;
}

//
// Find a swivel angle that is the middle value of the one with the
// largest valid range. If there are no valid ranges and the jt limits
// are turned off then choose 0 by default.
//
// Assumes that either SetGoal or SetGoalPos has been called first
//

int Limb::Solve(float x[], float* new_swivel, float* new_pos)
{
    int success;
    float swivel_angle = -phInfinity;

    x[3] = x3;

    if (check_limits)
    {
        int f_set;

        switch (solve)
        {
        case SolvePosOnly:
            f_set = choose_largest_range(swivel_angle, PSI, PSI + 1);
            if (f_set)
                solve_pos_aux_family(f_set, swivel_angle, x);
            else
                f_set = try_singularities(solve, swivel_angle, x);
            break;

        case SolvePosAndOrientation:
            f_set = choose_largest_range(swivel_angle, PSI, PSI + 1, PSI + 2, PSI + 3);
            if (f_set)
                solve_aux_family(f_set, swivel_angle, x);
            else
                f_set = try_singularities(solve, swivel_angle, x);
            break;

        default:
            f_set = 0;
            init_error("Limb::Solve");
            break;
        }

        success = f_set != 0;
    }

    // If no joint limits then arbitrarily choose 0
    else
    {
        swivel_angle = 0.0f;
        success = SolveByAngle(swivel_angle, x);
    }
    VERIFY(swivel_angle != -phInfinity);
    if (new_swivel)
        *new_swivel = swivel_angle;
    if (new_pos)
        solver.AngleToPos(swivel_angle, new_pos);

    return success;
}

//
// First try solving using the swivel angle. If this fails
// then see if the desired swivel angle is close to a singularity.
// If so then try evaluating the singularity since the PSI
// intervals are not computed reliably near a singularity.
//
int Limb::try_closeby_singularity(int solves, float& swivel_angle, float x[])
{
    // First try the swivel angle

    if (try_swivel_angle(solves, swivel_angle, x))
        return 1;

    for (int i = 0; i < num_singular; i++)
        if (_abs(swivel_angle - singular_pts[i]) < DTOR(1.0))
        {
            // Try the singularity
            if (try_swivel_angle(solves, singular_pts[i], x))
            {
                swivel_angle = singular_pts[i];
                return 1;
            }
        }

    return 0;
}

//
// Solves an IK problem for a swivel angle
//
int Limb::SolveByAngle(float swivel_angle, float x[7], float* new_swivel, float* new_pos)
{
    int success;

    if (swivel_angle < 0)
        swivel_angle += 2 * M_PI;
    if (swivel_angle > 2 * M_PI)
        swivel_angle -= 2 * M_PI;

    x[3] = x3;

    if (check_limits)
    {
        int f_set;

        switch (solve)
        {
        case SolvePosOnly:
            f_set = try_closeby_singularity(solve, swivel_angle, x);
            if (!f_set)
            {
                f_set = choose_closest_range(swivel_angle, PSI, PSI + 1);
                if (f_set)
                    solve_pos_aux_family(f_set, swivel_angle, x);
            }
            break;

        case SolvePosAndOrientation:
            f_set = try_closeby_singularity(solve, swivel_angle, x);
            if (!f_set)
            {
                f_set = choose_closest_range(swivel_angle, PSI, PSI + 1, PSI + 2, PSI + 3);
                if (f_set)
                    solve_aux_family(f_set, swivel_angle, x);
            }
            break;

        default:
            f_set = 0;
            init_error("Limb::Solve");
            break;
        }
        success = f_set != 0;
    }
    else
    {
        success = 1;

        switch (solve)
        {
        case SolvePosOnly: solve_pos_aux(swivel_angle, x); break;

        case SolvePosAndOrientation: solve_aux(swivel_angle, x); break;

        default: init_error("Limb::Solve"); break;
        }
    }

    if (new_swivel)
        *new_swivel = swivel_angle;
    if (new_pos)
        solver.AngleToPos(swivel_angle, new_pos);

    return success;
}

int Limb::SolveByPos(const float pos[3], float x[], float* new_swivel, float* new_pos)
{
    float swivel_angle = solver.PosToAngle(pos);
    return SolveByAngle(swivel_angle, x, new_swivel, new_pos);
}

int Limb::InLimits(const float x[7]) const
{
    for (int i = 0; i < 7; i++)
        if (!jt_limits[i].InRange(x[i]))
            return 0;

    return 1;
}

float roundup(float x)
{
    if (x < 0)
        x += 2 * M_PI;
    return x;
}
void dump_file(char* file, int euler_type, float min[], float max[], Matrix c, Matrix s, Matrix o)
{
    FILE* fp = fopen(file, "w");

    fprintf(fp, "%d\n", euler_type);
    fprintf(fp, "%f %f %f \n", roundup(min[2]), roundup(min[1]), roundup(min[0]));
    fprintf(fp, "%f %f %f \n", max[2], max[1], max[0]);
    for (int i = 0; i < 4; i++)
        fprintf(fp, "%f %f %f %f\n", c[i][0], c[i][1], c[i][2], c[i][3]);
    for (int i = 0; i < 4; i++)
        fprintf(fp, "%f %f %f %f\n", s[i][0], s[i][1], s[i][2], s[i][3]);
    for (int i = 0; i < 4; i++)
        fprintf(fp, "%f %f %f %f\n", o[i][0], o[i][1], o[i][2], o[i][3]);

    fclose(fp);
}

void Limb::Debug(char* file1, char* file2)
{
    Matrix s, c, o, c2, s2, o2;

    solver.R1R2Psi(c, s, o, c2, s2, o2);
    dump_file(file1, euler1, min, max, c, s, o);
    dump_file(file2, euler2, min + 4, max + 4, c2, s2, o2);
}

void Limb::ForwardKinematics(float x[7], Matrix R)
{
    Matrix temp;
    static float yaxis[] = {0, 1, 0};

    float t[3];

    t[0] = x[2];
    t[1] = x[1];
    t[2] = x[0];
    EulerEval(euler1, t, R);

    solver.Tmatrix(temp);
    hmatmult(R, temp, R);

    rotation_axis_to_matrix(yaxis, x[3], temp);
    hmatmult(R, temp, R);

    solver.Smatrix(temp);
    hmatmult(R, temp, R);

    t[0] = x[6];
    t[1] = x[5];
    t[2] = x[4];
    EulerEval(euler2, t, temp);
    hmatmult(R, temp, R);
}

int Limb::SolveAim(float x[3], float psi_angle)
{
    //   if (check_limits)
    // printf("warning limits for solveaim not yet implemented\n");

    Matrix R1;

    solver.SolveAim(psi_angle, R1);
    extract_s1(R1, x);
    return 1;
}
