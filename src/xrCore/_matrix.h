#pragma once
#ifndef __M__
#define __M__
#include "_vector2.h"
#include "_vector3d.h"
#include "_vector4.h"
#include "_std_extensions.h" // _valid<float>
/*
* DirectX-compliant, ie row-column order, ie m[Row][Col].
* Same as:
* m11 m12 m13 m14 first row.
* m21 m22 m23 m24 second row.
* m31 m32 m33 m34 third row.
* m41 m42 m43 m44 fourth row.
* Translation is (m41, m42, m43), (m14, m24, m34, m44) = (0, 0, 0, 1).
* Stored in memory as m11 m12 m13 m14 m21...
*
* Multiplication rules:
*
* [x'y'z'1] = [xyz1][M]
*
* x' = x*m11 + y*m21 + z*m31 + m41
* y' = x*m12 + y*m22 + z*m32 + m42
* z' = x*m13 + y*m23 + z*m33 + m43
* 1' = 0 + 0 + 0 + m44
*/

// NOTE_1: positive angle means clockwise rotation
// NOTE_2: mul(A,B) means transformation B, followed by A
// NOTE_3: I,J,K,C equals to R,N,D,T
// NOTE_4: The rotation sequence is ZXY

struct Fquaternion;

struct Fmatrix
{
    using TYPE     = float;
    using Self     = Fmatrix;
    using SelfRef  = Self&;
    using SelfCRef = const Self&;

    union
    {
        struct // Direct definition
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };

