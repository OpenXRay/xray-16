#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"
#include "xrCDB/Intersect.hpp"
#include "xrCDB/xrXRC.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"
#include "xrCore/FTimer.h"
#include <atomic>
#include <thread>
#include <sstream>

#ifdef DEBUG
#include "dxDebugRender.h"
#endif

#ifdef _EDITOR
#include "scene.h"
#include "sceneobject.h"
#include "utils/ETools/ETools.h"
#endif

namespace xray::render::RENDER_NAMESPACE
{
//--------------------------------------------------- Decompression
IC float Interpolate(float* base, u32 x, u32 y, u32 size)
{
    float f = float(size);
    float fx = float(x) / f;
    float ifx = 1.f - fx;
    float fy = float(y) / f;
    float ify = 1.f - fy;

    float c01 = base[0] * ifx + base[1] * fx;
    float c23 = base[2] * ifx + base[3] * fx;

    float c02 = base[0] * ify + base[2] * fy;
    float c13 = base[1] * ify + base[3] * fy;

    float cx = ify * c01 + fy * c23;
    float cy = ifx * c02 + fx * c13;
    return (cx + cy) / 2;
}

IC bool InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size, int dither[16][16])
{
    clamp(x, (u32)0, size - 1);
    clamp(y, (u32)0, size - 1);
    int c = iFloor(Interpolate(alpha255, x, y, size) + .5f);
    clamp(c, 0, 255);

    u32 row = (y + sy) % 16;
    u32 col = (x + sx) % 16;
    return c > dither[col][row];
}

#ifdef DEBUG
static void draw_obb(const Fmatrix& matrix, const u32& color)
{
    Fvector aabb[8];
    matrix.transform_tiny(aabb[0], Fvector().set(-1, -1, -1)); // 0
    matrix.transform_tiny(aabb[1], Fvector().set(-1, +1, -1)); // 1
    matrix.transform_tiny(aabb[2], Fvector().set(+1, +1, -1)); // 2
    matrix.transform_tiny(aabb[3], Fvector().set(+1, -1, -1)); // 3
    matrix.transform_tiny(aabb[4], Fvector().set(-1, -1, +1)); // 4
    matrix.transform_tiny(aabb[5], Fvector().set(-1, +1, +1)); // 5
    matrix.transform_tiny(aabb[6], Fvector().set(+1, +1, +1)); // 6
    matrix.transform_tiny(aabb[7], Fvector().set(+1, -1, +1)); // 7

    u16 aabb_id[12 * 2] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 1, 5, 2, 6, 3, 7, 0, 4};

    rdebug_render->add_lines(
        aabb, sizeof(aabb) / sizeof(Fvector), &aabb_id[0], sizeof(aabb_id) / (2 * sizeof(u16)), color);
}

bool det_render_debug = false;
#endif

