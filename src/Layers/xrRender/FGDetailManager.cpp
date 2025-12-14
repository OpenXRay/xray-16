// FGDetailManager.cpp - Framegraph detail manager implementation
#include "stdafx.h"
#include "FGDetailManager.h"
#include "DetailModel.h"
#include "FrameGraph/ShaderLoader.h"
#include "xrRender_console.h"
#include "xrCDB/Intersect.hpp"
#include "xrCDB/xrXRC.h"
#include "xrMaterialSystem/GameMtlLib.h"

// Phase 5: Grass wind tuning parameters (defined in xrEngine)
extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r3_grass_wind_min;
extern ENGINE_API float ps_r3_grass_wind_lerp_rate;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;
extern ENGINE_API u32 ps_r3_grass_wind_octaves;

namespace xray::render::RENDER_NAMESPACE
{

extern float ps_current_detail_height;

// Dither matrix initialization (EXACT copy from DetailManager.cpp)
static int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};

static void bwdithermap(int levels, int magic[16][16])
{
    /* Get size of each step */
    float N = 255.0f / (levels - 1);

    /*
     * Expand 4x4 dither pattern to 16x16.  4x4 leaves obvious patterning,
     * and doesn't give us full intensity range (only 17 sublevels).
     *
     * magicfact is (N - 1)/16 so that we get numbers in the matrix from 0 to
     * N - 1: mod N gives numbers in 0 to N - 1, don't ever want all
     * pixels incremented to the next level (this is reserved for the
     * pixel value with mod N == 0 at the next level).
     */

    float magicfact = (N - 1) / 16;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                for (int l = 0; l < 4; l++)
                {
                    magic[4 * k + i][4 * l + j] =
                        (int)(0.5 + magic4x4[i][j] * magicfact + (magic4x4[k][l] / 16.) * magicfact);
                }
            }
        }
    }
}

// Bilinear interpolation matching original Interpolate() function exactly
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
    return (cx + cy) / 2.f;
}

// Interpolate and dither matching original InterpolateAndDither() exactly
IC bool InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size, int dither_matrix[16][16])
{
    u32 cx = x, cy = y;
    clamp(cx, (u32)0, size - 1);
    clamp(cy, (u32)0, size - 1);
    int c = iFloor(Interpolate(alpha255, cx, cy, size) + .5f);
    clamp(c, 0, 255);

    u32 row = (y + sy) % 16;
    u32 col = (x + sx) % 16;
    return c > dither_matrix[col][row];
}

FGDetailManager::FGDetailManager()
{
    // Dither matrix initialized in Load()
    memset(dither, 0, sizeof(dither));
}

FGDetailManager::~FGDetailManager()
{
    Unload();
}

bool FGDetailManager::Load()
{
    ZoneScoped;

    if (!FS.exist("$level$", "level.details"))
    {
        Msg("! [FGDetailManager] level.details not found");
        return false;
    }

    string_path fn;
    FS.update_path(fn, "$level$", "level.details");
    dtFS = FS.r_open(fn);

    if (!dtFS)
    {
        Msg("! [FGDetailManager] Failed to open level.details");
        return false;
    }

    // Read header
    dtFS->r_chunk_safe(0, &dtH, sizeof(dtH));

    if (dtH.version() != DETAIL_VERSION)
    {
        Msg("! [FGDetailManager] Invalid detail version: %d (expected %d)", dtH.version(), DETAIL_VERSION);
        FS.r_close(dtFS);
        dtFS = nullptr;
        return false;
    }

    u32 object_count = dtH.object_count();
    Msg("* [FGDetailManager] Loading level.details: %u objects, %dx%d slots",
        object_count, dtH.x_size(), dtH.z_size());

    // Load detail models
    IReader* models_chunk = dtFS->open_chunk(1);
    if (!models_chunk)
    {
        Msg("! [FGDetailManager] Failed to read models chunk");
        FS.r_close(dtFS);
        dtFS = nullptr;
        return false;
    }

    for (u32 i = 0; i < object_count; i++)
    {
        IReader* model_chunk = models_chunk->open_chunk(i);
        if (!model_chunk)
        {
            Msg("! [FGDetailManager] Failed to read model %u", i);
            continue;
        }

        CDetail* detail = xr_new<CDetail>();
        detail->Load(model_chunk);
        detail_models.push_back(detail);

        model_chunk->close();
    }

    models_chunk->close();

    // Make dither matrix (matching original CDetailManager::Load exactly)
    bwdithermap(2, dither);

    Msg("* [FGDetailManager] Loaded %u detail models", detail_models.size());
    return true;
}