        struct
        {
            Fvector i;
            float _14_;
            Fvector j;
            float _24_;
            Fvector k;
            float _34_;
            Fvector c;
            float _44_;
        };
        float m[4][4]; // Array
    };

    // Class members
    ICF SelfRef set(const Self& a)
    {
        i.set(a.i);
        _14_ = a._14;
        j.set(a.j);
        _24_ = a._24;
        k.set(a.k);
        _34_ = a._34;
        c.set(a.c);
        _44_ = a._44;
        return *this;
    }

    ICF SelfRef set(const Fvector& R, const Fvector& N, const Fvector& D, const Fvector& C)
    {
        i.set(R);
        _14_ = 0;
        j.set(N);
        _24_ = 0;
        k.set(D);
        _34_ = 0;
        c.set(C);
        _44_ = 1;
        return *this;
    }

    SelfRef identity();
    SelfRef rotation(const Fquaternion& Q);
    SelfRef mk_xform(const Fquaternion& Q, const Fvector& V);

    // Multiply RES = A[4x4]*B[4x4] (WITH projection)
    SelfRef mul(const Self& A, const Self& B);

    // Multiply RES = A[4x3]*B[4x3] (no projection), faster than ordinary multiply
    SelfRef mul_43(const Self& A, const Self& B);

    IC SelfRef mulA_44(const Self& A) // mul after
    {
        Self B;
        B.set(*this);
        mul(A, B);
        return *this;
    };

    IC SelfRef mulB_44(const Self& B) // mul before
    {
        Self A;
        A.set(*this);
        mul(A, B);
        return *this;
    };
    ICF SelfRef mulA_43(const Self& A) // mul after (no projection)
    {
        Self B;
        B.set(*this);
        mul_43(A, B);
        return *this;
    };
    ICF SelfRef mulB_43(const Self& B) // mul before (no projection)
    {
        Self A;
        A.set(*this);
        mul_43(A, B);
        return *this;
    };

    SelfRef invert(const Self& a); // important: this is 4x3 invert, not the 4x4 one
    bool invert_b(const Self& a); // important: this is 4x3 invert, not the 4x4 one
    SelfRef invert_44(const Self& a);

    IC SelfRef invert() // slower than invert other matrix
    {
        Self a;
        a.set(*this);
        invert(a);
        return *this;
    }

    SelfRef transpose(const Self& matSource); // faster version of transpose

    IC SelfRef transpose() // self transpose - slower
    {
        Self a;
        a.set(*this);
        transpose(a);
        return *this;
    }

    IC SelfRef translate(const Fvector& Loc) // setup translation matrix
    {
        identity();
        c.set(Loc.x, Loc.y, Loc.z);
        return *this;
    }

    IC SelfRef translate(float _x, float _y, float _z) // setup translation matrix
    {
        identity();
        c.set(_x, _y, _z);
        return *this;
    }

    IC SelfRef translate_over(const Fvector& Loc) // modify only translation
    {
        c.set(Loc.x, Loc.y, Loc.z);
        return *this;
    }

    IC SelfRef translate_over(float _x, float _y, float _z) // modify only translation
    {
        c.set(_x, _y, _z);
        return *this;
    }

    IC SelfRef translate_add(const Fvector& Loc) // combine translation
    {
        c.add(Loc);
        return *this;
    }

    IC SelfRef scale(float x, float y, float z) // setup scale matrix
    {
        identity();
        m[0][0] = x;
        m[1][1] = y;
        m[2][2] = z;
        return *this;
    }

    IC SelfRef scale(const Fvector& v) // setup scale matrix
    {
        return scale(v.x, v.y, v.z);
    }

    SelfRef rotateX(float Angle); // rotation about X axis
    SelfRef rotateY(float Angle); // rotation about Y axis
    SelfRef rotateZ(float Angle); // rotation about Z axis

    SelfRef rotation(const Fvector& vdir, const Fvector& vnorm);

    SelfRef mapXYZ();
    SelfRef mapXZY();
    SelfRef mapYXZ();
    SelfRef mapYZX();
    SelfRef mapZXY();
    SelfRef mapZYX();

    SelfRef rotation(const Fvector& axis, float Angle);

    // mirror X
    IC SelfRef mirrorX()
    {
        identity();
        m[0][0] = -1;
        return *this;
    }

    IC SelfRef mirrorX_over()
    {
        m[0][0] = -1;
        return *this;
    }

    IC SelfRef mirrorX_add()
    {
        m[0][0] *= -1;
        return *this;
    }

    // mirror Y
    IC SelfRef mirrorY()
    {
        identity();
        m[1][1] = -1;
        return *this;
    }

    IC SelfRef mirrorY_over()
    {
        m[1][1] = -1;
        return *this;
    }

    IC SelfRef mirrorY_add()
    {
        m[1][1] *= -1;
        return *this;
    }

    // mirror Z
    IC SelfRef mirrorZ()
    {
        identity();
        m[2][2] = -1;
        return *this;
    }

    IC SelfRef mirrorZ_over()
    {
        m[2][2] = -1;
        return *this;
    }

    IC SelfRef mirrorZ_add()
    {
        m[2][2] *= -1;
        return *this;
    }

    SelfRef mul(const Self& A, float v);
    SelfRef mul(float v);
    SelfRef div(const Self& A, float v);
    SelfRef div(float v);

    // fov
    IC SelfRef build_projection(float fFOV, float fAspect, float fNearPlane, float fFarPlane)
    {
        return build_projection_HAT(tanf(fFOV / 2.f), fAspect, fNearPlane, fFarPlane);
    }

    // half_fov-angle-tangent
    IC SelfRef build_projection_HAT(float HAT, float fAspect, float fNearPlane, float fFarPlane)
    {
        VERIFY(_abs(fFarPlane - fNearPlane) > EPS_S);
        VERIFY(_abs(HAT) > EPS_S);

        float cot = 1.0f / HAT;
        float w = fAspect * cot;
        float h = 1.0f * cot;
        float Q = fFarPlane / (fFarPlane - fNearPlane);

        _11 = w;
        _12 = 0;
        _13 = 0;
        _14 = 0;
        _21 = 0;
        _22 = h;
        _23 = 0;
        _24 = 0;
        _31 = 0;
        _32 = 0;
        _33 = Q;
        _34 = 1.0f;
        _41 = 0;
        _42 = 0;
        _43 = -Q * fNearPlane;
        _44 = 0;
        return *this;
    }

    IC SelfRef build_projection_ortho(float w, float h, float zn, float zf)
    {
        _11 = 2.0f / w;
        _12 = 0;
        _13 = 0;
        _14 = 0;
        _21 = 0;
        _22 = 2.0f / h;
        _23 = 0;
        _24 = 0;
        _31 = 0;
        _32 = 0;
        _33 = 1.0f / (zf - zn);
        _34 = 0;
        _41 = 0;
        _42 = 0;
        _43 = zn / (zn - zf);
        _44 = 1.0f;
        return *this;
    }

    IC SelfRef build_camera(const Fvector& vFrom, const Fvector& vAt, const Fvector& vWorldUp)
    {
        // Get the z basis vector3, which points straight ahead. This is the
        // difference from the eyepoint to the lookat point.
        Fvector vView;
        vView.sub(vAt, vFrom).normalize();

        // Get the dot product, and calculate the projection of the z basis
        // vector3 onto the up vector3. The projection is the y basis vector3.
        float fDotProduct = vWorldUp.dotproduct(vView);

        Fvector vUp;
        vUp.mul(vView, -fDotProduct).add(vWorldUp).normalize();

        // The x basis vector3 is found simply with the cross product of the y
        // and z basis vectors
        Fvector vRight;
        vRight.crossproduct(vUp, vView);

        // Start building the Device.mView. The first three rows contains the basis
        // vectors used to rotate the view to point at the lookat point
        _11 = vRight.x;
        _12 = vUp.x;
        _13 = vView.x;
        _14 = 0.0f;
        _21 = vRight.y;
        _22 = vUp.y;
        _23 = vView.y;
        _24 = 0.0f;
        _31 = vRight.z;
        _32 = vUp.z;
        _33 = vView.z;
        _34 = 0.0f;

        // Do the translation values (rotations are still about the eyepoint)
        _41 = -vFrom.dotproduct(vRight);
        _42 = -vFrom.dotproduct(vUp);
        _43 = -vFrom.dotproduct(vView);
        _44 = 1.0f;
        return *this;
    }

    IC SelfRef build_camera_dir(const Fvector& vFrom, const Fvector& vView, const Fvector& vWorldUp)
    {
        // Get the dot product, and calculate the projection of the z basis
        // vector3 onto the up vector3. The projection is the y basis vector3.
        float fDotProduct = vWorldUp.dotproduct(vView);

        Fvector vUp;
        vUp.mul(vView, -fDotProduct).add(vWorldUp).normalize();

        // The x basis vector3 is found simply with the cross product of the y
        // and z basis vectors
        Fvector vRight;
        vRight.crossproduct(vUp, vView);

        // Start building the Device.mView. The first three rows contains the basis
        // vectors used to rotate the view to point at the lookat point
        _11 = vRight.x;
        _12 = vUp.x;
        _13 = vView.x;
        _14 = 0.0f;
        _21 = vRight.y;
        _22 = vUp.y;
        _23 = vView.y;
        _24 = 0.0f;
        _31 = vRight.z;
        _32 = vUp.z;
        _33 = vView.z;
        _34 = 0.0f;

        // Do the translation values (rotations are still about the eyepoint)
        _41 = -vFrom.dotproduct(vRight);
        _42 = -vFrom.dotproduct(vUp);
        _43 = -vFrom.dotproduct(vView);
        _44 = 1.0f;
        return *this;
    }

    IC SelfRef inertion(const Self& mat, float v)
    {
        float iv = 1.f - v;
        for (int i = 0; i < 4; i++)
        {
            m[i][0] = m[i][0] * v + mat.m[i][0] * iv;
            m[i][1] = m[i][1] * v + mat.m[i][1] * iv;
            m[i][2] = m[i][2] * v + mat.m[i][2] * iv;
            m[i][3] = m[i][3] * v + mat.m[i][3] * iv;
        }
        return *this;
    }

    ICF void transform_tiny(Fvector& dest, const Fvector& v) const // preferred to use
    {
        dest.x = v.x * _11 + v.y * _21 + v.z * _31 + _41;
        dest.y = v.x * _12 + v.y * _22 + v.z * _32 + _42;
        dest.z = v.x * _13 + v.y * _23 + v.z * _33 + _43;
    }

    ICF void transform_tiny32(Fvector2& dest, const Fvector& v) const // preferred to use
    {
        dest.x = v.x * _11 + v.y * _21 + v.z * _31 + _41;
        dest.y = v.x * _12 + v.y * _22 + v.z * _32 + _42;
    }

    ICF void transform_tiny23(Fvector& dest, const Fvector2& v) const // preferred to use
    {
        dest.x = v.x * _11 + v.y * _21 + _41;
        dest.y = v.x * _12 + v.y * _22 + _42;
        dest.z = v.x * _13 + v.y * _23 + _43;
    }

    ICF void transform_dir(Fvector& dest, const Fvector& v) const // preferred to use
    {
        dest.x = v.x * _11 + v.y * _21 + v.z * _31;
        dest.y = v.x * _12 + v.y * _22 + v.z * _32;
        dest.z = v.x * _13 + v.y * _23 + v.z * _33;
    }

    IC void transform(Fvector4& dest, const Fvector& v) const // preferred to use
    {
        dest.w = v.x * _14 + v.y * _24 + v.z * _34 + _44;
        dest.x = (v.x * _11 + v.y * _21 + v.z * _31 + _41) / dest.w;
        dest.y = (v.x * _12 + v.y * _22 + v.z * _32 + _42) / dest.w;
        dest.z = (v.x * _13 + v.y * _23 + v.z * _33 + _43) / dest.w;
    }

    IC void transform(Fvector& dest, const Fvector& v) const // preferred to use
    {
        float iw = 1.f / (v.x * _14 + v.y * _24 + v.z * _34 + _44);
        dest.x = (v.x * _11 + v.y * _21 + v.z * _31 + _41) * iw;
        dest.y = (v.x * _12 + v.y * _22 + v.z * _32 + _42) * iw;
        dest.z = (v.x * _13 + v.y * _23 + v.z * _33 + _43) * iw;
    }

    IC void transform(Fvector4& dest, const Fvector4& v) const // preferred to use
    {
        dest.w = v.x * _14 + v.y * _24 + v.z * _34 + v.w * _44;
        dest.x = v.x * _11 + v.y * _21 + v.z * _31 + v.w * _41;
        dest.y = v.x * _12 + v.y * _22 + v.z * _32 + v.w * _42;
        dest.z = v.x * _13 + v.y * _23 + v.z * _33 + v.w * _43;
    }

    ICF void transform_tiny(Fvector& v) const
    {
        Fvector res;
        transform_tiny(res, v);
        v.set(res);
    }

    IC void transform(Fvector& v) const
    {
        Fvector res;
        transform(res, v);
        v.set(res);
    }
    ICF void transform_dir(Fvector& v) const
    {
        Fvector res;
        transform_dir(res, v);
        v.set(res);
    }

    SelfRef setHPB(float h, float p, float b);
    IC SelfRef setXYZ(float x, float y, float z) { return setHPB(y, x, z); }
    IC SelfRef setXYZ(Fvector const& xyz) { return setHPB(xyz.y, xyz.x, xyz.z); }
    IC SelfRef setXYZi(float x, float y, float z) { return setHPB(-y, -x, -z); }
    IC SelfRef setXYZi(Fvector const& xyz) { return setHPB(-xyz.y, -xyz.x, -xyz.z); }
    //
    void getHPB(float& h, float& p, float& b) const;
    IC void getHPB(Fvector& hpb) const { getHPB(hpb.x, hpb.y, hpb.z); }
    IC void getXYZ(float& x, float& y, float& z) const { getHPB(y, x, z); }
    IC void getXYZ(Fvector& xyz) const { getXYZ(xyz.x, xyz.y, xyz.z); }

    IC void getXYZi(float& x, float& y, float& z) const
    {
        getHPB(y, x, z);
        x *= -1.f;
        y *= -1.f;
        z *= -1.f;
    }

    IC void getXYZi(Fvector& xyz) const
    {
        getXYZ(xyz.x, xyz.y, xyz.z);
        xyz.mul(-1.f);
    }
};

inline bool _valid(const Fmatrix& m)
{
    return _valid(m.i) && _valid(m._14_) && _valid(m.j) && _valid(m._24_)
        && _valid(m.k) && _valid(m._34_) && _valid(m.c) && _valid(m._44_);
}

extern XRCORE_API Fmatrix Fidentity;

#endif
