#pragma once
#ifndef __FBOX
#define __FBOX
#include "_vector3d.h"

template <class T>
class _box3
{
public:
    using TYPE = T;
    using Self = _box3<T>;
    using SelfRef = Self&;
    using SelfCRef = const Self&;
    using Tvector = _vector3<T>;
    using Tmatrix = _matrix<T>;

    union
    {
        struct
        {
            Tvector vMin;
            Tvector vMax;
        };
        struct
        {
            T x1, y1, z1;
            T x2, y2, z2;
        };
    };

    bool is_valid() const noexcept { return (x2 >= x1) && (y2 >= y1) && (z2 >= z1); }
    const T* data() const noexcept { return &vMin.x; }

    SelfRef set(const Tvector& _min, const Tvector& _max)
    {
        vMin.set(_min);
        vMax.set(_max);
        return *this;
    }

    SelfRef set(T x1, T y1, T z1, T x2, T y2, T z2)
    {
        vMin.set(x1, y1, z1);
        vMax.set(x2, y2, z2);
        return *this;
    }

    SelfRef set(SelfCRef b)
    {
        vMin.set(b.vMin);
        vMax.set(b.vMax);
        return *this;
    }

    SelfRef setb(const Tvector& center, const Tvector& dim)
    {
        vMin.sub(center, dim);
        vMax.add(center, dim);
        return *this;
    }

    SelfRef set_zero()
    {
        vMin.set(0, 0, 0);
        vMax.set(0, 0, 0);
        return *this;
    }

    SelfRef identity()
    {
        vMin.set(-0.5, -0.5, -0.5);
        vMax.set(0.5, 0.5, 0.5);
        return *this;
    }

    SelfRef invalidate()
    {
        vMin.set(type_max<T>, type_max<T>, type_max<T>);
        vMax.set(type_min<T>, type_min<T>, type_min<T>);
        return *this;
    }

    SelfRef shrink(T s)
    {
        vMin.add(s);
        vMax.sub(s);
        return *this;
    }

    SelfRef shrink(const Tvector& s)
    {
        vMin.add(s);
        vMax.sub(s);
        return *this;
    }

    SelfRef grow(T s)
    {
        vMin.sub(s);
        vMax.add(s);
        return *this;
    }

    SelfRef grow(const Tvector& s)
    {
        vMin.sub(s);
        vMax.add(s);
        return *this;
    }

    SelfRef add(const Tvector& p)
    {
        vMin.add(p);
        vMax.add(p);
        return *this;
    }

    SelfRef sub(const Tvector& p)
    {
        vMin.sub(p);
        vMax.sub(p);
        return *this;
    }

    SelfRef offset(const Tvector& p)
    {
        vMin.add(p);
        vMax.add(p);
        return *this;
    }

    SelfRef add(SelfCRef b, const Tvector& p)
    {
        vMin.add(b.vMin, p);
        vMax.add(b.vMax, p);
        return *this;
    }

    ICF bool contains(T x, T y, T z) const
    {
        return (x >= x1) && (x <= x2) && (y >= y1) && (y <= y2) && (z >= z1) && (z <= z2);
    }

    ICF bool contains(const Tvector& p) const { return contains(p.x, p.y, p.z); }
    ICF bool contains(SelfCRef b) const { return contains(b.vMin) && contains(b.vMax); }
    bool similar(SelfCRef b) const { return vMin.similar(b.vMin) && vMax.similar(b.vMax); }

    ICF SelfRef modify(const Tvector& p)
    {
        vMin.min(p);
        vMax.max(p);
        return *this;
    }

    ICF SelfRef modify(T x, T y, T z)
    {
        _vector3<T> tmp = {x, y, z};
        return modify(tmp);
    }

    SelfRef merge(SelfCRef b)
    {
        modify(b.vMin);
        modify(b.vMax);
        return *this;
    }

    SelfRef merge(SelfCRef b1, SelfCRef b2)
    {
        invalidate();
        merge(b1);
        merge(b2);
        return *this;
    }
    ICF SelfRef xform(SelfCRef B, const Tmatrix& m)
    {
        // The three edges transformed: you can efficiently transform an X-only vector3
        // by just getting the "X" column of the matrix
        Tvector vx, vy, vz;
        vx.mul(m.i, B.vMax.x - B.vMin.x);
        vy.mul(m.j, B.vMax.y - B.vMin.y);
        vz.mul(m.k, B.vMax.z - B.vMin.z);

        // Transform the vMin point
        m.transform_tiny(vMin, B.vMin);
        vMax.set(vMin);

        // Take the transformed vMin & axes and find _new_ extents
        // Using CPU code in the right place is faster...
        if (negative(vx.x))
            vMin.x += vx.x;
        else
            vMax.x += vx.x;
        if (negative(vx.y))
            vMin.y += vx.y;
        else
            vMax.y += vx.y;
        if (negative(vx.z))
            vMin.z += vx.z;
        else
            vMax.z += vx.z;
        if (negative(vy.x))
            vMin.x += vy.x;
        else
            vMax.x += vy.x;
        if (negative(vy.y))
            vMin.y += vy.y;
        else
            vMax.y += vy.y;
        if (negative(vy.z))
            vMin.z += vy.z;
        else
            vMax.z += vy.z;
        if (negative(vz.x))
            vMin.x += vz.x;
        else
            vMax.x += vz.x;
        if (negative(vz.y))
            vMin.y += vz.y;
        else
            vMax.y += vz.y;
        if (negative(vz.z))
            vMin.z += vz.z;
        else
            vMax.z += vz.z;
        return *this;
    }
    ICF SelfRef xform(const Tmatrix& m)
    {
        Self b;
        b.set(*this);
        return xform(b, m);
    }