void FGDetailManager::Unload()
{
    ZoneScoped;

    DestroyGPUBuffers();

    for (CDetail* detail : detail_models)
        xr_delete(detail);
    detail_models.clear();

    all_instances.clear();
    slot_aabbs.clear();

    if (dtFS)
    {
        FS.r_close(dtFS);
        dtFS = nullptr;
    }
}

void FGDetailManager::DecompressAllSlots()
{
    ZoneScoped;

    if (!dtFS)
    {
        Msg("! [FGDetailManager] Cannot decompress - level.details not loaded");
        return;
    }

    // Open slots chunk
    IReader* slots_chunk = dtFS->open_chunk(2);
    if (!slots_chunk)
    {
        Msg("! [FGDetailManager] Failed to read slots chunk");
        return;
    }

    DetailSlot* dtSlots = (DetailSlot*)slots_chunk->pointer();

    Msg("* [FGDetailManager] Decompressing %dx%d detail slots...", dtH.x_size(), dtH.z_size());

    all_instances.clear();
    all_instances.reserve(1024 * 1024);

    slot_aabbs.clear();
    slot_count = 0;

    u32 total_slots = 0;
    u32 empty_slots = 0;
    u32 no_collision_slots = 0;

    // Iterate through all slots using DATABASE coordinates (0 to size-1)
    for (u32 db_z = 0; db_z < dtH.z_size(); db_z++)
    {
        for (u32 db_x = 0; db_x < dtH.x_size(); db_x++)
        {
            total_slots++;

            // Convert database coords to world slot coords (matching original cache_Task)
            int sx = int(db_x) - dtH.x_offs();
            int sz = int(db_z) - dtH.z_offs();

            // Get slot data
            u32 slot_idx = db_z * dtH.x_size() + db_x;
            DetailSlot& DS = dtSlots[slot_idx];

            // Check if slot is empty (matching original cache_Task)
            bool slot_empty = (DS.id0 == DetailSlot::ID_Empty) &&
                              (DS.id1 == DetailSlot::ID_Empty) &&
                              (DS.id2 == DetailSlot::ID_Empty) &&
                              (DS.id3 == DetailSlot::ID_Empty);

            if (slot_empty)
            {
                empty_slots++;
                continue;
            }

            // Track instance base for this slot
            u32 instance_base = all_instances.size();

            // === Set up visibility box (matching original cache_Task) ===
            Fbox vis_box;
            vis_box.vMin.set(sx * DETAIL_SLOT_SIZE, DS.r_ybase(), sz * DETAIL_SLOT_SIZE);
            vis_box.vMax.set(vis_box.vMin.x + DETAIL_SLOT_SIZE,
                            DS.r_ybase() + DS.r_yheight(),
                            vis_box.vMin.z + DETAIL_SLOT_SIZE);
            vis_box.grow(EPS_L);

            // === Query collision geometry (matching original cache_Decompress) ===
            Fvector bC, bD;
            vis_box.get_CD(bC, bD);

            thread_local xrXRC thread_xrc;
            thread_xrc.box_query(CDB::OPT_FULL_TEST, g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);
            const auto triCount = thread_xrc.r_count();

            if (triCount == 0)
            {
                no_collision_slots++;
                continue;
            }

            CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
            Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

            // Build alpha255 shading table (matching original)
            float alpha255[4][4];
            for (int i = 0; i < 4; i++)
            {
                alpha255[i][0] = 255.f * float(DS.palette[i].a0) / 15.f;
                alpha255[i][1] = 255.f * float(DS.palette[i].a1) / 15.f;
                alpha255[i][2] = 255.f * float(DS.palette[i].a2) / 15.f;
                alpha255[i][3] = 255.f * float(DS.palette[i].a3) / 15.f;
            }

            // Density and jitter settings (matching original cache_Decompress exactly)
            float density = ps_r__Detail_density;
            float jitter = density / 1.7f;
            u32 d_size = iCeil(DETAIL_SLOT_SIZE / density);

            // Seed random generators with world slot coords (matching original)
            u32 p_rnd = sx * sz;
            CRandom r_selection(0x12071980 ^ p_rnd);
            CRandom r_jitter(0x12071980 ^ p_rnd);
            CRandom r_yaw(0x12071980 ^ p_rnd);
            CRandom r_scale(0x12071980 ^ p_rnd);

            // === Decompression loop (matching original cache_Decompress exactly) ===
            for (u32 z = 0; z <= d_size; z++)
            {
                for (u32 x = 0; x <= d_size; x++)
                {
                    // Dither shift
                    u32 shift_x = r_jitter.randI(16);
                    u32 shift_z = r_jitter.randI(16);

                    // Interpolate and dither palette for each object type
                    svector<int, 4> selected;

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

                    if (selected.empty())
                        continue;

                    // Select one object type
                    u32 index;
                    if (selected.size() == 1)
                        index = selected[0];
                    else
                        index = selected[r_selection.randI(selected.size())];

                    u32 object_id = DS.r_id(index);
                    if (object_id >= detail_models.size())
                        continue;

                    CDetail* Dobj = detail_models[object_id];

                    // Position (XZ) with jitter
                    float rx = (float(x) / float(d_size)) * DETAIL_SLOT_SIZE + vis_box.vMin.x;
                    float rz = (float(z) / float(d_size)) * DETAIL_SLOT_SIZE + vis_box.vMin.z;
                    Fvector Item_P;
                    Item_P.set(rx + r_jitter.randFs(jitter), vis_box.vMax.y, rz + r_jitter.randFs(jitter));

                    // Position (Y) - raycast down to find terrain
                    float y = vis_box.vMin.y - 5.f;
                    Fvector dir;
                    dir.set(0.f, -1.f, 0.f);

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
                            if (r_range >= 0.f)
                            {
                                float y_test = Item_P.y - r_range;
                                if (y_test > y)
                                    y = y_test;
                            }
                        }
                    }

                    // Skip if no valid terrain hit
                    if (y < vis_box.vMin.y)
                        continue;

                    Item_P.y = y;

                    // Scale (matching original exactly)
                    float scale = r_scale.randF(Dobj->m_fMinScale * 0.5f, Dobj->m_fMaxScale * 0.9f) * ps_current_detail_height;

                    // Rotation (consume random to match original sequence)
                    float yaw = r_yaw.randF(0.f, PI_MUL_2);

                    // Vis ID (matching original - use global Random, not r_selection)
                    u32 vis_id;
                    if (Dobj->m_Flags.is(DO_NO_WAVING))
                        vis_id = 0;
                    else
                    {
                        if (::Random.randI(0, 3) == 0)
                            vis_id = 2;
                        else
                            vis_id = 1;
                    }

                    // Create instance
                    InstanceData instance = {};
                    instance.pos = Item_P;
                    instance.scale = scale;
                    instance.rotation = yaw;
                    instance.hemi = DS.r_qclr(DS.c_hemi, 15);
                    instance.vis_id = vis_id;
                    instance.object_id = object_id;

                    if (instance.object_id <= 7) // ONLY GRASS (id 8-16 used for leaves and ground/water details?)
                        all_instances.push_back(instance);
                }
            }

            // Build AABB for this slot with actual instance count
            u32 slot_instance_count = all_instances.size() - instance_base;
            if (slot_instance_count > 0)
            {
                SlotAABB aabb = {};
                aabb.aabb_min.set(vis_box.vMin.x, vis_box.vMin.y, vis_box.vMin.z);
                aabb.aabb_max.set(vis_box.vMax.x, vis_box.vMax.y + 2.0f, vis_box.vMax.z);
                aabb.instance_base = instance_base;
                aabb.instance_count = slot_instance_count;
                aabb.slot_x = sx;
                aabb.slot_z = sz;

                slot_aabbs.push_back(aabb);
                slot_count++;
            }
        }
    }

    slots_chunk->close();

    total_instance_count = all_instances.size();

    Msg("* [FGDetailManager] Decompressed %u instances in %u slots (%.2f MB)",
        total_instance_count, slot_count,
        (total_instance_count * sizeof(InstanceData)) / (1024.f * 1024.f));
}

