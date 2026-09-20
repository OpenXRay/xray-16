#include "stdafx.h"
#include "FGDetailManager.h"
#include "DetailModel.h"
#include "FrameGraph/ShaderLoader.h"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "ResourceManager/TextureManager.h"
#include "xrRender_console.h"
#include "xrEngine/device.h"
#include "xrEngine/GrassInteractionCollector.h"
#include "xrCDB/Frustum.h"
#include "xrCDB/Intersect.hpp"
#include "xrCDB/xrXRC.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrCore/Profiler/Profiler.h"
#include "Profiler/GPUProfiler.h"
#include "FrameGraphPasses/ShaderConstants.h"
#include "FrameGraphPasses/VisibilityPassSetup.h"
#include "FrameGraph/PassResourceCache.h"
#include "FrameGraph/BindingSetBuilder.h"
#include "ResourceManager/DDSLoader.h"
#include <thread>
#include <atomic>

extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r3_grass_wind_min;
extern ENGINE_API float ps_r3_grass_wind_lerp_rate;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;
extern ENGINE_API float ps_r3_grass_interaction_radius_scale;
extern ENGINE_API float ps_r3_grass_interaction_frequency;
extern ENGINE_API float ps_r3_grass_interaction_damping;
extern ENGINE_API float ps_r3_grass_interaction_max_angle;
extern ENGINE_API u32 ps_r3_grass_wind_octaves;

extern ENGINE_API float ps_r3_grass_lod_close;
extern ENGINE_API float ps_r3_grass_lod_mid;

extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;


extern ENGINE_API float ps_r__Detail_l_aniso;
extern ENGINE_API float ps_r__Detail_l_ambient;
extern ENGINE_API float ps_r3_grass_wind_multiplier;
extern ENGINE_API float ps_r3_grass_wind_min;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r3_grass_interaction_displacement;
extern ENGINE_API Fvector3 ps_r3_grass_color_tip;
extern ENGINE_API Fvector3 ps_r3_grass_color_base;
extern ENGINE_API float ps_r3_grass_color_variation;
extern ENGINE_API Fvector3 ps_r3_grass_object_tints[64];
extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;

namespace xray::render::fg
{
extern int ps_r__detail_gpu;
extern float ps_current_detail_height;
extern float ps_current_detail_density;

static int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};

static u32 BuildBladeIndices(u32 lod, u16* indices)
{
    const u32 segments = FGDetailManager::LOD_SEGMENTS[lod];
    const u32 quads = (segments - 1u) * 2u;
    for (u32 tri = 0; tri < FGDetailManager::LOD_TRIANGLES[lod]; ++tri) {
        for (u32 corner = 0; corner < 3; ++corner) {
            u32 v;
            if (tri < quads) {
                const u32 b = (tri >> 1) * 2u;
                if ((tri & 1u) == 0u)
                    v = corner == 0u ? b : (corner == 1u ? b + 2u : b + 1u);
                else
                    v = corner == 0u ? b + 1u : (corner == 1u ? b + 2u : b + 3u);
            } else {
                const u32 b = (segments - 1u) * 2u;
                v = corner == 0u ? b : (corner == 1u ? segments * 2u : b + 1u);
            }
            indices[tri * 3 + corner] = u16(v);
        }
    }
    return FGDetailManager::LOD_TRIANGLES[lod] * 3 * sizeof(u16);
}

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
    header10.dxgiFormat = xray::render::resources::DDSDxgiFormat_R32_FLOAT;
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
    texDesc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
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
    GEnv.Backend->ExecuteCommandList(cmdList);

    Msg("  ✓ Heightmap texture loaded: %ux%u R32_FLOAT, %u mips",
        texDesc.width, texDesc.height, texDesc.mipLevels);

    return true;
}

