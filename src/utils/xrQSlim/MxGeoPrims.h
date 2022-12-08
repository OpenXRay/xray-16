#ifndef MXGEOPRIMS_INCLUDED // -*- C++ -*-
#define MXGEOPRIMS_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  MxGeoPrims

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxGeoPrims.h,v 1.26.2.1 2004/07/01 18:56:32 garland Exp $

 ************************************************************************/

//
// Standard names for ID types
//
typedef unsigned int MxVertexID;
typedef unsigned int MxFaceID;

//
// Convenient nicknames
//
#define VID MxVertexID
#define FID MxFaceID

#if !defined(MX_COLOR_RGBA) && !defined(MX_COLOR_ABGR)
#define MX_COLOR_RGBA
#endif

#if !defined(HAVE_RINT)
inline double rint(double x) { return floor(x + 0.5); }
#endif

class MxColor
{
private:
    inline unsigned char _ftop(float x) { return (unsigned char)((x > 1.0f ? 1.0f : x) * 255.0f); }
    inline float _ptof(unsigned char c) const { return ((float)c) / 255.0f; }
public:
    union
    {
#if defined(MX_COLOR_RGBA)
        struct
        {
            unsigned char r, g, b, a;
        } chan;
#elif defined(MX_COLOR_ABGR)
        struct
        {
            unsigned char a, b, g, r;
        } chan;
#else
#error "Packed color format illegal or left unspecified"
#endif
        unsigned int word;
    } as;

    MxColor() {}
    MxColor(float r, float g, float b)
    {
        as.chan.a = 0;
        set(r, g, b);
    }

    void set(float r, float g, float b)
    {
        as.chan.r = _ftop(r);
        as.chan.g = _ftop(g);
        as.chan.b = _ftop(b);
    }
    void set(const float* c)
    {
        as.chan.r = _ftop(c[0]);
        as.chan.g = _ftop(c[1]);
        as.chan.b = _ftop(c[2]);
    }
    void set(const double* c)
    {
        as.chan.r = _ftop((float)c[0]);
        as.chan.g = _ftop((float)c[1]);
        as.chan.b = _ftop((float)c[2]);
    }

    float R() const { return _ptof(as.chan.r); }
    float G() const { return _ptof(as.chan.g); }
    float B() const { return _ptof(as.chan.b); }
};

class MxTexCoord
{
public:
    float u[2];

    MxTexCoord() {}
    MxTexCoord(float s, float t)
    {
        u[0] = s;
        u[1] = t;
    }
    MxTexCoord(const MxTexCoord& t) { *this = t; }
    float& operator[](int i) { return u[i]; }
    float operator[](int i) const { return u[i]; }
    operator const float*() const { return u; }
    operator const float*() { return u; }
    operator float*() { return u; }
    MxTexCoord& operator=(const MxTexCoord& t)
    {
        u[0] = t[0];
        u[1] = t[1];
        return *this;
    }
};

class MxVertex
{
public:
    union
    {
        float pos[3];
        struct
        {
            MxVertexID parent, next, prev;
        } proxy;
    } as;

    MxVertex() {}
    MxVertex(float x, float y, float z)
    {
        as.pos[0] = x;
        as.pos[1] = y;
        as.pos[2] = z;
    }
    MxVertex(const MxVertex& v) { *this = v; }
    MxVertex& operator=(const MxVertex& v)
    {
        as.pos[0] = v.as.pos[0];
        as.pos[1] = v.as.pos[1];
        as.pos[2] = v.as.pos[2];
        return *this;
    }
    operator const float*() const { return as.pos; }
    operator const float*() { return as.pos; }
    operator float*() { return as.pos; }
    float& operator()(int i) { return as.pos[i]; }
    float operator()(int i) const { return as.pos[i]; }
    //
    // The [] operator is preferred over ()
    //
    float& operator[](int i) { return as.pos[i]; }
    float operator[](int i) const { return as.pos[i]; }
};

class MxNormal
{
private:
    inline short _ftos(float x) { return (short)rint((x > 1.0f ? 1.0f : x) * (float)SHRT_MAX); }
    inline short _dtos(double x) { return (short)rint((x > 1.0 ? 1.0 : x) * (double)SHRT_MAX); }
    inline float _stof(short s) const { return (float)s / (float)SHRT_MAX; }
    inline double _stod(short s) const { return (double)s / (double)SHRT_MAX; }
    short dir[3];

public:
    MxNormal() {}
    MxNormal(float x, float y, float z) { set(x, y, z); }
    MxNormal(const float* v) { set(v); }
    MxNormal(const double* v) { set(v); }
    inline void set(double x, double y, double z)
    {
        dir[0] = _dtos(x);
        dir[1] = _dtos(y);
        dir[2] = _dtos(z);
    }
    inline void set(const float* v)
    {
        dir[0] = _ftos(v[0]);
        dir[1] = _ftos(v[1]);
        dir[2] = _ftos(v[2]);
    }
    inline void set(const double* v)
    {
        dir[0] = _dtos(v[0]);
        dir[1] = _dtos(v[1]);
        dir[2] = _dtos(v[2]);
    }