bool FGDetailManager::CreateGPUBuffers(nvrhi::IDevice* device)
{
    ZoneScoped;

    if (!device)
    {
        Msg("! [FGDetailManager] CreateGPUBuffers: device is null");
        return false;
    }

    if (total_instance_count == 0)
    {
        Msg("! [FGDetailManager] CreateGPUBuffers: no instances (call DecompressAllSlots first)");
        return false;
    }

    // Capacity for visible buffer (same as total - worst case all visible)
    u32 visibleBufferCapacity = total_instance_count;

    // === Create GPU buffers ===

    // Instance buffer (all decompressed instances)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = total_instance_count * sizeof(InstanceData);
        desc.structStride = sizeof(InstanceData);
        desc.debugName = "DetailInstanceBuffer";
        desc.canHaveUAVs = false;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        instanceBuffer = device->createBuffer(desc);
        if (!instanceBuffer)
        {
            Msg("! [FGDetailManager] Failed to create instance buffer");
            return false;
        }
    }

    // Visible instances buffer (output from culling)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = visibleBufferCapacity * sizeof(InstanceData);
        desc.structStride = sizeof(InstanceData);
        desc.debugName = "DetailVisibleInstances";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        visibleIndicesBuffer = device->createBuffer(desc);
        if (!visibleIndicesBuffer)
        {
            Msg("! [FGDetailManager] Failed to create visible instances buffer");
            return false;
        }
    }

    // Visible count buffer
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.structStride = 0;
        desc.debugName = "DetailVisibleCount";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        visibleCountBuffer = device->createBuffer(desc);
        if (!visibleCountBuffer)
        {
            Msg("! [FGDetailManager] Failed to create visible count buffer");
            return false;
        }
    }

    // Draw args buffer (indirect draw arguments)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = 5 * sizeof(u32);  // DrawIndexedIndirectArgs
        desc.structStride = 0;
        desc.debugName = "DetailDrawArgs";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        drawArgsBuffer = device->createBuffer(desc);
        if (!drawArgsBuffer)
        {
            Msg("! [FGDetailManager] Failed to create draw args buffer");
            return false;
        }
    }

    // Slot AABB buffer
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = slot_aabbs.size() * sizeof(SlotAABB);
        desc.structStride = sizeof(SlotAABB);
        desc.debugName = "DetailSlotAABBs";
        desc.canHaveUAVs = false;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        slotAABBBuffer = device->createBuffer(desc);
        if (!slotAABBBuffer)
        {
            Msg("! [FGDetailManager] Failed to create slot AABB buffer");
            return false;
        }
    }

    // Visible slot IDs buffer (for virtual texturing)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = 8192 * sizeof(u32);  // MAX_VISIBLE_SLOTS
        desc.structStride = sizeof(u32);
        desc.debugName = "DetailVisibleSlotIDs";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        visibleSlotIDsBuffer = device->createBuffer(desc);
        if (!visibleSlotIDsBuffer)
        {
            Msg("! [FGDetailManager] Failed to create visible slot IDs buffer");
            return false;
        }
    }

    // Visible slot counter buffer
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.structStride = 0;
        desc.debugName = "DetailVisibleSlotCounter";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        visibleSlotCounterBuffer = device->createBuffer(desc);
        if (!visibleSlotCounterBuffer)
        {
            Msg("! [FGDetailManager] Failed to create visible slot counter buffer");
            return false;
        }
    }

    // Generate blade geometry
    xr_vector<BladeVertex> bladeVertices;
    xr_vector<u16> bladeIndices;
    GenerateBladeGeometry(bladeVertices, bladeIndices, 8);

    bladeVertexCount = bladeVertices.size();
    bladeIndexCount = bladeIndices.size();

    // Blade vertex buffer
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = bladeVertices.size() * sizeof(BladeVertex);
        desc.structStride = 0;
        desc.debugName = "DetailBladeVB";
        desc.canHaveUAVs = false;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = true;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;

        bladeVertexBuffer = device->createBuffer(desc);
        if (!bladeVertexBuffer)
        {
            Msg("! [FGDetailManager] Failed to create blade vertex buffer");
            return false;
        }
    }

    // Blade index buffer
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = bladeIndices.size() * sizeof(u16);
        desc.structStride = 0;
        desc.debugName = "DetailBladeIB";
        desc.canHaveUAVs = false;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = true;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::IndexBuffer;
        desc.keepInitialState = true;

        bladeIndexBuffer = device->createBuffer(desc);
        if (!bladeIndexBuffer)
        {
            Msg("! [FGDetailManager] Failed to create blade index buffer");
            return false;
        }
    }

    Msg("* [FGDetailManager] GPU buffers created: %.2f MB", (total_instance_count * sizeof(InstanceData)) / (1024.f * 1024.f));

    return true;
}