static u32 LoadOptionalDetailTexture(nvrhi::IDevice* device, const char* name, const char* debugName, nvrhi::TextureHandle& outTexture)
{
    resources::DDSData data;
    if (!resources::DDSLoader::LoadFromFile(name, data) || !data.isValid || data.mipLevels.empty())
        return 0;

    nvrhi::TextureDesc desc;
    desc.width = data.desc.width;
    desc.height = data.desc.height;
    desc.depth = 1;
    desc.arraySize = 1;
    desc.mipLevels = data.desc.mipLevels;
    desc.format = data.desc.format;
    desc.dimension = nvrhi::TextureDimension::Texture2D;
    desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
    desc.keepInitialState = true;
    desc.debugName = debugName;

    outTexture = device->createTexture(desc);
    if (!outTexture || !GEnv.Backend)
        return 0;

    auto* renderDevice = GEnv.Render->GetRenderDevice();
    for (u32 mip = 0; mip < data.mipLevels.size(); mip++)
    {
        const auto& ml = data.mipLevels[mip];
        renderDevice->UploadTextureDataToNVRHI(outTexture, 0, mip, ml.data, ml.size, ml.rowPitch, 0);
    }

    const u32 index = GEnv.Backend->RegisterBindlessTexture(outTexture);
    Msg("* [FGDetailManager] %s.dds loaded: %ux%u, %u mips, format=%d, bindless=%u",
        name, desc.width, desc.height, desc.mipLevels, (int)desc.format, index);
    return index;
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
    texDesc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
    texDesc.keepInitialState = true;
    texDesc.debugName = "BuildDetails";

    buildDetailsTexture = device->createTexture(texDesc);
    if (!buildDetailsTexture)
    {
        Msg("! [FGDetailManager] Failed to create build_details texture");
        return false;
    }

    auto* renderDevice = GEnv.Render->GetRenderDevice();
    for (u32 mip = 0; mip < ddsData.mipLevels.size(); mip++)
    {
        const auto& ml = ddsData.mipLevels[mip];
        renderDevice->UploadTextureDataToNVRHI(buildDetailsTexture, 0, mip, ml.data, ml.size, ml.rowPitch, 0);
    }

    if (GEnv.Backend)
    {
        buildDetailsBindlessIndex = GEnv.Backend->RegisterBindlessTexture(buildDetailsTexture);
        Msg("* [FGDetailManager] build_details.dds loaded: %ux%u, %u mips, format=%d, bindless=%u",
            texDesc.width, texDesc.height, texDesc.mipLevels, (int)texDesc.format, buildDetailsBindlessIndex);
    }

    buildDetailsPbrBindlessIndex = LoadOptionalDetailTexture(device, "build_details_pbr", "BuildDetailsPBR", buildDetailsPbrTexture);
    buildDetailsBumpBindlessIndex = LoadOptionalDetailTexture(device, "build_details_bump", "BuildDetailsBump", buildDetailsBumpTexture);

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
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
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
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;

        detailModelsBuffer = device->createBuffer(desc);
        if (!detailModelsBuffer)
        {
            Msg("! [FGDetailManager] Failed to create detail models buffer");
            return false;
        }

        Msg("* [FGDetailManager] Created detail models buffer: %u models", detail_models.size());

        nvrhi::BufferDesc pvDesc;
        pvDesc.byteSize = pulledVertexData.size() * sizeof(DecalPulledVertex);
        pvDesc.structStride = sizeof(DecalPulledVertex);
        pvDesc.debugName = "DetailPulledVerts";
        pvDesc.canHaveUAVs = false;
        pvDesc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        pvDesc.keepInitialState = true;
        pulledVertexBuffer = device->createBuffer(pvDesc);

        if (maxPulledIndexCount > 0)
            Msg("* [FGDetailManager] Pulled vertices: %u entries (%u models, max %u indices/model)",
                (u32)pulledVertexData.size(), (u32)(pulledVertexData.size() / maxPulledIndexCount), maxPulledIndexCount);
    }

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
    {
        {
            nvrhi::BufferDesc desc;
            desc.byteSize = LOD_TRIANGLES[lod] * 3 * sizeof(u16);
            desc.debugName = ("DetailBladeIndicesLOD" + std::to_string(lod)).c_str();
            desc.isIndexBuffer = true;
            desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
            desc.keepInitialState = true;
            bladeIndexBuffer[lod] = device->createBuffer(desc);
            if (!bladeIndexBuffer[lod])
            {
                Msg("! [FGDetailManager] Failed to create blade index buffer LOD%u", lod);
                return false;
            }
            bladeIndicesUploaded = false;
        }

    }


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


    auto* renderDevice = GEnv.Render->GetRenderDevice();

    fg::RenderDevice::BufferDesc cbDesc;
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 512;

    cbDesc.byteSize = sizeof(DetailCullParams);
    cbDesc.debugName = "DetailCullParams";
    cachedCullParamsCB = renderDevice->CreateBuffer(cbDesc);

    cbDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    cbDesc.byteSize = sizeof(InstanceGenParams);
    cbDesc.debugName = "InstanceGenParams";
    cachedInstanceGenParamsCB = renderDevice->CreateBuffer(cbDesc);

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(GrassObjectTint) * 64;
        desc.structStride = sizeof(GrassObjectTint);
        desc.debugName = "GrassObjectTints";
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        cachedGrassTintsBuffer = device->createBuffer(desc);
    }

    cachedResourcesInitialized = true;
    Msg("* [FGDetailManager] Cached per-frame resources created (4 samplers, 6 volatile CBs, 3 buffers)");
    return true;
}

void FGDetailManager::DestroyGPUBuffers()
{
    DestroyInstanceStorage();
    slotDataBuffer = nullptr;
    detailModelsBuffer = nullptr;
    instanceGenComputeShader = nullptr;
    instanceGenBindingLayout = nullptr;
    instanceGenPipeline = nullptr;
    instanceCountComputeShader = nullptr;
    instanceCountBindingLayout = nullptr;
    instanceCountPipeline = nullptr;

    for (u32 lod = 0; lod < LOD_COUNT; lod++)
        bladeIndexBuffer[lod] = nullptr;


    slotCullComputeShader = nullptr;
    slotCullBindingLayout = nullptr;
    slotCullPipeline = nullptr;

    computePipeline = nullptr;
    computeBindingLayout = nullptr;
    slotArgsComputeShader = nullptr;
    slotArgsBindingLayout = nullptr;
    slotArgsPipeline = nullptr;
    visibilityArgsComputeShader = nullptr;
    visibilityArgsBindingLayout = nullptr;
    visibilityArgsPipeline = nullptr;
    cullComputeShader = nullptr;


    pulledVertexBuffer = nullptr;
    buildDetailsTexture = nullptr;
    buildDetailsPbrTexture = nullptr;
    buildDetailsBumpTexture = nullptr;

    perlin4dTexture = nullptr;
    perlin4dComputeShader = nullptr;
    perlin4dBindingLayout = nullptr;
    perlin4dPipeline = nullptr;
    perlin4dCB = fg::BufferHandle();

    interactionTexture[0] = nullptr;
    interactionTexture[1] = nullptr;
    interactionValid[0] = false;
    interactionValid[1] = false;
    interactionCurrent = 0;
    interactionDispatchFrame = 0;
    interactionEntityBuffer = nullptr;
    interactionComputeShader = nullptr;
    interactionBindingLayout = nullptr;
    interactionPipeline = nullptr;
    interactionCB = fg::BufferHandle();


    cachedSmp_LinearWrap = nullptr;
    cachedSmp_PointClamp = nullptr;
    cachedSmp_LinearClamp = nullptr;
    cachedSmp_AnisoWrap = nullptr;
    cachedCullParamsCB = fg::BufferHandle();
    cachedInstanceGenParamsCB = fg::BufferHandle();
    cachedGrassTintsBuffer = nullptr;
    cachedResourcesInitialized = false;
}

