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


#ifndef _EQNH
#define _EQNH


#include "aint.h"

//
// Evaluate alpha*cos(x) + beta*sin(x) efficiently
// 

inline float sin_and_cos(float x, float alpha, float beta)
{
    while (x < 0)
	x += 2*M_PI;
    while (x > 2*M_PI)
	x -= 2*M_PI;

    float c = _cos(x);
    return (x > M_PI) ?
	(alpha*c - beta*_sqrt(1-c*c)) : 
	(alpha*c + beta*_sqrt(1-c*c));
}


//
// A class representing the equation alpha*cos(psi) + beta*sin(psi) + xi
// where alpha,beta,xi are such that
//	-1 <= alpha*cos(psi) + beta*sin(psi) + xi <= 1
// and
//      -Pi <= psi <= Pi 
//


struct PsiEquation
{
    unsigned char  status, num_crits, num_roots;
    unsigned char *status_ptr, *num_roots_ptr, *num_crits_ptr;
    float alpha, beta, xi; 
    float crit_pts[2], root_pts[2];

    // store temporary computations
    float a2b2;
    float atan2ba;

    void Reset(float a, float b, float x)  
    { 
		alpha = a; beta = b; xi = x;  
		a2b2 = a*a + b*b;
		atan2ba = atan2(b,a);
		num_crits = num_roots = status = 0;
		status_ptr = &status;
		num_roots_ptr = &num_roots;
		num_crits_ptr = &num_crits;
    }

    PsiEquation() {}
    PsiEquation(float a, float b, float x)
    {
		Reset(a,b,x);
    }


    float eval(float psi) const
    {
		return sin_and_cos(psi, alpha, beta) + xi;
		// return alpha*cos(psi) + beta*sin(psi) + xi;
    }

    float deriv(float psi) const
    {
		return sin_and_cos(psi, beta, -alpha);
		// return -alpha*sin(psi) + beta*cos(psi); 
    }


    //
    // Returns the critical points of the equation (1 or 2) 
    //
    int crit_points(float *c) const;

    //
    // Return the roots of the equation (1 or 2)
    // 
    int roots(float *c) const;

    //
    // Returns the solns of alpha*cos(v) + beta*sin(v) + xi = v
    // where -1 <= v <= 1 
    //
    int solve(float v, float *c) const;

    //
    // Calculates the set of intersections of psi such that 
    //		
    //  -1 <= low <=  alpha*cos(psi) + beta*sin(psi) + xi <=  high <= 1
    //
    // Returns the number of these regions between 0..3
    //
    // For eg, if the routine returns 1 then all angles between psi[0] and psi[1]
    // satisfy the constraints above.
    //
    //    int clip(float low, float high, AngleIntList &a) const;
		
    //
    // Calculates the range of psi above and below the specified value y
    //
    // returns the number of times the curve intersects y from 0 to 2  
    //
    // int partition(float y, AngleIntList &above,  AngleIntList &below) const;

    
};

#endif
