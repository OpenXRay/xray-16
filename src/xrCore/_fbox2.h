#pragma once

#include "_vector2.h"

struct Fbox2
{
    union
    {
        struct
        {
            Fvector2 min;
            Fvector2 max;
        };
        struct
        {
            float x1, y1;
            float x2, y2;
        };
    };

    auto& set(const Fvector2& _min, const Fvector2& _max)
    {
        min.set(_min);
        max.set(_max);
        return *this;
    }

    auto& set(float x1_, float y1_, float x2_, float y2_)
    {
        min.set(x1_, y1_);
        max.set(x2_, y2_);
        return *this;
    }

    auto& set(const Fbox2& b)
    {
        min.set(b.min);
        max.set(b.max);
        return *this;
    }


    auto& set_zero()
    {
        min.set(0.f, 0.f);
        max.set(0.f, 0.f);
        return *this;
    }

    auto& identity()
    {
        min.set(-0.5, -0.5);
        max.set(0.5, 0.5);
        return *this;
    }

    auto& invalidate()
    {
        min.set(type_max<float>, type_max<float>);
        max.set(type_min<float>, type_min<float>);
        return *this;
    }

    auto& shrink(float s)
    {
        min.add(s);
        max.sub(s);
        return *this;
    }

    auto& shrink(const Fvector2& s)
    {
        min.add(s);
        max.sub(s);
        return *this;
    }

    auto& grow(float s)
    {
        min.sub(s);
        max.add(s);
        return *this;
    }

    auto& grow(const Fvector2& s)
    {
        min.sub(s);
        max.add(s);
        return *this;
    }


    auto& add(const Fvector2& p)
    {
        min.add(p);
        max.add(p);
        return *this;
    }

    auto& offset(const Fvector2& p)
    {
        min.add(p);
        max.add(p);
        return *this;
    }

    auto& add(const Fbox2& b, const Fvector2& p)
    {
        min.add(b.min, p);
        max.add(b.max, p);
        return *this;
    }

    BOOL contains(float x, float y) { return (x >= x1) && (x <= x2) && (y >= y1) && (y <= y2); }
    BOOL contains(const Fvector2& p) { return contains(p.x, p.y); }
    BOOL contains(const Fbox2& b) { return contains(b.min) && contains(b.max); }
    BOOL similar(const Fbox2& b) { return min.similar(b.min) && max.similar(b.max); }

    auto& modify(const Fvector2& p)
    {
        min.min(p);
        max.max(p);
        return *this;
    }

    auto& merge(const Fbox2& b)
    {
        modify(b.min);
        modify(b.max);
        return *this;
    }

    auto& merge(const Fbox2& b1, const Fbox2& b2)
    {
        invalidate();
        merge(b1);
        merge(b2);
        return *this;
    }

    void getsize(Fvector2& R) const { R.sub(max, min); };
    void getradius(Fvector2& R) const
    {
        getsize(R);
        R.mul(0.5f);
    }

    float getradius() const
    {
        Fvector2 R;
        getsize(R);
        R.mul(0.5f);
        return R.magnitude();
    }

    void getcenter(Fvector2& C) const
    {
        C.x = (min.x + max.x) * 0.5f;
        C.y = (min.y + max.y) * 0.5f;
    }

    void getsphere(Fvector2& C, float& R) const
    {
        getcenter(C);
        R = C.distance_to(max);
    }

    // Detects if this box intersect other
    BOOL intersect(const Fbox2& box)
    {
        if (max.x < box.min.x)
            return FALSE;
        if (max.y < box.min.y)
            return FALSE;
        if (min.x > box.max.x)
            return FALSE;
        if (min.y > box.max.y)
            return FALSE;
        return TRUE;
    }

    // Make's this box valid AABB
    auto& sort()
    {
        float tmp;
        if (min.x > max.x)
        {
            tmp = min.x;
            min.x = max.x;
            max.x = tmp;
        }
        if (min.y > max.y)
        {
            tmp = min.y;
            min.y = max.y;
            max.y = tmp;
        }
        return *this;
    }