    void getsize(Tvector& R) const { R.sub(vMax, vMin); }

    void getradius(Tvector& R) const
    {
        getsize(R);
        R.mul(0.5f);
    }

    T getradius() const
    {
        Tvector R;
        getradius(R);
        return R.magnitude();
    }

    T getvolume() const
    {
        Tvector sz;
        getsize(sz);
        return sz.x * sz.y * sz.z;
    }

    SelfCRef getcenter(Tvector& C) const
    {
        C.x = (vMin.x + vMax.x) * 0.5f;
        C.y = (vMin.y + vMax.y) * 0.5f;
        C.z = (vMin.z + vMax.z) * 0.5f;
        return *this;
    }

    SelfCRef get_CD(Tvector& bc, Tvector& bd) const // center + dimensions
    {
        bd.sub(vMax, vMin).mul(.5f);
        bc.add(vMin, bd);
        return *this;
    }

    SelfRef scale(float s) // 0.1 means make 110%, -0.1 means make 90%
    {
        Fvector bd;
        bd.sub(vMax, vMin).mul(s);
        grow(bd);
        return *this;
    }

    SelfCRef getsphere(Tvector& C, T& R) const
    {
        getcenter(C);
        R = C.distance_to(vMax);
        return *this;
    }

    // Detects if this box intersect other
    ICF bool intersect(SelfCRef box)
    {
        if (vMax.x < box.vMin.x)
            return false;
        if (vMax.y < box.vMin.y)
            return false;
        if (vMax.z < box.vMin.z)
            return false;
        if (vMin.x > box.vMax.x)
            return false;
        if (vMin.y > box.vMax.y)
            return false;
        if (vMin.z > box.vMax.z)
            return false;
        return true;
    }

    // Does the vector3 intersects box
    bool Pick(const Tvector& start, const Tvector& dir)
    {
        T alpha, xt, yt, zt;
        Tvector rvmin, rvmax;

        rvmin.sub(vMin, start);
        rvmax.sub(vMax, start);

        if (!fis_zero(dir.x))
        {
            alpha = rvmin.x / dir.x;
            yt = alpha * dir.y;
            if (yt >= rvmin.y && yt <= rvmax.y)
            {
                zt = alpha * dir.z;
                if (zt >= rvmin.z && zt <= rvmax.z)
                    return true;
            }
            alpha = rvmax.x / dir.x;
            yt = alpha * dir.y;
            if (yt >= rvmin.y && yt <= rvmax.y)
            {
                zt = alpha * dir.z;
                if (zt >= rvmin.z && zt <= rvmax.z)
                    return true;
            }
        }

        if (!fis_zero(dir.y))
        {
            alpha = rvmin.y / dir.y;
            xt = alpha * dir.x;
            if (xt >= rvmin.x && xt <= rvmax.x)
            {
                zt = alpha * dir.z;
                if (zt >= rvmin.z && zt <= rvmax.z)
                    return true;
            }
            alpha = rvmax.y / dir.y;
            xt = alpha * dir.x;
            if (xt >= rvmin.x && xt <= rvmax.x)
            {
                zt = alpha * dir.z;
                if (zt >= rvmin.z && zt <= rvmax.z)
                    return true;
            }
        }

        if (!fis_zero(dir.z))
        {
            alpha = rvmin.z / dir.z;
            xt = alpha * dir.x;
            if (xt >= rvmin.x && xt <= rvmax.x)
            {
                yt = alpha * dir.y;
                if (yt >= rvmin.y && yt <= rvmax.y)
                    return true;
            }
            alpha = rvmax.z / dir.z;
            xt = alpha * dir.x;
            if (xt >= rvmin.x && xt <= rvmax.x)
            {
                yt = alpha * dir.y;
                if (yt >= rvmin.y && yt <= rvmax.y)
                    return true;
            }
        }
        return false;
    };

    u32& IntRref(T& x) { return (u32&)x; }

    enum ERP_Result
    {
        rpNone = 0,
        rpOriginInside = 1,
        rpOriginOutside = 2,
        fcv_forcedword = u32(-1)
    };

