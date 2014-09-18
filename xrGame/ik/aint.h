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
#ifndef _AINTH
#define _AINTH

//#ifdef _WINDOWS
//#define M_PI            3.14159265358979323846
//#endif

#define AINT_EPSILON     (1e-5f)
#define AINT_BIG_EPSILON (.01f)

inline int equal(float x, float y, const float eps = AINT_EPSILON)
{
    return (_abs(x - y) < eps);
}


inline int istwopi(float x, const float eps = AINT_EPSILON)
{
    return equal(x, 2.0f*M_PI, eps);
}

inline int iszero(float x, const float eps = AINT_EPSILON)
{
    return _abs(x) < eps;
}

inline int le(float x, float y, const float eps = AINT_EPSILON)
{
    return (x < y) || equal(x,y,eps);
}

inline int ge(float x, float y, const float eps = AINT_EPSILON)
{
    return (x > y) || equal(x,y,eps);
}
			  

//
// Puts an angle in the range 0..2*PI
// 
//inline float angle_normalize(float psi)
//{
//    if (fabs(psi-2*M_PI) < AINT_EPSILON)
//	psi = 2*M_PI;
//    else
//	while (psi > 2*M_PI)  psi -= 2* M_PI;
//
//    if (fabs(psi) < AINT_EPSILON)
//	psi = 0.0;
//    else
//	while (psi < 0) psi += 2* M_PI;
//
//    return psi;
//}


//
// Returns the closest distance between two angles measured either
// clockwise or counterclockwise. Always return a positive value

inline float angle_distance(float a1, float a2)
{
    float t1, t2; 

    a1 = angle_normalize(a1);
    a2 = angle_normalize(a2);

    if (a1 > a2)
    {
	t1 = 2*M_PI - a1 + a2;
	t2 = a1 - a2;
    }
    else
    {
	t1 = 2*M_PI - a2 + a1;
	t2 = a2 - a1;
    }
    if (t2 < t1)
	t1 = t2;
    if (t1 < AINT_EPSILON)
	t1 = 0.0;

    return t1;
}

//
// Stores a legal range of angles corresponding to a joint limit or something
// analogous. All angles are mapped to the range 0..2PI
//
// Angle ranges are represented in counterclockwise notation so low does
// not have to less than high
// 
// eg: low = 275 deg, high = 30 deg means that any angle in the range 275..360 or 0..30 
//     is legal
// By contrast  
//     low = 30, high = 275 means that any angle in the range 30..275 is legal
//


class AngleInt
{
private:
    friend class AngleIntIterator;
    friend class AngleIntList;
    float low, high;

    int merge_aux(const AngleInt &a, AngleInt &b, float eps) const;
    int merge(const AngleInt &a, AngleInt &b, float eps) const;

public:
    // Splits an angle interval of the form low > high into two intervals with high > low
    void split(AngleInt &l, AngleInt &h) const
    {
	l.Set(0, high);
	h.Set(low, 2*M_PI);
    }

    AngleInt() : low(0), high(2*M_PI) {}
    AngleInt(float l, float h);

    void SetLow(float l);
    void SetHigh(float l);
    void Set(float l, float h) { SetLow(l); SetHigh(h); }

    float Low() const { return low; }
    float High() const { return high; }

    int IsFullRange(float eps = AINT_BIG_EPSILON) const
    {
		return _abs(high-2*M_PI) < eps && _abs(low) < eps;
    }

    int IsEmpty(float eps = AINT_BIG_EPSILON) const
    {
		if (low <= high)
			return (
			_abs(low-high) < eps);
		else
			return (_abs(low-2*M_PI) + _abs(high) < eps);
		}

		// returns T if a is in the angle range
		int InRange(float a, float eps = AINT_EPSILON) const
		{
		if (IsEmpty())
			return 0;

		a = angle_normalize(a);
		if (iszero(a) || istwopi(a))
			return (low > high) || iszero(low) || istwopi(high);
		else
			return (low < high) ? 
			le(low,a,eps) && le(a,high,eps) : le(a,high,eps) || ge(a,low,eps);
    }


    // returns the magnitude of the angle between low and high
    float Range() const;

    // returns the midpoint of the range
    float Mid() const;

    int IsSubsetOf(const AngleInt& a, float eps = AINT_BIG_EPSILON) const;
    int IsSupersetOf(const AngleInt& a, float eps = AINT_BIG_EPSILON) const;
    int OldIsSupersetOf(const AngleInt& a, float eps = AINT_BIG_EPSILON) const;

    // determines if a can be merged with *this. Returns 1 if the nodes
    // can be merged and returns the result in b


    // Returns how far a value is from being within the angle interval
    float Distance(float a) const;

};

//
// Class for iterating through angles
// 
class AngleIntIterator
{
    int count;
    int n;
    float x;
    float dx;

public:

    // Iterates through an angle range from low+eps .. high-eps num > 0 times
    // If reverse is 1 iterate outside the range
    AngleIntIterator(const AngleInt &a, int num, float eps, int reverse = 0);


    // Retrieves next value in iteration. Returns 0 if last value has
    // been obtained
    int Next(float &a);

};


//
// An AngleIntList is used to store a set of AngleInts
// 
struct AngleIntListNode 
{
    AngleInt D;
    AngleIntListNode *next;
    short flag; 

    AngleIntListNode(float low, float high, AngleIntListNode *n)
	: D(low,high), next(n), flag(0) {}
};

class AngleIntList
{
    AngleIntListNode *head, *tail;
    friend class AngleIntListIterator;

    void remove(AngleIntListNode *t); 
    void add(float l, float h);

public:
    AngleIntList() : head(0), tail(0) {}

    void Clear()
    {
	while (head)
	{
	    AngleIntListNode *temp = head;
	    head = head->next;
	    delete temp;
	}
	head = tail = 0;
    }

    ~AngleIntList() { Clear(); }

    void AddList(AngleIntList &dest, float eps = AINT_BIG_EPSILON) const;

    void Copy(AngleIntList &dest) const;

    void Add(float l, float h, float eps = AINT_BIG_EPSILON);

    void Map(void (*f)(AngleInt &a, void *), void *data = 0) const
    {
	for (AngleIntListNode *t = head; t; t = t->next)
	    f(t->D, data);
    }

    int IsEmpty() const { return !head; }

    AngleInt * Largest() const;

	
    // returns T if a is in the angle range of any of the entries
    int InRange(float a, float eps = AINT_BIG_EPSILON) const
    {
	for (AngleIntListNode *t = head; t; t = t->next)
	    if (t->D.InRange(a, eps))
		return 1;
	return 0;
    }

    float Distance(float a) const;
    
    void wrap(float eps = AINT_BIG_EPSILON);

    int NumIntervals() const;
};

void Union(const AngleIntList &a,
	   const AngleIntList &b,
	   AngleIntList &c);

void Intersect(const AngleIntList &a,
	       const AngleIntList &b,
	       AngleIntList &c);


//
// Corresponding iterator
//
class AngleIntListIterator
{
    AngleIntListNode *a;

public:
    AngleIntListIterator() { a = 0; } 

    void Start(const AngleIntList &A)
    { a = A.head; } 

    AngleIntListIterator(const AngleIntList &A)
    { Start(A); }

    AngleInt *Next()
    {
	AngleIntListNode *t = a;
	if (a)
	    a = a->next;
	return t ? &t->D : 0;
    }
};



#endif