    // Does the vector3 intersects box
    BOOL Pick(const Fvector2& start, const Fvector2& dir)
    {
        float alpha, xt, yt;
        Fvector2 rvmin, rvmax;

        rvmin.sub(min, start);
        rvmax.sub(max, start);

        if (!fis_zero(dir.x))
        {
            alpha = rvmin.x / dir.x;
            yt = alpha * dir.y;
            if (yt >= rvmin.y && yt <= rvmax.y)
                return true;
            alpha = rvmax.x / dir.x;
            yt = alpha * dir.y;
            if (yt >= rvmin.y && yt <= rvmax.y)
                return true;
        }

        if (!fis_zero(dir.y))
        {
            alpha = rvmin.y / dir.y;
            xt = alpha * dir.x;
            if (xt >= rvmin.x && xt <= rvmax.x)
                return true;
            alpha = rvmax.y / dir.y;
            xt = alpha * dir.x;
            if (xt >= rvmin.x && xt <= rvmax.x)
                return true;
        }
        return false;
    }

    ICF BOOL pick_exact(const Fvector2& start, const Fvector2& dir)
    {
        float alpha, xt, yt;
        Fvector2 rvmin, rvmax;

        rvmin.sub(min, start);
        rvmax.sub(max, start);

        if (_abs(dir.x) != 0)
        {
            alpha = rvmin.x / dir.x;
            yt = alpha * dir.y;
            if (yt >= rvmin.y - EPS && yt <= rvmax.y + EPS)
                return true;
            alpha = rvmax.x / dir.x;
            yt = alpha * dir.y;
            if (yt >= rvmin.y - EPS && yt <= rvmax.y + EPS)
                return true;
        }
        if (_abs(dir.y) != 0)
        {
            alpha = rvmin.y / dir.y;
            xt = alpha * dir.x;
            if (xt >= rvmin.x - EPS && xt <= rvmax.x + EPS)
                return true;
            alpha = rvmax.y / dir.y;
            xt = alpha * dir.x;
            if (xt >= rvmin.x - EPS && xt <= rvmax.x + EPS)
                return true;
        }
        return false;
    }

    u32& IR(float& x) { return (u32&)x; }

    BOOL Pick2(const Fvector2& origin, const Fvector2& dir, Fvector2& coord)
    {
        BOOL Inside = TRUE;
        Fvector2 MaxT;
        MaxT.x = MaxT.y = -1.0f;

        // Find candidate planes.
        {
            if (origin[0] < min[0])
            {
                coord[0] = min[0];
                Inside = FALSE;
                if (IR(dir[0]))
                    MaxT[0] = (min[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
            }
            else if (origin[0] > max[0])
            {
                coord[0] = max[0];
                Inside = FALSE;
                if (IR(dir[0]))
                    MaxT[0] = (max[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
            }
        }
        {
            if (origin[1] < min[1])
            {
                coord[1] = min[1];
                Inside = FALSE;
                if (IR(dir[1]))
                    MaxT[1] = (min[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
            }
            else if (origin[1] > max[1])
            {
                coord[1] = max[1];
                Inside = FALSE;
                if (IR(dir[1]))
                    MaxT[1] = (max[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
            }
        }

        // Ray origin inside bounding box
        if (Inside)
        {
            coord = origin;
            return true;
        }

        // Get largest of the maxT's for final choice of intersection
        u32 WhichPlane = 0;
        if (MaxT[1] > MaxT[0])
            WhichPlane = 1;

        // Check final candidate actually inside box
        if (IR(MaxT[WhichPlane]) & 0x80000000)
            return false;

        if (0 == WhichPlane)
        {
            // 1
            coord[1] = origin[1] + MaxT[0] * dir[1];
            if ((coord[1] < min[1]) || (coord[1] > max[1]))
                return false;
            return true;
        }
        else
        {
            // 0
            coord[0] = origin[0] + MaxT[1] * dir[0];
            if ((coord[0] < min[0]) || (coord[0] > max[0]))
                return false;
            return true;
        }
    }

    void getpoint(int index, Fvector2& result) const
    {
        switch (index)
        {
        case 0: result.set(min.x, min.y); break;
        case 1: result.set(min.x, min.y); break;
        case 2: result.set(max.x, min.y); break;
        case 3: result.set(max.x, min.y); break;
        default: result.set(0.f, 0.f); break;
        }
    }

    void getpoints(Fvector2* result)
    {
        result[0].set(min.x, min.y);
        result[1].set(min.x, min.y);
        result[2].set(max.x, min.y);
        result[3].set(max.x, min.y);
    }
};

inline bool _valid(const Fbox2& c)
{
    return _valid(c.min) && _valid(c.max);
}
