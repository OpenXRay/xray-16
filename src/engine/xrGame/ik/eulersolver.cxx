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
#include "eulersolver.h"


typedef void (*euler_solver)(const Matrix G, float &t1, float &t2, float &t3, int family);

void euler_ZXY(const Matrix G, float &t1, float &t2, float &t3, int family);
void euler_YXZ(const Matrix G, float &t1, float &t2, float &t3, int family);
void euler_Yxz(const Matrix G, float &t1, float &t2, float &t3, int family);
void euler_zxY(const Matrix G, float &t1, float &t2, float &t3, int family);

//
// Encodes a location in a matrix and the sign of this term
//
typedef struct
{
    int row;
    int col;
    int sign;
} matrix_entry;


//
// e:
//	the euler solver to use
// simple_jt:
//	 indicates which variable (0,1,2) is the simple term in the rotation matrix
//	(ie: occurs as +/- sin(x) or +/- cos(x) without any other terms. 
// simple_jt_index:
//	indicates which entry in the rotation matrix corresponds to the
//	sin(x) or cos(x) term described above. This entry also encodes 
//      the sign of this term
// simple_jt_type
//      indicates whether the entry is a sin term or a cos term
// complex_jt1
//	indicates which variable (0,1,2) is the first complex term in the rotation
//	matrix. (ie: occurs as +/- sin(x)*f and +/- cos(x)*f where f is either
//      sin(simple_jt) or cos(simple_jt) 
// complex_jt1_index1:
//      indicates which entry in the matrix jt corresponds to sin(x)*f
// complex_jt1_index2
//      indicates which entry in the matrix jt corresponds to cos(x)*f
// complex_jt2, complex_jt2_index1, complex_jt2_index2
//	analogous to complex_jt1 for the second complex joint
//
// For example, the rotation matrix R = R(y)*R(x)*R(z) leads to the following
// system of equations:
// 
//	sin(x)     =  R[1][2]
//   sin(y)*cos(x) = -R[0][2]
//   cos(y)*cos(x) =  R[2][2]
//   sin(z)*cos(x) = -R[1][0]
//   cos(z)*cos(x) =  R[1][1] 
// 
// and there are two sets of solutions to these equations
//
// family1 corresponds to the set of solutions for x in -90 to 90 degrees
// family2 corresponds to the set of solutions for x in 90 to 270 degrees
//
//  
// R = R(z)*R(x)*R(y) yields the following set of equations:
//
//	sin(x)     = -R[2][1]
//   sin(y)*cos(x) =  R[2][0]
//   cos(y)*cos(x) =  R[2][2]
//   sin(z)*cos(x) =  R[0][1]
//   cos(z)*cos(x) =  R[1][1]
//  
struct EulerTableEntry
{
    int			simple_jt_type;
    int			simple_jt;
    matrix_entry	simple_jt_index;
    int			complex_jt1;
    matrix_entry	complex_jt1_index1;
    matrix_entry	complex_jt1_index2;
    int			complex_jt2;
    matrix_entry	complex_jt2_index1;
    matrix_entry	complex_jt2_index2;
} EulerTable[] = 
{
    { SinJtLimit, 1,{2,1,-1}, 0,{0,1,1},{1,1,1},  2,{2,0,1},{2,2,1} }, 
    { SinJtLimit, 1,{1,2,1},  0,{0,2,-1},{2,2,1}, 2,{1,0,-1},{1,1,1}},
    { SinJtLimit, 1,{1,2,-1}, 0,{0,2,-1},{2,2,1}, 2,{1,0,1},{1,1,1} },
    { SinJtLimit, 1,{2,1,1},  0,{0,1,-1},{1,1,1}, 2,{2,0,1},{2,2,1} } 
};

