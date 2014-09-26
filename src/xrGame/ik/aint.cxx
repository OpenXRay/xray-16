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
//#include <math.h>
#include "aint.h"


inline float min(float x, float y)
{
    return x < y ? x : y;
}

inline float max(float x, float y)
{
    return x > y ? x : y;
}



//
// Sets the low bound for the interval
// 
void AngleInt::SetLow(float l)
{
    low = angle_normalize(l);
}

//
// Sets the high bound for the interval
// 
void AngleInt::SetHigh(float h)
{
    high = angle_normalize(h);
}


AngleInt::AngleInt(float l, float h)
{
    SetLow(l);
    SetHigh(h);
}

float AngleInt::Mid() const
{
    float mid;
    if (High() > Low())
	mid = ((High() + Low()) / 2.0f);
    else
	mid = angle_normalize(M_PI + (High() + Low())/2.0f);

    return mid;
}

//
// If a is outside the interval then return a positive value indicating
// the minimum angular displacement to move a inside the interval.
// If a is inside the interval then return a negative value indicating
// the minimum angular displamcent to move a outside the interval
//
float AngleInt::Distance(float v) const
{
    const float eps   = AINT_EPSILON;
    const float TwoPi = 2*M_PI;

    float t1, t2;
    v = angle_normalize(v);

    if (IsEmpty(eps))
	return TwoPi;

    if (IsFullRange(eps))
	return -M_PI;

    if (iszero(v) || istwopi(v))
    {
	if (High() > Low())
	{
	    t1 = Low();
	    t2 = TwoPi - High();
	}
	else
	{
	    t1 = Low() - TwoPi;
	    t2 = -High();
	}
    }
    else if (High() > Low())
    {
	// 0 <= v < Low
	if (v < Low())
	{
	    t1 = Low() - v;
	    t2 = TwoPi - High() + v;
	}

	// Low <= v < High
	else if (v < High())
	{
	    t1 = v - High();
	    t2 = Low() - v;
	}

	// High <= v < 2*M_PI
	else 
	{
	    t1 = v - High();
	    t2 = TwoPi - v + Low();
	}
    }
    else // (Low() > High())
    {
	// 0 < v < High
	if (v < High())
	{
	    t1 = v - High(); 
	    t2 = Low() - v - TwoPi;
	}

	// High <= v < Low
	else if (v < Low())
	{
	    t1 = v - High();
	    t2 = Low() - v;
	}
	
	// Low <= v < 2*M_PI
	else
	{
	    t1 = Low() - v;
	    t2 = v - TwoPi - High();
	}
    }

    return (_abs(t1) < _abs(t2)) ? t1 : t2;
}


int AngleInt::OldIsSupersetOf(const AngleInt& a, float eps) const
{
    return InRange(a.Low(),eps) && InRange(a.High(),eps) && InRange(a.Mid(),eps);
}

int AngleInt::IsSupersetOf(const AngleInt& a, float eps) const
{
    if (low < high)
    {
	if (a.low < a.high)
	    return InRange(a.low, eps) && InRange(a.Mid(), eps) && InRange(a.high, eps);
	else
	    return InRange(a.low, eps) && istwopi(high,eps) && a.high < eps;
    }
    else
    {
	if (a.low < a.high)
	    return le(a.high,high,eps) || ge(a.low,low,eps);
	else
	    return InRange(a.low, eps) && InRange(a.high, eps);
    }
}


int AngleInt::IsSubsetOf(const AngleInt &a, float eps) const
{
    return a.IsSupersetOf(*this, eps);
}

//
// WARNING:
// Assumption is that a is not a superset or subset of *this and that
// a is not full range or fully empty
// 
 
int AngleInt::merge_aux(const AngleInt &a, AngleInt &b, float eps)  const
{
    int in1 = InRange(a.Low(), eps);
    int in2 = InRange(a.High(), eps);

    if (!in1 && !in2)
	return 0;

    if (in1 && in2)
    {
	float mid = (Low() + High()) / 2.0f;
	if (Low() < High())
	    mid += M_PI;

	if (a.InRange(mid, eps))
	    b.Set(0,2*M_PI);
	else
	    b.Set(Low(), High());
    }
    else if (in1)
	b.Set(Low(), a.High());
    else 
	b.Set(a.Low(), High());

    return 1;
}