void FGDetailManager::InvalidateShadersAndPipelines()
{
    instanceGenComputeShader = nullptr;
    instanceGenBindingLayout = nullptr;
    instanceGenPipeline = nullptr;
    instanceCountComputeShader = nullptr;
    instanceCountBindingLayout = nullptr;
    instanceCountPipeline = nullptr;

    slotCullComputeShader = nullptr;
    slotCullBindingLayout = nullptr;
    slotCullPipeline = nullptr;

    cullComputeShader = nullptr;
    computeBindingLayout = nullptr;
    computePipeline = nullptr;
    slotArgsComputeShader = nullptr;
    slotArgsBindingLayout = nullptr;
    slotArgsPipeline = nullptr;
    visibilityArgsComputeShader = nullptr;
    visibilityArgsBindingLayout = nullptr;
    visibilityArgsPipeline = nullptr;

    perlin4dComputeShader = nullptr;
    perlin4dBindingLayout = nullptr;
    perlin4dPipeline = nullptr;

    interactionComputeShader = nullptr;
    interactionBindingLayout = nullptr;
    interactionPipeline = nullptr;


    m_instancesNeedRegeneration = true;
}

bool FGDetailManager::CreatePerlin4DTexture(nvrhi::IDevice* device)
{
    if (!device)
        return false;

    nvrhi::TextureDesc desc;
    desc.width            = PERLIN4D_TEXTURE_SIZE;
    desc.height           = PERLIN4D_TEXTURE_SIZE;
    desc.depth            = PERLIN4D_TEXTURE_SIZE;
    desc.dimension        = nvrhi::TextureDimension::Texture3D;
    desc.format           = nvrhi::Format::RGBA16_FLOAT;
    desc.isUAV            = true;
    desc.initialState     = nvrhi::ResourceStates::NonPixelShaderResource;
    desc.keepInitialState = true;
    desc.debugName        = "Perlin4DVolume";

    perlin4dTexture = device->createTexture(desc);
    if (!perlin4dTexture)
    {
        Msg("! [FGDetailManager] Failed to create Perlin4D volume texture");
        return false;
    }

    Msg("* [FGDetailManager] Perlin4D volume texture created (%u^3 RGBA16F, %u KB)",
        PERLIN4D_TEXTURE_SIZE, PERLIN4D_TEXTURE_SIZE * PERLIN4D_TEXTURE_SIZE * PERLIN4D_TEXTURE_SIZE * 8 / 1024);

    return true;
}

bool FGDetailManager::LoadPerlin4DComputeShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
        return false;

    perlin4dComputeShader = shaderLoader->LoadComputeShader("perlin4d_gen", "main").handle;
    if (!perlin4dComputeShader)
    {
        Msg("! [FGDetailManager] Failed to load perlin4d_gen compute shader");
        return false;
    }

    return true;
}

bool FGDetailManager::CreatePerlin4DPipeline(nvrhi::IDevice* device)
{
    if (!device || !perlin4dComputeShader)
        return false;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("perlin4d_gen", ".cs");
    if (!refl)
    {
        Msg("! [FGDetailManager] Failed to get Perlin4D reflection");
        return false;
    }

    perlin4dBindingLayout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("DetailPerlin4D", *refl, device);
    if (!perlin4dBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create Perlin4D binding layout");
        return false;
    }

    nvrhi::ComputePipelineDesc pipelineDesc;
    pipelineDesc.CS = perlin4dComputeShader;
    pipelineDesc.bindingLayouts = { perlin4dBindingLayout };

    perlin4dPipeline = device->createComputePipeline(pipelineDesc);
    if (!perlin4dPipeline)
    {
        Msg("! [FGDetailManager] Failed to create Perlin4D compute pipeline");
        return false;
    }

    // Volatile constant buffer for per-frame dispatch params
    fg::RenderDevice::BufferDesc perlinCBDesc;
    perlinCBDesc.byteSize         = 16;
    perlinCBDesc.isVolatile       = true;
    perlinCBDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    perlinCBDesc.isConstantBuffer = true;
    perlinCBDesc.debugName        = "Perlin4DGenCB";
    perlin4dCB = GEnv.Render->GetRenderDevice()->CreateBuffer(perlinCBDesc);

    return true;
}

void FGDetailManager::DispatchPerlin4DCompute(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device, float time)
{
    if (!perlin4dPipeline || !perlin4dTexture || !perlin4dCB.IsValid())
        return;

    // CB data: must match perlin4d_gen.cs Perlin4DGenParams
    struct Perlin4DGenParams
    {
        float time;
        float tileScale;
        u32   textureSize;
        float pad0;
    };

    Perlin4DGenParams params;
    params.time        = time * 0.5f;  // Match g_TurbEvolution = fTimeGlobal * 0.5
    params.tileScale   = 4.0f;         // 4 noise periods across volume
    params.textureSize = PERLIN4D_TEXTURE_SIZE;
    params.pad0        = 0.f;

    // writeBuffer BEFORE setComputeState (NVRHI volatile CB ordering)
    auto* renderDevice = GEnv.Render->GetRenderDevice();
    cmdList->writeBuffer(renderDevice->GetNativeBuffer(perlin4dCB), &params, sizeof(params));

    auto* perlin4dRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("perlin4d_gen", ".cs");
    if (!perlin4dRefl)
        return;
    framegraph::BindingSetBuilder bsb(*perlin4dRefl, device, "Detail.Perlin4D");
    bsb.ConstantBuffer("Perlin4DGenParams", renderDevice->GetNativeBuffer(perlin4dCB))
       .TextureUAV("g_output", perlin4dTexture);
    auto bindSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), perlin4dBindingLayout, device);

    nvrhi::ComputeState cs;
    cs.pipeline = perlin4dPipeline;
    cs.bindings = { bindSet };
    cmdList->setComputeState(cs);

    // 3D dispatch: 64/4 = 16 groups per dimension
    cmdList->dispatch(PERLIN4D_TEXTURE_SIZE / 8, PERLIN4D_TEXTURE_SIZE / 8, PERLIN4D_TEXTURE_SIZE / 8);
}

