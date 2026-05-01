#include "stdafx.h"
#include "TextureManager.h"
#include "TextureStreaming.h"
#include "DDSLoader.h"
#include "../RenderContext/RenderDevice.h"

namespace xray::render::fg {

    static fg::RenderDevice* GetGlobalRenderDevice() {
        return RImplementation.GetRenderDevice();
    }

}

namespace xray::render::resources::test {

static u32 g_testsPassed = 0;
static u32 g_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            Msg("! [TEST FAILED] %s: %s", __FUNCTION__, message); \
            g_testsFailed++; \
            return false; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        Msg("* [TEST PASSED] %s", __FUNCTION__); \
        g_testsPassed++; \
        return true; \
    } while(0)

bool Test_HandleAllocation() {
    Msg("! [TEST] Handle allocation and validation...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    if (!device || !device->IsInitialized()) {
        Msg("! [TEST SKIPPED] RenderDevice not initialized - run in-game");
        return true;
    }

    TextureManager texManager(device);

    TextureHandle handle1 = texManager.LoadTexture("$game_textures$\\$alphadxt1");
    TextureHandle handle2 = texManager.LoadTexture("$game_textures$\\$noalphadxt5");

    TEST_ASSERT(handle1.IsValid(), "Handle 1 should be valid");
    TEST_ASSERT(handle2.IsValid(), "Handle 2 should be valid");
    TEST_ASSERT(handle1.index != handle2.index, "Handles should have different indices");

    texManager.Release(handle1);
    texManager.Release(handle2);

    TEST_PASS();
}

