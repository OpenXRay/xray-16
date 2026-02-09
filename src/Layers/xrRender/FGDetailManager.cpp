#include "stdafx.h"
#include "FGDetailManager.h"
#include "DetailModel.h"
#include "FrameGraph/ShaderLoader.h"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "ResourceManager/TextureManager.h"
#include "xrRender_console.h"
#include "xrCDB/Intersect.hpp"
#include "xrCDB/xrXRC.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrCore/Profiler/Profiler.h"
#include "Profiler/GPUProfiler.h"
#include "FrameGraphPasses/ShaderConstants.h"
#include "ResourceManager/DDSLoader.h"
#include <thread>
#include <atomic>

extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r3_grass_wind_min;
extern ENGINE_API float ps_r3_grass_wind_lerp_rate;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;
extern ENGINE_API u32 ps_r3_grass_wind_octaves;

extern ENGINE_API float ps_r3_grass_lod_close;
extern ENGINE_API float ps_r3_grass_lod_mid;

extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;

namespace xray::render::RENDER_NAMESPACE
{

extern float ps_current_detail_height;

static int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};

static void bwdithermap(int levels, int magic[16][16])
{
    float N = 255.0f / (levels - 1);
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

    bwdithermap(2, dither);

    PackSlotData();

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

    slot_aabbs.clear();
    slotDataCPU.clear();

    if (dtFS)
    {
        FS.r_close(dtFS);
        dtFS = nullptr;
    }
}

bool FGDetailManager::BakeHeightmap()
{
    ZoneScoped;

    if (!dtFS)
    {
        Msg("! [FGDetailManager] BakeHeightmap: level.details not loaded");
        return false;
    }

    heightmapWidth = dtH.x_size() * HEIGHTMAP_TEXELS_PER_SLOT;
    heightmapHeight = dtH.z_size() * HEIGHTMAP_TEXELS_PER_SLOT;
    heightmapTexelSize = DETAIL_SLOT_SIZE / float(HEIGHTMAP_TEXELS_PER_SLOT);
    heightmapWorldMinX = -float(dtH.x_offs()) * DETAIL_SLOT_SIZE;
    heightmapWorldMinZ = -float(dtH.z_offs()) * DETAIL_SLOT_SIZE;

    Msg("* [FGDetailManager] Heightmap params: %ux%u pixels, texel=%.2fm, world origin=(%.1f, %.1f)",
        heightmapWidth, heightmapHeight, heightmapTexelSize, heightmapWorldMinX, heightmapWorldMinZ);

    string_path heightmap_path;
    FS.update_path(heightmap_path, "$level$", "level_heightmap.dds");

    if (FS.exist(heightmap_path))
    {
        Msg("* [FGDetailManager] Heightmap already exists: %s — skipping bake", heightmap_path);
        return true;
    }

    Msg("* [FGDetailManager] Baking heightmap: %ux%u pixels (%.1f MB)...",
        heightmapWidth, heightmapHeight,
        float(heightmapWidth) * heightmapHeight * sizeof(float) / (1024.f * 1024.f));

    CTimer bake_timer;
    bake_timer.Start();

    const u32 pixel_count = heightmapWidth * heightmapHeight;
    xr_vector<float> pixels(pixel_count, HEIGHTMAP_NO_TERRAIN);

    IReader* slots_chunk = dtFS->open_chunk(2);
    if (!slots_chunk)
    {
        Msg("! [FGDetailManager] BakeHeightmap: failed to read slots chunk");
        return false;
    }
    DetailSlot* dtSlots = (DetailSlot*)slots_chunk->pointer();

    u32 num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 8;
    if (num_threads > 16) num_threads = 16;

    const u32 total_slots = dtH.x_size() * dtH.z_size();
    std::atomic<u32> slots_completed{0};

    auto worker = [&](u32 slot_start, u32 slot_end)
    {
        thread_local xrXRC thread_xrc;

        CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
        Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

        for (u32 slot_idx = slot_start; slot_idx < slot_end; slot_idx++)
        {
            u32 db_x = slot_idx % dtH.x_size();
            u32 db_z = slot_idx / dtH.x_size();

            int sx = int(db_x) - dtH.x_offs();
            int sz = int(db_z) - dtH.z_offs();

            DetailSlot& DS = dtSlots[slot_idx];

            Fbox vis_box;
            vis_box.vMin.set(sx * DETAIL_SLOT_SIZE, DS.r_ybase(), sz * DETAIL_SLOT_SIZE);
            vis_box.vMax.set(vis_box.vMin.x + DETAIL_SLOT_SIZE,
                            DS.r_ybase() + DS.r_yheight(),
                            vis_box.vMin.z + DETAIL_SLOT_SIZE);
            vis_box.grow(EPS_L);

            Fvector bC, bD;
            vis_box.get_CD(bC, bD);
            thread_xrc.box_query(CDB::OPT_FULL_TEST,
                g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);

            const auto tri_count = thread_xrc.r_count();
            if (tri_count == 0)
                continue;

            Fvector ray_dir;
            ray_dir.set(0.f, -1.f, 0.f);

            for (u32 local_z = 0; local_z < HEIGHTMAP_TEXELS_PER_SLOT; local_z++)
            {
                for (u32 local_x = 0; local_x < HEIGHTMAP_TEXELS_PER_SLOT; local_x++)
                {
                    u32 tx = db_x * HEIGHTMAP_TEXELS_PER_SLOT + local_x;
                    u32 tz = db_z * HEIGHTMAP_TEXELS_PER_SLOT + local_z;

                    float world_x = heightmapWorldMinX + (float(tx) + 0.5f) * heightmapTexelSize;
                    float world_z = heightmapWorldMinZ + (float(tz) + 0.5f) * heightmapTexelSize;

                    Fvector ray_origin;
                    ray_origin.set(world_x, vis_box.vMax.y, world_z);

                    float best_y = FLT_MAX;

                    for (size_t tid = 0; tid < tri_count; tid++)
                    {
                        CDB::TRI& T = tris[thread_xrc.r_begin()[tid].id];
                        SGameMtl* mtl = GMLib.GetMaterialByIdx(T.material);
                        if (mtl->Flags.test(SGameMtl::flPassable))
                            continue;

                        Fvector Tv[3] = {verts[T.verts[0]], verts[T.verts[1]], verts[T.verts[2]]};

                        Fvector tri_normal;
                        tri_normal.mknormal(Tv[0], Tv[1], Tv[2]);
                        if (tri_normal.y < 0.7f)
                            continue;

                        float r_u, r_v, r_range;
                        if (CDB::TestRayTri(ray_origin, ray_dir, Tv, r_u, r_v, r_range, TRUE))
                        {
                            if (r_range >= 0.f)
                            {
                                float hit_y = ray_origin.y - r_range;
                                if (hit_y >= vis_box.vMin.y && hit_y < best_y)
                                    best_y = hit_y;
                            }
                        }
                    }

                    if (best_y < FLT_MAX)
                        pixels[tz * heightmapWidth + tx] = best_y;
                }
            }

            u32 done = slots_completed.fetch_add(1) + 1;
            if (done % 5000 == 0)
            {
                Msg("  [Heightmap] %u / %u slots (%.0f%%)", done, total_slots,
                    100.f * done / total_slots);
            }
        }
    };

    xr_vector<std::thread> workers;
    workers.reserve(num_threads);
    u32 slots_per_thread = total_slots / num_threads;
    u32 remainder = total_slots % num_threads;

    u32 current_slot = 0;
    for (u32 i = 0; i < num_threads; i++)
    {
        u32 slot_end = current_slot + slots_per_thread + (i < remainder ? 1 : 0);
        if (current_slot < total_slots)
            workers.emplace_back(worker, current_slot, slot_end);
        current_slot = slot_end;
    }

    for (auto& t : workers)
        t.join();

    slots_chunk->close();

    float bake_time = bake_timer.GetElapsed_sec();
    Msg("* [FGDetailManager] Heightmap bake complete: %.2f sec (%u threads)", bake_time, num_threads);

    u32 valid_count = 0;
    float min_y = FLT_MAX, max_y = -FLT_MAX;
    for (u32 i = 0; i < pixel_count; i++)
    {
        if (pixels[i] > HEIGHTMAP_NO_TERRAIN)
        {
            valid_count++;
            if (pixels[i] < min_y) min_y = pixels[i];
            if (pixels[i] > max_y) max_y = pixels[i];
        }
    }
    Msg("  - Valid texels: %u / %u (%.1f%%)", valid_count, pixel_count,
        100.f * valid_count / pixel_count);
    Msg("  - Height range: [%.2f, %.2f] meters", min_y, max_y);

    VerifyPath(heightmap_path);
    IWriter* writer = FS.w_open(heightmap_path);
    if (!writer)
    {
        Msg("! [FGDetailManager] Failed to open heightmap for writing: %s", heightmap_path);
        return false;
    }

    u32 magic = 0x20534444;
    writer->w(&magic, sizeof(magic));

    xray::render::resources::DDS_HEADER header = {};
    header.dwSize = 124;
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_MIPMAPCOUNT;
    header.dwHeight = heightmapHeight;
    header.dwWidth = heightmapWidth;
    header.dwPitchOrLinearSize = heightmapWidth * sizeof(float);
    header.dwMipMapCount = 1;
    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_FOURCC;
    header.ddspf.dwFourCC = FOURCC_DX10;
    header.dwCaps = DDSCAPS_TEXTURE;
    writer->w(&header, sizeof(header));

    xray::render::resources::DDS_HEADER_DX10 header10 = {};
    header10.dxgiFormat = xray::render::resources::DXGI_FORMAT_R32_FLOAT;
    header10.resourceDimension = xray::render::resources::D3D10_RESOURCE_DIMENSION_TEXTURE2D;
    header10.arraySize = 1;
    writer->w(&header10, sizeof(header10));

    writer->w(pixels.data(), pixel_count * sizeof(float));

    FS.w_close(writer);

    u32 file_bytes = 4 + sizeof(header) + sizeof(header10) + pixel_count * sizeof(float);
    Msg("* [FGDetailManager] Heightmap saved: %s (%.1f MB)", heightmap_path,
        float(file_bytes) / (1024.f * 1024.f));

    {
        string_path preview_path;
        FS.update_path(preview_path, "$level$", "level_heightmap_preview.dds");

        IWriter* pw = FS.w_open(preview_path);
        if (pw)
        {
            xr_vector<u8> preview(pixel_count, 0);
            if (valid_count > 0 && max_y > min_y)
            {
                float range = max_y - min_y;
                for (u32 i = 0; i < pixel_count; i++)
                {
                    if (pixels[i] > HEIGHTMAP_NO_TERRAIN)
                    {
                        float norm = (pixels[i] - min_y) / range;
                        clamp(norm, 0.f, 1.f);
                        preview[i] = u8(norm * 255.f);
                    }
                }
            }

            u32 pm = 0x20534444;
            pw->w(&pm, sizeof(pm));

            xray::render::resources::DDS_HEADER ph = {};
            ph.dwSize = 124;
            ph.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH;
            ph.dwHeight = heightmapHeight;
            ph.dwWidth = heightmapWidth;
            ph.dwPitchOrLinearSize = heightmapWidth;
            ph.ddspf.dwSize = 32;
            ph.ddspf.dwFlags = DDPF_LUMINANCE;
            ph.ddspf.dwRGBBitCount = 8;
            ph.ddspf.dwRBitMask = 0xFF;
            ph.dwCaps = DDSCAPS_TEXTURE;
            pw->w(&ph, sizeof(ph));

            pw->w(preview.data(), pixel_count);
            FS.w_close(pw);

            Msg("* [FGDetailManager] Preview saved: %s (%.1f MB)", preview_path,
                float(4 + sizeof(ph) + pixel_count) / (1024.f * 1024.f));
        }
    }

    return true;
}

