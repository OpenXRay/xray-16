#pragma once

namespace xray::render::fg
{
struct SMAP_Rect
{
    Ivector2 min, max;
    bool intersect(SMAP_Rect& R)
    {
        if (max.x < R.min.x)
            return false;
        if (max.y < R.min.y)
            return false;
        if (min.x > R.max.x)
            return false;
        if (min.y > R.max.y)
            return false;
        return true;
    }
    bool valid()
    {
        if (min.x == max.x)
            return false;
        if (min.y == max.y)
            return false;
        return true;
    }
    void setup(Ivector2& p, u32 size)
    {
        min = max = p;
        max.add(size - 1);
    }
    void get_cp(Ivector2& p0, Ivector2& p1)
    {
        p0.set(max.x + 1, min.y); // right
        p1.set(min.x, max.y + 1); // down
    }
};

class SMAP_Allocator
{
    u32 psize; // pool size
    xr_vector<SMAP_Rect> stack; //
    xr_vector<Ivector2> cpoint; // critical points
private:
    void _add(SMAP_Rect& R)
    {
        stack.push_back(R);
        Ivector2 p0, p1;
        R.get_cp(p0, p1);
        s32 ps = s32(psize);
        if ((p0.x < ps) && (p0.y < ps))
            cpoint.push_back(p0); // 1st
        if ((p1.x < ps) && (p1.y < ps))
            cpoint.push_back(p1); // 2nd
    }

public:
    void initialize(u32 _size)
    {
        psize = _size;
        stack.clear();
        cpoint.clear();
    }
    BOOL push(SMAP_Rect& R, u32 _size)
    {
        VERIFY(_size <= psize && _size > 4);

        if (stack.empty())
        {
            Ivector2 p;
            p.set(0, 0);
            R.setup(p, _size);
            _add(R);
            return true;
        }

        for (u32 it = 0; it < cpoint.size(); it++)
        {
            R.setup(cpoint[it], _size);
            if (R.max.x >= int(psize))
                continue;
            if (R.max.y >= int(psize))
                continue;
            BOOL bIntersect = false;
            for (u32 t = 0; t < stack.size(); t++)
                if (stack[t].intersect(R))
                {
                    bIntersect = true;
                    break;
                }
            if (bIntersect)
                continue;

            cpoint.erase(cpoint.begin() + it);
            _add(R);
            return true;
        }

        return false;
    }

    BOOL reserve(u32 posX, u32 posY, u32 size)
    {
        if (size < 4 || size > psize)
            return FALSE;
        if (posX + size > psize || posY + size > psize)
            return FALSE;
        Ivector2 p;
        p.set(int(posX), int(posY));
        SMAP_Rect R{};
        R.setup(p, size);
        for (u32 t = 0; t < stack.size(); t++)
            if (stack[t].intersect(R))
                return FALSE;
        _add(R);
        return TRUE;
    }
};
} // namespace xray::render::fg
