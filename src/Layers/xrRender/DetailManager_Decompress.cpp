#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"
#include "xrCDB/Intersect.hpp"
#include "xrCDB/xrXRC.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrCore/Threading/ParallelFor.hpp"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"

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

    // Thread-safe synchronization
    Lock instances_lock;
    Lock progress_lock;
    u32 processed_count = 0;
    u32 last_reported = 0;

    // Process slots in parallel using engine's threading system
    xr_parallel_for(TaskRange<size_t>(0, slots_to_process.size()), [&](const TaskRange<size_t>& range)
    {
        // Process each slot in this range
        for (size_t slot_idx = range.begin(); slot_idx != range.end(); ++slot_idx)
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

            // Collect instances from this slot
            xr_vector<SlotItemWithObject> slot_instances;
            for (u32 obj_idx = 0; obj_idx < dm_obj_in_slot; obj_idx++)
            {
                SlotPart& part = temp_slot.G[obj_idx];
                u32 object_id = part.id;

                for (SlotItem* item : part.items)
                {
                    SlotItemWithObject instance;
                    instance.item = *item;
                    instance.object_id = object_id;
                    slot_instances.push_back(instance);

                    // Free heap-allocated item (no lock needed!)
                    delete item;
                }
                part.items.clear();
            }

            // Add to global list (thread-safe)
            {
                ScopeLock lock(&instances_lock);
                all_level_instances.insert(all_level_instances.end(), slot_instances.begin(), slot_instances.end());
            }

            // Progress update (thread-safe)
            {
                ScopeLock lock(&progress_lock);
                processed_count++;
                if (processed_count % 100 == 0 || processed_count == slots_to_process.size())
                {
                    if (processed_count != last_reported)
                    {
                        Msg("  ... processed %u/%u slots (%u instances)",
                            processed_count, (u32)slots_to_process.size(), (u32)all_level_instances.size());
                        last_reported = processed_count;
                    }
                }
            }
        }
    });

    total_instance_count = all_level_instances.size();
    full_level_loaded = true;

    float memory_mb = (total_instance_count * sizeof(SlotItemWithObject)) / (1024.0f * 1024.0f);

    Msg("* [DetailManager] Decompression complete:");
    Msg("  - Total instances: %u", total_instance_count);
    Msg("  - Memory (CPU): %.2f MB", memory_mb);
    Msg("  - Processed slots: %u/%u", (u32)slots_to_process.size(), total_slots);
}
#endif
} // namespace xray::render::RENDER_NAMESPACE