bool FGDetailManager::CreateInteractionResources(nvrhi::IDevice* device)
{
    if (!device)
        return false;

    for (u32 i = 0; i < 2; ++i)
    {
        nvrhi::TextureDesc desc;
        desc.width = INTERACTION_TEXTURE_SIZE;
        desc.height = INTERACTION_TEXTURE_SIZE;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
        desc.keepInitialState = true;
        desc.debugName = i == 0 ? "DetailInteraction0" : "DetailInteraction1";
        interactionTexture[i] = device->createTexture(desc);
        if (!interactionTexture[i])
        {
            Msg("! [FGDetailManager] Failed to create interaction texture %u", i);
            return false;
        }
        interactionValid[i] = false;
    }
    interactionCurrent = 0;
    interactionDispatchFrame = 0;

    nvrhi::BufferDesc bufDesc;
    bufDesc.byteSize = sizeof(InteractionEntity) * INTERACTION_MAX_ENTITIES;
    bufDesc.structStride = sizeof(InteractionEntity);
    bufDesc.debugName = "DetailInteractionEntities";
    bufDesc.initialState = nvrhi::ResourceStates::NonPixelShaderResource;
    bufDesc.keepInitialState = true;
    interactionEntityBuffer = device->createBuffer(bufDesc);
    if (!interactionEntityBuffer)
    {
        Msg("! [FGDetailManager] Failed to create interaction entity buffer");
        return false;
    }
    interactionEntities.reserve(INTERACTION_MAX_ENTITIES);

    Msg("* [FGDetailManager] Interaction window created (2x %u^2 RGBA16F, %.1f m span, %u MB)",
        INTERACTION_TEXTURE_SIZE, INTERACTION_TEXTURE_SIZE * INTERACTION_TEXEL_SIZE,
        2 * INTERACTION_TEXTURE_SIZE * INTERACTION_TEXTURE_SIZE * 8 / (1024 * 1024));
    return true;
}

bool FGDetailManager::LoadInteractionComputeShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
        return false;

    interactionComputeShader = shaderLoader->LoadComputeShader("detail_interaction", "main").handle;
    if (!interactionComputeShader)
    {
        Msg("! [FGDetailManager] Failed to load detail_interaction compute shader");
        return false;
    }
    return true;
}

bool FGDetailManager::CreateInteractionPipeline(nvrhi::IDevice* device)
{
    if (!device || !interactionComputeShader)
        return false;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* refl = shaderLoader->GetCachedReflection("detail_interaction", ".cs");
    if (!refl)
    {
        Msg("! [FGDetailManager] Failed to get interaction reflection");
        return false;
    }

    interactionBindingLayout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("DetailInteraction", *refl, device);
    if (!interactionBindingLayout)
    {
        Msg("! [FGDetailManager] Failed to create interaction binding layout");
        return false;
    }

    nvrhi::ComputePipelineDesc pipelineDesc;
    pipelineDesc.CS = interactionComputeShader;
    pipelineDesc.bindingLayouts = { interactionBindingLayout };
    interactionPipeline = device->createComputePipeline(pipelineDesc);
    if (!interactionPipeline)
    {
        Msg("! [FGDetailManager] Failed to create interaction compute pipeline");
        return false;
    }

    if (!interactionCB.IsValid())
    {
        fg::RenderDevice::BufferDesc cbDesc;
        cbDesc.byteSize = sizeof(InteractionParams);
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
        cbDesc.isConstantBuffer = true;
        cbDesc.debugName = "DetailInteractionCB";
        interactionCB = GEnv.Render->GetRenderDevice()->CreateBuffer(cbDesc);
    }
    return true;
}

