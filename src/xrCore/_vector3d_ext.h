#ifndef VECTOR3D_EXT_INCLUDED
#define VECTOR3D_EXT_INCLUDED

#include "_vector3d.h"

inline Fvector cr_fvector3(float f)
{
    Fvector res = {f, f, f};
    return res;
}

inline Fvector cr_fvector3(float x, float y, float z)
{
    Fvector res = {x, y, z};
    return res;
}

inline Fvector cr_fvector3_hp(float h, float p)
{
    Fvector res;
    res.setHP(h, p);
    return res;
}

inline Fvector operator+(const Fvector& v1, const Fvector& v2)
{
    return cr_fvector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

inline Fvector operator-(const Fvector& v1, const Fvector& v2)
{
    return cr_fvector3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline Fvector operator-(const Fvector& v) { return cr_fvector3(-v.x, -v.y, -v.z); }
inline Fvector operator*(const Fvector& v, float f) { return cr_fvector3(v.x * f, v.y * f, v.z * f); }
inline Fvector operator*(float f, const Fvector& v) { return cr_fvector3(v.x * f, v.y * f, v.z * f); }
inline Fvector operator/(const Fvector& v, float f)
{
    const float repr_f = 1.f / f;
    return cr_fvector3(v.x * repr_f, v.y * repr_f, v.z * repr_f);
}

inline Fvector _min(const Fvector& v1, const Fvector& v2)
{
    Fvector r;
    r.min(v1, v2);
    return r;
}

inline Fvector _max(const Fvector& v1, const Fvector& v2)
{
    Fvector r;
    r.max(v1, v2);
    return r;
}

inline Fvector _abs(const Fvector& v)
{
    Fvector r;
    r.abs(v);
    return r;
}

inline Fvector normalize(const Fvector& v)
{
    Fvector r(v);
    r.normalize();
    return r;
}

inline float magnitude(const Fvector& v) { return v.magnitude(); }
inline float sqaure_magnitude(const Fvector& v) { return v.square_magnitude(); }
float dotproduct(const Fvector& v1, const Fvector& v2);
// CrossProduct
Fvector crossproduct(const Fvector& v1, const Fvector& v2);

Fvector cr_vectorHP(float h, float p);

float angle_between_vectors(Fvector const& v1, Fvector const& v2);

Fvector rotate_point(Fvector const& point, float const angle);

#endif // VECTOR3D_EXT_INCLUDED