void FGDetailManager::DestroyGPUBuffers()
{
    instanceBuffer = nullptr;
    visibleIndicesBuffer = nullptr;
    visibleCountBuffer = nullptr;
    drawArgsBuffer = nullptr;
    slotAABBBuffer = nullptr;
    visibleSlotIDsBuffer = nullptr;
    visibleSlotCounterBuffer = nullptr;
    bladeVertexBuffer = nullptr;
    bladeIndexBuffer = nullptr;
    computePipeline = nullptr;
    graphicsPipeline = nullptr;
    computeBindingLayout = nullptr;
    graphicsBindingLayout = nullptr;
    cullComputeShader = nullptr;

    // Wind system cleanup
    windTexture = nullptr;
    windComputeShader = nullptr;
    windBindingLayout = nullptr;
    windPipeline = nullptr;
}

// ═══════════════════════════════════════════════════════
//  WIND SYSTEM
// ═══════════════════════════════════════════════════════

bool FGDetailManager::CreateWindTexture(nvrhi::IDevice* device)
{
    if (!device)
        return false;

    // Create 512x512 wind texture with UAV for compute shader output
    nvrhi::TextureDesc texDesc;
    texDesc.width = WIND_TEXTURE_SIZE;
    texDesc.height = WIND_TEXTURE_SIZE;
    texDesc.depth = 1;
    texDesc.arraySize = 1;
    texDesc.mipLevels = 1;
    texDesc.format = nvrhi::Format::RGBA16_FLOAT;  // Wind direction (RG) + unused (B) + strength (A)
    texDesc.dimension = nvrhi::TextureDimension::Texture2D;
    texDesc.isUAV = true;  // For compute shader write
    texDesc.isShaderResource = true;  // For vertex shader read
    texDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    texDesc.keepInitialState = false;
    texDesc.debugName = "DetailWindTexture";

    windTexture = device->createTexture(texDesc);
    if (!windTexture)
    {
        Msg("! [FGDetailManager] Failed to create wind texture");
        return false;
    }

    // Register in bindless descriptor heap for vertex shader access
    if (GEnv.Backend)
    {
        windTextureBindlessIndex = GEnv.Backend->RegisterBindlessTexture(windTexture);
        if (windTextureBindlessIndex == 0)
        {
            Msg("! [FGDetailManager] Failed to register wind texture in bindless heap");
        }
    }

    return true;
}