void FGDetailManager::DispatchInteraction(nvrhi::ICommandList* cmdList, nvrhi::IDevice* device)
{
    if (!interactionPipeline || !interactionTexture[0] || !interactionTexture[1] || !interactionEntityBuffer
        || !heightmapTexture || !interactionCB.IsValid())
        return;

    const u32 prev = interactionCurrent;
    const u32 write = prev ^ 1u;
    const float texel = INTERACTION_TEXEL_SIZE;
    const float halfSpan = 0.5f * INTERACTION_TEXTURE_SIZE * texel;
    const Fvector& cam = Device.vCameraPosition;

    Fvector2 origin;
    origin.x = floorf((cam.x - halfSpan) / texel) * texel;
    origin.y = floorf((cam.z - halfSpan) / texel) * texel;

    const int shiftX = int(lroundf((origin.x - interactionOrigin[prev].x) / texel));
    const int shiftY = int(lroundf((origin.y - interactionOrigin[prev].y) / texel));
    const bool prevValid = interactionValid[prev]
        && _abs(shiftX) < int(INTERACTION_TEXTURE_SIZE) && _abs(shiftY) < int(INTERACTION_TEXTURE_SIZE);

    interactionEntities.clear();
    xr_vector<GrassInteractionEntity> gameEntities;
    g_GrassInteractionCollector.GetEntitiesForFrame(gameEntities);
    for (const GrassInteractionEntity& e : gameEntities)
    {
        const float radius = e.radius * ps_r3_grass_interaction_radius_scale;
        if (radius <= 0.0f || e.weight <= 0.0f)
            continue;
        if (_abs(e.position.x - cam.x) > halfSpan + radius || _abs(e.position.z - cam.z) > halfSpan + radius)
            continue;
        InteractionEntity& out = interactionEntities.emplace_back();
        out.pos = e.position;
        out.radius = radius;
        out.vel = e.velocity;
        out.weight = e.weight;
        if (interactionEntities.size() >= INTERACTION_MAX_ENTITIES)
            break;
    }

    const float dt = _min(Device.fTimeDelta, 0.1f);
    const float omega0 = 2.0f * PI * _max(ps_r3_grass_interaction_frequency, 0.05f);
    const float zeta = clampr(ps_r3_grass_interaction_damping, 0.01f, 0.98f);
    const float omegaD = omega0 * _sqrt(1.0f - zeta * zeta);
    const float decay = expf(-zeta * omega0 * dt);
    const float cs = _cos(omegaD * dt);
    const float sn = _sin(omegaD * dt);

    InteractionParams params;
    params.originCur = origin;
    params.originPrev = interactionOrigin[prev];
    params.prevShiftX = shiftX;
    params.prevShiftY = shiftY;
    params.texelSize = texel;
    params.size = INTERACTION_TEXTURE_SIZE;
    params.spring.set(
        decay * (cs + zeta * omega0 / omegaD * sn),
        decay * sn / omegaD,
        -decay * omega0 * omega0 / omegaD * sn,
        decay * (cs - zeta * omega0 / omegaD * sn));
    params.contactRate = 1.0f - expf(-dt / 0.04f);
    params.dt = dt;
    params.maxVelocity = 12.0f;
    params.entityCount = u32(interactionEntities.size());
    params.heightmapMinX = heightmapWorldMinX;
    params.heightmapMinZ = heightmapWorldMinZ;
    params.heightmapTexelSize = heightmapTexelSize;
    params.prevValid = prevValid ? 1u : 0u;

    auto* renderDevice = GEnv.Render->GetRenderDevice();
    if (!interactionEntities.empty())
        cmdList->writeBuffer(interactionEntityBuffer, interactionEntities.data(), interactionEntities.size() * sizeof(InteractionEntity));
    cmdList->writeBuffer(renderDevice->GetNativeBuffer(interactionCB), &params, sizeof(params));

    auto* refl = GEnv.Render->GetShaderLoader()->GetCachedReflection("detail_interaction", ".cs");
    framegraph::BindingSetBuilder bsb(*refl, device, "Detail.Interaction");
    bsb.ConstantBuffer("InteractionParams", renderDevice->GetNativeBuffer(interactionCB))
       .BufferSRV("g_entities", interactionEntityBuffer)
       .Texture("g_prev", interactionTexture[prev])
       .Texture("g_heightmap", heightmapTexture)
       .TextureUAV("g_cur", interactionTexture[write]);
    auto bindSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), interactionBindingLayout, device);
    if (!bindSet)
        return;

    nvrhi::ComputeState state;
    state.pipeline = interactionPipeline;
    state.bindings = { bindSet };
    cmdList->setComputeState(state);
    cmdList->dispatch(INTERACTION_TEXTURE_SIZE / 8, INTERACTION_TEXTURE_SIZE / 8, 1);

    interactionOrigin[write] = origin;
    interactionValid[write] = true;
    interactionCurrent = write;
    interactionDispatchFrame = Device.dwFrame;
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
            aabb.instance_chunk = 0;
            std::fill(std::begin(aabb.padding2), std::end(aabb.padding2), 0u);
        }
    }

    slots_chunk->close();

    slot_count = total_slots;

    Msg("* [FGDetailManager] Computed %u slot AABBs", total_slots);
}

bool FGDetailManager::LoadCullComputeShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
        return false;
    slotCullComputeShader = shaderLoader->LoadComputeShader("detail_cell_cull", "main").handle;
    cullComputeShader = shaderLoader->LoadComputeShader("detail_cull", "main").handle;
    slotArgsComputeShader = shaderLoader->LoadComputeShader("detail_work_args", "main_slots").handle;
    visibilityArgsComputeShader = shaderLoader->LoadComputeShader("detail_work_args", "main").handle;
    return slotCullComputeShader && cullComputeShader && slotArgsComputeShader && visibilityArgsComputeShader;
}

bool FGDetailManager::LoadInstanceGenShader(framegraph::ShaderLoader* shaderLoader)
{
    if (!shaderLoader)
        return false;
    instanceGenComputeShader = shaderLoader->LoadComputeShader("detail_instance_gen", "main").handle;
    instanceCountComputeShader = shaderLoader->LoadComputeShader("detail_instance_count", "main").handle;
    return instanceGenComputeShader && instanceCountComputeShader;
}



void FGDetailManager::UploadBufferData(nvrhi::ICommandList* cmdList)
{
    if (!cmdList)
        return;

    if (slotDataBuffer && !slotDataCPU.empty())
        cmdList->writeBuffer(slotDataBuffer, slotDataCPU.data(), slotDataCPU.size() * sizeof(GPUSlotData));

    if (detailModelsBuffer && !cachedModelGPUData.empty())
        cmdList->writeBuffer(detailModelsBuffer, cachedModelGPUData.data(), cachedModelGPUData.size() * sizeof(DetailModelGPU));

    if (pulledVertexBuffer && !pulledVertexData.empty())
        cmdList->writeBuffer(pulledVertexBuffer, pulledVertexData.data(), pulledVertexData.size() * sizeof(DecalPulledVertex));

    pulledVertexData.clear();
    pulledVertexData.shrink_to_fit();

}

