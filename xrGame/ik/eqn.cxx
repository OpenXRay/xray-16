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
#include "eqn.h"


/*
 * Put angle in range 0 .. 2*M_PI. Bounds on range of Psi 
 */
/*
const double LowBound  = 0;
const double HighBound = 2*M_PI;
const double TwoPi = 2*M_PI;


static double angle_normalize(double theta)
{
    while (theta < LowBound)
	theta += TwoPi;
    while (theta > HighBound)
	theta -= TwoPi;

    return theta;
}
*/

static int solve_trig1_aux(float c, 
			   float a2b2,
			   float atan2ba,
			   float theta[2])
{
    float temp  = a2b2-c*c;
    int num;

    if (temp < 0.0f)
	return 0;

    temp  = atan2(_sqrt(temp), c);
    num =  (_abs(temp) > 1e-6f) ? 2 : 1;

    theta[0] = atan2ba;
    if (num == 2)
    {
        theta[1] = theta[0] - temp;
        theta[0] += temp;

	//theta[0] = angle_normalize(theta[0]);
	//theta[1] = angle_normalize(theta[1]);

	if (theta[0] > theta[1])
	{
		swap(theta[0],theta[1]);
	//	temp = theta[0]; 
	//    theta[0] = theta[1];
	 //   theta[1] = temp;
	}
    }
    return num;
}



/*
 *  Solve a*cos(theta) + b*sin(theta) = c
 *  Either one or two solutions. Return the answer in radians.
 *  Also sort the answers in increasing order.
 */

static int solve_trig1(float a, float b, float c, float theta[2])
{
    return solve_trig1_aux(c, a*a+b*b, atan2(b,a), theta);
}

#if 0
int consistency_check(double a, double b, double c, 
		      double a2b2, double atan2ba)
{
    float t[2], t2[2];

    int n  = solve_trig1(a, b, c, t);
    int n2 = solve_trig1_aux(a2b2, atan2ba, c, t2);

    if (n != n2)
    {
	printf("error\n");
	n  = solve_trig1(a, b, c, t);
	n2 = solve_trig1_aux(a2b2, atan2ba, c, t2);
    }

    for (int i = 0; i < n; i++)
	if (fabs(t[i] - t2[i]) < 1e-5)
	    printf("error2\n");
}
#endif

#define GOT_ROOTS (1)
#define GOT_CRITS (2)

//
// The critical points are where the derivative is 0
//
int PsiEquation::crit_points(float *t) const
{
    if (!(*status_ptr & GOT_CRITS))
    {
	// CANNOT use solve_trig1_aux here 
	*num_crits_ptr = (u8)solve_trig1(beta, -alpha, 0, (float *) crit_pts);
	*status_ptr |= GOT_CRITS;
    }

    switch(num_crits)
    {
    case 1:
	t[0] = crit_pts[0];
	break;
    case 2:
	t[0] = crit_pts[0];
	t[1] = crit_pts[1];
	break;
    default:
	break;
    }
    return num_crits;
}


//
// Return the roots of the equation
// 
int PsiEquation::roots(float *t) const
{
    if (!(*status_ptr & GOT_ROOTS))
    {
	*num_roots_ptr =(u8) solve_trig1_aux(-xi, a2b2, atan2ba, (float *) root_pts);
	*status_ptr  |= GOT_ROOTS;
    }

    switch(num_roots)
    {
    case 1:
	t[0] = root_pts[0];
	break;
    case 2:
	t[0] = root_pts[0];
	t[1] = root_pts[1];
	break;
    default:
	break;
    }
    return num_roots;
}

int PsiEquation::solve(float v, float *t) const
{
    // consistency_check(alpha,beta,-xi+v,a2b2,atan2ba);
    // return solve_trig1(alpha, beta, -xi+v, t);
    return solve_trig1_aux(-xi+v, a2b2, atan2ba, t);
}