bool FGDetailManager::LoadWindComputeShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
        return false;

    windComputeShader = shaderLoader->LoadComputeShader("detail_wind_fbm", "main").handle;
    if (!windComputeShader)
    {
        Msg("! [FGDetailManager] Failed to load detail_wind_fbm compute shader");
        return false;
    }

    return true;
}

bool FGDetailManager::CreateWindPipeline(nvrhi::IDevice* device)
{
    if (!device || !windComputeShader)
        return false;

    // Create binding layout for wind compute shader
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Compute;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // b0: WindParams
        nvrhi::BindingLayoutItem::Texture_UAV(0),             // u0: g_wind_output
    };

    windBindingLayout = device->createBindingLayout(layoutDesc);
    if (!windBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create wind binding layout");
        return false;
    }

    nvrhi::ComputePipelineDesc pipelineDesc;
    pipelineDesc.CS = windComputeShader;
    pipelineDesc.bindingLayouts = { windBindingLayout };

    windPipeline = device->createComputePipeline(pipelineDesc);
    if (!windPipeline)
    {
        Msg("! [FGDetailManager] Failed to create wind compute pipeline");
        return false;
    }

    return true;
}

void FGDetailManager::DispatchWindCompute(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, float time)
{
    if (!windPipeline || !windTexture)
        return;

    // WindParams must match HLSL cbuffer
    struct WindParams
    {
        float time;              // 0-3
        Fvector2 wind_direction; // 4-11
        float wind_speed;        // 12-15
        u32 octaves;             // 16-19
        float lacunarity;        // 20-23
        float gain;              // 24-27
        float padding1;          // 28-31
        Fvector2 scroll_speed;   // 32-39
        u32 texture_size;        // 40-43
        u32 pad[3];              // 44-55
    };

    // Get wind parameters from environment (matching original detail.diff logic)
    if (g_pGamePersistent)
    {
        // Use ps_r3_grass_wind_multiplier and ps_r3_grass_wind_min for wind speed calculation
        windSpeed = _max(g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier, ps_r3_grass_wind_min);
        float wind_rad = deg2rad(g_pGamePersistent->Environment().CurrentEnv.wind_direction);
        windDirection.set(cosf(wind_rad), sinf(wind_rad));
    }

    WindParams params = {};
    params.time = time;
    params.wind_direction = windDirection;
    params.wind_speed = windSpeed;
    params.octaves = ps_r3_grass_wind_octaves;  // Use tunable octave count
    params.lacunarity = 2.0f;
    params.gain = 0.5f;
    params.scroll_speed.set(windDirection.x * windSpeed * 5.0f, windDirection.y * windSpeed * 5.0f);
    params.texture_size = WIND_TEXTURE_SIZE;

    // Create volatile constant buffer
    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(WindParams);
    cbDesc.debugName = "WindParams";
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;

    nvrhi::BufferHandle windParamsCB = device->createBuffer(cbDesc);
    cmdList->writeBuffer(windParamsCB, &params, sizeof(params));

    // Track texture state
    cmdList->beginTrackingTextureState(windTexture, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);

    // Create binding set
    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, windParamsCB),
        nvrhi::BindingSetItem::Texture_UAV(0, windTexture),
    };

    nvrhi::BindingSetHandle windBindingSet = device->createBindingSet(bindDesc, windBindingLayout);

    // Dispatch compute
    nvrhi::ComputeState state;
    state.pipeline = windPipeline;
    state.bindings = { windBindingSet };
    cmdList->setComputeState(state);

    // 16x16 thread groups to cover 512x512 texture
    u32 numGroups = (WIND_TEXTURE_SIZE + 15) / 16;
    cmdList->dispatch(numGroups, numGroups, 1);

    // Transition to shader resource for vertex shader read
    cmdList->setTextureState(windTexture, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
}

