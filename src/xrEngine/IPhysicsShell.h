#pragma once
#include "xrCore/_types.h"
#include "xrCore/vector.h"

// fwd. decl.
template <class T> struct _vector3;
using Fvector = _vector3<float>;
template <class T> struct _matrix;
using Fmatrix = _matrix<float>;

class IPhysicsGeometry;
class IPhysicsElement
{
public:
    virtual const Fmatrix& XFORM() const = 0;
    virtual void get_LinearVel(Fvector& velocity) const = 0;
    virtual void get_AngularVel(Fvector& velocity) const = 0;
    virtual void get_Box(Fvector& sz, Fvector& c) const = 0;
    virtual const Fvector& mass_Center() const = 0;
    virtual u16 numberOfGeoms() const = 0;
    virtual const IPhysicsGeometry* geometry(u16 i) const = 0;
};

class IPhysicsShell
{
public:
    virtual const Fmatrix& XFORM() const = 0;
    virtual const IPhysicsElement& Element(u16 index) const = 0;
    virtual u16 get_ElementsNumber() const = 0;
};