    float operator[](unsigned int i) const
    {
        VERIFY(i < 3);
        return _stof(dir[i]);
    }
    short raw(unsigned int i) const { return dir[i]; }
    const short* raw() const { return dir; }
};

class MxEdge
{
public:
    MxVertexID v1, v2;

    MxEdge() { v1 = v2 = 0xFFFFFFFFU; }
    MxEdge(MxVertexID a, MxVertexID b)
    {
        v1 = a;
        v2 = b;
    }
    MxEdge(const MxEdge& e)
    {
        v1 = e.v1;
        v2 = e.v2;
    }

    MxVertexID opposite_vertex(MxVertexID v)
    {
        if (v == v1)
            return v2;
        else
        {
            VERIFY(v == v2);
            return v1;
        }
    }
};

class MxFace
{
public:
    MxVertexID v[3];

    MxFace() {}
    MxFace(MxVertexID v0, MxVertexID v1, MxVertexID v2)
    {
        v[0] = v0;
        v[1] = v1;
        v[2] = v2;
    }
    MxFace(const MxFace& f)
    {
        v[0] = f.v[0];
        v[1] = f.v[1];
        v[2] = f.v[2];
    }

    MxVertexID& operator()(int i) { return v[i]; }
    MxVertexID operator()(int i) const { return v[i]; }
    //
    // The [] operator is now preferred over the () operator.
    //
    MxVertexID& operator[](int i) { return v[i]; }
    MxVertexID operator[](int i) const { return v[i]; }
    int remap_vertex(MxVertexID from, MxVertexID to)
    {
        int nmapped = 0;
        for (int i = 0; i < 3; i++)
            if (v[i] == from)
            {
                v[i] = to;
                nmapped++;
            }
        return nmapped;
    }

    unsigned int find_vertex(MxVertexID i)
    {
        if (v[0] == i)
            return 0;
        else if (v[1] == i)
            return 1;
        else
        {
            VERIFY(v[2] == i);
            return 2;
        }
    }

    MxVertexID opposite_vertex(MxVertexID v0, MxVertexID v1)
    {
        if (v[0] != v0 && v[0] != v1)
            return v[0];
        else if (v[1] != v0 && v[1] != v1)
            return v[1];
        else
        {
            VERIFY(v[2] != v0 && v[2] != v1);
            return v[2];
        }
    }

    bool is_inorder(MxVertexID v0, MxVertexID v1)
    {
        if (v[0] == v0)
            return v[1] == v1;
        else if (v[1] == v0)
            return v[2] == v1;
        else
        {
            VERIFY(v[2] == v0);
            return v[0] == v1;
        }
    }
};

/*
inline ostream& operator<<(ostream& out, const MxVertex& v)
{
    return out << "v " << v(0) << " " << v(1) << " " << v(2);
}

inline ostream& operator<<(ostream& out, const MxFace& f)
{
    return out << "f " << f(0)+1 << " " <<  f(1)+1 << " " <<  f(2)+1;
}

inline ostream& operator<<(ostream& out, const MxColor& c)
{
    return out << "c " << c.R() << " " << c.G() << " " << c.B();
}

inline ostream& operator<<(ostream& out, const MxNormal& n)
{
    return out << "n " << n[0] << " " << n[1] << " " << n[2];
}

inline ostream& operator<<(ostream& out, const MxTexCoord& t)
{
    return out << "r " << t[0] << " " << t[1];
}
*/
#ifdef MXGL_INCLUDED
inline void glC(const MxColor& c) { glColor3ub(c.as.chan.r, c.as.chan.g, c.as.chan.b); }
inline void glT(const MxTexCoord& t) { glTexCoord2fv(t); }
inline void glV(const MxVertex& v) { glVertex3fv(v); }
inline void glN(const MxNormal& n) { glNormal3sv(n.raw()); }
inline void glC(const MxColor* c) { glC(*c); }
inline void glT(const MxTexCoord* t) { glT(*t); }
inline void glV(const MxVertex* v) { glV(*v); }
inline void glN(const MxNormal* n) { glN(*n); }
#endif

// MXGEOPRIMS_INCLUDED
#endif