int AngleInt::merge(const AngleInt &a, AngleInt &b, float eps)  const
{
    if (merge_aux(a,b, eps))
	return 1;
    else
	return a.merge_aux(*this,b,eps);
}


//
// Checks if a is in the range defined by low and high
//

//
// Returns the size of the range 
// 
float AngleInt::Range() const
{
    return (low < high) ? (high - low) : high + (2*M_PI - low);
}

void AngleIntList::add(float l, float h)
{
    AngleIntListNode *t = xr_new<AngleIntListNode>(l, h, (AngleIntListNode*)0);

    if (!head)
	head = tail = t;
    else
    {
	tail->next = t;
	tail =  t;
    }
}




void AngleIntList::remove(AngleIntListNode *t)
{
    AngleIntListNode *prev;

    if (head == t)
    {
	prev = 0;
	head = t->next;
    }
    else
    {
	prev = head;

	while (prev->next != t)
	    prev = prev->next;

	prev->next = t->next;
    }

    if (tail == t)
	tail = prev;

    delete t;
}



//
// Given that b is a subset of a within some eps then return
// c (based on a) such that numerically b ia subset of c 
// 
void swell(const AngleInt &a,
	   const AngleInt &b,
	   AngleInt &c)
{
    if (a.IsFullRange())
	c.Set(0,2*M_PI);
    else
    {
	float l = a.Low();
	float h = a.High(); 
	float l2 = b.Low();
	float h2 = b.High();

	if (l < h)
	{
	    if (l2 < h2)
		l = min(l,l2);
	    else
		l = l2;
	    h = max(h,h2);
	}
	else 
	{
	    if (l2 < h2)
	    {
		/*
		if (istwopi(h2))
		    h = h;
		else
		    h = max(h,h2);
	    */
	    }
	    else
	    {
		l = min(l,l2);
		h = max(h,h2);
	    }
	}
	c.Set(l, h);
    }
}

//
// Adds the interval (l,h) to the list. If possible merge it with
// other intervals 
//
void AngleIntList::Add(float l, float h, float eps)
{
    AngleInt a(l,h);
    AngleInt b;

    // interval to add is either emtpy or close to full range 0..2*M_PI
    if (a.IsEmpty())
	return;

    else if (a.IsFullRange())
    {
	Clear();
	add(0.0f, 2*M_PI - AINT_EPSILON);
    }
    
    // Put a into the list taking into account it may merge with another entry
    else
    {
	// check if any merges are required
	for (AngleIntListNode *temp = head; temp; temp = temp->next)
	{

	    // a is already completely contained
	    if (temp->D.IsSupersetOf(a, eps))
	    {
		swell(temp->D, a, temp->D);
		return; 
	    }


	    // a already completely contains a node 
	    else if (temp->D.IsSubsetOf(a, eps))
	    {
		swell(a, temp->D, a);
		// remove the smaller node
		remove(temp);

		// Add a recursively since it may merge with other nodes
		Add(a.Low(), a.High(), eps);
		return;
	    }

	    else if (temp->D.merge(a,b,eps))
	    {
		// Remove the original node
		remove(temp);
		
		// Add the merged node recursively since it may merge with other nodes
		Add(b.Low(), b.High(), eps);
		return;
	    }
	}

	// a is completely disjoint from the other nodes
	add(l, h);
    }
}

void AngleIntList::AddList(AngleIntList &dest, float eps) const
{
    for (AngleIntListNode *temp = head; temp; temp = temp->next)
	dest.Add(temp->D.Low(), temp->D.High(), eps);
    dest.wrap(eps);
}