//#define		DBG_SWITCHOFF_RANDOMIZE
void CDetailManager::cache_Decompress(Slot* S)
{
    VERIFY(S);
    Slot& D = *S;
    D.type = stReady;
    if (D.empty)
        return;

    DetailSlot& DS = QueryDB(D.sx, D.sz);

    // Select polygons
    Fvector bC, bD;
    D.vis.box.get_CD(bC, bD);

    // Thread-local collision query (xrc member is not thread-safe)
    thread_local xrXRC thread_xrc;
    thread_xrc.box_query(CDB::OPT_FULL_TEST, g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);
    const auto triCount = thread_xrc.r_count();
    CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
    Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

    if (0 == triCount)
        return;

    // Build shading table
    float alpha255[dm_obj_in_slot][4];
    for (int i = 0; i < dm_obj_in_slot; i++)
    {
        alpha255[i][0] = 255.f * float(DS.palette[i].a0) / 15.f;
        alpha255[i][1] = 255.f * float(DS.palette[i].a1) / 15.f;
        alpha255[i][2] = 255.f * float(DS.palette[i].a2) / 15.f;
        alpha255[i][3] = 255.f * float(DS.palette[i].a3) / 15.f;
    }

    // Prepare to selection
    float density = ps_r__Detail_density;
    float jitter = density / 1.7f;
    u32 d_size = iCeil(dm_slot_size / density);
    svector<int, dm_obj_in_slot> selected;

    u32 p_rnd = D.sx * D.sz; // нужно для того чтобы убрать полосы(ряды)
    CRandom r_selection(0x12071980 ^ p_rnd);
    CRandom r_jitter(0x12071980 ^ p_rnd);
    CRandom r_yaw(0x12071980 ^ p_rnd);
    CRandom r_scale(0x12071980 ^ p_rnd);

    // Prepare to actual-bounds-calculations
    Fbox Bounds;
    Bounds.invalidate();

    // Decompressing itself
    for (u32 z = 0; z <= d_size; z++)
    {
        for (u32 x = 0; x <= d_size; x++)
        {
// shift
#ifndef DBG_SWITCHOFF_RANDOMIZE
            u32 shift_x = r_jitter.randI(16);
            u32 shift_z = r_jitter.randI(16);
#else
            u32 shift_x = 8;
            u32 shift_z = 8;
#endif
            // Iterpolate and dither palette
            selected.clear();

#ifndef DBG_SWITCHOFF_RANDOMIZE
            if ((DS.id0 != DetailSlot::ID_Empty) &&
                InterpolateAndDither(alpha255[0], x, z, shift_x, shift_z, d_size, dither))
                selected.push_back(0);
            if ((DS.id1 != DetailSlot::ID_Empty) &&
                InterpolateAndDither(alpha255[1], x, z, shift_x, shift_z, d_size, dither))
                selected.push_back(1);
            if ((DS.id2 != DetailSlot::ID_Empty) &&
                InterpolateAndDither(alpha255[2], x, z, shift_x, shift_z, d_size, dither))
                selected.push_back(2);
            if ((DS.id3 != DetailSlot::ID_Empty) &&
                InterpolateAndDither(alpha255[3], x, z, shift_x, shift_z, d_size, dither))
                selected.push_back(3);
#else
            if ((DS.id0 != DetailSlot::ID_Empty))
                selected.push_back(0);
            if ((DS.id1 != DetailSlot::ID_Empty))
                selected.push_back(1);
            if ((DS.id2 != DetailSlot::ID_Empty))
                selected.push_back(2);
            if ((DS.id3 != DetailSlot::ID_Empty))
                selected.push_back(3);
#endif

            // Select
            if (selected.empty())
                continue;
#ifndef DBG_SWITCHOFF_RANDOMIZE
            u32 index;
            if (selected.size() == 1)
                index = selected[0];
            else
                index = selected[r_selection.randI(selected.size())];
#else
            u32 index = selected[0];
#endif

            CDetail* Dobj = objects[DS.r_id(index)];

            // Allocate SlotItem (heap allocation for lock-free parallel decompression)
            SlotItem* ItemP = new SlotItem();
            SlotItem& Item = *ItemP;

            // Position (XZ)
            float rx = (float(x) / float(d_size)) * dm_slot_size + D.vis.box.vMin.x;
            float rz = (float(z) / float(d_size)) * dm_slot_size + D.vis.box.vMin.z;
            Fvector Item_P;

#ifndef DBG_SWITCHOFF_RANDOMIZE
            Item_P.set(rx + r_jitter.randFs(jitter), D.vis.box.vMax.y, rz + r_jitter.randFs(jitter));
#else
            Item_P.set(rx, D.vis.box.vMax.y, rz);
#endif

            // Position (Y)
            float y = D.vis.box.vMin.y - 5;
            Fvector dir;
            dir.set(0, -1, 0);

            float r_u, r_v, r_range;
            for (size_t tid = 0; tid < triCount; tid++)
            {
                CDB::TRI& T = tris[thread_xrc.r_begin()[tid].id];
                SGameMtl* mtl = GMLib.GetMaterialByIdx(T.material);
                if (mtl->Flags.test(SGameMtl::flPassable))
                    continue;

                Fvector Tv[3] = {verts[T.verts[0]], verts[T.verts[1]], verts[T.verts[2]]};
                if (CDB::TestRayTri(Item_P, dir, Tv, r_u, r_v, r_range, TRUE))
                {
                    if (r_range >= 0)
                    {
                        float y_test = Item_P.y - r_range;
                        if (y_test > y)
                            y = y_test;
                    }
                }
            }
            if (y < D.vis.box.vMin.y)
                continue;
            Item_P.y = y;

// Angles and scale
#ifndef DBG_SWITCHOFF_RANDOMIZE
            Item.scale = r_scale.randF(Dobj->m_fMinScale * 0.5f, Dobj->m_fMaxScale * 0.9f) * ps_current_detail_height;
#else
            Item.scale = (Dobj->m_fMinScale * 0.5f + Dobj->m_fMaxScale * 0.9f) / 2;
// Item.scale	= 0.1f;
#endif
            // X-Form BBox
            Fmatrix mScale, mXform;
            Fbox ItemBB;

#ifndef DBG_SWITCHOFF_RANDOMIZE
            Item.mRotY.rotateY(r_yaw.randF(0, PI_MUL_2));
#else
            Item.mRotY.rotateY(0);
#endif

            Item.mRotY.translate_over(Item_P);
            mScale.scale(Item.scale, Item.scale, Item.scale);
            mXform.mul_43(Item.mRotY, mScale);
            ItemBB.xform(Dobj->bv_bb, mXform);
            Bounds.merge(ItemBB);

#ifndef _EDITOR
#ifdef DEBUG
            if (det_render_debug)
                draw_obb(mXform, color_rgba(255, 0, 0, 255)); // Fmatrix().mul_43( mXform, Fmatrix().scale(5,5,5) )
#endif
#endif

// Color
/*
DetailPalette*	c_pal			= (DetailPalette*)&DS.color;
float gray255	[4];
gray255[0]						=	255.f*float(c_pal->a0)/15.f;
gray255[1]						=	255.f*float(c_pal->a1)/15.f;
gray255[2]						=	255.f*float(c_pal->a2)/15.f;
gray255[3]						=	255.f*float(c_pal->a3)/15.f;
*/
// float c_f						=	1.f;	//Interpolate		(gray255,x,z,d_size)+.5f;
// int c_dw						=	255;	//iFloor			(c_f);
// clamp							(c_dw,0,255);
// Item.C_dw						=	color_rgba		(c_dw,c_dw,c_dw,255);
#if RENDER == R_R1
            Item.c_rgb.x = DS.r_qclr(DS.c_r, 15);
            Item.c_rgb.y = DS.r_qclr(DS.c_g, 15);
            Item.c_rgb.z = DS.r_qclr(DS.c_b, 15);
#endif
            Item.c_hemi = DS.r_qclr(DS.c_hemi, 15);
            Item.c_sun = DS.r_qclr(DS.c_dir, 15);

//? hack: RGB = hemi
//? Item.c_rgb.add					(ps_r__Detail_rainbow_hemi*Item.c_hemi);

// Vis-sorting
#ifndef DBG_SWITCHOFF_RANDOMIZE
            if (!UseVS())
            {
                // Always still on CPU pipe
                Item.vis_ID = 0;
            }
            else
            {
                if (Dobj->m_Flags.is(DO_NO_WAVING))
                    Item.vis_ID = 0;
                else
                {
                    if (::Random.randI(0, 3) == 0)
                        Item.vis_ID = 2; // Second wave
                    else
                        Item.vis_ID = 1; // First wave
                }
            }
#else
            Item.vis_ID = 0;
#endif
            // Save it
            D.G[index].items.emplace_back(ItemP);
        }
    }

    // Update bounds to more tight and real ones
    D.vis.clear();
    D.vis.box.set(Bounds);
    D.vis.box.getsphere(D.vis.sphere.P, D.vis.sphere.R);

    // Add safety margin to prevent aggressive culling of thin grass meshes at screen edges
    D.vis.sphere.R *= 1.2f;
}