bool FGDetailManager::LoadHeightmapTexture(nvrhi::IDevice* device)
{
    ZoneScoped;

    if (!device)
    {
        Msg("! [FGDetailManager] LoadHeightmapTexture: device is null");
        return false;
    }

    string_path heightmap_path;
    FS.update_path(heightmap_path, "$level$", "level_heightmap.dds");

    if (!FS.exist(heightmap_path))
    {
        Msg("! [FGDetailManager] Heightmap file not found: %s", heightmap_path);
        return false;
    }

    Msg("* [FGDetailManager] Loading heightmap: %s", heightmap_path);

    resources::DDSData ddsData;
    if (!resources::DDSLoader::LoadFromFile("level_heightmap", ddsData))
    {
        Msg("! [FGDetailManager] Failed to load heightmap DDS (searched in $level$)");
        return false;
    }

    if (!ddsData.isValid || ddsData.mipLevels.empty())
    {
        Msg("! [FGDetailManager] Invalid heightmap DDS data");
        return false;
    }

    if (ddsData.desc.format != nvrhi::Format::R32_FLOAT)
    {
        Msg("! [FGDetailManager] Unexpected heightmap format: expected R32_FLOAT, got %d", (int)ddsData.desc.format);
        return false;
    }

    if (ddsData.desc.width != heightmapWidth || ddsData.desc.height != heightmapHeight)
    {
        Msg("! [FGDetailManager] Heightmap dimension mismatch: expected %ux%u, got %ux%u",
            heightmapWidth, heightmapHeight, ddsData.desc.width, ddsData.desc.height);
        return false;
    }

    nvrhi::TextureDesc texDesc;
    texDesc.width = ddsData.desc.width;
    texDesc.height = ddsData.desc.height;
    texDesc.depth = 1;
    texDesc.arraySize = 1;
    texDesc.mipLevels = ddsData.desc.mipLevels;
    texDesc.format = ddsData.desc.format;
    texDesc.dimension = nvrhi::TextureDimension::Texture2D;
    texDesc.isRenderTarget = false;
    texDesc.isUAV = false;
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;
    texDesc.debugName = "DetailHeightmap";

    heightmapTexture = device->createTexture(texDesc);
    if (!heightmapTexture)
    {
        Msg("! [FGDetailManager] Failed to create heightmap texture");
        return false;
    }

    nvrhi::CommandListHandle cmdList = device->createCommandList();
    cmdList->open();

    for (u32 mip = 0; mip < ddsData.mipLevels.size(); mip++)
    {
        const auto& mipLevel = ddsData.mipLevels[mip];
        cmdList->writeTexture(heightmapTexture, 0, mip, mipLevel.data, mipLevel.rowPitch);
    }

    cmdList->close();
    device->executeCommandList(cmdList);

    Msg("  ✓ Heightmap texture loaded: %ux%u R32_FLOAT, %u mips",
        texDesc.width, texDesc.height, texDesc.mipLevels);

    return true;
}

bool FGDetailManager::LoadBuildDetailsTexture(nvrhi::IDevice* device)
{
    ZoneScoped;

    if (!device)
        return false;

    string_path path;
    FS.update_path(path, "$level$", "build_details.dds");

    if (!FS.exist(path))
    {
        Msg("! [FGDetailManager] build_details.dds not found: %s", path);
        return false;
    }

    resources::DDSData ddsData;
    if (!resources::DDSLoader::LoadFromFile("build_details", ddsData) || !ddsData.isValid || ddsData.mipLevels.empty())
    {
        Msg("! [FGDetailManager] Failed to load build_details.dds");
        return false;
    }

    nvrhi::TextureDesc texDesc;
    texDesc.width = ddsData.desc.width;
    texDesc.height = ddsData.desc.height;
    texDesc.depth = 1;
    texDesc.arraySize = 1;
    texDesc.mipLevels = ddsData.desc.mipLevels;
    texDesc.format = ddsData.desc.format;
    texDesc.dimension = nvrhi::TextureDimension::Texture2D;
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;
    texDesc.debugName = "BuildDetails";

    buildDetailsTexture = device->createTexture(texDesc);
    if (!buildDetailsTexture)
    {
        Msg("! [FGDetailManager] Failed to create build_details texture");
        return false;
    }

    pendingBuildDetailsUploads.resize(ddsData.mipLevels.size());
    for (u32 mip = 0; mip < ddsData.mipLevels.size(); mip++)
    {
        const auto& ml = ddsData.mipLevels[mip];
        pendingBuildDetailsUploads[mip].data.assign(ml.data, ml.data + ml.size);
        pendingBuildDetailsUploads[mip].rowPitch = ml.rowPitch;
    }

    if (GEnv.Backend)
    {
        buildDetailsBindlessIndex = GEnv.Backend->RegisterBindlessTexture(buildDetailsTexture);
        Msg("* [FGDetailManager] build_details.dds loaded: %ux%u, %u mips, format=%d, bindless=%u",
            texDesc.width, texDesc.height, texDesc.mipLevels, (int)texDesc.format, buildDetailsBindlessIndex);
    }

    resources::DDSData pbrData;
    if (resources::DDSLoader::LoadFromFile("build_details_pbr", pbrData) && pbrData.isValid && !pbrData.mipLevels.empty())
    {
        nvrhi::TextureDesc pbrDesc;
        pbrDesc.width = pbrData.desc.width;
        pbrDesc.height = pbrData.desc.height;
        pbrDesc.depth = 1;
        pbrDesc.arraySize = 1;
        pbrDesc.mipLevels = pbrData.desc.mipLevels;
        pbrDesc.format = pbrData.desc.format;
        pbrDesc.dimension = nvrhi::TextureDimension::Texture2D;
        pbrDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        pbrDesc.keepInitialState = true;
        pbrDesc.debugName = "BuildDetailsPBR";

        buildDetailsPbrTexture = device->createTexture(pbrDesc);
        if (buildDetailsPbrTexture)
        {
            pendingBuildDetailsPbrUploads.resize(pbrData.mipLevels.size());
            for (u32 mip = 0; mip < pbrData.mipLevels.size(); mip++)
            {
                const auto& ml = pbrData.mipLevels[mip];
                pendingBuildDetailsPbrUploads[mip].data.assign(ml.data, ml.data + ml.size);
                pendingBuildDetailsPbrUploads[mip].rowPitch = ml.rowPitch;
            }

            if (GEnv.Backend)
            {
                buildDetailsPbrBindlessIndex = GEnv.Backend->RegisterBindlessTexture(buildDetailsPbrTexture);
                Msg("* [FGDetailManager] build_details_pbr.dds loaded: %ux%u, %u mips, format=%d, bindless=%u",
                    pbrDesc.width, pbrDesc.height, pbrDesc.mipLevels, (int)pbrDesc.format, buildDetailsPbrBindlessIndex);
            }
        }
    }

    return true;
}

void FGDetailManager::PackSlotData()
{
    ZoneScoped;

    if (!dtFS)
    {
        Msg("! [FGDetailManager] PackSlotData: level.details not loaded");
        return;
    }

    IReader* slots_chunk = dtFS->open_chunk(2);
    if (!slots_chunk)
    {
        Msg("! [FGDetailManager] PackSlotData: failed to read slots chunk");
        return;
    }

    DetailSlot* dtSlots = (DetailSlot*)slots_chunk->pointer();
    u32 total_slots = dtH.x_size() * dtH.z_size();

    Msg("* [FGDetailManager] Packing %dx%d detail slots...", dtH.x_size(), dtH.z_size());

    slotDataCPU.clear();
    slotDataCPU.resize(total_slots);

    for (u32 db_z = 0; db_z < dtH.z_size(); db_z++)
    {
        for (u32 db_x = 0; db_x < dtH.x_size(); db_x++)
        {
            int sx = int(db_x) - dtH.x_offs();
            int sz = int(db_z) - dtH.z_offs();
            u32 slot_idx = db_z * dtH.x_size() + db_x;

            DetailSlot& src = dtSlots[slot_idx];
            GPUSlotData& dst = slotDataCPU[slot_idx];

            dst.world_min_x = sx * DETAIL_SLOT_SIZE;
            dst.world_min_z = sz * DETAIL_SLOT_SIZE;

            dst.y_base = src.r_ybase();
            dst.y_height = src.r_yheight();

            dst.packed_ids = (u32(src.id0) << 0) |
                             (u32(src.id1) << 8) |
                             (u32(src.id2) << 16) |
                             (u32(src.id3) << 24);

            dst.packed_palette_01 =
                (u32(src.palette[0].a0) << 0)  | (u32(src.palette[0].a1) << 4)  |
                (u32(src.palette[0].a2) << 8)  | (u32(src.palette[0].a3) << 12) |
                (u32(src.palette[1].a0) << 16) | (u32(src.palette[1].a1) << 20) |
                (u32(src.palette[1].a2) << 24) | (u32(src.palette[1].a3) << 28);

            dst.packed_palette_23 =
                (u32(src.palette[2].a0) << 0)  | (u32(src.palette[2].a1) << 4)  |
                (u32(src.palette[2].a2) << 8)  | (u32(src.palette[2].a3) << 12) |
                (u32(src.palette[3].a0) << 16) | (u32(src.palette[3].a1) << 20) |
                (u32(src.palette[3].a2) << 24) | (u32(src.palette[3].a3) << 28);

            dst.hemi = src.r_qclr(src.c_hemi, 15);
        }
    }

    slots_chunk->close();

    float size_mb = float(slotDataCPU.size() * sizeof(GPUSlotData)) / (1024.f * 1024.f);
    Msg("* [FGDetailManager] Packed %u slots (%.2f MB)", slotDataCPU.size(), size_mb);
}