void FGDetailManager::GenerateBladeGeometry(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments)
{
    vertices.clear();
    indices.clear();

    float width_base = 0.05f;
    float width_tip = 0.01f;
    float height = 1.0f;

    for (int i = 0; i <= segments; i++)
    {
        float t = float(i) / float(segments);
        float y = t * height;
        float width = width_base + (width_tip - width_base) * t;

        BladeVertex v_left;
        v_left.pos.set(-width * 0.5f, y, 0.0f);
        v_left.uv.set(0.0f, 1.0f - t);
        v_left.t = t;
        v_left.width_scale = width;
        vertices.push_back(v_left);

        BladeVertex v_right;
        v_right.pos.set(width * 0.5f, y, 0.0f);
        v_right.uv.set(1.0f, 1.0f - t);
        v_right.t = t;
        v_right.width_scale = width;
        vertices.push_back(v_right);
    }

    for (int i = 0; i < segments; i++)
    {
        u16 base = i * 2;
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
}

void FGDetailManager::ComputeSlotAABBs()
{
    // AABBs are now built during DecompressAllSlots for accuracy
    // AABBs already built during decompression
}

bool FGDetailManager::LoadCullComputeShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
    {
        Msg("! [FGDetailManager] LoadCullComputeShader: shaderLoader is null");
        return false;
    }

    cullComputeShader = shaderLoader->LoadComputeShader("detail_cull", "main").handle;
    if (!cullComputeShader)
    {
        Msg("! [FGDetailManager] Failed to load detail_cull.cs");
        return false;
    }
    return true;
}


bool FGDetailManager::LoadGraphicsShaders(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
    {
        Msg("! [FGDetailManager] LoadGraphicsShaders: shaderLoader is null");
        return false;
    }

    // Load vertex shader
    auto vsResult = shaderLoader->LoadVertexShader("detail_gpu", "main");
    if (!vsResult.handle)
    {
        Msg("! [FGDetailManager] Failed to load detail_gpu.vs");
        return false;
    }
    vertexShader = vsResult.handle;

    // Load pixel shader
    auto psResult = shaderLoader->LoadPixelShader("detail_gpu", "main");
    if (!psResult.handle)
    {
        Msg("! [FGDetailManager] Failed to load detail_gpu.ps");
        return false;
    }
    pixelShader = psResult.handle;
    return true;
}

void FGDetailManager::UploadBufferData(nvrhi::ICommandList* cmdList)
{
    if (!cmdList || total_instance_count == 0)
        return;

    // Upload instance data
    cmdList->writeBuffer(instanceBuffer, all_instances.data(), all_instances.size() * sizeof(InstanceData));

    // Upload blade geometry
    xr_vector<BladeVertex> bladeVertices;
    xr_vector<u16> bladeIndices;
    GenerateBladeGeometry(bladeVertices, bladeIndices, 8);
    cmdList->writeBuffer(bladeVertexBuffer, bladeVertices.data(), bladeVertices.size() * sizeof(BladeVertex));
    cmdList->writeBuffer(bladeIndexBuffer, bladeIndices.data(), bladeIndices.size() * sizeof(u16));

    // Upload slot AABBs
    cmdList->writeBuffer(slotAABBBuffer, slot_aabbs.data(), slot_aabbs.size() * sizeof(SlotAABB));
}