float AngleIntList::Distance(float a) const
{
    float dist = 2*M_PI;

    for (AngleIntListNode *t = head; t; t = t->next)
    {
	float temp = t->D.Distance(a);
	if (temp < dist)
	    dist = temp;
    }

    return dist;
}

int AngleIntList::NumIntervals() const
{
    int count  = 0;
    for (AngleIntListNode *t = head; t; t = t->next, count++);
    return count;
}

AngleInt *AngleIntList::Largest() const
{
    if (!head)
	return 0;

    AngleInt *l = &head->D;
    float d = head->D.Range(); 

    for (AngleIntListNode *t = head->next; t; t = t->next)
    {
	float d2 = t->D.Range();
	if (d2 > d)
	{
	    d = d2;
	    l = &t->D;
	}
    }

    return l;
}

void AngleIntList::Copy(AngleIntList &dest) const
{
    dest.Clear(); 
    for (AngleIntListNode *temp = head; temp; temp = temp->next)
	dest.Add(temp->D.Low(), temp->D.High());
}


//
// For iterating througn an angle interval from low+eps..high-eps. 
// reverse specifies whether you want to iterate *outside* the interval
// instead of inside of it
//
AngleIntIterator::AngleIntIterator(const AngleInt &a, int num, float eps, int reverse)
{
    count = 0;

    // Handle null cases first

    if ((a.IsEmpty() && !reverse) || (a.IsFullRange() && reverse))
	n = 0;

    else if (reverse)
    {
	AngleIntIterator A(AngleInt(a.High(), a.Low()), num, eps, 0);
	*this = A;
    }

    else
    {
	dx = a.Range() - 2*eps;

	// 2*eps is larger than the range
	if (dx < 0.0)
	    n = 0;
	else if (num == 1)
	{
	    x = a.Mid();
	    n = 1;
	}
	else 
	{
	    x = a.Low() + eps;
	    dx /= (num - 1);
	    n = num;
	}
    }
}


int AngleIntIterator::Next(float &a)
{
    if (count == n)
	return 0;

    a = angle_normalize(x);
    x += dx;
    count++;

    return 1;
}



//
// Intersect one psi interval with another and return the 
// list of intersections in c
// 

static void aint_intersect_aux(const AngleInt &a, const AngleInt &b, AngleIntList &c)
{
#if 0
    // Degenerate cases of null intersection at 0/2pi boundary
    if (iszero(a.Low()) && istwopi(b.High()) && a.High() < b.Low())
	return;
    if (iszero(b.Low()) && istwopi(a.High()) && b.High() < a.Low())
	return;
#endif
    
    const float eps = AINT_EPSILON;

    int in1 = a.InRange(b.Low() + 2*eps, eps);
    int in2 = b.InRange(a.Low() + 2*eps, eps);

	    // no overlap
    if (!in1 && !in2)
	return; 

    if (in1)
	c.Add(b.Low(), min(b.High(), a.High()));

    else if (in2)
	c.Add(a.Low(), min(b.High(), a.High()));		    
}

static void aint_intersect(const AngleInt &a, const AngleInt &b, AngleIntList &c)
{
    if (a.IsFullRange())
	c.Add(b.Low(), b.High());

    else if (b.IsFullRange())
	c.Add(a.Low(), a.High());

    else if ((!a.IsEmpty()) && (!b.IsEmpty()))
    {
	if (a.Low() > a.High())
	{
	    if (b.Low() > b.High())
	    {
		AngleInt x1, x2, y1, y2;
		
		a.split(x1, y1);
		b.split(x2, y2);

		aint_intersect_aux(x1, x2, c);
		aint_intersect_aux(x1, y2, c);
		aint_intersect_aux(y1, x2, c);
		aint_intersect_aux(y1, y2, c);
	    }
	    else
	    {
		AngleInt x, y;

		a.split(x,y);

		aint_intersect_aux(x,b,c);
		aint_intersect_aux(y,b,c);
	    }

	}
	else if (b.Low() > b.High())
	{
	    AngleInt x, y;

	    b.split(x,y);

	    aint_intersect_aux(a,x,c);
	    aint_intersect_aux(a,y,c);
	}
	else
	    aint_intersect_aux(a,b,c);
    }
}