bool FGDetailManager::CreateGPUBuffers(nvrhi::IDevice* device)
{
    ZoneScoped;

    if (!device)
    {
        Msg("! [FGDetailManager] CreateGPUBuffers: device is null");
        return false;
    }

    Msg("* [FGDetailManager] Creating GPU buffers");
    if (!slotDataCPU.empty())
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = slotDataCPU.size() * sizeof(GPUSlotData);
        desc.structStride = sizeof(GPUSlotData);
        desc.debugName = "DetailSlotData";
        desc.canHaveUAVs = false;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        slotDataBuffer = device->createBuffer(desc);
        if (!slotDataBuffer)
        {
            Msg("! [FGDetailManager] Failed to create slot data buffer");
            return false;
        }

        Msg("* [FGDetailManager] Created slot data buffer: %u slots (%.2f MB)",
            slotDataCPU.size(),
            float(desc.byteSize) / (1024.f * 1024.f));
    }

    {
        constexpr float MIN_DENSITY = 0.04f;
        constexpr u32 MAX_CAPACITY_BYTES = 512u * 1024u * 1024u;
        constexpr u32 MAX_CAPACITY = MAX_CAPACITY_BYTES / sizeof(InstanceData);
        u32 d_size = u32(std::ceil(2.0f / MIN_DENSITY));
        u32 grid_per_slot = (d_size + 1) * (d_size + 1);
        generatedInstancesCapacity = std::min(u32(float(slot_count) * float(grid_per_slot) * 0.08f), MAX_CAPACITY);
        generatedInstancesCapacity = std::max(generatedInstancesCapacity, 1000000u);

        nvrhi::BufferDesc desc;
        desc.byteSize = generatedInstancesCapacity * sizeof(InstanceData);
        desc.structStride = sizeof(InstanceData);
        desc.debugName = "DetailGeneratedInstances";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        generatedInstancesBuffer = device->createBuffer(desc);
        if (!generatedInstancesBuffer)
        {
            Msg("! [FGDetailManager] Failed to create generated instances buffer");
            return false;
        }

        Msg("* [FGDetailManager] Created generated instances buffer: %.2f MB",
            float(desc.byteSize) / (1024.f * 1024.f));
    }

    visibleBufferCapacity = std::max(generatedInstancesCapacity / 4, 100000u);
    Msg("* [FGDetailManager] Initial visible buffer capacity: %u (%.1f MB per LOD)",
        visibleBufferCapacity, (visibleBufferCapacity * sizeof(u32)) / (1024.f * 1024.f));

    if (!detail_models.empty())
    {
        BuildDetailModelGPUData();

        nvrhi::BufferDesc desc;
        desc.byteSize = cachedModelGPUData.size() * sizeof(DetailModelGPU);
        desc.structStride = sizeof(DetailModelGPU);
        desc.debugName = "DetailModelsMetadata";
        desc.canHaveUAVs = false;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        detailModelsBuffer = device->createBuffer(desc);
        if (!detailModelsBuffer)
        {
            Msg("! [FGDetailManager] Failed to create detail models buffer");
            return false;
        }

        Msg("* [FGDetailManager] Created detail models buffer: %u models", detail_models.size());

        nvrhi::BufferDesc pvDesc;
        pvDesc.byteSize = decalPulledVertexData.size() * sizeof(DecalPulledVertex);
        pvDesc.structStride = sizeof(DecalPulledVertex);
        pvDesc.debugName = "DetailDecalPulledVerts";
        pvDesc.canHaveUAVs = false;
        pvDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        pvDesc.keepInitialState = true;
        decalPulledVertexBuffer = device->createBuffer(pvDesc);

        if (maxDecalIndexCount > 0)
        {
            Msg("* [FGDetailManager] Decal pulled vertices: %u entries (%u decal models, max %u indices/model)",
                (u32)decalPulledVertexData.size(), (u32)(decalPulledVertexData.size() / maxDecalIndexCount), maxDecalIndexCount);

            nvrhi::BufferDesc ibDesc;
            ibDesc.byteSize = maxDecalIndexCount * sizeof(u16);
            ibDesc.isIndexBuffer = true;
            ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
            ibDesc.keepInitialState = true;
            ibDesc.debugName = "DetailDecalIB";
            decalIndexBuffer = device->createBuffer(ibDesc);
        }
    }

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        {
            nvrhi::BufferDesc desc;
            desc.byteSize = visibleBufferCapacity * sizeof(u32);
            desc.structStride = sizeof(u32);
            desc.debugName = ("DetailVisibleLOD" + std::to_string(lod)).c_str();
            desc.canHaveUAVs = true;
            desc.canHaveTypedViews = false;
            desc.isVertexBuffer = false;
            desc.isIndexBuffer = false;
            desc.isConstantBuffer = false;
            desc.isDrawIndirectArgs = false;
            desc.canHaveRawViews = false;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            visibleInstancesBuffer[lod] = device->createBuffer(desc);
            if (!visibleInstancesBuffer[lod])
            {
                Msg("! [FGDetailManager] Failed to create visible instances buffer LOD%u", lod);
                return false;
            }
        }

        {
            nvrhi::BufferDesc desc;
            desc.byteSize = 5 * sizeof(u32);
            desc.structStride = 0;
            desc.debugName = ("DetailDrawArgsLOD" + std::to_string(lod)).c_str();
            desc.canHaveUAVs = true;
            desc.canHaveTypedViews = false;
            desc.isVertexBuffer = false;
            desc.isIndexBuffer = false;
            desc.isConstantBuffer = false;
            desc.isDrawIndirectArgs = true;
            desc.canHaveRawViews = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            drawArgsBuffer[lod] = device->createBuffer(desc);
            if (!drawArgsBuffer[lod])
            {
                Msg("! [FGDetailManager] Failed to create draw args buffer LOD%u", lod);
                return false;
            }
        }
    }

    {
        u32 decalCapacity = std::max(visibleBufferCapacity / 4, 10000u);
        nvrhi::BufferDesc desc;
        desc.byteSize = decalCapacity * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "DetailVisibleDecals";
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        visibleDecalInstancesBuffer = device->createBuffer(desc);
        if (!visibleDecalInstancesBuffer)
        {
            Msg("! [FGDetailManager] Failed to create visible decal instances buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = 5 * sizeof(u32);
        desc.debugName = "DetailDrawArgsDecal";
        desc.canHaveUAVs = true;
        desc.isDrawIndirectArgs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        decalDrawArgsBuffer = device->createBuffer(desc);
        if (!decalDrawArgsBuffer)
        {
            Msg("! [FGDetailManager] Failed to create decal draw args buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = slot_aabbs.size() * sizeof(SlotAABB);
        desc.structStride = sizeof(SlotAABB);
        desc.debugName = "DetailSlotAABBs";
        desc.canHaveUAVs = true;
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

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = slot_aabbs.size() * sizeof(u32);
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

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = 3 * sizeof(u32);
        desc.structStride = 0;
        desc.debugName = "DetailCullDispatchArgs";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        visibleSlotCounterBuffer = device->createBuffer(desc);
        if (!visibleSlotCounterBuffer)
        {
            Msg("! [FGDetailManager] Failed to create cull dispatch args buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = slot_aabbs.size() * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "DetailSlotVisibility";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = false;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        slotVisibilityBuffer = device->createBuffer(desc);
        if (!slotVisibilityBuffer)
        {
            Msg("! [FGDetailManager] Failed to create slot visibility buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.debugName = "DetailInstanceCounter";
        desc.canHaveUAVs = true;
        desc.canHaveTypedViews = false;
        desc.isVertexBuffer = false;
        desc.isIndexBuffer = false;
        desc.isConstantBuffer = false;
        desc.isDrawIndirectArgs = false;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        instanceCounterBuffer = device->createBuffer(desc);
        if (!instanceCounterBuffer)
        {
            Msg("! [FGDetailManager] Failed to create instance counter buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.debugName = "DetailInstanceCountReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        instanceCountReadbackBuffer = device->createBuffer(desc);
    }

    const u32 numBlocks = ((u32)slot_aabbs.size() + PREFIX_SUM_BLOCK_SIZE - 1) / PREFIX_SUM_BLOCK_SIZE;
    const u32 paddedSlotCount = numBlocks * PREFIX_SUM_BLOCK_SIZE;

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = paddedSlotCount * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "DetailPerSlotCounts";
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        perSlotCountsBuffer = device->createBuffer(desc);
        if (!perSlotCountsBuffer)
        {
            Msg("! [FGDetailManager] Failed to create per-slot counts buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = paddedSlotCount * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "DetailPerSlotPrefix";
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        perSlotPrefixBuffer = device->createBuffer(desc);
        if (!perSlotPrefixBuffer)
        {
            Msg("! [FGDetailManager] Failed to create per-slot prefix buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = numBlocks * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "DetailBlockTotals";
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        blockTotalsBuffer = device->createBuffer(desc);
        if (!blockTotalsBuffer)
        {
            Msg("! [FGDetailManager] Failed to create block totals buffer");
            return false;
        }
    }

    Msg("* [FGDetailManager] Created prefix sum buffers: %u slots, %u blocks of %u (padded to %u)",
        (u32)slot_aabbs.size(), numBlocks, PREFIX_SUM_BLOCK_SIZE, paddedSlotCount);

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = slot_aabbs.size() * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "DetailPerSlotLocalCounters";
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        perSlotLocalCountersBuffer = device->createBuffer(desc);
        if (!perSlotLocalCountersBuffer)
        {
            Msg("! [FGDetailManager] Failed to create per-slot local counters buffer");
            return false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32) * 5;
        desc.debugName = "DetailStatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        statsReadbackBuffer = device->createBuffer(desc);
        if (!statsReadbackBuffer)
        {
            Msg("! [FGDetailManager] Failed to create stats readback buffer");
            return false;
        }
    }

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        GenerateBladeGeometry(bladeVertices[lod], bladeIndices[lod], LOD_SEGMENTS[lod]);
        bladeVertexCount[lod] = static_cast<u32>(bladeVertices[lod].size());
        bladeIndexCount[lod] = static_cast<u32>(bladeIndices[lod].size());

        Msg("* [FGDetailManager] LOD%u: %u segments, %u vertices, %u indices",
            lod, LOD_SEGMENTS[lod], bladeVertexCount[lod], bladeIndexCount[lod]);

        {
            nvrhi::BufferDesc desc;
            desc.byteSize = bladeVertices[lod].size() * sizeof(BladeVertex);
            desc.structStride = 0;
            desc.debugName = ("DetailBladeVB_LOD" + std::to_string(lod)).c_str();
            desc.canHaveUAVs = false;
            desc.canHaveTypedViews = false;
            desc.isVertexBuffer = true;
            desc.isIndexBuffer = false;
            desc.isConstantBuffer = false;
            desc.isDrawIndirectArgs = false;
            desc.canHaveRawViews = false;
            desc.initialState = nvrhi::ResourceStates::VertexBuffer;
            desc.keepInitialState = true;

            bladeVertexBuffer[lod] = device->createBuffer(desc);
            if (!bladeVertexBuffer[lod])
            {
                Msg("! [FGDetailManager] Failed to create blade vertex buffer LOD%u", lod);
                return false;
            }
        }

        {
            nvrhi::BufferDesc desc;
            desc.byteSize = bladeIndices[lod].size() * sizeof(u16);
            desc.structStride = 0;
            desc.debugName = ("DetailBladeIB_LOD" + std::to_string(lod)).c_str();
            desc.canHaveUAVs = false;
            desc.canHaveTypedViews = false;
            desc.isVertexBuffer = false;
            desc.isIndexBuffer = true;
            desc.isConstantBuffer = false;
            desc.isDrawIndirectArgs = false;
            desc.canHaveRawViews = false;
            desc.initialState = nvrhi::ResourceStates::IndexBuffer;
            desc.keepInitialState = true;

            bladeIndexBuffer[lod] = device->createBuffer(desc);
            if (!bladeIndexBuffer[lod])
            {
                Msg("! [FGDetailManager] Failed to create blade index buffer LOD%u", lod);
                return false;
            }
        }
    }


    Msg("* [FGDetailManager] GPU buffers created (instance capacity: %u, %.2f MB)",
        generatedInstancesCapacity, float(generatedInstancesCapacity * sizeof(InstanceData)) / (1024.f * 1024.f));

    if (!CreateCachedResources(device))
    {
        Msg("! [FGDetailManager] Failed to create cached per-frame resources");
        return false;
    }

    return true;
}

bool FGDetailManager::CreateCachedResources(nvrhi::IDevice* device)
{
    if (!device || cachedResourcesInitialized)
        return true;

    {
        nvrhi::SamplerDesc desc;
        desc.setAllFilters(true);
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
        cachedSmp_LinearWrap = device->createSampler(desc);
    }
    {
        nvrhi::SamplerDesc desc;
        desc.setAllFilters(false);
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
        cachedSmp_PointClamp = device->createSampler(desc);
    }
    {
        nvrhi::SamplerDesc desc;
        desc.setAllFilters(true);
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
        cachedSmp_LinearClamp = device->createSampler(desc);
    }
    {
        nvrhi::SamplerDesc desc;
        desc.setAllFilters(true);
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
        desc.setMaxAnisotropy(16.0f);
        cachedSmp_AnisoWrap = device->createSampler(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = 32;
        desc.structStride = 32;
        desc.debugName = "DummyMaterials_Detail";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        cachedDummyMaterials = device->createBuffer(desc);
    }
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = 4;
        desc.format = nvrhi::Format::R32_UINT;
        desc.canHaveTypedViews = true;
        desc.debugName = "DummySlotIndirection";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        cachedDummySlotIndirection = device->createBuffer(desc);
    }

    nvrhi::BufferDesc cbDesc;
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 16;

    cbDesc.byteSize = sizeof(passes::DynamicTransforms);
    cbDesc.debugName = "DynTransforms_Detail";
    cachedDynTransformsCB = device->createBuffer(cbDesc);

    cbDesc.byteSize = 32;
    cbDesc.debugName = "ShaderParams_Detail";
    cachedShaderParamsCB = device->createBuffer(cbDesc);

    cbDesc.byteSize = sizeof(passes::StaticGlobals);
    cbDesc.debugName = "StaticGlobals_Detail";
    cachedStaticGlobalsCB = device->createBuffer(cbDesc);

    cbDesc.byteSize = sizeof(DetailFrameConstants);
    cbDesc.debugName = "DetailGlobals";
    cachedDetailGlobalsCB = device->createBuffer(cbDesc);

    cbDesc.byteSize = 48;
    cbDesc.debugName = "DynLight_Detail";
    cachedDynLightCB = device->createBuffer(cbDesc);

    cbDesc.byteSize = sizeof(DetailCullParams);
    cbDesc.debugName = "DetailCullParams";
    cachedCullParamsCB = device->createBuffer(cbDesc);

    cbDesc.byteSize = sizeof(InstanceGenParams);
    cbDesc.debugName = "InstanceGenParams";
    cachedInstanceGenParamsCB = device->createBuffer(cbDesc);

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(GrassObjectTint) * 64;
        desc.structStride = sizeof(GrassObjectTint);
        desc.debugName = "GrassObjectTints";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        cachedGrassTintsBuffer = device->createBuffer(desc);
    }

    cachedResourcesInitialized = true;
    Msg("* [FGDetailManager] Cached per-frame resources created (4 samplers, 6 volatile CBs, 3 buffers)");
    return true;
}

void FGDetailManager::DestroyGPUBuffers()
{
    slotDataBuffer = nullptr;
    generatedInstancesBuffer = nullptr;
    detailModelsBuffer = nullptr;
    instanceGenComputeShader = nullptr;
    instanceGenBindingLayout = nullptr;
    instanceGenPipeline = nullptr;

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        visibleInstancesBuffer[lod] = nullptr;
        drawArgsBuffer[lod] = nullptr;
        bladeVertexBuffer[lod] = nullptr;
        bladeIndexBuffer[lod] = nullptr;
    }

    slotAABBBuffer = nullptr;
    visibleDecalInstancesBuffer = nullptr;
    decalDrawArgsBuffer = nullptr;
    visibleSlotIDsBuffer = nullptr;
    visibleSlotCounterBuffer = nullptr;
    slotVisibilityBuffer = nullptr;

    instanceCounterBuffer = nullptr;
    instanceCountReadbackBuffer = nullptr;
    instanceCountReadbackPending = false;
    totalGeneratedInstances = 0;
    perSlotCountsBuffer = nullptr;
    perSlotPrefixBuffer = nullptr;
    blockTotalsBuffer = nullptr;
    perSlotLocalCountersBuffer = nullptr;
    prefixSumScanShader = nullptr;
    prefixSumTopShader = nullptr;
    prefixSumBindingLayout = nullptr;
    prefixSumScanPipeline = nullptr;
    prefixSumTopPipeline = nullptr;

    slotCullComputeShader = nullptr;
    slotCullBindingLayout = nullptr;
    slotCullPipeline = nullptr;

    computePipeline = nullptr;
    computeBindingLayout = nullptr;
    cullComputeShader = nullptr;

    graphicsPipeline = nullptr;
    decalGraphicsPipeline = nullptr;
    graphicsBindingLayout = nullptr;

    decalPulledVertexBuffer = nullptr;
    decalIndexBuffer = nullptr;
    buildDetailsTexture = nullptr;

    windTexture = nullptr;
    windComputeShader = nullptr;
    windBindingLayout = nullptr;
    windPipeline = nullptr;

    statsReadbackBuffer = nullptr;
    statsReadbackPending = false;

    cachedSmp_LinearWrap = nullptr;
    cachedSmp_PointClamp = nullptr;
    cachedSmp_LinearClamp = nullptr;
    cachedSmp_AnisoWrap = nullptr;
    cachedDummyMaterials = nullptr;
    cachedDummySlotIndirection = nullptr;
    cachedDynTransformsCB = nullptr;
    cachedShaderParamsCB = nullptr;
    cachedStaticGlobalsCB = nullptr;
    cachedDetailGlobalsCB = nullptr;
    cachedDynLightCB = nullptr;
    cachedCullParamsCB = nullptr;
    cachedInstanceGenParamsCB = nullptr;
    cachedGrassTintsBuffer = nullptr;
    cachedResourcesInitialized = false;
}

bool FGDetailManager::CreateWindTexture(nvrhi::IDevice* device)
{
    if (!device)
        return false;

    if (!RImplementation.m_renderDevice)
    {
        Msg("! [FGDetailManager] No render device available for wind texture");
        return false;
    }

    auto* resourceManager = RImplementation.m_renderDevice->GetFGResourceManager();
    if (!resourceManager)
    {
        Msg("! [FGDetailManager] No resource manager available for wind texture");
        return false;
    }

    auto* textureManager = resourceManager->GetTextureManager();
    if (!textureManager)
    {
        Msg("! [FGDetailManager] No texture manager available for wind texture");
        return false;
    }

    xray::render::resources::TextureHandle texHandle = textureManager->LoadTexture(
        "shaders\\perlin_noise",
        xray::render::resources::TexturePriority::Critical
    );

    if (!texHandle.IsValid())
    {
        Msg("! [FGDetailManager] Failed to load shaders/perlin_noise.dds");
        return false;
    }

    nvrhi::ITexture* nvrhiTexture = textureManager->GetNVRHITexture(texHandle);
    if (!nvrhiTexture)
    {
        Msg("! [FGDetailManager] Failed to get NVRHI handle for perlin_noise");
        return false;
    }

    windTexture = nvrhiTexture;

    if (GEnv.Backend)
    {
        windTextureBindlessIndex = GEnv.Backend->RegisterBindlessTexture(nvrhiTexture);
        if (windTextureBindlessIndex == 0)
        {
            Msg("! [FGDetailManager] Failed to register perlin_noise in bindless heap");
            return false;
        }
        Msg("* [FGDetailManager] Wind texture (perlin_noise) loaded, bindless index: %u", windTextureBindlessIndex);
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

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Compute;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_UAV(0),
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
    (void)cmdList;
    (void)device;
    (void)time;
}

void FGDetailManager::GenerateBladeGeometry(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments)
{
    vertices.clear();
    indices.clear();

    float width_base = ps_r3_grass_blade_width;
    float height = ps_r3_grass_blade_height;

    for (int i = 0; i < segments; i++)
    {
        float t = float(i) / float(segments);
        float y = t * height;

        float taper = 1.0f - powf(t, 0.7f);
        float width = width_base * taper;

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

    {
        BladeVertex v_tip;
        v_tip.pos.set(0.0f, height, 0.0f);
        v_tip.uv.set(0.5f, 0.0f);
        v_tip.t = 1.0f;
        v_tip.width_scale = 0.0f;
        vertices.push_back(v_tip);
    }

    u16 tipIndex = static_cast<u16>(segments * 2);

    for (int i = 0; i < segments - 1; i++)
    {
        u16 base = i * 2;
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    {
        u16 base = (segments - 1) * 2;
        indices.push_back(base);
        indices.push_back(tipIndex);
        indices.push_back(base + 1);
    }
}

void FGDetailManager::RegenerateBladeGeometry(nvrhi::ICommandList* cmdList)
{
    if (!cmdList)
        return;

    Msg("* [FGDetailManager] Regenerating blade geometry (width=%.3f, height=%.2f)",
        ps_r3_grass_blade_width, ps_r3_grass_blade_height);

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        GenerateBladeGeometry(bladeVertices[lod], bladeIndices[lod], LOD_SEGMENTS[lod]);
        bladeVertexCount[lod] = static_cast<u32>(bladeVertices[lod].size());
        bladeIndexCount[lod] = static_cast<u32>(bladeIndices[lod].size());

        cmdList->writeBuffer(bladeVertexBuffer[lod], bladeVertices[lod].data(), bladeVertices[lod].size() * sizeof(BladeVertex));
        cmdList->writeBuffer(bladeIndexBuffer[lod], bladeIndices[lod].data(), bladeIndices[lod].size() * sizeof(u16));
    }
}

void FGDetailManager::ComputeSlotAABBs()
{
    ZoneScoped;

    if (!dtFS)
    {
        Msg("! [FGDetailManager] ComputeSlotAABBs: level.details not loaded");
        return;
    }

    IReader* slots_chunk = dtFS->open_chunk(2);
    if (!slots_chunk)
    {
        Msg("! [FGDetailManager] ComputeSlotAABBs: failed to read slots chunk");
        return;
    }

    DetailSlot* dtSlots = (DetailSlot*)slots_chunk->pointer();
    u32 total_slots = dtH.x_size() * dtH.z_size();

    Msg("* [FGDetailManager] Computing slot AABBs for %dx%d grid...", dtH.x_size(), dtH.z_size());

    slot_aabbs.clear();
    slot_aabbs.resize(total_slots);

    for (u32 db_z = 0; db_z < dtH.z_size(); db_z++)
    {
        for (u32 db_x = 0; db_x < dtH.x_size(); db_x++)
        {
            int sx = int(db_x) - dtH.x_offs();
            int sz = int(db_z) - dtH.z_offs();
            u32 slot_idx = db_z * dtH.x_size() + db_x;

            const DetailSlot& slot = dtSlots[slot_idx];

            float world_x = sx * DETAIL_SLOT_SIZE;
            float world_z = sz * DETAIL_SLOT_SIZE;

            SlotAABB& aabb = slot_aabbs[slot_idx];
            aabb.aabb_min = Fvector3(world_x, -50.0f, world_z);
            aabb.aabb_max = Fvector3(world_x + DETAIL_SLOT_SIZE, 100.0f, world_z + DETAIL_SLOT_SIZE);
            aabb.instance_base = 0;
            aabb.instance_count = 0;
            aabb.slot_x = sx;
            aabb.slot_z = sz;
            aabb.padding0 = aabb.padding1 = 0.f;
            aabb.padding2 = Fvector4(0, 0, 0, 0);
        }
    }

    slots_chunk->close();

    slot_count = total_slots;

    Msg("* [FGDetailManager] Computed %u slot AABBs", total_slots);
}

bool FGDetailManager::LoadCullComputeShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
    {
        Msg("! [FGDetailManager] LoadCullComputeShader: shaderLoader is null");
        return false;
    }

    slotCullComputeShader = shaderLoader->LoadComputeShader("detail_cell_cull", "main").handle;
    if (!slotCullComputeShader)
    {
        Msg("! [FGDetailManager] Failed to load detail_cell_cull.cs");
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

bool FGDetailManager::LoadInstanceGenShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
    {
        Msg("! [FGDetailManager] LoadInstanceGenShader: shaderLoader is null");
        return false;
    }

    instanceGenComputeShader = shaderLoader->LoadComputeShader("detail_instance_gen", "main").handle;
    if (!instanceGenComputeShader)
    {
        Msg("! [FGDetailManager] Failed to load detail_instance_gen.cs");
        return false;
    }

    Msg("* [FGDetailManager] Loaded instance generation shader");
    return true;
}

bool FGDetailManager::LoadPrefixSumShaders(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
    {
        Msg("! [FGDetailManager] LoadPrefixSumShaders: shaderLoader is null");
        return false;
    }

    prefixSumScanShader = shaderLoader->LoadComputeShader("detail_prefix_sum", "main_scan_blocks").handle;
    if (!prefixSumScanShader)
    {
        Msg("! [FGDetailManager] Failed to load detail_prefix_sum.cs (main_scan_blocks)");
        return false;
    }

    prefixSumTopShader = shaderLoader->LoadComputeShader("detail_prefix_sum", "main_scan_top").handle;
    if (!prefixSumTopShader)
    {
        Msg("! [FGDetailManager] Failed to load detail_prefix_sum.cs (main_scan_top)");
        return false;
    }

    Msg("* [FGDetailManager] Loaded prefix sum shaders");
    return true;
}

bool FGDetailManager::CreatePrefixSumPipeline(ng::RenderDevice* renderDevice)
{
    if (!renderDevice || !prefixSumScanShader || !prefixSumTopShader)
    {
        Msg("! [FGDetailManager] CreatePrefixSumPipeline: invalid parameters");
        return false;
    }

    nvrhi::IDevice* device = renderDevice->GetNVRHIDevice();

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Compute;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(6),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(1),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(2),
    };

    prefixSumBindingLayout = device->createBindingLayout(layoutDesc);
    if (!prefixSumBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create prefix sum binding layout");
        return false;
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = prefixSumScanShader;
        pipeDesc.bindingLayouts = { prefixSumBindingLayout };
        prefixSumScanPipeline = device->createComputePipeline(pipeDesc);
        if (!prefixSumScanPipeline)
        {
            Msg("! [FGDetailManager] Failed to create prefix sum scan pipeline");
            return false;
        }
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = prefixSumTopShader;
        pipeDesc.bindingLayouts = { prefixSumBindingLayout };
        prefixSumTopPipeline = device->createComputePipeline(pipeDesc);
        if (!prefixSumTopPipeline)
        {
            Msg("! [FGDetailManager] Failed to create prefix sum top pipeline");
            return false;
        }
    }

    Msg("* [FGDetailManager] Created prefix sum pipelines");
    return true;
}

bool FGDetailManager::LoadGraphicsShaders(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
    {
        Msg("! [FGDetailManager] LoadGraphicsShaders: shaderLoader is null");
        return false;
    }

    auto vsResult = shaderLoader->LoadVertexShader("detail_gpu", "main");
    if (!vsResult.handle)
    {
        Msg("! [FGDetailManager] Failed to load detail_gpu.vs");
        return false;
    }
    vertexShader = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("detail_gpu", "main");
    if (!psResult.handle)
    {
        Msg("! [FGDetailManager] Failed to load detail_gpu.ps");
        return false;
    }
    pixelShader = psResult.handle;

    auto decalVsResult = shaderLoader->LoadVertexShader("detail_decal", "main");
    if (!decalVsResult.handle)
    {
        Msg("! [FGDetailManager] Failed to load detail_decal.vs");
        return false;
    }
    decalVertexShader = decalVsResult.handle;

    auto decalPsResult = shaderLoader->LoadPixelShader("detail_decal", "main");
    if (!decalPsResult.handle)
    {
        Msg("! [FGDetailManager] Failed to load detail_decal.ps");
        return false;
    }
    decalPixelShader = decalPsResult.handle;

    return true;
}

void FGDetailManager::UploadBufferData(nvrhi::ICommandList* cmdList)
{
    if (!cmdList)
        return;

    if (slotDataBuffer && !slotDataCPU.empty())
        cmdList->writeBuffer(slotDataBuffer, slotDataCPU.data(), slotDataCPU.size() * sizeof(GPUSlotData));

    if (detailModelsBuffer && !cachedModelGPUData.empty())
        cmdList->writeBuffer(detailModelsBuffer, cachedModelGPUData.data(), cachedModelGPUData.size() * sizeof(DetailModelGPU));

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        cmdList->writeBuffer(bladeVertexBuffer[lod], bladeVertices[lod].data(), bladeVertices[lod].size() * sizeof(BladeVertex));
        cmdList->writeBuffer(bladeIndexBuffer[lod], bladeIndices[lod].data(), bladeIndices[lod].size() * sizeof(u16));
    }

    if (decalPulledVertexBuffer && !decalPulledVertexData.empty())
        cmdList->writeBuffer(decalPulledVertexBuffer, decalPulledVertexData.data(), decalPulledVertexData.size() * sizeof(DecalPulledVertex));

    if (decalIndexBuffer && maxDecalIndexCount > 0)
    {
        xr_vector<u16> seqIndices(maxDecalIndexCount);
        for (u32 j = 0; j < maxDecalIndexCount; j++)
            seqIndices[j] = (u16)j;
        cmdList->writeBuffer(decalIndexBuffer, seqIndices.data(), seqIndices.size() * sizeof(u16));
    }

    decalPulledVertexData.clear();
    decalPulledVertexData.shrink_to_fit();

    if (buildDetailsTexture && !pendingBuildDetailsUploads.empty())
    {
        for (u32 mip = 0; mip < pendingBuildDetailsUploads.size(); mip++)
        {
            auto& ml = pendingBuildDetailsUploads[mip];
            cmdList->writeTexture(buildDetailsTexture, 0, mip, ml.data.data(), ml.rowPitch);
        }
        pendingBuildDetailsUploads.clear();
        pendingBuildDetailsUploads.shrink_to_fit();
    }

    if (buildDetailsPbrTexture && !pendingBuildDetailsPbrUploads.empty())
    {
        for (u32 mip = 0; mip < pendingBuildDetailsPbrUploads.size(); mip++)
        {
            auto& ml = pendingBuildDetailsPbrUploads[mip];
            cmdList->writeTexture(buildDetailsPbrTexture, 0, mip, ml.data.data(), ml.rowPitch);
        }
        pendingBuildDetailsPbrUploads.clear();
        pendingBuildDetailsPbrUploads.shrink_to_fit();
    }

    cmdList->writeBuffer(slotAABBBuffer, slot_aabbs.data(), slot_aabbs.size() * sizeof(SlotAABB));
}

bool FGDetailManager::CreateComputePipeline(ng::RenderDevice* renderDevice)
{
    if (!renderDevice || !cullComputeShader || !slotCullComputeShader)
    {
        Msg("! [FGDetailManager] CreateComputePipeline: invalid parameters");
        return false;
    }

    nvrhi::IDevice* device = renderDevice->GetNVRHIDevice();

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(1),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(2),
        };

        slotCullBindingLayout = device->createBindingLayout(layoutDesc);
        if (!slotCullBindingLayout)
        {
            Msg("! [FGDetailManager] Failed to create slot cull binding layout");
            return false;
        }

        nvrhi::ComputePipelineDesc pipelineDesc;
        pipelineDesc.CS = slotCullComputeShader;
        pipelineDesc.bindingLayouts = { slotCullBindingLayout };

        slotCullPipeline = device->createComputePipeline(pipelineDesc);
        if (!slotCullPipeline)
        {
            Msg("! [FGDetailManager] Failed to create slot cull pipeline");
            return false;
        }
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2),
            nvrhi::BindingLayoutItem::Texture_SRV(3),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(4),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(2),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(3),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(4),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(6),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(7),
        };

        computeBindingLayout = device->createBindingLayout(layoutDesc);
        if (!computeBindingLayout)
        {
            Msg("! [FGDetailManager] Failed to create instance cull binding layout");
            return false;
        }

        nvrhi::ComputePipelineDesc pipelineDesc;
        pipelineDesc.CS = cullComputeShader;
        pipelineDesc.bindingLayouts = { computeBindingLayout };

        computePipeline = device->createComputePipeline(pipelineDesc);
        if (!computePipeline)
        {
            Msg("! [FGDetailManager] Failed to create instance cull pipeline");
            return false;
        }
    }

    return true;
}

bool FGDetailManager::CreateInstanceGenPipeline(ng::RenderDevice* renderDevice)
{
    if (!renderDevice || !instanceGenComputeShader)
    {
        Msg("! [FGDetailManager] CreateInstanceGenPipeline: invalid parameters");
        return false;
    }

    nvrhi::IDevice* device = renderDevice->GetNVRHIDevice();

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::Compute;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(6),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1),
        nvrhi::BindingLayoutItem::Texture_SRV(2),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(4),
        nvrhi::BindingLayoutItem::Sampler(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
        nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(2),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(3),
        nvrhi::BindingLayoutItem::StructuredBuffer_UAV(4),
    };

    instanceGenBindingLayout = device->createBindingLayout(layoutDesc);
    if (!instanceGenBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create instance gen binding layout");
        return false;
    }

    nvrhi::ComputePipelineDesc pipelineDesc;
    pipelineDesc.CS = instanceGenComputeShader;
    pipelineDesc.bindingLayouts = { instanceGenBindingLayout };

    instanceGenPipeline = device->createComputePipeline(pipelineDesc);
    if (!instanceGenPipeline)
    {
        Msg("! [FGDetailManager] Failed to create instance gen pipeline");
        return false;
    }

    Msg("* [FGDetailManager] Created instance generation pipeline");
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

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(1),
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(3),
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
        nvrhi::BindingLayoutItem::TypedBuffer_SRV(32),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(33),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(34),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(35),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(36),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(37),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(38),
        nvrhi::BindingLayoutItem::Sampler(0),
        nvrhi::BindingLayoutItem::Sampler(1),
        nvrhi::BindingLayoutItem::Sampler(2),
        nvrhi::BindingLayoutItem::Sampler(3),
        nvrhi::BindingLayoutItem::Sampler(4),
        nvrhi::BindingLayoutItem::Sampler(5),
    };

    graphicsBindingLayout = device->createBindingLayout(layoutDesc);
    if (!graphicsBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create graphics binding layout");
        return false;
    }

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
            .setArraySize(2)
            .setOffset(20)
            .setElementStride(sizeof(BladeVertex)),
    };

    inputLayout = device->createInputLayout(attributes, 3, vertexShader);
    if (!inputLayout)
    {
        Msg("! [FGDetailManager] Failed to create input layout");
        return false;
    }

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.VS = vertexShader;
    pipelineDesc.PS = pixelShader;
    pipelineDesc.inputLayout = inputLayout;
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

    pipelineDesc.bindingLayouts.push_back(graphicsBindingLayout);
    auto* backend = renderDevice->GetBackend();
    if (backend) {
        auto* bindlessLayout = backend->GetBindlessLayout();
        if (bindlessLayout) {
            pipelineDesc.bindingLayouts.push_back(bindlessLayout);
        }
    }

    pipelineDesc.renderState.rasterState.fillMode = nvrhi::RasterFillMode::Solid;
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

    pipelineDesc.renderState.depthStencilState.depthTestEnable = true;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = true;
    pipelineDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;

    pipelineDesc.renderState.blendState.targets[0].disableBlend();

    graphicsPipeline = device->createGraphicsPipeline(pipelineDesc, framebuffer);
    if (!graphicsPipeline)
    {
        Msg("! [FGDetailManager] Failed to create graphics pipeline");
        return false;
    }

    if (decalVertexShader && decalPixelShader)
    {
        nvrhi::GraphicsPipelineDesc decalPipeDesc = pipelineDesc;
        decalPipeDesc.VS = decalVertexShader;
        decalPipeDesc.PS = decalPixelShader;
        decalPipeDesc.inputLayout = nullptr;

        decalGraphicsPipeline = device->createGraphicsPipeline(decalPipeDesc, framebuffer);
        if (!decalGraphicsPipeline)
        {
            Msg("! [FGDetailManager] Failed to create decal graphics pipeline");
            return false;
        }
    }

    return true;
}

void FGDetailManager::DispatchCulling(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* device,
    nvrhi::ITexture* hiZPyramid,
    const Fmatrix& viewProj,
    const Fmatrix& prevViewProj,
    const Fvector4* frustumPlanes,
    u32 frustumPlaneCount,
    const Fvector& cameraPos,
    float fadeDistance,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    if (!cullComputeShader || !slotCullComputeShader || slot_count == 0)
    {
        return;
    }

    if (!computePipeline || !slotCullPipeline)
        return;

    float current_density = ps_current_detail_density;
    if (std::abs(current_density - m_lastDensity) > 0.001f)
    {
        m_instancesNeedRegeneration = true;
        m_lastDensity = current_density;
        Msg("[DetailManager] Density changed to %.3f - regeneration needed", current_density);
    }

    ResizeVisibleBuffersIfNeeded(device);

    if (m_instancesNeedRegeneration)
    {
        constexpr u32 MAX_INSTANCES = 128u * 1024u * 1024u;
        u32 d_size = u32(std::ceil(2.0f / ps_current_detail_density));
        u32 grid_per_slot = (d_size + 1) * (d_size + 1);
        u32 neededGenCapacity = std::min(u32(float(slot_count) * float(grid_per_slot) * 0.08f), MAX_INSTANCES);
        neededGenCapacity = std::max(neededGenCapacity, 1000000u);

        if (neededGenCapacity > generatedInstancesCapacity)
        {
            Msg("[DetailManager] Growing generated buffer: %u -> %u (density=%.3f, %.1f MB)",
                generatedInstancesCapacity, neededGenCapacity, ps_current_detail_density,
                (neededGenCapacity * sizeof(InstanceData)) / (1024.f * 1024.f));
            generatedInstancesCapacity = neededGenCapacity;
            nvrhi::BufferDesc desc;
            desc.byteSize = generatedInstancesCapacity * sizeof(InstanceData);
            desc.structStride = sizeof(InstanceData);
            desc.debugName = "DetailGeneratedInstances";
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            generatedInstancesBuffer = device->createBuffer(desc);
        }

        if (visibleBufferCapacity < generatedInstancesCapacity)
        {
            Msg("[DetailManager] Pre-grow visible buffers: %u -> %u", visibleBufferCapacity, generatedInstancesCapacity);
            visibleBufferCapacity = generatedInstancesCapacity;
            for (u32 lod = 0; lod < LOD_COUNT; lod++)
            {
                nvrhi::BufferDesc desc;
                desc.byteSize = visibleBufferCapacity * sizeof(u32);
                desc.structStride = sizeof(u32);
                desc.debugName = ("DetailVisibleLOD" + std::to_string(lod)).c_str();
                desc.canHaveUAVs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;
                visibleInstancesBuffer[lod] = device->createBuffer(desc);
            }
            u32 dc = std::max(visibleBufferCapacity / 4, 10000u);
            nvrhi::BufferDesc desc;
            desc.byteSize = dc * sizeof(u32);
            desc.structStride = sizeof(u32);
            desc.debugName = "DetailVisibleDecals";
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            visibleDecalInstancesBuffer = device->createBuffer(desc);
        }
        RegenerateAllInstances(cmdList, device, gpuProfiler);
    }

    u32 decalCapacity = std::max(visibleBufferCapacity / 4, 10000u);

    DetailCullParams params;
    params.viewProj.transpose(viewProj);
    params.prevViewProj.transpose(prevViewProj);
    params.cameraPos = cameraPos;
    params.fadeDistanceSqr = fadeDistance * fadeDistance;

    for (u32 i = 0; i < 6; i++)
    {
        if (i < frustumPlaneCount)
            params.frustumPlanes[i] = frustumPlanes[i];
        else
            params.frustumPlanes[i].set(0, 0, 0, 1000000.0f);
    }

    params.visibleBladeCapacity = visibleBufferCapacity;
    params.totalSlotCount = slot_count;
    params.hizWidth = hiZWidth;
    params.hizHeight = hiZHeight;
    params.hizMipLevels = hiZMipLevels;
    params.lodDistanceCloseSqr = ps_r3_grass_lod_close * ps_r3_grass_lod_close;
    params.lodDistanceMidSqr = ps_r3_grass_lod_mid * ps_r3_grass_lod_mid;
    params.detailDensity = ps_current_detail_density;
    params.visibleDecalCapacity = decalCapacity;
    params.cullPad0 = params.cullPad1 = params.cullPad2 = 0;

    cmdList->writeBuffer(cachedCullParamsCB, &params, sizeof(params));

    cmdList->beginTrackingBufferState(slotVisibilityBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingBufferState(visibleSlotIDsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingBufferState(visibleSlotCounterBuffer, nvrhi::ResourceStates::UnorderedAccess);

    if (generatedInstancesBuffer)
        cmdList->beginTrackingBufferState(generatedInstancesBuffer, nvrhi::ResourceStates::ShaderResource);

    if (instanceCounterBuffer)
        cmdList->beginTrackingBufferState(instanceCounterBuffer, nvrhi::ResourceStates::UnorderedAccess);
    if (perSlotCountsBuffer)
        cmdList->beginTrackingBufferState(perSlotCountsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    if (perSlotPrefixBuffer)
        cmdList->beginTrackingBufferState(perSlotPrefixBuffer, nvrhi::ResourceStates::UnorderedAccess);
    if (blockTotalsBuffer)
        cmdList->beginTrackingBufferState(blockTotalsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    if (perSlotLocalCountersBuffer)
        cmdList->beginTrackingBufferState(perSlotLocalCountersBuffer, nvrhi::ResourceStates::UnorderedAccess);

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        cmdList->beginTrackingBufferState(visibleInstancesBuffer[lod], nvrhi::ResourceStates::UnorderedAccess);
        cmdList->beginTrackingBufferState(drawArgsBuffer[lod], nvrhi::ResourceStates::UnorderedAccess);
    }
    cmdList->beginTrackingBufferState(visibleDecalInstancesBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingBufferState(decalDrawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->beginTrackingTextureState(hiZPyramid, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

    u32 dispatchArgs[3] = { 0, 1, 1 };
    cmdList->writeBuffer(visibleSlotCounterBuffer, dispatchArgs, sizeof(dispatchArgs));

    struct IndirectDrawArgs { u32 indexCount, instanceCount, startIndex; s32 baseVertex; u32 startInstance; };
    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        IndirectDrawArgs args = { bladeIndexCount[lod], 0, 0, 0, 0 };
        cmdList->writeBuffer(drawArgsBuffer[lod], &args, sizeof(args));
    }
    {
        IndirectDrawArgs decalArgs = { maxDecalIndexCount, 0, 0, 0, 0 };
        cmdList->writeBuffer(decalDrawArgsBuffer, &decalArgs, sizeof(decalArgs));
    }

    const u32 threadGroupSize = 256;

    if (gpuProfiler) gpuProfiler->BeginPass(cmdList, "Details.SlotCull");
    {
        nvrhi::BindingSetDesc bindDesc;
        bindDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(5, cachedCullParamsCB),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(0, slotAABBBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, slotVisibilityBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(1, visibleSlotIDsBuffer),
            nvrhi::BindingSetItem::RawBuffer_UAV(2, visibleSlotCounterBuffer),
        };

        nvrhi::BindingSetHandle slotCullBindingSet = device->createBindingSet(bindDesc, slotCullBindingLayout);

        nvrhi::ComputeState state;
        state.pipeline = slotCullPipeline;
        state.bindings = { slotCullBindingSet };
        cmdList->setComputeState(state);

        u32 numGroups = (slot_count + threadGroupSize - 1) / threadGroupSize;
        cmdList->dispatch(numGroups, 1, 1);
    }

    if (gpuProfiler) gpuProfiler->EndPass(cmdList, "Details.SlotCull");

    cmdList->setBufferState(visibleSlotCounterBuffer, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(visibleSlotIDsBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->commitBarriers();

    if (gpuProfiler) gpuProfiler->BeginPass(cmdList, "Details.InstanceCull");
    {
        if (!generatedInstancesBuffer)
            return;

        nvrhi::BindingSetDesc bindDesc;
        bindDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(5, cachedCullParamsCB),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(0, generatedInstancesBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(1, visibleSlotIDsBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(2, slotAABBBuffer),
            nvrhi::BindingSetItem::Texture_SRV(3, hiZPyramid),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(4, detailModelsBuffer),
            nvrhi::BindingSetItem::Sampler(0, cachedSmp_PointClamp),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, visibleInstancesBuffer[0]),
            nvrhi::BindingSetItem::RawBuffer_UAV(1, drawArgsBuffer[0]),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(2, visibleInstancesBuffer[1]),
            nvrhi::BindingSetItem::RawBuffer_UAV(3, drawArgsBuffer[1]),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(4, visibleInstancesBuffer[2]),
            nvrhi::BindingSetItem::RawBuffer_UAV(5, drawArgsBuffer[2]),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(6, visibleDecalInstancesBuffer),
            nvrhi::BindingSetItem::RawBuffer_UAV(7, decalDrawArgsBuffer),
        };

        nvrhi::BindingSetHandle instanceCullBindingSet = device->createBindingSet(bindDesc, computeBindingLayout);

        nvrhi::ComputeState state;
        state.pipeline = computePipeline;
        state.bindings = { instanceCullBindingSet };
        state.indirectParams = visibleSlotCounterBuffer;
        cmdList->setComputeState(state);

        cmdList->dispatchIndirect(0);
    }

    if (gpuProfiler) gpuProfiler->EndPass(cmdList, "Details.InstanceCull");

    cmdList->setBufferState(slotVisibilityBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(visibleSlotCounterBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(visibleSlotIDsBuffer, nvrhi::ResourceStates::UnorderedAccess);

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        cmdList->setBufferState(visibleInstancesBuffer[lod], nvrhi::ResourceStates::ShaderResource);
        cmdList->setBufferState(drawArgsBuffer[lod], nvrhi::ResourceStates::IndirectArgument);
    }
    cmdList->setBufferState(visibleDecalInstancesBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(decalDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
    cmdList->commitBarriers();
}

void FGDetailManager::ScheduleStatsReadback(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device)
{
    if (!device || !cmdList || !statsReadbackBuffer)
        return;

    cmdList->setBufferState(visibleSlotCounterBuffer, nvrhi::ResourceStates::CopySource);
    for (u32 lod = 0; lod < LOD_COUNT; lod++)
        cmdList->setBufferState(drawArgsBuffer[lod], nvrhi::ResourceStates::CopySource);
    if (decalDrawArgsBuffer)
        cmdList->setBufferState(decalDrawArgsBuffer, nvrhi::ResourceStates::CopySource);
    cmdList->commitBarriers();

    cmdList->copyBuffer(statsReadbackBuffer, 0, visibleSlotCounterBuffer, 0, sizeof(u32));
    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        if (drawArgsBuffer[lod])
        {
            cmdList->copyBuffer(
                statsReadbackBuffer, sizeof(u32) * (1 + lod),
                drawArgsBuffer[lod], sizeof(u32),
                sizeof(u32)
            );
        }
    }
    if (decalDrawArgsBuffer)
    {
        cmdList->copyBuffer(
            statsReadbackBuffer, sizeof(u32) * 4,
            decalDrawArgsBuffer, sizeof(u32),
            sizeof(u32)
        );
    }

    cmdList->setBufferState(visibleSlotCounterBuffer, nvrhi::ResourceStates::UnorderedAccess);
    for (u32 lod = 0; lod < LOD_COUNT; lod++)
        cmdList->setBufferState(drawArgsBuffer[lod], nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(visibleDecalInstancesBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(decalDrawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->commitBarriers();

    statsReadbackPending = true;
}

void FGDetailManager::ProcessStatsReadback(nvrhi::IDevice* device)
{
    if (!statsReadbackPending || !statsReadbackBuffer || !device)
        return;

    statsFrameCounter++;
    const u32 throttleInterval = xray::profiler::GetCPUProfiler().GetThrottleInterval();
    if ((statsFrameCounter % throttleInterval) != 0)
        return;

    void* mappedData = device->mapBuffer(statsReadbackBuffer, nvrhi::CpuAccessMode::Read);
    if (mappedData)
    {
        const u32* counts = static_cast<const u32*>(mappedData);
        cullingStats.visibleSlotsCount = counts[0];
        cullingStats.visibleLOD0Count = counts[1];
        cullingStats.visibleLOD1Count = counts[2];
        cullingStats.visibleLOD2Count = counts[3];
        cullingStats.visibleDecalCount = counts[4];
        device->unmapBuffer(statsReadbackBuffer);
    }

    statsReadbackPending = false;
}

void FGDetailManager::BuildDetailModelGPUData()
{
    maxDecalIndexCount = 0;
    for (auto* m : detail_models)
    {
        if (m->m_Flags.flags & 0x0001)
            maxDecalIndexCount = std::max(maxDecalIndexCount, m->number_indices);
    }

    cachedModelGPUData.resize(detail_models.size());
    xr_vector<DecalPulledVertex> pulledVerts;
    u32 runningDecalOffset = 0;

    for (u32 i = 0; i < detail_models.size(); i++)
    {
        CDetail* m = detail_models[i];
        auto& d = cachedModelGPUData[i];
        d.minScale = m->m_fMinScale;
        d.maxScale = m->m_fMaxScale;
        d.flags = *reinterpret_cast<const float*>(&m->m_Flags.flags);

        if (m->number_vertices > 0)
        {
            float pxmin = FLT_MAX, pzmin = FLT_MAX, pxmax = -FLT_MAX, pzmax = -FLT_MAX;
            float umin = FLT_MAX, vmin = FLT_MAX, umax = -FLT_MAX, vmax = -FLT_MAX;
            for (u32 v = 0; v < m->number_vertices; v++)
            {
                pxmin = std::min(pxmin, m->vertices[v].P.x);
                pzmin = std::min(pzmin, m->vertices[v].P.z);
                pxmax = std::max(pxmax, m->vertices[v].P.x);
                pzmax = std::max(pzmax, m->vertices[v].P.z);
                umin = std::min(umin, m->vertices[v].u);
                vmin = std::min(vmin, m->vertices[v].v);
                umax = std::max(umax, m->vertices[v].u);
                vmax = std::max(vmax, m->vertices[v].v);
            }
            d.geomExtentX = pxmax - pxmin;
            d.geomExtentZ = pzmax - pzmin;
            d.uv_min_x = umin;
            d.uv_min_y = vmin;
            d.uv_max_x = umax;
            d.uv_max_y = vmax;
        }
        else
        {
            d.geomExtentX = d.geomExtentZ = 0.f;
            d.uv_min_x = d.uv_min_y = d.uv_max_x = d.uv_max_y = 0.f;
        }

        bool isDecal = (m->m_Flags.flags & 0x0001) != 0;
        if (isDecal && maxDecalIndexCount > 0)
        {
            d.decalVertexBase = runningDecalOffset;
            d.decalIndexCount = m->number_indices;
            for (u32 j = 0; j < m->number_indices; j++)
            {
                u16 idx = m->indices[j];
                DecalPulledVertex dv;
                dv.px = m->vertices[idx].P.x;
                dv.py = m->vertices[idx].P.y;
                dv.pz = m->vertices[idx].P.z;
                dv.u = m->vertices[idx].u;
                dv.v = m->vertices[idx].v;
                pulledVerts.push_back(dv);
            }
            for (u32 j = m->number_indices; j < maxDecalIndexCount; j++)
                pulledVerts.push_back(DecalPulledVertex{});
            runningDecalOffset += maxDecalIndexCount;
        }
        else
        {
            d.decalVertexBase = 0;
            d.decalIndexCount = 0;
        }
        d.pad = 0;
    }

    if (pulledVerts.empty())
        pulledVerts.push_back(DecalPulledVertex{});

    decalPulledVertexData = std::move(pulledVerts);
}

nvrhi::BindingSetHandle FGDetailManager::CreateInstanceGenBindingSet(nvrhi::IDevice* device) const
{
    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(5, cachedCullParamsCB),
        nvrhi::BindingSetItem::ConstantBuffer(6, cachedInstanceGenParamsCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(1, slotDataBuffer),
        nvrhi::BindingSetItem::Texture_SRV(2, heightmapTexture),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(3, detailModelsBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(4, perSlotPrefixBuffer),
        nvrhi::BindingSetItem::Sampler(0, cachedSmp_PointClamp),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, generatedInstancesBuffer),
        nvrhi::BindingSetItem::RawBuffer_UAV(1, instanceCounterBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(2, perSlotCountsBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(3, perSlotLocalCountersBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(4, slotAABBBuffer),
    };
    return device->createBindingSet(bindDesc, instanceGenBindingLayout);
}

void FGDetailManager::RegenerateAllInstances(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    if (!instanceGenPipeline || !heightmapTexture || !slotDataBuffer ||
        !prefixSumScanPipeline || !prefixSumTopPipeline)
    {
        Msg("! [DetailManager] RegenerateAllInstances: missing resources (genPSO=%p hmap=%p slots=%p scanPSO=%p topPSO=%p)",
            instanceGenPipeline.Get(), heightmapTexture.Get(), slotDataBuffer.Get(),
            prefixSumScanPipeline.Get(), prefixSumTopPipeline.Get());
        m_instancesNeedRegeneration = false;
        return;
    }

    constexpr u32 MAX_DISPATCH_1D = 65535;
    u32 numBlocks = (slot_count + PREFIX_SUM_BLOCK_SIZE - 1) / PREFIX_SUM_BLOCK_SIZE;
    u32 numGroupsX = std::min(slot_count, MAX_DISPATCH_1D);
    u32 numGroupsY = (slot_count + MAX_DISPATCH_1D - 1) / MAX_DISPATCH_1D;

    DetailCullParams cullParams = {};
    cullParams.detailDensity = ps_current_detail_density;
    cullParams.totalSlotCount = slot_count;
    for (u32 i = 0; i < 6; i++)
        cullParams.frustumPlanes[i].set(0, 0, 0, 1000000.0f);
    cmdList->writeBuffer(cachedCullParamsCB, &cullParams, sizeof(cullParams));

    cmdList->setBufferState(generatedInstancesBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(perSlotCountsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(perSlotPrefixBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(blockTotalsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(perSlotLocalCountersBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(instanceCounterBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(slotAABBBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->commitBarriers();

    cmdList->clearBufferUInt(perSlotCountsBuffer, 0);
    cmdList->clearBufferUInt(instanceCounterBuffer, 0);
    cmdList->clearBufferUInt(perSlotLocalCountersBuffer, 0);
    cmdList->commitBarriers();

    auto dispatchInstanceGen = [&](u32 mode, const char* passName) {
        if (gpuProfiler) gpuProfiler->BeginPass(cmdList, passName);

        InstanceGenParams genParams;
        genParams.heightmapWorldMinX = heightmapWorldMinX;
        genParams.heightmapWorldMinZ = heightmapWorldMinZ;
        genParams.heightmapTexelSize = heightmapTexelSize;
        genParams.detailHeightMultiplier = ps_current_detail_height;
        genParams.genMode = mode;
        genParams.prefixSumBlockSize = PREFIX_SUM_BLOCK_SIZE;
        genParams.prefixSumTotalBlocks = numBlocks;
        genParams.instanceCapacity = generatedInstancesCapacity;
        genParams.detailModelCount = u32(detail_models.size());
        genParams.pad0 = genParams.pad1 = genParams.pad2 = 0;
        cmdList->writeBuffer(cachedInstanceGenParamsCB, &genParams, sizeof(genParams));

        auto bindingSet = CreateInstanceGenBindingSet(device);
        nvrhi::ComputeState state;
        state.pipeline = instanceGenPipeline;
        state.bindings = { bindingSet };
        cmdList->setComputeState(state);
        cmdList->dispatch(numGroupsX, numGroupsY, 1);

        if (gpuProfiler) gpuProfiler->EndPass(cmdList, passName);
    };

    auto dispatchPrefixSum = [&](nvrhi::ComputePipelineHandle pipeline, u32 groups, const char* passName) {
        if (gpuProfiler) gpuProfiler->BeginPass(cmdList, passName);

        nvrhi::BindingSetDesc bindDesc;
        bindDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(6, cachedInstanceGenParamsCB),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, perSlotCountsBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(1, perSlotPrefixBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_UAV(2, blockTotalsBuffer),
        };
        auto bindingSet = device->createBindingSet(bindDesc, prefixSumBindingLayout);

        nvrhi::ComputeState state;
        state.pipeline = pipeline;
        state.bindings = { bindingSet };
        cmdList->setComputeState(state);
        cmdList->dispatch(groups, 1, 1);

        if (gpuProfiler) gpuProfiler->EndPass(cmdList, passName);
    };

    dispatchInstanceGen(0, "Details.Regen.Count");

    cmdList->setBufferState(perSlotCountsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->commitBarriers();

    dispatchPrefixSum(prefixSumScanPipeline, numBlocks, "Details.Regen.ScanBlocks");

    cmdList->setBufferState(perSlotPrefixBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(blockTotalsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->commitBarriers();

    dispatchPrefixSum(prefixSumTopPipeline, 1, "Details.Regen.ScanTop");

    cmdList->setBufferState(perSlotPrefixBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->commitBarriers();

    dispatchInstanceGen(1, "Details.Regen.Scatter");

    cmdList->setBufferState(generatedInstancesBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(slotAABBBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(perSlotPrefixBuffer, nvrhi::ResourceStates::UnorderedAccess);

    if (instanceCountReadbackBuffer)
    {
        cmdList->setBufferState(instanceCounterBuffer, nvrhi::ResourceStates::CopySource);
        cmdList->commitBarriers();
        cmdList->copyBuffer(instanceCountReadbackBuffer, 0, instanceCounterBuffer, 0, sizeof(u32));
        cmdList->setBufferState(instanceCounterBuffer, nvrhi::ResourceStates::UnorderedAccess);
        instanceCountReadbackPending = true;
    }

    cmdList->commitBarriers();

    m_instancesNeedRegeneration = false;

    Msg("[DetailManager] RegenerateAllInstances: 4-pass GPU dispatch complete (%u slots, density=%.3f, capacity=%u)",
        slot_count, ps_current_detail_density, generatedInstancesCapacity);
}

void FGDetailManager::ResizeVisibleBuffersIfNeeded(nvrhi::IDevice* device)
{
    if (!instanceCountReadbackPending || !instanceCountReadbackBuffer || !device)
        return;

    void* mapped = device->mapBuffer(instanceCountReadbackBuffer, nvrhi::CpuAccessMode::Read);
    if (!mapped)
        return;

    totalGeneratedInstances = *static_cast<const u32*>(mapped);
    device->unmapBuffer(instanceCountReadbackBuffer);
    instanceCountReadbackPending = false;

    if (totalGeneratedInstances >= generatedInstancesCapacity * 95 / 100)
        Msg("! [DetailManager] Generated instances at capacity: %u/%u (%.0f%%) - some grass may be missing",
            totalGeneratedInstances, generatedInstancesCapacity,
            100.f * float(totalGeneratedInstances) / float(generatedInstancesCapacity));

    u32 newCapacity = std::max(totalGeneratedInstances, 100000u);
    bool needsGrow = newCapacity > visibleBufferCapacity;
    bool needsShrink = newCapacity < visibleBufferCapacity / 2;
    if (!needsGrow && !needsShrink)
        return;

    Msg("[DetailManager] Resizing visible buffers: %u -> %u (generated=%u)",
        visibleBufferCapacity, newCapacity, totalGeneratedInstances);

    visibleBufferCapacity = newCapacity;

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = visibleBufferCapacity * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = ("DetailVisibleLOD" + std::to_string(lod)).c_str();
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        visibleInstancesBuffer[lod] = device->createBuffer(desc);
    }

    u32 decalCapacity = std::max(visibleBufferCapacity / 4, 10000u);
    nvrhi::BufferDesc desc;
    desc.byteSize = decalCapacity * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.debugName = "DetailVisibleDecals";
    desc.canHaveUAVs = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    visibleDecalInstancesBuffer = device->createBuffer(desc);
}

} // namespace xray::render::RENDER_NAMESPACE