/*
 * Returns the regions of intersections of 
 *
 *	a * cos(psi) + b*sin(psi) + c = low 
 * and
 *	a * cos(psi) + b*sin(psi) + c = high
 * 
 * from 0 to 3 possible regions
 *
 */

#if 0
int PsiEquation::clip(float low, 
		      float high, 
		      AngleIntList &a) const
{
    float s[2], t[2], psi[6];

    int  m, n;

    m = solve_trig1_aux(low-xi,  a2b2, atan2ba,s);
    n = solve_trig1_aux(high-xi, a2b2, atan2ba,t);
    // m = solve_trig1(alpha,beta,low-xi,s);
    // n = solve_trig1(alpha,beta,high-xi,t);


    /* If no intersections curve is either entirely in or out */
    if (n == 0 && m == 0)
    {
	/* Evaluate one point and see if it within range */
	float t = eval(0.0);

	if (t > low && t < high)
	{
	    psi[0] = LowBound;
	    psi[1] = HighBound;
	    n = 1;
	}
	else
	    n = 0;
    }

    else 
    {
	int j, k, l;

	k = l = 0;
	j = 1;

	/* If curve intersects the low boundary first */ 
	if (m && (s[0] < t[0] || n == 0))
	{
	    /* Check deriv to see if curve going out of or into boundary */
	    if (deriv(s[0]) < 0)
		psi[0] = LowBound;
	    else
	    {
		psi[0] = s[0];
		k = 1;
	    }
	}

	/* Curve intersets high boundary first */
	else
	{
	    /* Check deriv to see if curve going out of or into boundary */
	    if (deriv(t[0]) > 0)
		psi[0] = LowBound;
	    else
	    {
		psi[0] = t[0];
		l = 1; 
	    }
	}

	/* Sort the intersections */

	while (k < m && l < n)
	    if (s[k] < t[l]) 
		psi[j++] = s[k++];
	    else
		psi[j++] = t[l++];

	while (k < m)
	    psi[j++] = s[k++]; 

	while (l < n)
	    psi[j++] = t[l++];
	
	/* If odd number of boundary pts need to add right bounds */

	if (j & 0x1)
	    psi[j++] = HighBound;

	n = j/2;
    }

    // Only add intervals larger than epsilon
    for (m = 0; m < n; m++)
	if (fabs(psi[2*m] - psi[2*m+1]) > 1e-5)
	    a.Add(psi[2*m],psi[2*m+1]);

    return n;
}
#endif

#if 0
//
// Calculates the range of psi above and below the specified value y
//
// ie: above contains all ranges of psi such that
//   1 <= y <=  alpha*cos(psi) + beta*sin(psi) + xi 
// and below contains all the range of psi such that   
//   0 >= y >=  alpha*cos(psi) + beta*sin(psi) + xi 
//

int PsiEquation::partition(float y,
			   AngleIntList &above,
			   AngleIntList &below) const
{
    float s[2];
    int n = solve_trig1(alpha, beta, y - xi, s); 

    // Curve is entirely above or below y
    switch(n)
    {
    case 0:
	/* Evaluate one point and see if it within range */
	float t = eval(0.0);
	if (t > y)
	    above.Add(LowBound, HighBound);
	else
	    below.Add(LowBound, HighBound);
	break;
    case 1:
	/* Check if curve is going out of or into line */
	if (deriv(s[0]) < 0)
	{
	    above.Add(LowBound, s[0]);
	    below.Add(s[0], HighBound);
	}
	else
	{
	    below.Add(LowBound, s[0]);
	    above.Add(s[0], HighBound);
	}
	break;

    case 2:
	if (deriv(s[0]) < 0)
	{
	    above.Add(LowBound, s[0]);
	    below.Add(s[0], s[1]);
	    above.Add(s[1], HighBound);
	}
	else
	{
	    below.Add(LowBound, s[0]);
	    above.Add(s[0], s[1]);
	    below.Add(s[1], HighBound);
	}
	break;
    }

    return n;
}

#endif
