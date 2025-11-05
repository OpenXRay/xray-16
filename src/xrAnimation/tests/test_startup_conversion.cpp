#include "Common/Platform.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "xrCore/xrCore.h"
#include "xrCore/FS.h"
#include "xrCore/FMesh.hpp"

#include "StartupConversionInventory.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace
{
std::string MakeUniqueAlias(const char* prefix)
{
    static std::atomic<int> counter{ 0 };
    return std::string("$") + prefix + "_" + std::to_string(counter.fetch_add(1)) + "$";
}

void WriteBinaryFile(const fs::path& path, const std::vector<uint8_t>& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("failed to open file for writing: " + path.string());

    if (!data.empty())
        stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

void WriteOgfWithMotionRefs(const fs::path& path, const std::vector<std::string>& references)
{
    std::vector<uint8_t> chunk;
    auto append_u32 = [&chunk](uint32_t value)
    {
        for (int shift = 0; shift < 32; shift += 8)
            chunk.push_back(static_cast<uint8_t>((value >> shift) & 0xFF));
    };
    auto append_stringz = [&chunk](const std::string& value)
    {
        chunk.insert(chunk.end(), value.begin(), value.end());
        chunk.push_back(0);
    };

    append_u32(static_cast<uint32_t>(references.size()));
    for (const auto& ref : references)
        append_stringz(ref);

    std::vector<uint8_t> file_data;
    auto append_chunk_header = [&file_data](uint32_t id, uint32_t size)
    {
        for (int shift = 0; shift < 32; shift += 8)
            file_data.push_back(static_cast<uint8_t>((id >> shift) & 0xFF));
        for (int shift = 0; shift < 32; shift += 8)
            file_data.push_back(static_cast<uint8_t>((size >> shift) & 0xFF));
    };

    append_chunk_header(OGF_S_MOTION_REFS2, static_cast<uint32_t>(chunk.size()));
    file_data.insert(file_data.end(), chunk.begin(), chunk.end());

    WriteBinaryFile(path, file_data);
}

void WriteTextFile(const fs::path& path, const std::string& contents)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("failed to open file for writing: " + path.string());
    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

fs::file_time_type ToFileTime(const std::chrono::system_clock::time_point& tp)
{
    return fs::file_time_type::clock::now() + (tp - std::chrono::system_clock::now());
}

void SetFileTimestamp(const fs::path& path, const std::chrono::system_clock::time_point& tp)
{
    std::error_code ec;
    fs::last_write_time(path, ToFileTime(tp), ec);
    if (ec)
        throw std::runtime_error("failed to set file time: " + path.string());
}

} // namespace

class StartupConversionStageTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        using namespace std::chrono;
        temp_root_ = fs::temp_directory_path() / fs::path("ozz_inventory_" + MakeUniqueAlias("session"));
        meshes_root_ = temp_root_ / "meshes";
        converted_root_ = temp_root_ / "converted";
        actors_root_ = meshes_root_ / "actors";
        skeleton_root_ = converted_root_ / "skeletons" / "actors";
        bundle_root_ = converted_root_ / "bundles" / "actors";
        anim_root_ = converted_root_ / "anims" / "actors";

        fs::create_directories(actors_root_);
        fs::create_directories(skeleton_root_);
        fs::create_directories(bundle_root_);
        fs::create_directories(anim_root_);

        const auto source_time = std::chrono::system_clock::now() - 2min;
        const auto output_time = std::chrono::system_clock::now() - 1min;

        WriteOgfWithMotionRefs(actors_root_ / "test_visual.ogf", { "actors\\test_motion", "actors\\idle_motion" });
        SetFileTimestamp(actors_root_ / "test_visual.ogf", source_time);

        WriteTextFile(actors_root_ / "test_motion.omf", "motion");
        SetFileTimestamp(actors_root_ / "test_motion.omf", source_time);
        WriteTextFile(actors_root_ / "idle_motion.omf", "idle");
        SetFileTimestamp(actors_root_ / "idle_motion.omf", source_time);

        WriteTextFile(skeleton_root_ / "test_visual.ozz", "skeleton");
        SetFileTimestamp(skeleton_root_ / "test_visual.ozz", output_time);

        WriteTextFile(bundle_root_ / "test_visual.ozzx", "bundle");
        SetFileTimestamp(bundle_root_ / "test_visual.ozzx", output_time);

        WriteTextFile(anim_root_ / "test_motion.ozz", "anim");
        SetFileTimestamp(anim_root_ / "test_motion.ozz", output_time);
        WriteTextFile(anim_root_ / "idle_motion.ozz", "anim");
        SetFileTimestamp(anim_root_ / "idle_motion.ozz", output_time);

        mesh_alias_ = MakeUniqueAlias("mesh_stage");
        convert_alias_ = MakeUniqueAlias("converted_stage");

        FS.append_path(mesh_alias_.c_str(), meshes_root_.string().c_str(), nullptr, true);
        FS.append_path(convert_alias_.c_str(), converted_root_.string().c_str(), nullptr, true);
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(temp_root_, ec);
    }

    XRay::Animation::InventoryScanConfig BuildConfig() const
    {
        XRay::Animation::InventoryScanConfig config;
        config.visual_roots = { xr_string(mesh_alias_.c_str()) };
        return config;
    }

    XRay::Animation::LegacyAssetInventory BuildInventory() const
    {
        return XRay::Animation::BuildLegacyAssetInventory(BuildConfig());
    }

    fs::path temp_root_;
    fs::path meshes_root_;
    fs::path converted_root_;
    fs::path actors_root_;
    fs::path skeleton_root_;
    fs::path bundle_root_;
    fs::path anim_root_;
    std::string mesh_alias_;
    std::string convert_alias_;
};