bool FGDetailManager::CreateComputePipeline(fg::RenderDevice* renderDevice)
{
    if (!renderDevice || !cullComputeShader || !slotCullComputeShader || !slotArgsComputeShader || !visibilityArgsComputeShader)
        return false;
    auto* device = renderDevice->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    const auto create = [&](const char* name, const char* suffix, const char* key,
        const nvrhi::ShaderHandle& shader, nvrhi::BindingLayoutHandle& layout,
        nvrhi::ComputePipelineHandle& pipeline, bool chunks)
    {
        if (pipeline && (!chunks || m_cullSourceLayout == generatedInstances->bindingLayout))
            return true;
        auto* reflection = shaderLoader->GetCachedReflection(name, suffix);
        if (!reflection)
            return false;
        layout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(key, *reflection, device);
        if (!layout)
            return false;
        nvrhi::ComputePipelineDesc desc;
        desc.CS = shader;
        desc.bindingLayouts = { layout };
        if (chunks)
        {
            desc.bindingLayouts.push_back(GEnv.Backend->GetBindlessLayout());
            desc.bindingLayouts.push_back(generatedInstances->bindingLayout);
        }
        pipeline = device->createComputePipeline(desc);
        if (pipeline && chunks)
            m_cullSourceLayout = generatedInstances->bindingLayout;
        return bool(pipeline);
    };
    if (!create("detail_cell_cull", ".cs", "DetailSlotCull", slotCullComputeShader, slotCullBindingLayout, slotCullPipeline, false) ||
        !create("detail_work_args", ".cs:main_slots", "DetailSlotArgs", slotArgsComputeShader, slotArgsBindingLayout, slotArgsPipeline, false) ||
        !create("detail_work_args", ".cs", "DetailVisibilityArgs", visibilityArgsComputeShader, visibilityArgsBindingLayout, visibilityArgsPipeline, false))
        return false;
    if (generatedInstances && !generatedInstances->chunks.empty())
        return create("detail_cull", ".cs", "DetailCull", cullComputeShader, computeBindingLayout, computePipeline, true);
    return true;
}

bool FGDetailManager::CreateInstanceGenPipeline(fg::RenderDevice* renderDevice)
{
    if (!renderDevice || !instanceGenComputeShader || !instanceCountComputeShader)
        return false;
    auto* device = renderDevice->GetNVRHIDevice();
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    for (u32 i = 0; i < 2; ++i)
    {
        const bool count = i == 0;
        auto& layout = count ? instanceCountBindingLayout : instanceGenBindingLayout;
        auto& pipeline = count ? instanceCountPipeline : instanceGenPipeline;
        if (pipeline)
            continue;
        auto* reflection = shaderLoader->GetCachedReflection(count ? "detail_instance_count" : "detail_instance_gen", ".cs");
        if (!reflection)
            return false;
        layout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(
            count ? "DetailInstanceCount" : "DetailInstanceEmit", *reflection, device);
        if (!layout)
            return false;
        nvrhi::ComputePipelineDesc desc;
        desc.CS = count ? instanceCountComputeShader : instanceGenComputeShader;
        desc.bindingLayouts = { layout };
        pipeline = device->createComputePipeline(desc);
        if (!pipeline)
            return false;
    }
    return true;
}