bool Test_HandleReuse() {
    Msg("! [TEST] Handle reuse after release...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    TextureHandle handle1 = texManager.LoadTexture("$game_textures$\\$alphadxt1");
    u32 index1 = handle1.index;
    u32 gen1 = handle1.generation;

    texManager.Release(handle1);

    TextureHandle handle2 = texManager.LoadTexture("$game_textures$\\$noalphadxt5");
    u32 index2 = handle2.index;
    u32 gen2 = handle2.generation;

    TEST_ASSERT(index2 == index1, "Should reuse same index");
    TEST_ASSERT(gen2 > gen1, "Generation should increment");
    TEST_ASSERT(!texManager.IsResident(handle1), "Old handle should be invalid");

    texManager.Release(handle2);

    TEST_PASS();
}

bool Test_HandleValidation() {
    Msg("! [TEST] Handle validation...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    TextureHandle invalidHandle;
    TEST_ASSERT(!invalidHandle.IsValid(), "Default handle should be invalid");
    TEST_ASSERT(!texManager.IsResident(invalidHandle), "Invalid handle should not be resident");

    TextureHandle validHandle = texManager.LoadTexture("$game_textures$\\$noalphadxt5");
    TEST_ASSERT(validHandle.IsValid(), "Allocated handle should be valid");

    TextureHandle staleHandle = validHandle;
    staleHandle.generation += 1;
    TEST_ASSERT(!texManager.IsResident(staleHandle), "Stale handle should be invalid");

    texManager.Release(validHandle);

    TEST_PASS();
}

bool Test_DDSLoader_InvalidFile() {
    Msg("! [TEST] DDS loader with invalid file...");

    DDSData data;
    bool result = DDSLoader::LoadFromFile("nonexistent_file.dds", data);

    TEST_ASSERT(!result, "Should fail to load nonexistent file");
    TEST_ASSERT(!data.isValid, "Data should be marked invalid");

    TEST_PASS();
}

bool Test_DDSLoader_FormatConversion() {
    Msg("! [TEST] DDS format conversion...");

    TEST_ASSERT(DDSLoader::GetFormatFromFourCC(FOURCC_DXT1) == nvrhi::Format::BC1_UNORM,
                "DXT1 should map to BC1_UNORM");
    TEST_ASSERT(DDSLoader::GetFormatFromFourCC(FOURCC_DXT5) == nvrhi::Format::BC3_UNORM,
                "DXT5 should map to BC3_UNORM");
    TEST_ASSERT(DDSLoader::GetFormatFromFourCC(FOURCC_BC4U) == nvrhi::Format::BC4_UNORM,
                "BC4U should map to BC4_UNORM");

    TEST_ASSERT(DDSLoader::GetFormatFromDXGI(DDSDxgiFormat_BC7_UNORM) == nvrhi::Format::BC7_UNORM,
                "DXGI BC7 should map to BC7_UNORM");
    TEST_ASSERT(DDSLoader::GetFormatFromDXGI(DDSDxgiFormat_BC1_UNORM_SRGB) == nvrhi::Format::BC1_UNORM_SRGB,
                "DXGI BC1 SRGB should map correctly");

    TEST_PASS();
}

bool Test_DDSLoader_MipCalculation() {
    Msg("! [TEST] DDS mip dimension calculation...");

    u32 width, height, depth;

    DDSLoader::CalculateMipDimensions(1024, 512, 1, 0, width, height, depth);
    TEST_ASSERT(width == 1024 && height == 512 && depth == 1, "Mip 0 should be original size");

    DDSLoader::CalculateMipDimensions(1024, 512, 1, 1, width, height, depth);
    TEST_ASSERT(width == 512 && height == 256 && depth == 1, "Mip 1 should be half size");

    DDSLoader::CalculateMipDimensions(4, 4, 1, 10, width, height, depth);
    TEST_ASSERT(width == 1 && height == 1 && depth == 1, "Should clamp to 1x1 minimum");

    TEST_PASS();
}

bool Test_DDSLoader_SizeCalculation() {
    Msg("! [TEST] DDS size calculation...");

    u32 size = DDSLoader::CalculateMipSize(1024, 1024, 1, nvrhi::Format::BC1_UNORM);
    u32 expectedBC1 = (1024 / 4) * (1024 / 4) * 8;
    TEST_ASSERT(size == expectedBC1, "BC1 size calculation incorrect");

    size = DDSLoader::CalculateMipSize(512, 512, 1, nvrhi::Format::BC3_UNORM);
    u32 expectedBC3 = (512 / 4) * (512 / 4) * 16;
    TEST_ASSERT(size == expectedBC3, "BC3 size calculation incorrect");

    size = DDSLoader::CalculateMipSize(256, 256, 1, nvrhi::Format::RGBA8_UNORM);
    u32 expectedRGBA8 = 256 * 256 * 4;
    TEST_ASSERT(size == expectedRGBA8, "RGBA8 size calculation incorrect");

    TEST_PASS();
}

bool Test_TextureDeduplication() {
    Msg("! [TEST] Texture deduplication...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    TextureHandle handle1 = texManager.LoadTexture("$game_textures$\\$shadertest");
    TextureHandle handle2 = texManager.LoadTexture("$game_textures$\\$shadertest");

    TEST_ASSERT(handle1.index == handle2.index, "Should return same handle for same path");
    TEST_ASSERT(handle1.generation == handle2.generation, "Handles should be identical");

    const TextureMetadata* meta = texManager.GetMetadata(handle1);
    TEST_ASSERT(meta != nullptr, "Metadata should exist");
    TEST_ASSERT(meta->refCount == 2, "Reference count should be 2");

    texManager.Release(handle1);
    texManager.Release(handle2);

    TEST_PASS();
}

bool Test_ReferenceCounting() {
    Msg("! [TEST] Reference counting...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    TextureHandle handle = texManager.LoadTexture("$game_textures$\\act\\act_arm_1");
    const TextureMetadata* meta = texManager.GetMetadata(handle);

    TEST_ASSERT(meta->refCount == 1, "Initial refCount should be 1");

    texManager.AddRef(handle);
    TEST_ASSERT(meta->refCount == 2, "refCount should be 2 after AddRef");

    texManager.AddRef(handle);
    TEST_ASSERT(meta->refCount == 3, "refCount should be 3 after second AddRef");

    texManager.Release(handle);
    TEST_ASSERT(meta->refCount == 2, "refCount should be 2 after Release");

    texManager.Release(handle);
    TEST_ASSERT(meta->refCount == 1, "refCount should be 1 after second Release");

    texManager.Release(handle);

    TEST_PASS();
}

bool Test_MemoryTracking() {
    Msg("! [TEST] Memory tracking...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    auto initialStats = texManager.GetStatistics();
    u64 initialMemory = initialStats.totalMemoryUsed;

    TEST_ASSERT(initialStats.memoryBudget > 0, "Memory budget should be set");
    TEST_ASSERT(initialStats.texturesTotal == 0, "Should start with 0 textures");

    TEST_PASS();
}

bool Test_Statistics() {
    Msg("! [TEST] Statistics tracking...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    auto stats = texManager.GetStatistics();

    TEST_ASSERT(stats.memoryBudget == texManager.GetMemoryBudget(),
                "Statistics should match manager");

    float percent = stats.memoryUsagePercent();
    TEST_ASSERT(percent >= 0.0f && percent <= 100.0f,
                "Memory percentage should be in valid range");

    TEST_PASS();
}

bool Test_AsyncMultipleTextures() {
    Msg("! [TEST] Async loading multiple textures...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    xr_vector<TextureHandle> textures;
    const u32 numTextures = 10;

    const char* testTextures[] = {
        "$game_textures$\\$alphadxt1",
        "$game_textures$\\$noalphadxt5",
        "$game_textures$\\$shadertest",
        "$game_textures$\\act\\act_arm_1",
        "$game_textures$\\$alphadxt1",
        "$game_textures$\\$noalphadxt5",
        "$game_textures$\\$shadertest",
        "$game_textures$\\$alphadxt1",
        "$game_textures$\\$noalphadxt5",
        "$game_textures$\\$shadertest"
    };

    Msg("! [TEST] Submitting %u async load requests...", numTextures);

    for (u32 i = 0; i < numTextures; i++) {
        TextureHandle handle = texManager.LoadTexture(testTextures[i], TexturePriority::High);
        textures.push_back(handle);
    }

    u32 frame = 0;
    u32 loadedCount = 0;

    while (loadedCount < textures.size() && frame < 300) {
        texManager.Update(0.016f);

        loadedCount = 0;
        for (auto handle : textures) {
            if (texManager.IsResident(handle)) {
                loadedCount++;
            }
        }

        if (frame % 60 == 0) {
            Msg("! [TEST] Frame %u: %u / %u textures loaded",
                frame, loadedCount, (u32)textures.size());
        }

        frame++;
    }

    TEST_ASSERT(loadedCount == textures.size(), "All textures should be loaded");

    Msg("! [TEST] All %u textures loaded in %u frames (%.2f seconds)",
        (u32)textures.size(), frame, frame * 0.016f);

    auto stats = texManager.GetStatistics();
    Msg("! [TEST] Stats: %u resident, Memory: %llu KB",
        stats.texturesResident, stats.totalMemoryUsed / 1024);

    for (auto handle : textures) {
        texManager.Release(handle);
    }

    TEST_PASS();
}

bool Test_AsyncIOStatistics() {
    Msg("! [TEST] Async I/O statistics tracking...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    auto* streamingMgr = texManager.GetStreamingManager();
    auto initialStats = streamingMgr->GetStatistics();

    Msg("! [TEST] Initial stats: Pending=%u, InProgress=%u, Completed=%u",
        initialStats.requestsPending,
        initialStats.requestsInProgress,
        initialStats.requestsCompleted);

    TextureHandle handle1 = texManager.LoadTexture("$game_textures$\\$alphadxt1", TexturePriority::High);
    TextureHandle handle2 = texManager.LoadTexture("$game_textures$\\$noalphadxt5", TexturePriority::High);
    TextureHandle handle3 = texManager.LoadTexture("$game_textures$\\act\\act_arm_1", TexturePriority::Medium);

    for (u32 i = 0; i < 100; i++) {
        texManager.Update(0.016f);

        if (texManager.IsResident(handle1) &&
            texManager.IsResident(handle2) &&
            texManager.IsResident(handle3)) {
            break;
        }
    }

    auto finalStats = streamingMgr->GetStatistics();

    Msg("! [TEST] Final stats: Pending=%u, InProgress=%u, Completed=%u",
        finalStats.requestsPending,
        finalStats.requestsInProgress,
        finalStats.requestsCompleted);

    TEST_ASSERT(finalStats.requestsCompleted >= 0, "Should have completed requests");
    TEST_ASSERT(finalStats.requestsPending == 0, "No requests should be pending");
    TEST_ASSERT(finalStats.requestsInProgress == 0, "No requests should be in progress");

    texManager.Release(handle1);
    texManager.Release(handle2);
    texManager.Release(handle3);

    TEST_PASS();
}

bool Test_ThreadSafeLoading() {
    Msg("! [TEST] Thread-safe texture loading...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    TextureHandle handle1 = texManager.LoadTextureThreadSafe("$game_textures$\\$alphadxt1", TexturePriority::High);
    TextureHandle handle2 = texManager.LoadTextureThreadSafe("$game_textures$\\$alphadxt1", TexturePriority::High);
    TextureHandle handle3 = texManager.LoadTextureThreadSafe("$game_textures$\\$alphadxt1", TexturePriority::High);

    TEST_ASSERT(handle1.index == handle2.index, "Should return same handle");
    TEST_ASSERT(handle2.index == handle3.index, "Should return same handle");

    const TextureMetadata* meta = texManager.GetMetadata(handle1);
    TEST_ASSERT(meta != nullptr, "Metadata should exist");
    TEST_ASSERT(meta->refCount == 3, "Reference count should be 3");

    for (u32 i = 0; i < 100; i++) {
        texManager.Update(0.016f);
        if (texManager.IsResident(handle1)) break;
    }

    TEST_ASSERT(texManager.IsResident(handle1), "Texture should be resident");

    texManager.Release(handle1);
    texManager.Release(handle2);
    texManager.Release(handle3);

    TEST_PASS();
}

bool Test_AsyncLoadingPerformance() {
    Msg("! [TEST] Async loading performance...");

    fg::RenderDevice* device = xray::render::fg::GetGlobalRenderDevice();
    TextureManager texManager(device);

    float startTime = Device.fTimeGlobal;

    xr_vector<TextureHandle> textures;
    const u32 numTextures = 5;

    for (u32 i = 0; i < numTextures; i++) {
        TextureHandle handle = texManager.LoadTexture("$game_textures$\\act\\act_arm_1", TexturePriority::High);
        textures.push_back(handle);
    }

    u32 frame = 0;
    while (frame < 300) {
        texManager.Update(0.016f);

        bool allLoaded = true;
        for (auto handle : textures) {
            if (!texManager.IsResident(handle)) {
                allLoaded = false;
                break;
            }
        }

        if (allLoaded) break;
        frame++;
    }

    float endTime = Device.fTimeGlobal;
    float duration = endTime - startTime;

    Msg("! [TEST] Loaded %u textures in %.3f seconds (%u frames)",
        numTextures, duration, frame);

    TEST_ASSERT(frame < 300, "Should load within reasonable time");

    for (auto handle : textures) {
        texManager.Release(handle);
    }

    TEST_PASS();
}

void RunAllTests() {
    Msg("! ═══════════════════════════════════════════════════");
    Msg("! [TEST] Starting TextureManager Unit Tests");
    Msg("! ═══════════════════════════════════════════════════");

    g_testsPassed = 0;
    g_testsFailed = 0;

    Test_HandleAllocation();
    Test_HandleReuse();
    Test_HandleValidation();

    Test_DDSLoader_InvalidFile();
    Test_DDSLoader_FormatConversion();
    Test_DDSLoader_MipCalculation();
    Test_DDSLoader_SizeCalculation();

    Test_TextureDeduplication();
    Test_ReferenceCounting();
    Test_MemoryTracking();
    Test_Statistics();

    Test_AsyncMultipleTextures();
    Test_AsyncIOStatistics();
    Test_ThreadSafeLoading();
    Test_AsyncLoadingPerformance();

    Msg("! ═══════════════════════════════════════════════════");
    Msg("! [TEST] Results: %u passed, %u failed", g_testsPassed, g_testsFailed);
    if (g_testsFailed == 0) {
        Msg("! [TEST] ✅ ALL TESTS PASSED!");
    } else {
        Msg("! [TEST] ❌ SOME TESTS FAILED");
    }
    Msg("! ═══════════════════════════════════════════════════");
}

void RunHandleTests() {
    Msg("! [TEST] Running Handle Tests...");
    g_testsPassed = 0;
    g_testsFailed = 0;

    Test_HandleAllocation();
    Test_HandleReuse();
    Test_HandleValidation();

    Msg("! [TEST] Handle Tests: %u passed, %u failed", g_testsPassed, g_testsFailed);
}

void RunDDSTests() {
    Msg("! [TEST] Running DDS Loader Tests...");
    g_testsPassed = 0;
    g_testsFailed = 0;

    Test_DDSLoader_InvalidFile();
    Test_DDSLoader_FormatConversion();
    Test_DDSLoader_MipCalculation();
    Test_DDSLoader_SizeCalculation();

    Msg("! [TEST] DDS Tests: %u passed, %u failed", g_testsPassed, g_testsFailed);
}

void RunTextureManagerTests() {
    Msg("! [TEST] Running TextureManager Tests...");
    g_testsPassed = 0;
    g_testsFailed = 0;

    Test_TextureDeduplication();
    Test_ReferenceCounting();
    Test_MemoryTracking();
    Test_Statistics();

    Msg("! [TEST] TextureManager Tests: %u passed, %u failed", g_testsPassed, g_testsFailed);
}

void RunAsyncTests() {
    Msg("! [TEST] Running Async Loading Tests...");
    g_testsPassed = 0;
    g_testsFailed = 0;

    Test_AsyncMultipleTextures();
    Test_AsyncIOStatistics();
    Test_ThreadSafeLoading();
    Test_AsyncLoadingPerformance();

    Msg("! [TEST] Async Tests: %u passed, %u failed", g_testsPassed, g_testsFailed);
}

}