//
// Union one psi interval with another and return the 
// list of intersections in c
// 

static void aint_union_aux(const AngleInt &a, const AngleInt &b, AngleIntList &c)
{
    const float eps = AINT_EPSILON;
    int in1 = a.InRange(b.Low() + 2*eps, eps);
    int in2 = b.InRange(a.Low() + 2*eps, eps);


    // no overlap add both into c
    if (!in1 && !in2)
    {
	c.Add(a.Low(),a.High());
	c.Add(b.Low(),b.High());	
    }
    else if (in1)
	c.Add(a.Low(), max(b.High(), a.High()));

    else 
	c.Add(b.Low(), max(b.High(), a.High()));

}

static void aint_union(const AngleInt &a,
		   const AngleInt &b,
		   AngleIntList &c)
{
    if (a.IsFullRange())
	c.Add(a.Low(), a.High());
    else if (b.IsFullRange())
	c.Add(b.Low(), b.High());
    else if (a.IsEmpty())
    {
	if (!b.IsEmpty())
	    c.Add(b.Low(), b.High());
    }
    else if (b.IsEmpty())
	c.Add(a.Low(), a.High());

    else if (a.Low() > a.High())
    {
	if (b.Low() > b.High())
	{
	    AngleInt x1, x2, y1, y2;
	    a.split(x1, y1);
	    b.split(x2, y2);

	    aint_union_aux(x1, x2, c);
	    aint_union_aux(x1, y2, c);
	    aint_union_aux(y1, x2, c);
	    aint_union_aux(y1, y2, c);
	}
	else
	{
	    AngleInt x, y;

	    a.split(x,y);
	    
	    aint_union_aux(x,b,c);
	    aint_union_aux(y,b,c);
	}
    }

    else if (b.Low() > b.High())
    {
	    AngleInt x, y;
	    b.split(x,y);
	    aint_union_aux(a, x, c);
	    aint_union_aux(a, y, c);
    }
    else
	aint_union_aux(a, b, c);
}


void AngleIntList::wrap(float eps)
{
    AngleIntListNode *s, *t;

    s = 0;
    t = 0;

    for (AngleIntListNode *temp = head; temp; temp = temp->next)
    {
	if (_abs(temp->D.Low()) < eps)
	{
	    s = temp;
	    if (t) 
		break;
	}

	if (_abs(temp->D.High()-2*M_PI) < eps)
	{
	    t = temp;
	    if (s)
		break;
	}
    }

    if ((s && t) && (s != t))
    {
	float low = t->D.Low();
        float high = s->D.High(); 

	remove(s);
	remove(t);

	Add(low, high);
    }
    
}


//
// Take the union of two sets of psi intervals
//
void Union(const AngleIntList &a,
	   const AngleIntList &b,
	   AngleIntList &c)
{
    AngleInt *ap, *bp; 

    c.Clear();
    if (a.IsEmpty())
	b.Copy(c);
    else if (b.IsEmpty())
	a.Copy(c);
    else
    {
		for (AngleIntListIterator aa(a); ; )
		{
			ap = aa.Next();
			for (AngleIntListIterator bb(b); ; )
			{
				bp = bb.Next();
				aint_union(*ap, *bp, c);
			}
		}
	}
}

//
// Take the intersection of two sets of psi intervals
//
void Intersect(const AngleIntList &a,
	       const AngleIntList &b,
	       AngleIntList &c)
{
    AngleInt *ap, *bp; 

    c.Clear();
    if (a.IsEmpty() || b.IsEmpty())
	return; 

	AngleIntListIterator aa(a);
    for (ap = aa.Next(); ap; ap = aa.Next())
	{
		AngleIntListIterator bb(b);
		for (bp = bb.Next(); bp; bp = bb.Next())
		{
			aint_intersect(*ap, *bp, c);
		}
	}
    c.wrap();
}