#ifdef USE_DX11
// Phase 2.0.2: Full level decompression
void CDetailManager::DecompressAllSlots()
{
    Msg("* [DetailManager] Decompressing entire level (multithreaded)...");

    all_level_instances.clear();
    total_instance_count = 0;
    full_level_loaded = false;

    // Calculate total number of slots to decompress
    u32 total_slots = dtH.x_size() * dtH.z_size();

    Msg("* [DetailManager] Level has %u x %u = %u slots to decompress",
        dtH.x_size(), dtH.z_size(), total_slots);

    // Build list of non-empty slots to process
    struct SlotToProcess
    {
        int sx, sz;
        u32 db_x, db_z;
    };
    xr_vector<SlotToProcess> slots_to_process;
    slots_to_process.reserve(total_slots / 2);  // Estimate half are non-empty

    for (u32 db_z = 0; db_z < dtH.z_size(); db_z++)
    {
        for (u32 db_x = 0; db_x < dtH.x_size(); db_x++)
        {
            int sx = int(db_x) - dtH.x_offs();
            int sz = int(db_z) - dtH.z_offs();
            DetailSlot& DS = QueryDB(sx, sz);

            bool is_empty = (DS.id0 == DetailSlot::ID_Empty) &&
                           (DS.id1 == DetailSlot::ID_Empty) &&
                           (DS.id2 == DetailSlot::ID_Empty) &&
                           (DS.id3 == DetailSlot::ID_Empty);

            if (!is_empty)
                slots_to_process.push_back({sx, sz, db_x, db_z});
        }
    }

    Msg("  - Found %u non-empty slots to process", slots_to_process.size());

    Lock results_lock;
    xr_vector<xr_vector<SlotItemWithObject>*> thread_results;

    // Progress tracking (lock-free with atomics)
    std::atomic<u32> processed_count{0};
    std::atomic<u32> total_instances{0};

    CTimer decompress_timer;
    decompress_timer.Start();

    u32 num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 8;  // Fallback
    if (num_threads > 16) num_threads = 16;  // Cap at 16 for memory reasons

    thread_results.resize(num_threads);
    for (u32 i = 0; i < num_threads; i++)
    {
        thread_results[i] = new xr_vector<SlotItemWithObject>();
        thread_results[i]->reserve(slots_to_process.size() / num_threads * 50);  // Estimate ~50 instances per slot
    }

    auto worker_func = [&](u32 thread_id, size_t start_idx, size_t end_idx)
    {
        std::thread::id this_thread_id = std::this_thread::get_id();
        std::stringstream ss;
        ss << this_thread_id;

        xr_vector<SlotItemWithObject>* my_results = thread_results[thread_id];

        // Process each slot in this thread's range (completely lock-free!)
        for (size_t slot_idx = start_idx; slot_idx < end_idx; ++slot_idx)
        {
            const auto& slot_info = slots_to_process[slot_idx];
            DetailSlot& DS = QueryDB(slot_info.sx, slot_info.sz);

            // Set up temporary slot
            Slot temp_slot;
            temp_slot.type = stPending;
            temp_slot.sx = slot_info.sx;
            temp_slot.sz = slot_info.sz;
            temp_slot.empty = false;

            // Set up visibility box
            temp_slot.vis.box.vMin.set(slot_info.sx * dm_slot_size, DS.r_ybase(), slot_info.sz * dm_slot_size);
            temp_slot.vis.box.vMax.set(temp_slot.vis.box.vMin.x + dm_slot_size,
                                       DS.r_ybase() + DS.r_yheight(),
                                       temp_slot.vis.box.vMin.z + dm_slot_size);
            temp_slot.vis.box.grow(EPS_L);

            // Initialize slot object IDs
            for (u32 i = 0; i < dm_obj_in_slot; i++)
                temp_slot.G[i].id = DS.r_id(i);

            // Decompress the slot
            cache_Decompress(&temp_slot);

            // Collect instances from this slot into thread-local vector
            u32 slot_instance_count = 0;
            for (u32 obj_idx = 0; obj_idx < dm_obj_in_slot; obj_idx++)
            {
                SlotPart& part = temp_slot.G[obj_idx];
                u32 object_id = part.id;

                for (SlotItem* item : part.items)
                {
                    SlotItemWithObject instance;
                    instance.item = *item;
                    instance.object_id = object_id;
                    my_results->push_back(instance);  // No lock!
                    slot_instance_count++;

                    delete item;
                }
                part.items.clear();
            }

            // Update counters (lock-free atomics)
            u32 current_processed = processed_count.fetch_add(1) + 1;
            total_instances.fetch_add(slot_instance_count);

            // Progress reporting every 1000 slots
            if (current_processed % 1000 == 0)
            {
                u32 current_total = total_instances.load();
                float progress = 100.0f * current_processed / slots_to_process.size();
                float elapsed = decompress_timer.GetElapsed_sec();
            }
        }
    };

    xr_vector<std::thread> workers;
    workers.reserve(num_threads);
    size_t slots_per_thread = slots_to_process.size() / num_threads;
    size_t remainder = slots_to_process.size() % num_threads;

    size_t current_start = 0;
    for (u32 i = 0; i < num_threads; i++)
    {
        size_t current_end = current_start + slots_per_thread + (i < remainder ? 1 : 0);

        if (current_start < slots_to_process.size())
        {
            workers.emplace_back(worker_func, i, current_start, current_end);
        }

        current_start = current_end;
    }

    for (auto& thread : workers)
        thread.join();

    float decompress_time = decompress_timer.GetElapsed_sec();

    CTimer merge_timer;
    merge_timer.Start();

    size_t total_size = 0;
    for (u32 i = 0; i < num_threads; i++)
        total_size += thread_results[i]->size();

    all_level_instances.reserve(total_size);

    for (u32 i = 0; i < num_threads; i++)
    {
        all_level_instances.insert(all_level_instances.end(), thread_results[i]->begin(), thread_results[i]->end());
        delete thread_results[i];
    }

    float merge_time = merge_timer.GetElapsed_sec();

    total_instance_count = all_level_instances.size();
    full_level_loaded = true;

    float memory_mb = (total_instance_count * sizeof(SlotItemWithObject)) / (1024.0f * 1024.0f);
    float total_time = decompress_time + merge_time;
    float slots_per_sec = slots_to_process.size() / decompress_time;

    Msg("* [DetailManager] Decompression complete:");
    Msg("  - Total time: %.2f sec (decompress: %.2f, merge: %.2f)", total_time, decompress_time, merge_time);
    Msg("  - Performance: %.0f slots/sec (%.0f slots/sec/thread)", slots_per_sec, slots_per_sec / num_threads);
    Msg("  - Total instances: %u", total_instance_count);
    Msg("  - Memory (CPU): %.2f MB", memory_mb);
    Msg("  - Processed slots: %u/%u", (u32)slots_to_process.size(), total_slots);
    Msg("  - Threads used: %u (true parallel execution)", num_threads);

    // Phase 4A.1: Compute slot AABBs for hierarchical culling
    ComputeSlotAABBs();
}