void FGDetailManager::DispatchCulling(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* device,
    nvrhi::ITexture* hiZPyramid,
    VisibilityFrame& frame,
    const std::shared_ptr<GenerationWork>& generation,
    const Fmatrix& prevViewProj,
    float fadeDistance,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels,
    xray::profiler::GPUProfiler* gpuProfiler)
{
    frame.lease = GEnv.Backend->OpenSubmissionLease();
    R_ASSERT2(frame.lease, "[DetailManager] visibility submission lease unavailable");
    ClearDrawArgs(cmdList, frame);
    if (generation)
        RecordGeneration(cmdList, device, *generation);
    if (!frame.source || frame.source->chunks.empty())
        return;
    R_ASSERT2(computePipeline && slotCullPipeline && slotArgsPipeline && visibilityArgsPipeline &&
        perlin4dTexture && interactionTexture[interactionCurrent], "[DetailManager] culling resources unavailable");
    R_ASSERT2(m_cullSourceLayout == frame.source->bindingLayout, "[DetailManager] culling source layout mismatch");
    auto* renderDevice = GEnv.Render->GetRenderDevice();
    auto& params = frame.cullParams;
    params.prevViewProj = prevViewProj;
    params.fadeDistanceSqr = fadeDistance * fadeDistance;
    params.hizWidth = hiZWidth;
    params.hizHeight = hiZHeight;
    params.hizMipLevels = hiZMipLevels;
    cmdList->writeBuffer(renderDevice->GetNativeBuffer(cachedCullParamsCB), &params, sizeof(params));
    for (u32 kind = 0; kind < VIS_KIND_COUNT; ++kind)
    {
        const u32 args[5] = { kind < LOD_COUNT ? LOD_TRIANGLES[kind] * 3u : frame.source->maxPulledIndexCount, 0, 0, 0, 0 };
        cmdList->writeBuffer(frame.drawArgs[kind], args, sizeof(args));
    }
    if (!bladeIndicesUploaded)
    {
        for (u32 lod = 0; lod < LOD_COUNT; ++lod)
        {
            u16 indices[LOD_TRIANGLES[0] * 3];
            const u32 bytes = BuildBladeIndices(lod, indices);
            cmdList->writeBuffer(bladeIndexBuffer[lod], indices, bytes);
        }
        bladeIndicesUploaded = true;
    }
    if (gpuProfiler)
        gpuProfiler->BeginPass(cmdList, "DetailCull.SlotCull");
    {
        auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("detail_cell_cull", ".cs");
        R_ASSERT2(reflection, "[DetailManager] slot culling shader unavailable");
        framegraph::BindingSetBuilder bindings(*reflection, device, "Detail.SlotCull");
        bindings.ConstantBuffer("DetailCullParams", renderDevice->GetNativeBuffer(cachedCullParamsCB))
            .BufferSRV("g_slot_aabbs", frame.source->slots)
            .BufferUAV("g_visible_slot_ids", frame.visibleSlots)
            .BufferUAV("g_visible_slot_counter", frame.visibleSlotCount);
        auto set = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindings.Build(), slotCullBindingLayout, device);
        R_ASSERT2(set, "[DetailManager] slot culling binding set unavailable");
        nvrhi::ComputeState state;
        state.pipeline = slotCullPipeline;
        state.bindings = { set };
        cmdList->setComputeState(state);
        const u32 groups = slot_count / 256u + u32(slot_count % 256u != 0);
        const u32 rows = (groups + 1023u) / 1024u;
        cmdList->dispatch(rows > 1 ? 1024u : groups, rows, 1);
    }
    if (gpuProfiler)
        gpuProfiler->EndPass(cmdList, "DetailCull.SlotCull");
    RecordVisibilityWork(cmdList, device, frame, true);
    if (gpuProfiler)
        gpuProfiler->BeginPass(cmdList, "DetailCull.InstanceCull");
    {
        auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("detail_cull", ".cs");
        R_ASSERT2(reflection, "[DetailManager] instance culling shader unavailable");
        auto globals = framegraph::GetPassResourceCache().GetOrCreateVolatileCB("Detail", "DetailGlobals", sizeof(DetailFrameConstants), renderDevice);
        DetailFrameConstants constants;
        FillFrameConstants(constants);
        cmdList->writeBuffer(globals, &constants, sizeof(constants));
        for (u32 chunk : frame.visibleChunks)
            cmdList->setBufferState(frame.source->chunks[chunk].buffer, nvrhi::ResourceStates::NonPixelShaderResource);
        framegraph::BindingSetBuilder bindings(*reflection, device, "Detail.Cull");
        bindings.ConstantBuffer("DetailCullParams", renderDevice->GetNativeBuffer(cachedCullParamsCB))
            .ConstantBuffer("DetailGlobals", globals)
            .BufferSRV("g_visible_slot_ids", frame.visibleSlots)
            .BufferSRV("g_visible_slot_count", frame.visibleSlotCount)
            .BufferSRV("g_slot_aabbs", frame.source->slots)
            .Texture("g_hiz_pyramid", hiZPyramid)
            .BufferSRV("g_detail_models", frame.source->models)
            .Texture("g_Perlin4D", perlin4dTexture)
            .Texture("g_Interaction", interactionTexture[interactionCurrent])
            .BufferUAV("g_visible_lod0", frame.visible[0])
            .BufferUAV("g_indirect_args_lod0", frame.drawArgs[0])
            .BufferUAV("g_visible_lod1", frame.visible[1])
            .BufferUAV("g_indirect_args_lod1", frame.drawArgs[1])
            .BufferUAV("g_visible_lod2", frame.visible[2])
            .BufferUAV("g_indirect_args_lod2", frame.drawArgs[2])
            .BufferUAV("g_visible_decals", frame.visible[VIS_KIND_DECAL])
            .BufferUAV("g_indirect_args_decal", frame.drawArgs[VIS_KIND_DECAL])
            .BufferUAV("g_visible_billboard", frame.visible[VIS_KIND_MESH])
            .BufferUAV("g_indirect_args_billboard", frame.drawArgs[VIS_KIND_MESH])
            .BufferUAV("g_prepared_lod0", frame.prepared[0])
            .BufferUAV("g_prepared_lod1", frame.prepared[1])
            .BufferUAV("g_prepared_lod2", frame.prepared[2])
            .BufferUAV("g_work_status", frame.workStatus);
        auto set = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindings.Build(), computeBindingLayout, device);
        R_ASSERT2(set, "[DetailManager] instance culling binding set unavailable");
        nvrhi::ComputeState state;
        state.pipeline = computePipeline;
        state.bindings = { set, GEnv.Backend->GetBindlessDescriptorTable(), frame.source->descriptorTable };
        state.indirectParams = frame.slotDispatch;
        cmdList->setComputeState(state);
        cmdList->dispatchIndirect(0);
    }
    if (gpuProfiler)
        gpuProfiler->EndPass(cmdList, "DetailCull.InstanceCull");
    RecordVisibilityWork(cmdList, device, frame, false);
    for (u32 kind = 0; kind < VIS_KIND_COUNT; ++kind)
    {
        cmdList->setBufferState(frame.drawArgs[kind], nvrhi::ResourceStates::IndirectArgument);
        cmdList->setBufferState(frame.visible[kind], nvrhi::ResourceStates::NonPixelShaderResource);
    }
    for (auto& prepared : frame.prepared)
        cmdList->setBufferState(prepared, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(frame.packets, nvrhi::ResourceStates::NonPixelShaderResource);
    cmdList->setBufferState(frame.swDispatch, nvrhi::ResourceStates::IndirectArgument);
}



void FGDetailManager::BuildDetailModelGPUData()
{
    const u32 maxPulledIndices = MAX_PULLED_TRIANGLES * 3;
    maxPulledIndexCount = 0;
    for (u32 i = 0; i < detail_models.size(); i++)
    {
        const u32 count = detail_models[i]->number_indices;
        if (count > maxPulledIndices)
            Msg("! [FGDetailManager] Detail model %u has %u indices, drawing the first %u", i, count, maxPulledIndices);
        maxPulledIndexCount = std::max(maxPulledIndexCount, std::min(count, maxPulledIndices));
    }

    cachedModelGPUData.resize(detail_models.size());
    xr_vector<DecalPulledVertex> pulledVerts;
    u32 runningOffset = 0;

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
            float pymin = FLT_MAX, pymax = -FLT_MAX;
            float umin = FLT_MAX, vmin = FLT_MAX, umax = -FLT_MAX, vmax = -FLT_MAX;
            for (u32 v = 0; v < m->number_vertices; v++)
            {
                pxmin = std::min(pxmin, m->vertices[v].P.x);
                pzmin = std::min(pzmin, m->vertices[v].P.z);
                pxmax = std::max(pxmax, m->vertices[v].P.x);
                pzmax = std::max(pzmax, m->vertices[v].P.z);
                pymin = std::min(pymin, m->vertices[v].P.y);
                pymax = std::max(pymax, m->vertices[v].P.y);
                umin = std::min(umin, m->vertices[v].u);
                vmin = std::min(vmin, m->vertices[v].v);
                umax = std::max(umax, m->vertices[v].u);
                vmax = std::max(vmax, m->vertices[v].v);
            }
            d.geomExtentX = pxmax - pxmin;
            d.geomExtentZ = pzmax - pzmin;
            d.geomExtentY = pymax - pymin;
            d.uv_min_x = umin;
            d.uv_min_y = vmin;
            d.uv_max_x = umax;
            d.uv_max_y = vmax;
        }
        else
        {
            d.geomExtentX = d.geomExtentZ = d.geomExtentY = 0.f;
            d.uv_min_x = d.uv_min_y = d.uv_max_x = d.uv_max_y = 0.f;
        }

        if (m->number_indices > 0 && maxPulledIndexCount > 0)
        {
            d.pulledVertexBase = runningOffset;
            d.pulledIndexCount = std::min<u32>(m->number_indices, maxPulledIndices);
            for (u32 j = 0; j < d.pulledIndexCount; j++)
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
            for (u32 j = d.pulledIndexCount; j < maxPulledIndexCount; j++)
                pulledVerts.push_back(DecalPulledVertex{});
            runningOffset += maxPulledIndexCount;
        }
        else
        {
            d.pulledVertexBase = 0;
            d.pulledIndexCount = 0;
        }
    }

    if (pulledVerts.empty())
        pulledVerts.push_back(DecalPulledVertex{});

    pulledVertexData = std::move(pulledVerts);
}





