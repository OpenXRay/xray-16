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

#ifndef _EULERSOLVER
#define _EULERSOLVER

#include "math3d.h"
#include "jtlimits.h"

//
// Encodes various euler angle conventions. Upper case means a 
// positive rotation, lower case means a negative rotation. 
//
// For example, xYZ means R(-x)*R(Y)*R(Z). 
//
// Note that only a small subset of the 6*4*6 possible euler 
// rotations are encoded.
//
// Do not renumber these entries as they are used to index
// an internal table

enum 
{
    ZXY = 0,	// left shoulder, ankle, hip
    YXZ = 1,	// left wrist
    Yxz = 2,	// right wrist
    zxY = 3	// right shoulder, ankle, hip
};


// Given a matrix find the corresponding euler angles
void EulerSolve(int euler_type, const Matrix R, float t[3], int family = 1);
void EulerSolve2(int euler_type, const Matrix R, float f1[3], float f2[3]);

void EulerEval(int euler_type, const float t[3], Matrix R);		 


class EulerPsiSolver
{
private:
    int euler_type; // ZXY, YXZ, etc
    int jt_type;    // simple jt is either sin(theta) or cos(theta)
    // index[0] = index of simple jt, index[1],index[2] indices of complex joints
    short index[3];

    short num_singular;
    float singular[2]; 

    SimpleJtLimit  j0;
    ComplexJtLimit j1;
    ComplexJtLimit j2;

public:

    EulerPsiSolver(int etype, 
		   const Matrix c,
		   const Matrix s,
		   const Matrix o,
		   const float low[3],
		   const float high[3]);

    ~EulerPsiSolver() {}

    // Solve for psi ranges that lie in joint limits. Return each 
    // family for each joint in psi1[0..2] and psi2[0..2]
    void SolvePsiRanges(AngleIntList psi1[3], 
			AngleIntList psi2[3]) const;

    // Given a matrix or psi angle find the corresponding euler angles
    void Solve(const Matrix R, float t[3], int family = 1) const;
    void Solve(float psi, float t[3], int family = 1) const;

    void Solve2(const Matrix R,
		float f1[3], 
		float f2[3]) const;


    // Given a psi angle find the derivatives of the euler angles relative to psi
    void Derivatives(float psi, float t[3], int family = 1) const;

    int Singularities(float psi[2]) const; 
};


#endif