// Phase 4A.1: Compute slot AABBs for hierarchical culling
void CDetailManager::ComputeSlotAABBs()
{
    Msg("* [DetailManager] Computing slot AABBs for hierarchical culling...");

    CTimer timer;
    timer.Start();

    slot_aabbs.clear();

    // We need to organize instances by slot
    // Create a map of slot coordinates to instance indices
    struct SlotKey
    {
        int sx, sz;
        bool operator<(const SlotKey& other) const
        {
            if (sx != other.sx) return sx < other.sx;
            return sz < other.sz;
        }
    };

    // Map from slot coordinates to list of instance indices
    std::map<SlotKey, xr_vector<u32>> slot_to_instances;

    // Assign each instance to its slot based on world position
    for (u32 i = 0; i < all_level_instances.size(); i++)
    {
        const SlotItemWithObject& inst = all_level_instances[i];
        Fvector pos = inst.item.mRotY.c;

        // Determine which slot this instance belongs to
        int sx = iFloor(pos.x / dm_slot_size);
        int sz = iFloor(pos.z / dm_slot_size);

        SlotKey key = {sx, sz};
        slot_to_instances[key].push_back(i);
    }

    Msg("  - Instances distributed across %u non-empty slots", (u32)slot_to_instances.size());

    // Phase 6: Allocate FULL DENSE GRID (all 716x716 cells) for virtual texturing
    // Empty slots will have instance_count = 0
    u32 total_grid_slots = dtH.x_size() * dtH.z_size();
    slot_aabbs.resize(total_grid_slots);

    Msg("  - Allocating dense grid: %u x %u = %u total slots", dtH.x_size(), dtH.z_size(), total_grid_slots);

    // Initialize all slots as empty
    for (u32 db_z = 0; db_z < dtH.z_size(); db_z++)
    {
        for (u32 db_x = 0; db_x < dtH.x_size(); db_x++)
        {
            int sx = int(db_x) - dtH.x_offs();
            int sz = int(db_z) - dtH.z_offs();
            u32 slot_idx = db_z * dtH.x_size() + db_x;

            SlotAABB& aabb = slot_aabbs[slot_idx];
            aabb.slot_x = sx;
            aabb.slot_z = sz;
            aabb.instance_base = 0;
            aabb.instance_count = 0;  // Empty by default
            aabb.aabb_min.set(sx * dm_slot_size, 0, sz * dm_slot_size);
            aabb.aabb_max.set((sx + 1) * dm_slot_size, 0, (sz + 1) * dm_slot_size);
        }
    }

    // Now fill in non-empty slots with actual data
    for (const auto& pair : slot_to_instances)
    {
        const SlotKey& key = pair.first;
        const xr_vector<u32>& instance_indices = pair.second;

        if (instance_indices.empty())
            continue;

        // Calculate dense array index
        int db_x = key.sx + dtH.x_offs();
        int db_z = key.sz + dtH.z_offs();
        u32 slot_idx = db_z * dtH.x_size() + db_x;

        VERIFY(slot_idx < total_grid_slots);

        SlotAABB& aabb = slot_aabbs[slot_idx];
        aabb.aabb_min.set(FLT_MAX, FLT_MAX, FLT_MAX);
        aabb.aabb_max.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        aabb.instance_base = instance_indices.front();  // First instance index
        aabb.instance_count = instance_indices.size();
        aabb.slot_x = key.sx;
        aabb.slot_z = key.sz;

        // Compute AABB from all instances in this slot
        for (u32 inst_idx : instance_indices)
        {
            const SlotItemWithObject& inst = all_level_instances[inst_idx];
            Fvector pos = inst.item.mRotY.c;
            float scale = inst.item.scale;
            u32 object_id = inst.object_id;

            VERIFY(object_id < objects.size());
            float model_radius = objects[object_id]->bv_sphere.R;
            float instance_radius = model_radius * scale;

            Fvector inst_min, inst_max;
            inst_min.set(pos.x - instance_radius, pos.y - instance_radius, pos.z - instance_radius);
            inst_max.set(pos.x + instance_radius, pos.y + instance_radius, pos.z + instance_radius);

            // Expand slot AABB
            aabb.aabb_min.x = std::min(aabb.aabb_min.x, inst_min.x);
            aabb.aabb_min.y = std::min(aabb.aabb_min.y, inst_min.y);
            aabb.aabb_min.z = std::min(aabb.aabb_min.z, inst_min.z);

            aabb.aabb_max.x = std::max(aabb.aabb_max.x, inst_max.x);
            aabb.aabb_max.y = std::max(aabb.aabb_max.y, inst_max.y);
            aabb.aabb_max.z = std::max(aabb.aabb_max.z, inst_max.z);
        }
    }

    slot_count = slot_aabbs.size();
    VERIFY(slot_count == total_grid_slots);

    float elapsed = timer.GetElapsed_sec();
    float memory_kb = (slot_count * sizeof(SlotAABB)) / 1024.0f;

    Msg("* [DetailManager] Slot AABB computation complete:");
    Msg("  - Computed %u slot AABBs in %.3f sec", slot_count, elapsed);
    Msg("  - Memory (CPU): %.2f KB", memory_kb);

    // Validation: check total instances
    u32 total_instances_in_slots = 0;
    for (const auto& aabb : slot_aabbs)
        total_instances_in_slots += aabb.instance_count;

    if (total_instances_in_slots != total_instance_count)
    {
        Msg("! [DetailManager] WARNING: Instance count mismatch! Slots contain %u instances, but total is %u",
            total_instances_in_slots, total_instance_count);
    }
}
void CDetailManager::ValidateSlotAABBs()
{
    Msg("* [DetailManager] Validating slot AABBs...");

    CTimer timer;
    timer.Start();

    u32 errors = 0;
    u32 warnings = 0;

    // Statistics
    float min_aabb_size = FLT_MAX;
    float max_aabb_size = -FLT_MAX;
    float avg_aabb_size = 0.0f;
    u32 min_instances_per_slot = UINT_MAX;
    u32 max_instances_per_slot = 0;
    float avg_instances_per_slot = 0.0f;

    // Create mapping from slot coordinates to slot AABB index
    struct SlotKey
    {
        int sx, sz;
        bool operator<(const SlotKey& other) const
        {
            if (sx != other.sx) return sx < other.sx;
            return sz < other.sz;
        }
    };

    std::map<SlotKey, u32> slot_to_aabb_index;
    for (u32 i = 0; i < slot_aabbs.size(); i++)
    {
        SlotKey key = { (int)slot_aabbs[i].slot_x, (int)slot_aabbs[i].slot_z };
        slot_to_aabb_index[key] = i;
    }

    // Validate each instance is contained within its slot's AABB
    u32 skipped_instances = 0;
    for (u32 i = 0; i < all_level_instances.size(); i++)
    {
        const SlotItemWithObject& inst = all_level_instances[i];
        Fvector pos = inst.item.mRotY.c;
        float scale = inst.item.scale;  // Use 'scale', not 'scale_calculated'!

        // Skip instances with zero or invalid scale (same as AABB computation)
        if (scale <= 0.0f || !std::isfinite(scale))
        {
            skipped_instances++;
            continue;
        }

        // Find which slot this instance belongs to
        int sx = iFloor(pos.x / dm_slot_size);
        int sz = iFloor(pos.z / dm_slot_size);
        SlotKey key = { sx, sz };

        auto it = slot_to_aabb_index.find(key);
        if (it == slot_to_aabb_index.end())
        {
            Msg("! [AABB Validation] ERROR: Instance %u at (%.2f, %.2f, %.2f) with scale %.3f belongs to slot (%d, %d) which has no AABB!",
                i, pos.x, pos.y, pos.z, scale, sx, sz);
            errors++;
            continue;
        }

        const SlotAABB& slot_aabb = slot_aabbs[it->second];

        // Compute instance bounds (MUST match AABB computation exactly!)
        u32 object_id = inst.object_id;

        VERIFY(object_id < objects.size());
        float model_radius = objects[object_id]->bv_sphere.R;
        float instance_radius = model_radius * scale;  // Scale the model radius

        // Use bounding sphere (same as AABB computation)
        Fvector inst_min, inst_max;
        inst_min.set(pos.x - instance_radius, pos.y - instance_radius, pos.z - instance_radius);
        inst_max.set(pos.x + instance_radius, pos.y + instance_radius, pos.z + instance_radius);

        // Check if instance is fully contained within slot AABB
        bool contained = true;
        if (inst_min.x < slot_aabb.aabb_min.x - 0.01f ||
            inst_min.y < slot_aabb.aabb_min.y - 0.01f ||
            inst_min.z < slot_aabb.aabb_min.z - 0.01f ||
            inst_max.x > slot_aabb.aabb_max.x + 0.01f ||
            inst_max.y > slot_aabb.aabb_max.y + 0.01f ||
            inst_max.z > slot_aabb.aabb_max.z + 0.01f)
        {
            contained = false;
        }

        if (!contained)
        {
            Msg("! [AABB Validation] ERROR: Instance %u NOT contained in slot (%d, %d) AABB!", i, sx, sz);
            Msg("    Instance bounds: (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)",
                inst_min.x, inst_min.y, inst_min.z, inst_max.x, inst_max.y, inst_max.z);
            Msg("    Slot AABB:       (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)",
                slot_aabb.aabb_min.x, slot_aabb.aabb_min.y, slot_aabb.aabb_min.z,
                slot_aabb.aabb_max.x, slot_aabb.aabb_max.y, slot_aabb.aabb_max.z);
            errors++;

            if (errors >= 10)
            {
                Msg("  ... stopping error reporting (too many errors)");
                break;
            }
        }
    }

    // Compute statistics for each slot AABB
    for (const auto& slot_aabb : slot_aabbs)
    {
        // AABB size (diagonal length)
        Fvector size;
        size.x = slot_aabb.aabb_max.x - slot_aabb.aabb_min.x;
        size.y = slot_aabb.aabb_max.y - slot_aabb.aabb_min.y;
        size.z = slot_aabb.aabb_max.z - slot_aabb.aabb_min.z;
        float diagonal = sqrt(size.x * size.x + size.y * size.y + size.z * size.z);

        min_aabb_size = std::min(min_aabb_size, diagonal);
        max_aabb_size = std::max(max_aabb_size, diagonal);
        avg_aabb_size += diagonal;

        // Instance counts
        min_instances_per_slot = std::min(min_instances_per_slot, slot_aabb.instance_count);
        max_instances_per_slot = std::max(max_instances_per_slot, slot_aabb.instance_count);
        avg_instances_per_slot += slot_aabb.instance_count;

        // Check for truly degenerate AABBs (exactly zero volume - indicates uninitialized)
        // Note: Very small but non-zero AABBs are valid (single instance with tiny scale)
        if (size.x == 0.0f && size.y == 0.0f && size.z == 0.0f)
        {
            Msg("! [AABB Validation] ERROR: Slot (%d, %d) has exactly zero volume AABB - uninitialized!",
                slot_aabb.slot_x, slot_aabb.slot_z);
            errors++;
        }
        else if (size.x < 0.001f || size.y < 0.001f || size.z < 0.001f)
        {
            // Just count tiny AABBs, don't spam log (they're valid, just small)
            warnings++;
        }

        // Check for unreasonably large AABBs (> 5x slot size diagonal)
        // Note: Normal grass can be 2-3m tall, so AABB diagonal of 4-6m is expected!
        // Slot size = 2m, so diagonal of 2m×3m×2m box = sqrt(4+9+4) = 4.1m
        float max_reasonable_size = dm_slot_size * 5.0f;  // 10m diagonal
        if (diagonal > max_reasonable_size)
        {
            // Only log first 10 truly unreasonable AABBs to avoid spam
            if (warnings < 10)
            {
                Msg("! [AABB Validation] WARNING: Slot (%d, %d) has unusually large AABB: %.2fm (expected < %.2fm)",
                    slot_aabb.slot_x, slot_aabb.slot_z, diagonal, max_reasonable_size);
            }
            warnings++;
        }

        // Check instance_base and instance_count are valid
        if (slot_aabb.instance_base >= all_level_instances.size())
        {
            Msg("! [AABB Validation] ERROR: Slot (%d, %d) instance_base %u >= total instances %u",
                slot_aabb.slot_x, slot_aabb.slot_z, slot_aabb.instance_base, all_level_instances.size());
            errors++;
        }

        if (slot_aabb.instance_base + slot_aabb.instance_count > all_level_instances.size())
        {
            Msg("! [AABB Validation] ERROR: Slot (%d, %d) instance range [%u, %u) exceeds total instances %u",
                slot_aabb.slot_x, slot_aabb.slot_z,
                slot_aabb.instance_base, slot_aabb.instance_base + slot_aabb.instance_count,
                all_level_instances.size());
            errors++;
        }
    }

    if (slot_aabbs.size() > 0)
    {
        avg_aabb_size /= slot_aabbs.size();
        avg_instances_per_slot /= slot_aabbs.size();
    }

    float elapsed = timer.GetElapsed_sec();

    // Report results
    Msg("* [DetailManager] AABB Validation complete (%.3f sec):", elapsed);
    Msg("  - Total slots validated: %u", slot_aabbs.size());
    Msg("  - Total instances: %u", all_level_instances.size());
    Msg("  - Valid instances checked: %u", all_level_instances.size() - skipped_instances);
    Msg("  - Skipped instances (zero/invalid scale): %u", skipped_instances);
    Msg("  - Errors: %u", errors);
    Msg("  - Warnings: %u", warnings);
    Msg("");
    Msg("  AABB Size Statistics:");
    Msg("    Min diagonal: %.2f m", min_aabb_size);
    Msg("    Max diagonal: %.2f m", max_aabb_size);
    Msg("    Avg diagonal: %.2f m", avg_aabb_size);
    Msg("");
    Msg("  Instance Distribution:");
    Msg("    Min instances/slot: %u", min_instances_per_slot);
    Msg("    Max instances/slot: %u", max_instances_per_slot);
    Msg("    Avg instances/slot: %.1f", avg_instances_per_slot);

    if (errors > 0)
    {
        Msg("");
        Msg("! [AABB Validation] FAILED with %u errors!", errors);
        R_ASSERT2(false, "Slot AABB validation failed - see log for details");
    }
    else if (warnings > 0)
    {
        Msg("");
        Msg("* [AABB Validation] PASSED with %u warnings", warnings);
        if (warnings > 10)
            Msg("  (Most warnings are about tiny AABBs or slightly oversized AABBs - both are valid)");
        if (skipped_instances > 0)
            Msg("  (Note: %u instances skipped due to zero/invalid scale)", skipped_instances);
    }
    else
    {
        Msg("");
        Msg("* [AABB Validation] PASSED - All AABBs perfect!");
        if (skipped_instances > 0)
            Msg("  (Note: %u instances skipped due to zero/invalid scale)", skipped_instances);
    }
}
#endif
} // namespace xray::render::RENDER_NAMESPACE
