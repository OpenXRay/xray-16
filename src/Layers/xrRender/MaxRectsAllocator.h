#pragma once

#include "xrCore/xrCore.h"
#include <algorithm>
#include <climits>

namespace xray::render::fg
{

struct MaxRectsRect
{
    u32 x = 0;
    u32 y = 0;
    u32 w = 0;
    u32 h = 0;
};

class MaxRectsAllocator
{
    u32 m_size = 0;
    xr_vector<MaxRectsRect> m_free;
    xr_vector<MaxRectsRect> m_used;

    static bool Intersects(const MaxRectsRect& a, const MaxRectsRect& b)
    {
        return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y);
    }

    void SplitFree(u32 freeIdx, const MaxRectsRect& used)
    {
        const MaxRectsRect f = m_free[freeIdx];
        m_free.erase(m_free.begin() + freeIdx);

        if (used.x > f.x)
            m_free.push_back({f.x, f.y, used.x - f.x, f.h});
        if (used.x + used.w < f.x + f.w)
            m_free.push_back({used.x + used.w, f.y, (f.x + f.w) - (used.x + used.w), f.h});
        if (used.y > f.y)
            m_free.push_back({used.x, f.y, used.w, used.y - f.y});
        if (used.y + used.h < f.y + f.h)
            m_free.push_back({used.x, used.y + used.h, used.w, (f.y + f.h) - (used.y + used.h)});
    }

    void PruneFree()
    {
        for (u32 i = 0; i < m_free.size();)
        {
            bool contained = false;
            for (u32 j = 0; j < m_free.size(); ++j)
            {
                if (i == j)
                    continue;
                const MaxRectsRect& a = m_free[i];
                const MaxRectsRect& b = m_free[j];
                if (a.x >= b.x && a.y >= b.y && a.x + a.w <= b.x + b.w && a.y + a.h <= b.y + b.h)
                {
                    contained = true;
                    break;
                }
            }
            if (contained)
                m_free.erase(m_free.begin() + i);
            else
                ++i;
        }
    }

public:
    void initialize(u32 size)
    {
        m_size = size;
        m_free.clear();
        m_used.clear();
        m_free.push_back({0, 0, size, size});
    }

    bool push(MaxRectsRect& out, u32 size)
    {
        if (size < 4 || size > m_size)
            return false;

        s32 bestScore = INT_MAX;
        s32 bestShort = INT_MAX;
        s32 bestIdx = -1;
        MaxRectsRect best{};

        for (u32 i = 0; i < m_free.size(); ++i)
        {
            const MaxRectsRect& f = m_free[i];
            if (f.w < size || f.h < size)
                continue;
            const s32 leftoverX = s32(f.w) - s32(size);
            const s32 leftoverY = s32(f.h) - s32(size);
            const s32 shortSide = std::min(leftoverX, leftoverY);
            const s32 longSide = std::max(leftoverX, leftoverY);
            if (shortSide < bestShort || (shortSide == bestShort && longSide < bestScore))
            {
                bestShort = shortSide;
                bestScore = longSide;
                bestIdx = s32(i);
                best = {f.x, f.y, size, size};
            }
        }

        if (bestIdx < 0)
            return false;

        SplitFree(u32(bestIdx), best);
        PruneFree();
        m_used.push_back(best);
        out = best;
        return true;
    }

    bool reserve(u32 posX, u32 posY, u32 size)
    {
        if (size < 4 || size > m_size)
            return false;
        if (posX + size > m_size || posY + size > m_size)
            return false;

        MaxRectsRect used{posX, posY, size, size};
        for (const MaxRectsRect& u : m_used)
        {
            if (Intersects(u, used))
                return false;
        }

        for (u32 i = 0; i < m_free.size();)
        {
            if (Intersects(m_free[i], used))
            {
                SplitFree(i, used);
                continue;
            }
            ++i;
        }
        PruneFree();
        m_used.push_back(used);
        return true;
    }
};

} // namespace xray::render::fg