bool FGDetailManager::CreateComputePipeline(ng::RenderDevice* renderDevice)
{
    if (!renderDevice || !cullComputeShader)
    {
        Msg("! [FGDetailManager] CreateComputePipeline: invalid parameters");
        return false;
    }

    nvrhi::IDevice* device = renderDevice->GetNVRHIDevice();

    // Create binding layout for compute shader
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Compute;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // b5: DetailCullParams
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),    // t0: slot AABBs
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1),    // t1: all instances
        nvrhi::BindingLayoutItem::Texture_SRV(2),             // t2: Hi-Z pyramid
        nvrhi::BindingLayoutItem::Sampler(0),                 // s0: point sampler
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),    // u0: visible instances
        nvrhi::BindingLayoutItem::RawBuffer_UAV(1),           // u1: visible count
        nvrhi::BindingLayoutItem::RawBuffer_UAV(2),           // u2: indirect args
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(33),   // u33: visible slot IDs
        nvrhi::BindingLayoutItem::RawBuffer_UAV(34),          // u34: visible slot counter
    };

    computeBindingLayout = device->createBindingLayout(layoutDesc);
    if (!computeBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create compute binding layout");
        return false;
    }

    nvrhi::ComputePipelineDesc pipelineDesc;
    pipelineDesc.CS = cullComputeShader;
    pipelineDesc.bindingLayouts = { computeBindingLayout };

    computePipeline = device->createComputePipeline(pipelineDesc);
    if (!computePipeline)
    {
        Msg("! [FGDetailManager] Failed to create compute pipeline");
        return false;
    }
    return true;
}

