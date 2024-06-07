#pragma once

#include "_vector3d.h"

struct Fbox3
{
public:
    union
    {
        struct
        {
            Fvector3 vMin;
            Fvector3 vMax;
        };
        struct
        {
            float x1, y1, z1;
            float x2, y2, z2;
        };
    };

    bool is_valid() const noexcept { return (x2 >= x1) && (y2 >= y1) && (z2 >= z1); }
    const float* data() const noexcept { return &vMin.x; }

    auto& set(const Fvector3& _min, const Fvector3& _max)
    {
        vMin.set(_min);
        vMax.set(_max);
        return *this;
    }

    auto& set(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        vMin.set(x1, y1, z1);
        vMax.set(x2, y2, z2);
        return *this;
    }

    auto& set(const Fbox3& b)
    {
        vMin.set(b.vMin);
        vMax.set(b.vMax);
        return *this;
    }

    auto& setb(const Fvector3& center, const Fvector3& dim)
    {
        vMin.sub(center, dim);
        vMax.add(center, dim);
        return *this;
    }

    auto& set_zero()
    {
        vMin.set(0, 0, 0);
        vMax.set(0, 0, 0);
        return *this;
    }

    auto& identity()
    {
        vMin.set(-0.5, -0.5, -0.5);
        vMax.set(0.5, 0.5, 0.5);
        return *this;
    }

    auto& invalidate()
    {
        vMin.set(type_max<float>, type_max<float>, type_max<float>);
        vMax.set(type_min<float>, type_min<float>, type_min<float>);
        return *this;
    }

    auto& shrink(float s)
    {
        vMin.add(s);
        vMax.sub(s);
        return *this;
    }

    auto& shrink(const Fvector3& s)
    {
        vMin.add(s);
        vMax.sub(s);
        return *this;
    }

    auto& grow(float s)
    {
        vMin.sub(s);
        vMax.add(s);
        return *this;
    }

    auto& grow(const Fvector3& s)
    {
        vMin.sub(s);
        vMax.add(s);
        return *this;
    }

    auto& add(const Fvector3& p)
    {
        vMin.add(p);
        vMax.add(p);
        return *this;
    }

    auto& sub(const Fvector3& p)
    {
        vMin.sub(p);
        vMax.sub(p);
        return *this;
    }

    auto& offset(const Fvector3& p)
    {
        vMin.add(p);
        vMax.add(p);
        return *this;
    }

    auto& add(const Fbox3& b, const Fvector3& p)
    {
        vMin.add(b.vMin, p);
        vMax.add(b.vMax, p);
        return *this;
    }

    ICF bool contains(float x, float y, float z) const
    {
        return (x >= x1) && (x <= x2) && (y >= y1) && (y <= y2) && (z >= z1) && (z <= z2);
    }

    ICF bool contains(const Fvector3& p) const { return contains(p.x, p.y, p.z); }
    ICF bool contains(const Fbox3& b) const { return contains(b.vMin) && contains(b.vMax); }

    [[nodiscard]] bool similar(const Fbox3& b) const { return vMin.similar(b.vMin) && vMax.similar(b.vMax); }

    ICF auto& modify(const Fvector3& p)
    {
        vMin.min(p);
        vMax.max(p);
        return *this;
    }

    ICF auto& modify(float x, float y, float z)
    {
        return modify(Fvector3{ x, y, z });
    }

    auto& merge(const Fbox3& b)
    {
        modify(b.vMin);
        modify(b.vMax);
        return *this;
    }

    auto& merge(const Fbox3& b1, const Fbox3& b2)
    {
        invalidate();
        merge(b1);
        merge(b2);
        return *this;
    }

    ICF auto& xform(const Fbox3& B, const Fmatrix& m)
    {
        // The three edges transformed: you can efficiently transform an X-only vector3
        // by just getting the "X" column of the matrix
        Fvector3 vx, vy, vz;
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

    ICF auto& xform(const Fmatrix& m)
    {
        Fbox3 b;
        b.set(*this);
        return xform(b, m);
    }

    Fmatrix get_xform() const
    {
        Fvector center, extent;
        center.add(vMin, vMax).div(2.0f);
        extent.sub(vMax, vMin).div(2.0f);

        Fmatrix transformMatrix;
        transformMatrix.identity();

        transformMatrix.translate(center);
        transformMatrix.scale(extent);

        return transformMatrix;
    }

    void getsize(Fvector3& R) const { R.sub(vMax, vMin); }

    void getradius(Fvector3& R) const
    {
        getsize(R);
        R.mul(0.5f);
    }

    float getradius() const
    {
        Fvector3 R;
        getradius(R);
        return R.magnitude();
    }

    float getvolume() const
    {
        Fvector3 sz;
        getsize(sz);
        return sz.x * sz.y * sz.z;
    }

    const auto& getcenter(Fvector3& C) const
    {
        C.x = (vMin.x + vMax.x) * 0.5f;
        C.y = (vMin.y + vMax.y) * 0.5f;
        C.z = (vMin.z + vMax.z) * 0.5f;
        return *this;
    }

    const auto& get_CD(Fvector3& bc, Fvector3& bd) const // center + dimensions
    {
        bd.sub(vMax, vMin).mul(.5f);
        bc.add(vMin, bd);
        return *this;
    }

    auto& scale(float s) // 0.1 means make 110%, -0.1 means make 90%
    {
        Fvector bd;
        bd.sub(vMax, vMin).mul(s);
        grow(bd);
        return *this;
    }

    const auto& getsphere(Fvector3& C, float& R) const
    {
        getcenter(C);
        R = C.distance_to(vMax);
        return *this;
    }

    // Detects if this box intersect other
    ICF bool intersect(const Fbox3& box) const
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
    bool Pick(const Fvector3& start, const Fvector3& dir) const
    {
        float alpha, xt, yt, zt;
        Fvector3 rvmin, rvmax;

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

    u32& IntRref(float& x) { return (u32&)x; }

    enum ERP_Result : u32
    {
        rpNone = 0,
        rpOriginInside = 1,
        rpOriginOutside = 2,
    };

    ERP_Result Pick2(const Fvector3& origin, const Fvector3& dir, Fvector3& coord)
    {
        bool Inside = true;
        Fvector3 MaxT;
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

    void getpoint(int index, Fvector3& result) const
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

    void getpoints(Fvector3* result) const
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

    auto& modify(const Fbox3& src, const Fmatrix& M)
    {
        Fvector3 pt;
        for (int i = 0; i < 8; i++)
        {
            src.getpoint(i, pt);
            M.transform_tiny(pt);
            modify(pt);
        }
        return *this;
    }
};

using Fbox = Fbox3;

inline bool _valid(const Fbox3& c)
{
    return _valid(c.vMin) && _valid(c.vMax);
}
