 
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
#include "StdAfx.h"

#include "mathTrig.h"

//
// Normalize an angle to the range -Pi..Pi
//

//static double angle_normalize_signed(double x)
//{
//    while (x > M_PI)  x -= 2*M_PI;
//    while (x < -M_PI) x += 2*M_PI;
//
//    return x;
//}

#if 0  
//
// Return the distance between two angles measured either
// clockwise or anticlockwise depending on which gives
// a lower magnitude. Assumes that angles are in the 
// range -Pi .. Pi
//
float angle_distance(float x, float y)
{
    unsigned int signx = x > 0.0;
    unsigned int signy = y > 0.0;
    float dist; 

    dist = _abs(x-y);

    // If angles are of opposite signs check whether clockwise
    // or anticlockwise distances are closer 
    if (signx != signy)
    {
        float temp = (2*M_PI) - dist;
        if (temp < dist)
            dist = temp;
    }
    return dist;
}
#endif

//
// Solve a*cos(theta) + b*sin(theta) = c
// Either one or two solutions. Return the answer in radians
//

int solve_trig1(float a, float b, float c, float theta[2])
{
    float temp  = (a*a+b*b-c*c);

    if (temp < 0.0)
    {
	// temp is practically zero
 
	if (_abs(temp / (_abs(a*a) + _abs(b*b) + _abs(c*c))) < 1e-6)
	{
	    // printf("Special case\n");
	    theta[0] = (float) (2*atan(-b/(-a-c)));
	    return 1;
	}
	else
	    return 0;
    }

    temp  = (float) atan2((float)_sqrt(temp),(float) c);//.(float) c
    int num =  (!iszero(temp)) ? 2 : 1;

    // Calculate answer in radians
    theta[0] = (float) atan2(b,a);
    if (num == 2)
    {
        theta[1] = theta[0] - temp;
        theta[0] += temp;

	//theta[0] = angle_normalize_signed(theta[0]);
	//theta[1] = angle_normalize_signed(theta[1]);
    }
    return num;
}



//
// Solve the system
//      a*cos(theta) - b*sin(theta) = c
//      a*sin(theta) + b*cos(theta) = d
//
// There is at most one solution. The answer is returned in radians
// 

float solve_trig2(float a, float b, float c, float d)
{
    return (float)atan2(a*d-b*c,a*c+b*d);
}


// 
// arccos routine that returns up to two solutions. 
//
int myacos(float x, float solns[2])
{
    if (_abs(x) > 1)
	return 0;

    solns[0] = angle_normalize_signed(acos(x));

    if (iszero(solns[0]))
	return 1;
    
    solns[1] = -solns[0];

    return 2;
}

// 
// arcsin routine that returns up to two solutions. 
//
int myasin(float x, float solns[2])
{
    if (_abs(x) > 1)
	return 0;

    solns[0] = (float)angle_normalize_signed(asin(x));

    if (iszero(solns[0]))
	return 1;
    
    if (solns[0] > 0)
	solns[1] = M_PI - solns[0];
    else 
	solns[1] = -M_PI - solns[0];

    return 2;
}




