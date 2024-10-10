#include "stdafx.h"
#include "DetailManager.h"

void CDetailManager::cache_Initialize()
{
    // Centroid
    cache_cx = 0;
    cache_cz = 0;

    // Initialize cache-grid
    Slot* slt = cache_pool;
    for (u32 i = 0; i < dm_cache_line; i++)
        for (u32 j = 0; j < dm_cache_line; j++, slt++)
        {
            cache[i][j] = slt;
            cache_Task(j, i, slt);
        }
    VERIFY(cache_Validate());

    for (u32 _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
    {
        for (u32 _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
        {
            CacheSlot1& MS = cache_level1[_mz1][_mx1];
            for (u32 _z = 0; _z < dm_cache1_count; _z++)
            {
                for (u32 _x = 0; _x < dm_cache1_count; _x++)
                {
                    MS.slots[_z * dm_cache1_count + _x] =
                        &cache[_mz1 * dm_cache1_count + _z][_mx1 * dm_cache1_count + _x];
                }
            }
        }
    }
}

CDetailManager::Slot* CDetailManager::cache_Query(int r_x, int r_z)
{
    int gx = w2cg_X(r_x + cache_cx);
    VERIFY(gx >= 0 && gx < dm_cache_line);
    int gz = w2cg_Z(r_z + cache_cz);
    VERIFY(gz >= 0 && gz < dm_cache_line);
    return cache[gz][gx];
}

void CDetailManager::cache_Task(int gx, int gz, Slot* D)
{
    int sx = cg2w_X(gx);
    int sz = cg2w_Z(gz);
    DetailSlot& DS = QueryDB(sx, sz);

    D->empty = (DS.id0 == DetailSlot::ID_Empty) && (DS.id1 == DetailSlot::ID_Empty) &&
        (DS.id2 == DetailSlot::ID_Empty) && (DS.id3 == DetailSlot::ID_Empty);

    // Unpacking
    u32 old_type = D->type;
    D->type = stPending;
    D->sx = sx;
    D->sz = sz;

    D->vis.box.vMin.set(sx * dm_slot_size, DS.r_ybase(), sz * dm_slot_size);
    D->vis.box.vMax.set(D->vis.box.vMin.x + dm_slot_size, DS.r_ybase() + DS.r_yheight(), D->vis.box.vMin.z + dm_slot_size);
    D->vis.box.grow(EPS_L);

    for (u32 i = 0; i < dm_obj_in_slot; i++)
    {
        D->G[i].id = DS.r_id(i);
        for (u32 clr = 0; clr < D->G[i].items.size(); clr++)
            poolSI.destroy(D->G[i].items[clr]);
        D->G[i].items.clear();
    }

    if (old_type != stPending)
    {
        VERIFY(stPending == D->type);
        cache_task.push_back(D);
    }
}

BOOL CDetailManager::cache_Validate()
{
    for (u32 z = 0; z < dm_cache_line; z++)
    {
        for (u32 x = 0; x < dm_cache_line; x++)
        {
            int w_x = cg2w_X(x);
            int w_z = cg2w_Z(z);
            Slot* D = cache[z][x];

            if (D->sx != w_x)
                return FALSE;
            if (D->sz != w_z)
                return FALSE;
        }
    }
    return TRUE;
}

void CDetailManager::cache_Update(int v_x, int v_z, Fvector& view)
{
    ZoneScoped;

    bool bNeedMegaUpdate = (cache_cx != v_x) || (cache_cz != v_z);
    // ***** Cache shift
    while (cache_cx != v_x)
    {
        if (v_x > cache_cx)
        {
            // shift matrix to left
            cache_cx++;
            for (u32 z = 0; z < dm_cache_line; z++)
            {
                Slot* S = cache[z][0];
                for (u32 x = 1; x < dm_cache_line; x++)
                    cache[z][x - 1] = cache[z][x];
                cache[z][dm_cache_line - 1] = S;
                cache_Task(dm_cache_line - 1, z, S);
            }
            //R_ASSERT(cache_Validate());
        }
        else
        {
            // shift matrix to right
            cache_cx--;
            for (u32 z = 0; z < dm_cache_line; z++)
            {
                Slot* S = cache[z][dm_cache_line - 1];
                for (u32 x = dm_cache_line - 1; x > 0; x--)
                    cache[z][x] = cache[z][x - 1];
                cache[z][0] = S;
                cache_Task(0, z, S);
            }
            //R_ASSERT(cache_Validate());
        }
    }
    while (cache_cz != v_z)
    {
        if (v_z > cache_cz)
        {
            // shift matrix down a bit
            cache_cz++;
            for (u32 x = 0; x < dm_cache_line; x++)
            {
                Slot* S = cache[dm_cache_line - 1][x];
                for (u32 z = dm_cache_line - 1; z > 0; z--)
                    cache[z][x] = cache[z - 1][x];
                cache[0][x] = S;
                cache_Task(x, 0, S);
            }
            //R_ASSERT(cache_Validate());
        }
        else
        {
            // shift matrix up
            cache_cz--;
            for (u32 x = 0; x < dm_cache_line; x++)
            {
                Slot* S = cache[0][x];
                for (u32 z = 1; z < dm_cache_line; z++)
                    cache[z - 1][x] = cache[z][x];
                cache[dm_cache_line - 1][x] = S;
                cache_Task(x, dm_cache_line - 1, S);
            }
            // R_ASSERT (cache_Validate());
        }
    }

    // Task performer
    if (cache_task.size() == dm_cache_size) // full unpack, first time
    {
        while (!cache_task.empty())
        {
            // Decompress and remove task
            const u32 bestId = cache_task.size() - 1;
            cache_Decompress(cache_task[bestId]);
            cache_task.erase(bestId);
        }
    }
    else
    {
        const u32 invalidIndex = u32(-1);
        u32 bestIndexes[dm_max_decompress];
        std::fill_n(bestIndexes, dm_max_decompress, invalidIndex);

        float bestDistances[dm_max_decompress];
        std::fill_n(bestDistances, dm_max_decompress, flt_max);

        auto bestDistancesBegin = std::begin(bestDistances);
        auto bestDistancesEnd = std::end(bestDistances);
        ptrdiff_t maxIdx = 0;

        for (u32 i = 0, size = cache_task.size(); i < size; ++i)
        {
            // Gain access to data
            Slot* S = cache_task[i];
            VERIFY(stPending == S->type);

            // Estimate
            Fvector C;
            S->vis.box.getcenter(C);
            float D = view.distance_to_sqr(C);

            // Select
            if (D < bestDistances[maxIdx])
            {
                bestDistances[maxIdx] = D;
                bestIndexes[maxIdx] = i;

                const auto maxIt = std::max_element(bestDistancesBegin, bestDistancesEnd);
                maxIdx = std::distance(bestDistancesBegin, maxIt);
            }
        }

        // We sort indexes and process them in reverse order to erase correctly from cache_task
        std::sort(bestIndexes, bestIndexes + dm_max_decompress);

        for (int i = dm_max_decompress - 1; i >= 0; --i)
        {
            const u32 bestId = bestIndexes[i];

            if (bestId != invalidIndex)
            {
                // Decompress and remove task
                cache_Decompress(cache_task[bestId]);
                cache_task.erase(bestId);
            }
        }
    }

    if (bNeedMegaUpdate)
    {
        for (u32 _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
        {
            for (u32 _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
            {
                CacheSlot1& MS = cache_level1[_mz1][_mx1];
                MS.empty = TRUE;
                MS.vis.clear();
                for (u32 _i = 0; _i < dm_cache1_count * dm_cache1_count; _i++)
                {
                    Slot* PS = *MS.slots[_i];
                    Slot& S = *PS;
                    MS.vis.box.merge(S.vis.box);
                    if (!S.empty)
                        MS.empty = FALSE;
                }
                MS.vis.box.getsphere(MS.vis.sphere.P, MS.vis.sphere.R);
            }
        }
    }
}

DetailSlot& CDetailManager::QueryDB(int sx, int sz)
{
    int db_x = sx + dtH.x_offs();
    int db_z = sz + dtH.z_offs();
    if ((db_x >= 0) && (db_x < int(dtH.x_size())) && (db_z >= 0) && (db_z < int(dtH.z_size())))
    {
        u32 linear_id = db_z * dtH.x_size() + db_x;
        return dtSlots[linear_id];
    }
    else
    {
        // Empty slot
        DS_empty.w_id(0, DetailSlot::ID_Empty);
        DS_empty.w_id(1, DetailSlot::ID_Empty);
        DS_empty.w_id(2, DetailSlot::ID_Empty);
        DS_empty.w_id(3, DetailSlot::ID_Empty);
        return DS_empty;
    }
}