void euler_extract(const EulerTableEntry &E, 
		   const Matrix R, 
		   float vals[3],
		   int family)
{
    float *t[3];
    float v, x[2], y[2]; 
    matrix_entry *p, *q;

    t[0] = vals + E.simple_jt;
    t[1] = vals + E.complex_jt1;
    t[2] = vals + E.complex_jt2;

    p = (matrix_entry *) &E.simple_jt_index;
    v = p->sign * R[p->row][p->col];

    p = (matrix_entry *) &E.complex_jt1_index1;
    q = (matrix_entry *) &E.complex_jt1_index2; 
    y[0] = p->sign * R[p->row][p->col];
    x[0] = q->sign * R[q->row][q->col];

    p = (matrix_entry *) &E.complex_jt2_index1;
    q = (matrix_entry *) &E.complex_jt2_index2; 
    y[1] = p->sign * R[p->row][p->col];
    x[1] = q->sign * R[q->row][q->col];


    if (E.simple_jt_type == SinJtLimit) 
    {
	if (family == 1)
	{
	    // Family1 cos(v) > 0

	    *t[0] = asin1(v);
	    *t[1] = angle_normalize(atan2(y[0],x[0]));
	    *t[2] = angle_normalize(atan2(y[1],x[1]));
	}
	else
	{
	    // Family2 cos(v) < 0

	    *t[0] = asin2(v);
	    *t[1] = angle_normalize(atan2(-y[0],-x[0]));
	    *t[2] = angle_normalize(atan2(-y[1],-x[1]));
	}
    }
    else
    {
	printf("CosType not yet implemented in euler_extract\n");
    }
}


//
// Same as euler_extract except return both families in f1 and f2
//
void euler_extract2(const EulerTableEntry &E, 
		    const Matrix R, 
		    float f1[3],
		    float f2[3])
{
    float *t1[3], *t2[3];
    float v, x[2], y[2]; 
    matrix_entry *p, *q;

    t1[0] = f1 + E.simple_jt;
    t1[1] = f1 + E.complex_jt1;
    t1[2] = f1 + E.complex_jt2;

    t2[0] = f2 + E.simple_jt;
    t2[1] = f2 + E.complex_jt1;
    t2[2] = f2 + E.complex_jt2;

    p = (matrix_entry *) &E.simple_jt_index;
    v = p->sign * R[p->row][p->col];

    p = (matrix_entry *) &E.complex_jt1_index1;
    q = (matrix_entry *) &E.complex_jt1_index2; 
    y[0] = p->sign * R[p->row][p->col];
    x[0] = q->sign * R[q->row][q->col];

    p = (matrix_entry *) &E.complex_jt2_index1;
    q = (matrix_entry *) &E.complex_jt2_index2; 
    y[1] = p->sign * R[p->row][p->col];
    x[1] = q->sign * R[q->row][q->col];


    if (E.simple_jt_type == SinJtLimit) 
    {
	// Family1 cos(v) > 0

	*t1[0] = asin1(v);
	*t1[1] = angle_normalize(atan2(y[0],x[0]));
	*t1[2] = angle_normalize(atan2(y[1],x[1]));

	*t2[0] = angle_normalize( M_PI - *t1[0] );
	*t2[1] = angle_normalize( *t1[1] + M_PI );
	*t2[2] = angle_normalize( *t1[2] + M_PI );
    }
    else
	printf("CosType not yet implemented in euler_extract\n");
}


inline EulerTableEntry *euler_entry(int euler_type)
{
    if (euler_type < 0 || euler_type >= (sizeof(EulerTable) / sizeof(EulerTable[0])))
    {
	fprintf(stderr, "bad euler entry %d detected\n", euler_type);
	exit(0);
    }
    return EulerTable + euler_type;
} 

//
// Given a rotation matrix and a family to choose from find the euler angles
//
void EulerSolve(int euler_type, const Matrix R, float t[3], int family) 
{
    EulerTableEntry *e = euler_entry(euler_type);
    euler_extract(*e, R, t, family);
}

void EulerSolve2(int euler_type, const Matrix R, float f1[3], float f2[3])
{
    EulerTableEntry *e = euler_entry(euler_type);
    euler_extract2(*e, R, f1, f2);
}

void EulerEval(int euler_type, const float t[3], Matrix R)
{
    float X[] = {1,0,0};
    float Y[] = {0,1,0};
    float Z[] = {0,0,1};

    float *a[3];
    int s[3];

    switch(euler_type)
    {
    case ZXY:
	a[0] = Z; a[1] = X; a[2] = Y;
	s[0] = s[1] = s[2] = 1;
	break;

    case YXZ:
	a[0] = Y; a[1] = X; a[2] = Z;
	s[0] = s[1] = s[2] = 1;
	break;

    case Yxz:
	a[0] = Y; a[1] = X; a[2] = Z;
	s[0] = 1; s[1] = s[2] = -1;
	break;

    case zxY:
	a[0] = Z; a[1] = X; a[2] = Y;
	s[0] = s[1] = -1; s[2] = 1;
	break;

    default:
	fprintf(stderr, "bad euler entry %d detected\n", euler_type);
	exit(0);
    }

    Matrix r;
    rotation_axis_to_matrix(a[0], t[0]*s[0],R);
    rotation_axis_to_matrix(a[1], t[1]*s[1],r);
    hmatmult(R, R, r);
    rotation_axis_to_matrix(a[2], t[2]*s[2],r);
    hmatmult(R, R, r);
}