void FGDetailManager::FillFrameConstants(DetailFrameConstants& fc)
{
    float windAngleDeg = 0.0f;
    if (g_pGamePersistent)
    {
        windSpeed = _max(g_pGamePersistent->Environment().CurrentEnv.wind_velocity * ps_r3_grass_wind_multiplier, ps_r3_grass_wind_min);
        windAngleDeg = g_pGamePersistent->Environment().CurrentEnv.wind_direction;
        float windRad = deg2rad(windAngleDeg);
        windDirection.set(_cos(windRad), _sin(windRad));
    }

    const float quant = 16384.0f;
    fc.consts.set(1.0f / quant, 1.0f / quant, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    fc.wave.set(1.0f / 5.0f, 1.0f / 7.0f, 1.0f / 3.0f, Device.fTimeGlobal);
    fc.dir2D.set(windDirection.x, windDirection.y, 0.0f, 0.0f);
    fc.dir2D_2.set(-windDirection.y, windDirection.x, 0.0f, 0.0f);
    fc.viewProj = Device.mFullTransform;
    fc.detail_params.set(float(dtH.x_size()), float(dtH.z_size()), float(dtH.x_offs()), float(dtH.z_offs()));
    fc.g_wind_direction.set(windAngleDeg, windSpeed, 0.0f, 0.0f);
    fc.grass_wind_displacement = ps_r3_grass_wind_displacement;
    fc.grass_interaction_displacement = ps_r3_grass_interaction_displacement;
    fc.grass_interaction_max_angle = ps_r3_grass_interaction_max_angle;
    fc.grass_blade_width = ps_r3_grass_blade_width;
    fc.grass_color_tip.set(ps_r3_grass_color_tip.x, ps_r3_grass_color_tip.y, ps_r3_grass_color_tip.z, 0.0f);
    fc.grass_color_base.set(ps_r3_grass_color_base.x, ps_r3_grass_color_base.y, ps_r3_grass_color_base.z, 0.0f);
    fc.grass_color_variation = ps_r3_grass_color_variation;
    fc.grass_blade_height = ps_r3_grass_blade_height;
    fc.buildDetailsIndex = buildDetailsBindlessIndex;
    fc.buildDetailsPbrIndex = buildDetailsPbrBindlessIndex;
    fc.buildDetailsBumpIndex = buildDetailsBumpBindlessIndex;

    const u32 cur = interactionCurrent;
    const u32 prev = cur ^ 1u;
    const bool live = interactionDispatchFrame == Device.dwFrame && interactionValid[cur];
    const float invSpan = 1.0f / (INTERACTION_TEXTURE_SIZE * INTERACTION_TEXEL_SIZE);
    fc.interaction_window.set(interactionOrigin[cur].x, interactionOrigin[cur].y, invSpan, live ? 1.0f : 0.0f);
    fc.interaction_window_prev.set(interactionOrigin[prev].x, interactionOrigin[prev].y, invSpan, (live && interactionValid[prev]) ? 1.0f : 0.0f);
    fc.grass_pad0 = fc.grass_pad1 = fc.grass_pad2 = 0.0f;
}

void FGDetailManager::UploadGrassTints(nvrhi::ICommandList* cmdList)
{
    if (!cmdList || !cachedGrassTintsBuffer)
        return;
    GrassObjectTint tintData[64];
    for (int i = 0; i < 64; i++)
    {
        tintData[i].r = ps_r3_grass_object_tints[i].x;
        tintData[i].g = ps_r3_grass_object_tints[i].y;
        tintData[i].b = ps_r3_grass_object_tints[i].z;
        tintData[i].pad = 1.0f;
    }
    cmdList->writeBuffer(cachedGrassTintsBuffer, tintData, sizeof(tintData));
}

void FGDetailManager::ClearDrawArgs(nvrhi::ICommandList* cmdList, VisibilityFrame& frame)
{
    const u32 zero[5] = {};
    for (auto& args : frame.drawArgs)
        cmdList->writeBuffer(args, zero, sizeof(zero));
    cmdList->clearBufferUInt(frame.packets, 0u);
    cmdList->clearBufferUInt(frame.workStatus, 0u);
    cmdList->clearBufferUInt(frame.visibleSlotCount, 0u);
    cmdList->clearBufferUInt(frame.slotDispatch, 0u);
    cmdList->clearBufferUInt(frame.swDispatch, 0u);
}

} // namespace xray::render::fg