    ERP_Result Pick2(const Tvector& origin, const Tvector& dir, Tvector& coord)
    {
        bool Inside = true;
        Tvector MaxT;
        MaxT.x = MaxT.y = MaxT.z = -1.0f;

        // Find candidate planes.
        {
            if (origin[0] < vMin[0])
            {
                coord[0] = vMin[0];
                Inside = false;
                if (IntRref(dir[0]))
                    MaxT[0] = (vMin[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
            }
            else if (origin[0] > vMax[0])
            {
                coord[0] = vMax[0];
                Inside = false;
                if (IntRref(dir[0]))
                    MaxT[0] = (vMax[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
            }
        }
        {
            if (origin[1] < vMin[1])
            {
                coord[1] = vMin[1];
                Inside = false;
                if (IntRref(dir[1]))
                    MaxT[1] = (vMin[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
            }
            else if (origin[1] > vMax[1])
            {
                coord[1] = vMax[1];
                Inside = false;
                if (IntRref(dir[1]))
                    MaxT[1] = (vMax[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
            }
        }
        {
            if (origin[2] < vMin[2])
            {
                coord[2] = vMin[2];
                Inside = false;
                if (IntRref(dir[2]))
                    MaxT[2] = (vMin[2] - origin[2]) / dir[2]; // Calculate T distances to candidate planes
            }
            else if (origin[2] > vMax[2])
            {
                coord[2] = vMax[2];
                Inside = false;
                if (IntRref(dir[2]))
                    MaxT[2] = (vMax[2] - origin[2]) / dir[2]; // Calculate T distances to candidate planes
            }
        }

        // Ray origin inside bounding box
        if (Inside)
        {
            coord = origin;
            return rpOriginInside;
        }

        // Get largest of the maxT's for final choice of intersection
        u32 WhichPlane = 0;
        if (MaxT[1] > MaxT[0])
            WhichPlane = 1;
        if (MaxT[2] > MaxT[WhichPlane])
            WhichPlane = 2;

        // Check final candidate actually inside box
        if (IntRref(MaxT[WhichPlane]) & 0x80000000) return rpNone;

        if (0 == WhichPlane)
        {
            // 1 & 2
            coord[1] = origin[1] + MaxT[0] * dir[1];

            if ((coord[1] < vMin[1]) || (coord[1] > vMax[1])) return rpNone;

            coord[2] = origin[2] + MaxT[0] * dir[2];

            if ((coord[2] < vMin[2]) || (coord[2] > vMax[2])) return rpNone;

            return rpOriginOutside;
        }
        if (1 == WhichPlane)
        {
            // 0 & 2
            coord[0] = origin[0] + MaxT[1] * dir[0];

            if ((coord[0] < vMin[0]) || (coord[0] > vMax[0])) return rpNone;

            coord[2] = origin[2] + MaxT[1] * dir[2];

            if ((coord[2] < vMin[2]) || (coord[2] > vMax[2])) return rpNone;

            return rpOriginOutside;
        }
        if (2 == WhichPlane)
        {
            // 0 & 1
            coord[0] = origin[0] + MaxT[2] * dir[0];

            if ((coord[0] < vMin[0]) || (coord[0] > vMax[0])) return rpNone;

            coord[1] = origin[1] + MaxT[2] * dir[1];

            if ((coord[1] < vMin[1]) || (coord[1] > vMax[1])) return rpNone;

            return rpOriginOutside;
        }
        return rpNone;
    }

    void getpoint(int index, Tvector& result) const
    {
        switch (index)
        {
        case 0: result.set(vMin.x, vMin.y, vMin.z); break;
        case 1: result.set(vMin.x, vMin.y, vMax.z); break;
        case 2: result.set(vMax.x, vMin.y, vMax.z); break;
        case 3: result.set(vMax.x, vMin.y, vMin.z); break;
        case 4: result.set(vMin.x, vMax.y, vMin.z); break;
        case 5: result.set(vMin.x, vMax.y, vMax.z); break;
        case 6: result.set(vMax.x, vMax.y, vMax.z); break;
        case 7: result.set(vMax.x, vMax.y, vMin.z); break;
        default: result.set(0, 0, 0); break;
        }
    }

    void getpoints(Tvector* result)
    {
        result[0].set(vMin.x, vMin.y, vMin.z);
        result[1].set(vMin.x, vMin.y, vMax.z);
        result[2].set(vMax.x, vMin.y, vMax.z);
        result[3].set(vMax.x, vMin.y, vMin.z);
        result[4].set(vMin.x, vMax.y, vMin.z);
        result[5].set(vMin.x, vMax.y, vMax.z);
        result[6].set(vMax.x, vMax.y, vMax.z);
        result[7].set(vMax.x, vMax.y, vMin.z);
    }

    SelfRef modify(SelfCRef src, const Tmatrix& M)
    {
        Tvector pt;
        for (int i = 0; i < 8; i++)
        {
            src.getpoint(i, pt);
            M.transform_tiny(pt);
            modify(pt);
        }
        return *this;
    }
};

using Fbox = _box3<float>;
using Fbox3 = _box3<float>;

using Dbox = _box3<double>;
using Dbox3 = _box3<double>;

template <class T>
bool _valid(const _box3<T>& c)
{
    return _valid(c.vMin) && _valid(c.vMax);
}

#endif