inline void get_psi_parameters(const Matrix c,
			       const Matrix s,
			       const Matrix o,
			       const matrix_entry &m,
			       float &alpha,
			       float &beta,
			       float &gamma)
{
    if (m.sign == 1)
    {
	alpha = c[m.row][m.col];
	beta = s[m.row][m.col];
	gamma = o[m.row][m.col];
    }
    else
    {
	alpha = -c[m.row][m.col];
	beta = -s[m.row][m.col];
	gamma = -o[m.row][m.col];
    }
}

//
// Create one simple joint limit and two complex joint limits 
// based on the euler convention 
//
EulerPsiSolver::EulerPsiSolver(int etype, 
			 const Matrix c, 
			 const Matrix s, 
			 const Matrix o,
			 const float low[3],
			 const float high[3]) : euler_type(etype)
{
    EulerTableEntry *e = euler_entry(euler_type);

    jt_type = e->simple_jt_type;
    index[0] =(short) e->simple_jt;
    index[1] =(short)  e->complex_jt1;
    index[2] = (short) e->complex_jt2;

    float a0, b0, c0, a1, b1, c1, a2, b2, c2;

    get_psi_parameters(c, s, o, e->simple_jt_index, a0, b0, c0);
    j0.init(e->simple_jt_type, a0, b0, c0, 
	    low[e->simple_jt], high[e->simple_jt]);

    get_psi_parameters(c, s, o, e->complex_jt1_index1, a1, b1, c1);
    get_psi_parameters(c, s, o, e->complex_jt1_index2, a2, b2, c2);
    j1.init(e->simple_jt_type, a1, b1, c1, a2, b2, c2, a0, b0, c0, 
	    low[e->complex_jt1], high[e->complex_jt1]);

    get_psi_parameters(c, s, o, e->complex_jt2_index1, a1, b1, c1);
    get_psi_parameters(c, s, o, e->complex_jt2_index2, a2, b2, c2);
    j2.init(e->simple_jt_type, a1, b1, c1, a2, b2, c2, a0, b0, c0, 
	    low[e->complex_jt2], high[e->complex_jt2]);

    num_singular = (short) j1.Singularities(singular);
}


int EulerPsiSolver::Singularities(float psi[2]) const
{
    for (int i = 0; i < num_singular; i++)
	psi[i] = singular[i];
    return num_singular;
}


//
// Solve for psi ranges that lie in joint limits. Return each 
// family for each joint in psi1[0..2] and psi2[0..2]
// 

void EulerPsiSolver::SolvePsiRanges(AngleIntList psi1[3], 
				    AngleIntList psi2[3]) const
{
    EulerTableEntry *e = euler_entry(euler_type);

    j0.PsiLimits(psi1[e->simple_jt],   psi2[e->simple_jt]);
    j1.PsiLimits(num_singular, (float *) singular, 
		 psi1[e->complex_jt1], psi2[e->complex_jt1]); 
    j2.PsiLimits(num_singular, (float *) singular, 
		 psi1[e->complex_jt2], psi2[e->complex_jt2]); 
}

//
// Given a rotation matrix and a family to choose from find the euler angles
//
void EulerPsiSolver::Solve(const Matrix R, float t[3], int family) const
{
    EulerTableEntry *e = euler_entry(euler_type);

    euler_extract(*e, R, t, family);
}



//
// Given a value of psi and a family to choose from find the euler angles
//

void EulerPsiSolver::Solve(float psi, float t[3], int family) const
{
    t[ index[0] ] = j0.theta(family, psi);
    t[ index[1] ] = j1.theta(family, psi);
    t[ index[2] ] = j2.theta(family, psi);
}


//
// Same as Solve except return both families in f1 and f2
//
void EulerPsiSolver::Solve2(const Matrix R, float f1[3], float f2[3]) const
{
    EulerTableEntry *e = euler_entry(euler_type);
    euler_extract2(*e, R, f1, f2);
}


//
// Find the derivatives of the euler angles wrt to psi
//

void EulerPsiSolver::Derivatives(float psi, float t[3], int family) const
{
    t[ index[0] ] = j0.theta_d(family, psi);
    t[ index[1] ] = j1.theta_d(family, psi);
    t[ index[2] ] = j2.theta_d(family, psi);
}