TEST_F(StartupConversionStageTest, BuildsInventoryFromCustomAlias)
{
    const auto inventory = BuildInventory();
    ASSERT_EQ(inventory.visuals.size(), 1u);
    ASSERT_EQ(inventory.motions.size(), 2u);

    const auto& asset = inventory.visuals.front();
    EXPECT_EQ(asset.normalized_identifier, xr_string("actors\\test_visual"));
    ASSERT_EQ(asset.sources.size(), 1u);
    const auto& source = asset.sources.front();
    EXPECT_EQ(source.motion_references.size(), 2u);
    EXPECT_NE(std::find(source.motion_references.begin(), source.motion_references.end(), xr_string("actors\\test_motion.omf")), source.motion_references.end());
    EXPECT_NE(std::find(source.motion_references.begin(), source.motion_references.end(), xr_string("actors\\idle_motion.omf")), source.motion_references.end());
}

TEST_F(StartupConversionStageTest, DigestRemainsStableForIdenticalInventories)
{
    const auto first_inventory = BuildInventory();
    const auto second_inventory = BuildInventory();

    const auto digest_a = XRay::Animation::ComputeLegacyAssetInventoryDigest(first_inventory);
    const auto digest_b = XRay::Animation::ComputeLegacyAssetInventoryDigest(second_inventory);

    EXPECT_EQ(digest_a, digest_b);
}

TEST_F(StartupConversionStageTest, DigestChangesWhenMotionContentChanges)
{
    auto inventory = BuildInventory();
    const auto* motion_before = inventory.FindMotion(xr_string("actors\\test_motion"));
    ASSERT_NE(motion_before, nullptr);
    const auto size_before = motion_before->file_size;
    const auto timestamp_before = motion_before->modified_time_seconds;

    const auto digest_original = XRay::Animation::ComputeLegacyAssetInventoryDigest(inventory);

    WriteTextFile(actors_root_ / "test_motion.omf", "updated motion payload");

    inventory = BuildInventory();
    const auto* motion_after = inventory.FindMotion(xr_string("actors\\test_motion"));
    ASSERT_NE(motion_after, nullptr);
    EXPECT_NE(motion_after->file_size, size_before);
    EXPECT_NE(motion_after->modified_time_seconds, timestamp_before);

    const auto digest_updated = XRay::Animation::ComputeLegacyAssetInventoryDigest(inventory);
    EXPECT_NE(digest_original, digest_updated);
}

TEST_F(StartupConversionStageTest, DigestRoundTripsThroughConfigFile)
{
    const auto inventory = BuildInventory();
    const auto digest = XRay::Animation::ComputeLegacyAssetInventoryDigest(inventory);

    const fs::path config_path = temp_root_ / "user.ltx";
    ASSERT_TRUE(XRay::Animation::StoreInventoryDigestInConfig(config_path, digest));

    const auto loaded = XRay::Animation::LoadInventoryDigestFromConfig(config_path);
    EXPECT_EQ(loaded, digest);

    const xr_string updated_digest = xr_string((digest + "ff").c_str());
    ASSERT_TRUE(XRay::Animation::StoreInventoryDigestInConfig(config_path, updated_digest));
    const auto reloaded = XRay::Animation::LoadInventoryDigestFromConfig(config_path);
    EXPECT_EQ(reloaded, updated_digest);
}
