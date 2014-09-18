#ifndef GFXVEC2_INCLUDED // -*- C++ -*-
#define GFXVEC2_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  2D Vector class

  $Id: vec2.h,v 1.19 2002/09/20 16:02:25 garland Exp $

 ************************************************************************/

//#include "gfx.h"

template<class T>
class TVec2 {
private:
    T elt[2];

public:
    // Standard constructors
    //
    TVec2(T s=0) { *this = s; }
    TVec2(T x, T y) { elt[0]=x; elt[1]=y; }

    // Copy constructors & assignment operators
    template<class U> TVec2(const TVec2<U>& v) { *this = v; }
    template<class U> TVec2(const U v[2]) { elt[0]=v[0]; elt[1]=v[1]; }
    template<class U> TVec2& operator=(const TVec2<U>& v)
	{ elt[0]=v[0];  elt[1]=v[1];  return *this; }
    TVec2& operator=(T s) { elt[0]=elt[1]=s; return *this; }


    // Descriptive interface
    //
    typedef T value_type;
    static int dim() { return 2; }


    // Access methods
    //
    operator       T*()       { return elt; }
    operator const T*() const { return elt; }

    // In-place arithmetic methods
    //
    inline TVec2& operator+=(const TVec2& v);
    inline TVec2& operator-=(const TVec2& v);
    inline TVec2& operator*=(T s);
    inline TVec2& operator/=(T s);
};

////////////////////////////////////////////////////////////////////////
//
// Method definitions
//
template<class T> inline TVec2<T>& TVec2<T>::operator+=(const TVec2<T>& v)
	{ elt[0] += v[0];   elt[1] += v[1];   return *this; }

template<class T> inline TVec2<T>& TVec2<T>::operator-=(const TVec2<T>& v)
	{ elt[0] -= v[0];   elt[1] -= v[1];   return *this; }

template<class T> inline TVec2<T>& TVec2<T>::operator*=(T s)
	{ elt[0] *= s;   elt[1] *= s;   return *this; }

template<class T> inline TVec2<T>& TVec2<T>::operator/=(T s)
	{ elt[0] /= s;   elt[1] /= s;   return *this; }

////////////////////////////////////////////////////////////////////////
//
// Operator defintions
//

template<class T>
inline TVec2<T> operator+(const TVec2<T> &u, const TVec2<T> &v)
	{ return TVec2<T>(u[0]+v[0], u[1]+v[1]); }

template<class T>
inline TVec2<T> operator-(const TVec2<T> &u, const TVec2<T> &v)
	{ return TVec2<T>(u[0]-v[0], u[1]-v[1]); }

template<class T> inline TVec2<T> operator-(const TVec2<T> &v)
	{ return TVec2<T>(-v[0], -v[1]); }

#if _MSC_VER>=1200
// Normally, we use the <class T, class N> construct below to allow the scalar
// argument to be different than the template type.  This, for example, allows
// the user to write things like v/2.  Unfortunately, Microsoft VC6.0 (aka
// v1200) gets confused by this.  We used to include explicit versions for the
// case of int's, but this was causing silent (and incorrect) coercion of
// floats to ints.
//

  template<class T> inline TVec2<T> operator*(T s, const TVec2<T> &v)
	{ return TVec2<T>(v[0]*s, v[1]*s); }
  template<class T> inline TVec2<T> operator*(const TVec2<T> &v, T s)
	{ return s*v; }

  template<class T> inline TVec2<T> operator/(const TVec2<T> &v, T s)
	{ return TVec2<T>(v[0]/s, v[1]/s); }

#else
  template<class T, class N> inline TVec2<T> operator*(N s, const TVec2<T> &v)
	{ return TVec2<T>(v[0]*s, v[1]*s); }
  template<class T, class N> inline TVec2<T> operator*(const TVec2<T> &v, N s)
	{ return s*v; }

  template<class T, class N> inline TVec2<T> operator/(const TVec2<T> &v, N s)
	{ return TVec2<T>(v[0]/s, v[1]/s); }
#endif

template<class T> inline T operator*(const TVec2<T> &u, const TVec2<T>& v)
	{ return u[0]*v[0] + u[1]*v[1]; }

template<class T> inline TVec2<T> perp(const TVec2<T> &v)
	{ return TVec2<T>(v[1], -v[0]); }
/*
template<class T>
inline std::ostream &operator<<(std::ostream &out, const TVec2<T> &v)
	{ return out << v[0] << " " << v[1]; }

template<class T>
inline std::istream &operator>>(std::istream &in, TVec2<T>& v)
	{ return in >> v[0] >> v[1]; }
*/

////////////////////////////////////////////////////////////////////////
//
// Misc. function definitions
//

template<class T> inline T norm2(const TVec2<T>& v)  { return v*v; }
template<class T> inline T norm(const TVec2<T>& v)   { return _sqrt(norm2(v)); }

template<class T> inline void unitize(TVec2<T>& v)
{
    T l = norm2(v);
    if( l!=1.0 && l!=0.0 )  v /= _sqrt(l);
}

typedef TVec2<double> Vec2;
typedef TVec2<float>  Vec2f;

// GFXVEC2_INCLUDED
#endif
