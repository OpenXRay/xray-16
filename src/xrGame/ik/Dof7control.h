 
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

#ifndef _SRSH
#define _SRSH

#include "math3d.h"

//
// Given matrices G, S, T solve the equation 
//
// G = R2*S*Ry*T*R1
//	for R1,Ry,R2 
// where
//	R1 and R2 represent general rotation matrices
//      Ry represents a rotation about the y axis
// and
//      G is the desired goal matrix
//      S, T are constant matrices 
// 

//
// In the case of the arm:
//	 R2 : Wrist joints 
//	 S  : Wrist to Elbow transformation
//       Ry : Elbow joint
//       T  : Elbow to Shoulder transformation
//       R1 : Shoulder joints 
//

class SRS
{
private:
    short project_to_workspace;
    //
    // Stores equation of circle for a given problem
    //
    float u[3];
    float v[3];
    float n[3];
    float c[3];
    float radius;

    //
    // Stores projection axis for determining u and the positive 
    // direction axis for determining positive direction of angle
    float proj_axis[3];
    float pos_axis[3];
    
    //
    // Stores end effector position in world frame and R1 frame
    //
    float ee[3]; 
    float ee_r1[3];
    //
    // Stores position of middle revolute joint in R1 frame
    // 
    float p_r1[3]; 

    //
    // Stores angle of R joint
    //
    float upper_len;   // Len of T pos vector
    float lower_len;   // Len of S pos vector
    float reciprocal_upper_len; 

    float r_angle;

    // 
    // Stores goal transformation
    //
    Matrix G;

    //
    // Stores constant matrices and rotation of revolute joint
    // and their product S*Ry*T. 

    Matrix T, S, SRT;

    //
    // Ry = Rotation matrix by flexion joint (only used by aiming routines)
    // axis = axis of aiming vector in hand coordinates
    Matrix Ry;
    float axis[3];

    void evaluate_circle(float angle, float p[3]);

public:
    void ProjectOn() 
	{ project_to_workspace = 1; }

    void ProjectOff() 
	{ project_to_workspace = 0; }

    //
    // Given the position of the R joint find the corresponding 
    // swivel angle. Must call SetGoal or SetGoalPos first.
    // 
	float Length( ) const { return get_translation( T ) + get_translation( S ); }
    float PosToAngle( const float p[3] );

    //
    // Given the swivel angle calculate the pos of the R joint. 
    // Must call SetGoal or SetGoalPos first.
    // 
    void AngleToPos(float psi, float p[3]);

    // Sets the goal matrix, the projection axis, and the 
    // positive direction axis
    // Returns 1 if the goal is feasible
    int  SetGoal(const Matrix  G, float &rangle);
	void EvaluateCircle(const float p[3]);
    // Solve for both R1 and R2 given the pos or angle of the R joint
    // returns the angle of the R joint 

    void SolveR1R2(const float pos[3], Matrix  R1, Matrix  R2);
    void SolveR1R2(float angle, Matrix  R1, Matrix  R2);


    // Must call SetGoal first 
    // Returns the psi equations of the rotation matrix R1
    // ie: alpha[i][j]*cos(phi) + 
    //     beta[i][j]*sin(phi) + 
    //     xi[i][j] = R1[i][j] 
 
    int R1Psi(Matrix alpha, Matrix beta, Matrix xi);

    // Must call SetGoal first 
    // Returns the psi equations of the rotation matrix R1 and R2 analogous
    // to R1Psi
    //

    int R1R2Psi(Matrix alpha,  Matrix beta, Matrix xi,
		Matrix alpha2, Matrix beta2, Matrix xi2); 

    
    // Sets the goal pos. EE is a constant matrix that specifies
    // the value of R2*E where E is a matrix that puts the base
    // of the last S joint to a desired EE site
    //
    // Returns 1 if the goal is feasible
    //
    // Thus the problem to solve is to find R1 and Ry st 
    //
    // g = [0,0,0,1]*EE*S*Ry*T*R1 

    int  SetGoalPos(const float g[3], const Matrix  EE, float &rangle);

    // Solves only for R1 after a call to SetGoalPos
    void SolveR1(const float pos[3],  Matrix  R1);
    void SolveR1(float angle,  Matrix  R);


    // Constructor takes the T and S matrices
    void init(const Matrix  T, const Matrix  S, const float a[3], const float p[3]);

    SRS(const Matrix  T1, const Matrix  S1, const float a[3], const float p[3]) 
    { 
        init(T1,S1,a,p); 
    }

    SRS()  {}

    ~SRS() {}


    void Tmatrix(Matrix  TT)
    {
	cpmatrix(TT, T);
    }
    void Smatrix(Matrix  SS)
    {
	cpmatrix(SS, S);
    }
	void SetTMatrix(const Matrix  TT)
	{
	cpmatrix(T,TT);
	}

	void SetSMatrix(const Matrix  SS)
	{
		cpmatrix(S,SS);
	}

    // Sets the goal for an aiming problem
    // goal is the point we want to point to 
    // axis is the pointing axis in the hand frame
    // flex_angle is the amount of flexion in the elbow 
/*
    void SRS::SetAimGoal(const float goal[3],
*/
    void SetAimGoal(const float goal[3],
		     const float axis[3],
		     float flex_angle);


    //
    // Solves the aiming problem for a given angle of the hand circle
    // (Must call SetAimGoal first)
    //
/*
    void SRS::SolveAim(float psi_angle, Matrix  R1);
*/
    void SolveAim(float psi_angle, Matrix  R1);
}; 

#endif