bool FGDetailManager::CreateGraphicsPipeline(ng::RenderDevice* renderDevice, nvrhi::IFramebuffer* framebuffer)
{
    if (!renderDevice || !framebuffer || !vertexShader || !pixelShader)
    {
        Msg("! [FGDetailManager] CreateGraphicsPipeline: invalid parameters");
        return false;
    }

    nvrhi::IDevice* device = renderDevice->GetNVRHIDevice();

    // Create binding layout for graphics shaders
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // b0: dynamic_transforms
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(1),  // b1: shader_params
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // b2: static_globals
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(3),  // b3: $Globals (detail params)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // b4: dynamic_light
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // t8: g_Materials
        nvrhi::BindingLayoutItem::TypedBuffer_SRV(32),        // t32: slot_indirection (dummy)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(33),   // t33: visible instances
        nvrhi::BindingLayoutItem::Sampler(0),                 // s0: g_LinearSampler
        nvrhi::BindingLayoutItem::Sampler(1),                 // s1: smp_nofilter
        nvrhi::BindingLayoutItem::Sampler(2),                 // s2: smp_rtlinear
        nvrhi::BindingLayoutItem::Sampler(3),                 // s3: smp_linear
        nvrhi::BindingLayoutItem::Sampler(4),                 // s4: smp_base
        nvrhi::BindingLayoutItem::Sampler(5),                 // s5: smp_material
    };

    graphicsBindingLayout = device->createBindingLayout(layoutDesc);
    if (!graphicsBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create graphics binding layout");
        return false;
    }

    // Create input layout for blade vertex buffer
    // Must match v_blade_sdf in detail_gpu.vs:
    //   float3 pos : POSITION;
    //   float2 tc : TEXCOORD;
    //   float t : COLOR0;
    //   float width_scale : COLOR1;
    // Note: COLOR0/COLOR1 use setArraySize(2) since they have the same format
    nvrhi::VertexAttributeDesc attributes[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(0)
            .setElementStride(sizeof(BladeVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(12)
            .setElementStride(sizeof(BladeVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::R32_FLOAT)
            .setArraySize(2)  // COLOR0 at offset 20, COLOR1 at offset 24
            .setOffset(20)
            .setElementStride(sizeof(BladeVertex)),
    };

    inputLayout = device->createInputLayout(attributes, 3, vertexShader);
    if (!inputLayout)
    {
        Msg("! [FGDetailManager] Failed to create input layout");
        return false;
    }

    // Create graphics pipeline
    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.VS = vertexShader;
    pipelineDesc.PS = pixelShader;
    pipelineDesc.inputLayout = inputLayout;
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

    // Add binding layouts - regular layout + bindless layout for SM6.6
    pipelineDesc.bindingLayouts.push_back(graphicsBindingLayout);
    auto* backend = renderDevice->GetBackend();
    if (backend) {
        auto* bindlessLayout = backend->GetBindlessLayout();
        if (bindlessLayout) {
            pipelineDesc.bindingLayouts.push_back(bindlessLayout);
        }
    }

    // Rasterizer state
    pipelineDesc.renderState.rasterState.fillMode = nvrhi::RasterFillMode::Solid;
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;  // Grass is double-sided

    // Depth state
    pipelineDesc.renderState.depthStencilState.depthTestEnable = true;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = true;
    pipelineDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;

    // Blend state (alpha test handled in shader)
    pipelineDesc.renderState.blendState.targets[0].disableBlend();

    graphicsPipeline = device->createGraphicsPipeline(pipelineDesc, framebuffer);
    if (!graphicsPipeline)
    {
        Msg("! [FGDetailManager] Failed to create graphics pipeline");
        return false;
    }
    return true;
}

void FGDetailManager::DispatchCulling(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* device,
    nvrhi::ITexture* hiZPyramid,
    const Fmatrix& viewProj,
    const Fvector4* frustumPlanes,
    u32 frustumPlaneCount,
    const Fvector& cameraPos,
    float fadeDistance,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels)
{
    if (!cullComputeShader || total_instance_count == 0 || slot_count == 0)
        return;

    // Create and fill constant buffer (must match HLSL DetailCullParams)
    struct DetailCullParams
    {
        Fmatrix viewProj;
        Fvector3 cameraPos;
        float fadeDistanceSqr;
        Fvector4 frustumPlanes[6];
        u32 totalInstanceCount;
        u32 totalSlotCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
        u32 pad[3];
    };

    DetailCullParams params;
    params.viewProj.transpose(viewProj);
    params.cameraPos = cameraPos;
    params.fadeDistanceSqr = fadeDistance * fadeDistance;

    for (u32 i = 0; i < 6; i++)
    {
        if (i < frustumPlaneCount)
            params.frustumPlanes[i] = frustumPlanes[i];  // Already Fvector4, direct copy
        else
            params.frustumPlanes[i].set(0, 0, 0, 1000000.0f);
    }

    params.totalInstanceCount = total_instance_count;
    params.totalSlotCount = slot_count;
    params.hizWidth = hiZWidth;
    params.hizHeight = hiZHeight;
    params.hizMipLevels = hiZMipLevels;
    params.pad[0] = params.pad[1] = params.pad[2] = 0;

    // Create constant buffer
    nvrhi::BufferDesc cbDesc;
    cbDesc.byteSize = sizeof(DetailCullParams);
    cbDesc.debugName = "DetailCullParams";
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;

    nvrhi::BufferHandle cullParamsCB = device->createBuffer(cbDesc);
    cmdList->writeBuffer(cullParamsCB, &params, sizeof(params));

    // Begin tracking buffer states
    cmdList->beginTrackingBufferState(visibleIndicesBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingBufferState(visibleCountBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingBufferState(drawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingBufferState(visibleSlotIDsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingBufferState(visibleSlotCounterBuffer, nvrhi::ResourceStates::UnorderedAccess);

    // Track Hi-Z texture state for compute shader read
    cmdList->beginTrackingTextureState(hiZPyramid, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

    // Clear counters
    u32 zero = 0;
    cmdList->clearBufferUInt(visibleCountBuffer, zero);
    cmdList->clearBufferUInt(visibleSlotCounterBuffer, zero);

    // Initialize draw args
    struct IndirectDrawArgs { u32 indexCount, instanceCount, startIndex; s32 baseVertex; u32 startInstance; };
    IndirectDrawArgs args = { bladeIndexCount, 0, 0, 0, 0 };
    cmdList->writeBuffer(drawArgsBuffer, &args, sizeof(args));

    if (!computePipeline)
        return;

    // Create point sampler for Hi-Z
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.minFilter = false;  // Point filter
    samplerDesc.magFilter = false;  // Point filter
    samplerDesc.mipFilter = false;
    samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
    samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
    samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
    nvrhi::SamplerHandle pointSampler = device->createSampler(samplerDesc);

    // Create binding set
    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(5, cullParamsCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(0, slotAABBBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(1, instanceBuffer),
        nvrhi::BindingSetItem::Texture_SRV(2, hiZPyramid),
        nvrhi::BindingSetItem::Sampler(0, pointSampler),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, visibleIndicesBuffer),
        nvrhi::BindingSetItem::RawBuffer_UAV(1, visibleCountBuffer),
        nvrhi::BindingSetItem::RawBuffer_UAV(2, drawArgsBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(33, visibleSlotIDsBuffer),
        nvrhi::BindingSetItem::RawBuffer_UAV(34, visibleSlotCounterBuffer),
    };

    nvrhi::BindingSetHandle cullBindingSet = device->createBindingSet(bindDesc, computeBindingLayout);

    // Dispatch
    nvrhi::ComputeState state;
    state.pipeline = computePipeline;
    state.bindings = { cullBindingSet };
    cmdList->setComputeState(state);

    const u32 threadGroupSize = 256;
    u32 numGroups = (slot_count + threadGroupSize - 1) / threadGroupSize;
    cmdList->dispatch(numGroups, 1, 1);

    // Transition for graphics
    cmdList->setBufferState(visibleIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(drawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
}

} // namespace xray::render::RENDER_NAMESPACE
