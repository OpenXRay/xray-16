#include "stdafx.h"

#include "StartupConversionInventory.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/memory/unique_ptr.h>

#include "xrCore/FMesh.hpp"
#include "xrCore/FS.h"
#include "xrCore/log.h"
#include "xrCore/_std_extensions.h"
#include "xrCore/xr_ini.h"

#include "LegacyOgfConverter.h"
#include "LegacyOmfConverter.h"
#include "OzzBundle.h"
#include "Layers/xrRender/ModelNaming.h"
#include "xrCore/Threading/ParallelFor.hpp"

namespace fs = std::filesystem;

namespace XRay
{
namespace Animation
{
namespace
{
struct ChunkView
{
    const std::byte* data = nullptr;
    size_t size = 0;
};

struct BinaryReader
{
    const std::byte* data = nullptr;
    size_t size = 0;
    size_t offset = 0;

    template <class T>
    T Read()
    {
        if (offset + sizeof(T) > size)
            throw std::runtime_error("unexpected end of chunk while reading data");
        T value;
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    xr_string ReadStringZ()
    {
        const auto* begin = data + offset;
        const auto* end = data + size;
        const auto* cursor = begin;
        while (cursor < end && *reinterpret_cast<const char*>(cursor) != '\0')
            ++cursor;

        if (cursor == end)
            throw std::runtime_error("unterminated string in chunk");

        xr_string value(reinterpret_cast<const char*>(begin), static_cast<size_t>(cursor - begin));
        offset += static_cast<size_t>(cursor - begin) + 1;
        return value;
    }
};

xr_string TrimCopy(const xr_string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == xr_string::npos)
        return {};
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

xr_string ToLowerCopy(xr_string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

xr_string CanonicalizeRelativePath(xr_string value)
{
    value = ToLowerCopy(value);
    std::replace(value.begin(), value.end(), '/', '\\');
    while (!value.empty() && (value.front() == '\\' || value.front() == '/'))
        value.erase(value.begin());
    return value;
}

xr_string CanonicalizeMotionReference(xr_string value)
{
    value = CanonicalizeRelativePath(std::move(value));
    if (value.empty())
        return value;
    if (value.size() < 4 || value.substr(value.size() - 4) != ".omf")
        value.append(".omf");
    return value;
}

xr_string CanonicalizeMotionName(const xr_string& value)
{
    xr_string canonical = CanonicalizeMotionReference(value);
    const auto dot = canonical.find_last_of('.');
    if (dot != xr_string::npos)
        canonical.erase(dot);
    return canonical;
}

std::optional<std::int64_t> QueryFileTimestampSeconds(const xr_string& root_alias, const xr_string& relative_path)
{
    if (relative_path.empty())
        return std::nullopt;

    string_path resolved;
    if (!FS.update_path(resolved, root_alias.c_str(), relative_path.c_str(), false))
        return std::nullopt;

    std::string resolved_str(resolved);
    const char preferred_separator = static_cast<char>(fs::path::preferred_separator);
    std::replace(resolved_str.begin(), resolved_str.end(), '\\', preferred_separator);

    std::error_code ec;
    const fs::path path(resolved_str);
    if (!fs::exists(path, ec) || ec)
        return std::nullopt;

    using namespace std::chrono;
    const auto file_time = fs::last_write_time(path, ec);
    if (ec)
        return std::nullopt;

    const auto system_time = time_point_cast<system_clock::duration>(file_time - fs::file_time_type::clock::now() + system_clock::now());
    return duration_cast<seconds>(system_time.time_since_epoch()).count();
}

std::optional<std::int64_t> QueryFileSizeBytes(const xr_string& root_alias, const xr_string& relative_path)
{
    if (relative_path.empty())
        return std::nullopt;

    string_path resolved;
    if (!FS.update_path(resolved, root_alias.c_str(), relative_path.c_str(), false))
        return std::nullopt;

    std::string resolved_str(resolved);
    const char preferred_separator = static_cast<char>(fs::path::preferred_separator);
    std::replace(resolved_str.begin(), resolved_str.end(), '\\', preferred_separator);

    std::error_code ec;
    const fs::path path(resolved_str);
    if (!fs::exists(path, ec) || ec)
        return std::nullopt;

    const auto size = fs::file_size(path, ec);
    if (ec)
        return std::nullopt;

    return static_cast<std::int64_t>(size);
}

std::string TrimCopyStd(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return {};
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool EqualsIgnoreCase(const std::string& lhs, pcstr rhs)
{
    return xr_stricmp(lhs.c_str(), rhs) == 0;
}

std::unordered_map<u32, ChunkView> ParseChunks(const std::byte* data, size_t size)
{
    std::unordered_map<u32, ChunkView> chunks;
    size_t offset = 0;

    while (offset + sizeof(u32) * 2 <= size)
    {
        u32 id = 0;
        u32 chunk_size = 0;
        std::memcpy(&id, data + offset, sizeof(u32));
        offset += sizeof(u32);
        std::memcpy(&chunk_size, data + offset, sizeof(u32));
        offset += sizeof(u32);

        if (offset > size || chunk_size > size - offset)
            throw std::runtime_error("chunk extends past buffer bounds");

        chunks.emplace(id, ChunkView{ data + offset, chunk_size });
        offset += chunk_size;
    }

    return chunks;
}

std::vector<std::byte> ReadFileBytes(const xr_string& root_alias, const xr_string& relative_path)
{
    IReader* reader = FS.r_open(root_alias.c_str(), relative_path.c_str());
    if (!reader)
        return {};

    std::vector<std::byte> buffer(static_cast<size_t>(reader->length()));
    if (!buffer.empty())
        std::memcpy(buffer.data(), reader->pointer(), buffer.size());
    FS.r_close(reader);
    return buffer;
}

bool ResolveAbsolutePath(const xr_string& root_alias, const xr_string& relative_path, fs::path& out_path)
{
    string_path resolved;
    if (!FS.update_path(resolved, root_alias.c_str(), relative_path.c_str(), false))
        return false;
    out_path = fs::path(resolved);
    return true;
}

bool EnsureParentDirectoryExists(const fs::path& file_path)
{
    const fs::path parent = file_path.parent_path();
    if (parent.empty())
        return true;

    std::error_code ec;
    fs::create_directories(parent, ec);
    if (ec)
    {
        Msg("! [ozz] Failed to create directory '%s' (%s)", parent.string().c_str(), ec.message().c_str());
        return false;
    }
    return true;
}

xr_string BuildBundleRelativePath(const LegacyVisualAsset& visual)
{
    xr_string relative = visual.normalized_identifier;
    relative.append(".ozzx");
    return relative;
}

xr_string BuildMotionOutputRelativePath(const LegacyMotionAsset& motion)
{
    xr_string relative = motion.relative_path;
    const auto dot = relative.find_last_of('.');
    if (dot != xr_string::npos)
        relative.replace(dot, relative.size() - dot, ".ozz");
    else
        relative.append(".ozz");
    return relative;
}

xr_string DetermineBundleOutputAlias(const LegacyVisualAsset& visual, const StartupConversionParams& params)
{
    const LegacyVisualSource* primary = visual.PrimarySource();
    if (primary && !primary->location.root_alias.empty())
        return primary->location.root_alias;
    return params.ozz_output_alias;
}

xr_string DetermineMotionOutputAlias(const LegacyMotionAsset& motion, const StartupConversionParams& params)
{
    if (!motion.root_alias.empty())
        return motion.root_alias;
    return params.ozz_output_alias;
}

bool FileExists(const xr_string& alias, const xr_string& relative_path)
{
    fs::path absolute;
    if (!ResolveAbsolutePath(alias, relative_path, absolute))
    {
        Msg("! [ozz][debug] FileExists: Failed to resolve path alias='%s' relative='%s'", alias.c_str(), relative_path.c_str());
        return false;
    }

    std::error_code ec;
    const bool exists = fs::exists(absolute, ec);

    Msg("  [ozz][debug] FileExists: alias='%s' relative='%s' -> absolute='%s' exists=%s",
        alias.c_str(), relative_path.c_str(), absolute.generic_string().c_str(), exists ? "YES" : "NO");
    return exists;
}

bool BundleExists(const LegacyVisualAsset& visual, const StartupConversionParams& params)
{
    const xr_string relative = BuildBundleRelativePath(visual);
    const xr_string alias = DetermineBundleOutputAlias(visual, params);
    return FileExists(alias, relative);
}

bool MotionExists(const LegacyMotionAsset& motion, const StartupConversionParams& params)
{
    const xr_string relative = BuildMotionOutputRelativePath(motion);
    const xr_string alias = DetermineMotionOutputAlias(motion, params);
    return FileExists(alias, relative);
}

void SerializeString(ozz::io::OArchive& archive, const std::string& value)
{
    const uint32_t length = static_cast<uint32_t>(value.size());
    archive << length;
    if (length > 0)
        archive << ozz::io::MakeArray(value.c_str(), length);
}

void SerializeMotionMarks(ozz::io::OArchive& archive, const LegacyMotionMetadata& metadata)
{
    const uint32_t mark_count = static_cast<uint32_t>(metadata.marks.size());
    archive << mark_count;
    for (const auto& mark : metadata.marks)
    {
        SerializeString(archive, std::string(mark.name.c_str()));
        const uint32_t interval_count = static_cast<uint32_t>(mark.intervals.size());
        archive << interval_count;
        for (const auto& interval : mark.intervals)
        {
            archive << interval.first;
            archive << interval.second;
        }
    }
}

void SerializeMotionMetadata(ozz::io::OArchive& archive, const LegacyMotionMetadata& metadata)
{
    SerializeString(archive, std::string(metadata.name.c_str()));
    archive << metadata.flags;
    archive << metadata.bone_or_part;
    archive << metadata.motion_id;
    archive << metadata.speed;
    archive << metadata.power;
    archive << metadata.accrue;
    archive << metadata.falloff;
    SerializeMotionMarks(archive, metadata);
}

bool WriteBundleFile(const xr_string& output_alias,
                     const LegacyVisualAsset& visual,
                     const LegacyVisualConversionResult& conversion)
{
    const xr_string relative = BuildBundleRelativePath(visual);
    fs::path output_path;
    if (!ResolveAbsolutePath(output_alias, relative, output_path))
    {
        Msg("! [ozz] Failed to resolve bundle output path for '%s'", visual.normalized_identifier.c_str());
        return false;
    }

    if (!EnsureParentDirectoryExists(output_path))
        return false;

    OzzxBundle bundle;
    bundle.version = 3u;
    bundle.model_type = conversion.model_type;
    bundle.skeleton = conversion.skeleton_binary;
    bundle.mesh = conversion.mesh_binary;
    bundle.motion_refs = conversion.motion_refs;
    bundle.bone_metadata = conversion.bone_metadata;
    bundle.user_data = conversion.user_data;
    bundle.embedded_animation_data = conversion.embedded_animation_binary;

    const std::string path_string = output_path.string();

    if (!WriteOzzxBundle(output_path, bundle))
    {
        Msg("! [ozz] Failed to write bundle '%s'", path_string.c_str());
        return false;
    }

    Msg("[ozz] wrote bundle %s", path_string.c_str());

    return true;
}

bool WriteOzzAnimations(const xr_string& output_alias,
                        const xr_string& output_relative,
                        const xr_vector<ConvertedOmfAnimation>& animations)
{
    fs::path output_path;
    if (!ResolveAbsolutePath(output_alias, output_relative, output_path))
    {
        Msg("! [ozz] Failed to resolve animation output path '%s'", output_relative.c_str());
        return false;
    }

    if (!EnsureParentDirectoryExists(output_path))
        return false;

    const std::string output_string = output_path.string();
    ozz::io::File output(output_string.c_str(), "wb");
    if (!output.opened())
    {
        Msg("! [ozz] Failed to open animation output '%s'", output_string.c_str());
        return false;
    }

    ozz::io::OArchive archive(&output);
    const uint32_t animation_count = static_cast<uint32_t>(animations.size());
    archive << animation_count;

    for (const auto& entry : animations)
    {
        if (!entry.animation)
            continue;
        archive << *entry.animation;
        SerializeMotionMetadata(archive, entry.metadata);
        SerializeBoneMotions(archive, entry);
    }

    Msg("[ozz] wrote animation %s", output_string.c_str());

    return true;
}

xr_vector<xr_string> SplitMotionReferenceList(const xr_string& combined)
{
    xr_vector<xr_string> result;
    size_t cursor = 0;
    while (cursor < combined.size())
    {
        const size_t separator = combined.find(',', cursor);
        const size_t length = separator == xr_string::npos ? combined.size() - cursor : separator - cursor;
        xr_string token = combined.substr(cursor, length);
        token = TrimCopy(token);
        if (!token.empty())
            result.emplace_back(std::move(token));
        if (separator == xr_string::npos)
            break;
        cursor = separator + 1;
    }
    return result;
}

xr_vector<xr_string> ExtractMotionReferences(const xr_string& root_alias, const xr_string& relative_path)
{
    xr_vector<xr_string> references;
    try
    {
        auto bytes = ReadFileBytes(root_alias, relative_path);
        if (bytes.empty())
            return references;

        const auto chunks = ParseChunks(bytes.data(), bytes.size());

        if (const auto refs2_it = chunks.find(OGF_S_MOTION_REFS2); refs2_it != chunks.end())
        {
            BinaryReader reader{ refs2_it->second.data, refs2_it->second.size };
            const u32 count = reader.Read<u32>();
            references.reserve(count);
            for (u32 index = 0; index < count; ++index)
            {
                xr_string entry = reader.ReadStringZ();
                entry = TrimCopy(entry);
                if (!entry.empty())
                    references.emplace_back(std::move(entry));
            }
        }
        else if (const auto refs_it = chunks.find(OGF_S_MOTION_REFS); refs_it != chunks.end())
        {
            BinaryReader reader{ refs_it->second.data, refs_it->second.size };
            if (reader.size > 0)
            {
                xr_string combined = reader.ReadStringZ();
                auto tokens = SplitMotionReferenceList(combined);
                references.insert(references.end(), tokens.begin(), tokens.end());
            }
        }
    }
    catch (const std::exception& ex)
    {
        Msg("! [ozz] Failed to read motion refs from %s:%s (%s)", root_alias.c_str(), relative_path.c_str(), ex.what());
        references.clear();
    }

    for (auto& ref : references)
        ref = CanonicalizeMotionReference(ref);

    return references;
}

LegacyAssetLocation MakeLocation(const xr_string& root_alias, const FS_File& file)
{
    LegacyAssetLocation location;
    location.root_alias = root_alias;
    location.relative_path = CanonicalizeRelativePath(file.name);
    location.file_size = file.size >= 0 ? static_cast<std::int64_t>(file.size) : 0;
    location.modified_time_seconds = file.time_write >= 0 ? static_cast<std::int64_t>(file.time_write) : 0;
    location.stored_in_vfs = (file.attrib & FS_File::flVFS) != 0;

    if (!location.stored_in_vfs)
    {
        if (const auto direct_size = QueryFileSizeBytes(location.root_alias, location.relative_path))
            location.file_size = *direct_size;
        if (const auto direct_time = QueryFileTimestampSeconds(location.root_alias, location.relative_path))
            location.modified_time_seconds = *direct_time;
    }

    return location;
}

std::optional<fs::path> ResolveExistingUserConfigPath()
{
    string_path buffer;
    std::error_code ec;

    if (FS.update_path(buffer, "$app_data_root$", "user.ltx", false))
    {
        fs::path candidate(buffer);
        if (fs::exists(candidate, ec) && !ec)
            return candidate;
    }

    if (FS.update_path(buffer, "$fs_root$", "user.ltx", false))
    {
        fs::path candidate(buffer);
        if (fs::exists(candidate, ec) && !ec)
            return candidate;
    }

    return std::nullopt;
}

fs::path ResolveUserConfigPathForWrite()
{
    string_path buffer;

    if (FS.update_path(buffer, "$app_data_root$", "user.ltx", false))
        return fs::path(buffer);

    if (FS.update_path(buffer, "$fs_root$", "user.ltx", false))
        return fs::path(buffer);

    return fs::path("user.ltx");
}

} // namespace

const LegacyVisualSource* LegacyVisualAsset::PrimarySource() const
{
    if (sources.empty())
        return nullptr;
    return &sources.front();
}

LegacyVisualSource* LegacyVisualAsset::PrimarySource()
{
    if (sources.empty())
        return nullptr;
    return &sources.front();
}

const LegacyVisualSource* LegacyVisualAsset::FindSource(const xr_string& root_alias, const xr_string& relative_path) const
{
    const xr_string canonical_root = ToLowerCopy(root_alias);
    const xr_string canonical_path = CanonicalizeRelativePath(relative_path);
    for (const auto& source : sources)
    {
        if (ToLowerCopy(source.location.root_alias) == canonical_root && source.location.relative_path == canonical_path)
            return &source;
    }
    return nullptr;
}

const LegacyVisualAsset* LegacyAssetInventory::FindVisual(const xr_string& normalized_identifier) const
{
    const auto it = visual_lookup.find(normalized_identifier);
    if (it == visual_lookup.end())
        return nullptr;
    return &visuals[it->second];
}

LegacyVisualAsset* LegacyAssetInventory::FindVisual(const xr_string& normalized_identifier)
{
    const auto it = visual_lookup.find(normalized_identifier);
    if (it == visual_lookup.end())
        return nullptr;
    return &visuals[it->second];
}

const LegacyMotionAsset* LegacyAssetInventory::FindMotion(const xr_string& canonical_name) const
{
    const auto it = motion_lookup.find(canonical_name);
    if (it == motion_lookup.end())
        return nullptr;
    return &motions[it->second];
}

LegacyAssetInventory BuildLegacyAssetInventory(const InventoryScanConfig& config)
{
    LegacyAssetInventory inventory;

    auto ensure_visual_entry = [&](const xr_string& identifier) -> LegacyVisualAsset& {
        const auto it = inventory.visual_lookup.find(identifier);
        if (it != inventory.visual_lookup.end())
            return inventory.visuals[it->second];

        inventory.visuals.emplace_back();
        auto& asset = inventory.visuals.back();
        asset.normalized_identifier = identifier;
        inventory.visual_lookup.emplace(identifier, inventory.visuals.size() - 1);
        return asset;
    };

    std::unordered_set<xr_string> unique_motion_sources;

    for (const auto& root : config.visual_roots)
    {
        FS_FileSet ogf_files;
        FS.file_list(ogf_files, root.c_str(), FS_ListFiles, "*.ogf");

        for (const auto& file : ogf_files)
        {
            const auto location = MakeLocation(root, file);
            const xr_string normalized = xray::render::detail::NormalizeModelIdentifier(location.relative_path.c_str());
            if (normalized.empty())
                continue;

            auto& asset = ensure_visual_entry(normalized);
            LegacyVisualSource source;
            source.location = location;
            source.motion_references = ExtractMotionReferences(root, source.location.relative_path);
            asset.sources.emplace_back(std::move(source));
        }

        FS_FileSet omf_files;
        FS.file_list(omf_files, root.c_str(), FS_ListFiles, "*.omf");

        for (const auto& file : omf_files)
        {
            LegacyMotionAsset motion;
            motion.root_alias = ToLowerCopy(root);
            motion.relative_path = CanonicalizeRelativePath(file.name);
            motion.canonical_name = CanonicalizeMotionName(motion.relative_path);
            motion.file_size = file.size >= 0 ? static_cast<std::int64_t>(file.size) : 0;
            motion.modified_time_seconds = file.time_write >= 0 ? static_cast<std::int64_t>(file.time_write) : 0;
            motion.stored_in_vfs = (file.attrib & FS_File::flVFS) != 0;

            if (!motion.stored_in_vfs)
            {
                if (const auto direct_size = QueryFileSizeBytes(motion.root_alias, motion.relative_path))
                    motion.file_size = *direct_size;
                if (const auto direct_time = QueryFileTimestampSeconds(motion.root_alias, motion.relative_path))
                    motion.modified_time_seconds = *direct_time;
            }

            xr_string unique_key = motion.root_alias;
            unique_key.append("|");
            unique_key.append(motion.relative_path);
            if (!unique_motion_sources.insert(unique_key).second)
                continue;

            if (inventory.motion_lookup.find(motion.canonical_name) != inventory.motion_lookup.end())
                continue;

            inventory.motion_lookup.emplace(motion.canonical_name, inventory.motions.size());
            inventory.motions.emplace_back(std::move(motion));
        }
    }

    return inventory;
}

LegacyAssetInventory BuildDefaultLegacyAssetInventory()
{
    InventoryScanConfig config;
    config.visual_roots = { "$level$", "$game_meshes$" };
    return BuildLegacyAssetInventory(config);
}

xr_string ComputeLegacyAssetInventoryDigest(const LegacyAssetInventory& inventory)
{
    std::string accumulator;
    accumulator.reserve(4096);

    xr_vector<std::size_t> visual_indices;
    visual_indices.resize(inventory.visuals.size());
    std::iota(visual_indices.begin(), visual_indices.end(), 0u);
    std::sort(visual_indices.begin(), visual_indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        return xr_strcmp(inventory.visuals[lhs].normalized_identifier.c_str(), inventory.visuals[rhs].normalized_identifier.c_str()) < 0;
    });

    for (const std::size_t index : visual_indices)
    {
        const LegacyVisualAsset& visual = inventory.visuals[index];
        accumulator.append("visual|");
        accumulator.append(visual.normalized_identifier.c_str());
        accumulator.push_back('\n');

        xr_vector<const LegacyVisualSource*> sources;
        sources.reserve(visual.sources.size());
        for (const auto& source : visual.sources)
            sources.push_back(&source);
        std::sort(sources.begin(), sources.end(), [](const LegacyVisualSource* lhs, const LegacyVisualSource* rhs) {
            const int root_compare = xr_strcmp(lhs->location.root_alias.c_str(), rhs->location.root_alias.c_str());
            if (root_compare != 0)
                return root_compare < 0;
            return xr_strcmp(lhs->location.relative_path.c_str(), rhs->location.relative_path.c_str()) < 0;
        });

        for (const LegacyVisualSource* source : sources)
        {
            const auto& location = source->location;
            accumulator.append("source|");
            accumulator.append(location.root_alias.c_str());
            accumulator.push_back('\n');
            accumulator.append(location.relative_path.c_str());
            accumulator.push_back('\n');
            accumulator.append(std::to_string(location.file_size));
            accumulator.push_back('\n');
            accumulator.append(std::to_string(location.modified_time_seconds));
            accumulator.push_back('\n');
            accumulator.append(location.stored_in_vfs ? "1" : "0");
            accumulator.push_back('\n');

            xr_vector<xr_string> references = source->motion_references;
            std::sort(references.begin(), references.end(), [](const xr_string& lhs, const xr_string& rhs) {
                return xr_strcmp(lhs.c_str(), rhs.c_str()) < 0;
            });

            for (const auto& reference : references)
            {
                accumulator.append("motion_ref|");
                accumulator.append(reference.c_str());
                accumulator.push_back('\n');
            }
        }
    }

    xr_vector<std::size_t> motion_indices;
    motion_indices.resize(inventory.motions.size());
    std::iota(motion_indices.begin(), motion_indices.end(), 0u);
    std::sort(motion_indices.begin(), motion_indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        return xr_strcmp(inventory.motions[lhs].canonical_name.c_str(), inventory.motions[rhs].canonical_name.c_str()) < 0;
    });

    for (const std::size_t index : motion_indices)
    {
        const LegacyMotionAsset& motion = inventory.motions[index];
        accumulator.append("motion|");
        accumulator.append(motion.canonical_name.c_str());
        accumulator.push_back('\n');
        accumulator.append(motion.root_alias.c_str());
        accumulator.push_back('\n');
        accumulator.append(motion.relative_path.c_str());
        accumulator.push_back('\n');
        accumulator.append(std::to_string(motion.file_size));
        accumulator.push_back('\n');
        accumulator.append(std::to_string(motion.modified_time_seconds));
        accumulator.push_back('\n');
        accumulator.append(motion.stored_in_vfs ? "1" : "0");
        accumulator.push_back('\n');
    }

const u32 crc = accumulator.empty() ? 0u : crc32(accumulator.data(), static_cast<u32>(accumulator.size()));
char buffer[9];
std::snprintf(buffer, sizeof(buffer), "%08x", crc);
return xr_string(buffer);
}

xr_string LoadInventoryDigestFromConfig(const fs::path& config_path)
{
    if (config_path.empty())
        return {};

    std::error_code ec;
    if (!fs::exists(config_path, ec) || ec)
        return {};

    std::ifstream stream(config_path);
    if (!stream)
        return {};

    const std::string section_header = std::string("[") + kInventoryDigestSection + "]";
    bool in_section = false;
    std::string line;
    while (std::getline(stream, line))
    {
        const std::string trimmed = TrimCopyStd(line);
        if (trimmed.empty() || trimmed.front() == ';')
            continue;

        if (trimmed.front() == '[' && trimmed.back() == ']')
        {
            const std::string section_name = TrimCopyStd(trimmed.substr(1, trimmed.size() - 2));
            in_section = EqualsIgnoreCase(section_name, kInventoryDigestSection);
            continue;
        }

        if (!in_section)
            continue;

        const auto equals = trimmed.find('=');
        if (equals == std::string::npos)
            continue;

        std::string key = TrimCopyStd(trimmed.substr(0, equals));
        if (!EqualsIgnoreCase(key, kInventoryDigestKey))
            continue;

        std::string value = TrimCopyStd(trimmed.substr(equals + 1));
        return xr_string(value.c_str());
    }

    return {};
}

xr_string LoadInventoryDigestFromUserConfig()
{
    if (const auto existing = ResolveExistingUserConfigPath())
        return LoadInventoryDigestFromConfig(*existing);

    return {};
}

bool StoreInventoryDigestInConfig(const fs::path& config_path, const xr_string& digest)
{
    if (config_path.empty())
        return false;

    if (!config_path.parent_path().empty())
    {
        std::error_code ec;
        fs::create_directories(config_path.parent_path(), ec);
    }

    std::vector<std::string> lines;
    {
        std::error_code ec;
        if (fs::exists(config_path, ec) && !ec)
        {
            std::ifstream stream(config_path);
            if (stream)
            {
                std::string line;
                while (std::getline(stream, line))
                    lines.emplace_back(std::move(line));
            }
        }
    }

    const std::string section_header = std::string("[") + kInventoryDigestSection + "]";
    const std::string key_line = std::string(kInventoryDigestKey) + " = " + digest.c_str();

    bool section_found = false;
    bool key_written = false;
    bool in_section = false;
    int insert_index = -1;

    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        const std::string trimmed = TrimCopyStd(lines[index]);
        if (!trimmed.empty() && trimmed.front() == ';')
            continue;

        if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']')
        {
            const std::string section_name = TrimCopyStd(trimmed.substr(1, trimmed.size() - 2));
            in_section = EqualsIgnoreCase(section_name, kInventoryDigestSection);
            if (in_section)
            {
                section_found = true;
                insert_index = static_cast<int>(index + 1);
            }
            continue;
        }

        if (!in_section)
            continue;

        const auto equals = trimmed.find('=');
        if (equals == std::string::npos)
        {
            insert_index = static_cast<int>(index + 1);
            continue;
        }

        std::string key = TrimCopyStd(trimmed.substr(0, equals));
        if (EqualsIgnoreCase(key, kInventoryDigestKey))
        {
            lines[index] = key_line;
            key_written = true;
        }
        insert_index = static_cast<int>(index + 1);
    }

    if (section_found && !key_written)
    {
        if (insert_index < 0 || insert_index > static_cast<int>(lines.size()))
            lines.emplace_back(key_line);
        else
            lines.insert(lines.begin() + insert_index, key_line);
    }
    else if (!section_found)
    {
        if (!lines.empty() && !TrimCopyStd(lines.back()).empty())
            lines.emplace_back();
        lines.emplace_back(section_header);
        lines.emplace_back(key_line);
    }

    std::ofstream stream(config_path, std::ios::binary | std::ios::trunc);
    if (!stream)
        return false;

    for (const auto& line : lines)
        stream << line << '\n';

    return true;
}

bool StoreInventoryDigestInUserConfig(const xr_string& digest)
{
    const fs::path config_path = ResolveUserConfigPathForWrite();
    return StoreInventoryDigestInConfig(config_path, digest);
}

bool VerifyConvertedOutputs(const LegacyAssetInventory& inventory, const StartupConversionParams& params)
{
    for (const auto& visual : inventory.visuals)
    {
        if (!BundleExists(visual, params))
            return false;
    }

    for (const auto& motion : inventory.motions)
    {
        if (!MotionExists(motion, params))
            return false;
    }

    return true;
}

bool ConvertInventoryToOzz(const LegacyAssetInventory& inventory,
                           const StartupConversionParams& params,
                           StartupConversionStats& out_stats,
                           ProgressCallback progress_callback)
{
    using namespace std::chrono;
    const auto start_time = high_resolution_clock::now();

    out_stats = {};

    Msg("[ozz][debug] ConvertInventoryToOzz: ozz_output_alias='%s', inventory has %zu visuals, %zu motions",
        params.ozz_output_alias.c_str(), inventory.visuals.size(), inventory.motions.size());

    struct VisualConversionWork
    {
        enum class Failure
        {
            None,
            MissingSource,
            ReadFailed,
            ConvertFailed
        };

        const LegacyVisualAsset* visual = nullptr;
        const LegacyVisualSource* primary = nullptr;
        bool convert_bundle = false;
        xr_string output_alias;
        xr_vector<xr_string> motion_names;
        LegacyVisualConversionResult conversion;
        bool success = false;
        Failure failure = Failure::None;
        xr_string convert_error;
    };

    struct MotionTask
    {
        const LegacyMotionAsset* asset = nullptr;
        xr_string output_relative;
        xr_string output_alias;
        bool convert = false;
        const LegacyVisualConversionResult* source_conversion = nullptr;
    };

    std::vector<VisualConversionWork> visual_tasks;
    visual_tasks.reserve(inventory.visuals.size());

    std::unordered_map<xr_string, MotionTask> motion_tasks;
    motion_tasks.reserve(inventory.motions.size());

    for (const auto& visual : inventory.visuals)
    {
        const LegacyVisualSource* primary = visual.PrimarySource();
        if (!primary)
        {
            Msg("! [ozz] Visual '%s' has no primary source", visual.normalized_identifier.c_str());
            ++out_stats.failures;
            continue;
        }

        VisualConversionWork work;
        work.visual = &visual;
        work.primary = primary;
        work.output_alias = DetermineBundleOutputAlias(visual, params);
        const xr_string bundle_relative = BuildBundleRelativePath(visual);
        work.convert_bundle = !FileExists(work.output_alias, bundle_relative);

        Msg("  [ozz][debug] Visual '%s': convert_bundle=%s (alias='%s', relative='%s')",
            visual.normalized_identifier.c_str(),
            work.convert_bundle ? "YES" : "NO",
            work.output_alias.c_str(),
            bundle_relative.c_str());

        for (const auto& reference : primary->motion_references)
        {
            xr_string canonical_name = CanonicalizeMotionName(reference);
            if (canonical_name.empty())
                continue;

            if (std::find(work.motion_names.begin(), work.motion_names.end(), canonical_name) != work.motion_names.end())
                continue;

            work.motion_names.emplace_back(canonical_name);

            auto [mt_it, inserted] = motion_tasks.try_emplace(work.motion_names.back());
            if (inserted)
            {
                const LegacyMotionAsset* motion = inventory.FindMotion(work.motion_names.back());
                if (!motion)
                {
                    Msg("! [ozz] Missing motion metadata for '%s' referenced by '%s'",
                        work.motion_names.back().c_str(), visual.normalized_identifier.c_str());
                    ++out_stats.failures;
                    motion_tasks.erase(mt_it);
                }
                else
                {
                    mt_it->second.asset = motion;
                    mt_it->second.output_relative = BuildMotionOutputRelativePath(*motion);
                    mt_it->second.output_alias = DetermineMotionOutputAlias(*motion, params);
                    mt_it->second.convert = !FileExists(mt_it->second.output_alias, mt_it->second.output_relative);

                    Msg("  [ozz][debug] Motion '%s': convert=%s (alias='%s', relative='%s')",
                        canonical_name.c_str(),
                        mt_it->second.convert ? "YES" : "NO",
                        mt_it->second.output_alias.c_str(),
                        mt_it->second.output_relative.c_str());
                }
            }
        }

        visual_tasks.emplace_back(std::move(work));
    }

    if (visual_tasks.empty())
        return out_stats.failures == 0;

    // Count how many conversions we'll actually do
    size_t bundles_to_convert = 0;
    size_t bundles_to_skip = 0;
    for (const auto& work : visual_tasks)
    {
        if (work.convert_bundle)
            ++bundles_to_convert;
        else
            ++bundles_to_skip;
    }

    size_t motions_to_convert = 0;
    size_t motions_to_skip = 0;
    for (const auto& motion_pair : motion_tasks)
    {
        if (motion_pair.second.convert)
            ++motions_to_convert;
        else
            ++motions_to_skip;
    }

    Msg("[ozz][debug] Conversion plan: bundles (convert=%zu, skip=%zu), motions (convert=%zu, skip=%zu)",
        bundles_to_convert, bundles_to_skip, motions_to_convert, motions_to_skip);

    // Progress tracking
    ConversionProgress progress;
    progress.total_assets = bundles_to_convert + motions_to_convert;
    std::atomic<size_t> completed_assets_atomic{0};

    const auto visual_start = high_resolution_clock::now();

    xr_parallel_for(TaskRange<size_t>(0, visual_tasks.size()), [&](const TaskRange<size_t>& range)
    {
        for (size_t idx = range.begin(); idx != range.end(); ++idx)
        {
            auto& work = visual_tasks[idx];
            if (!work.primary)
            {
                work.failure = VisualConversionWork::Failure::MissingSource;
                continue;
            }

            // If bundle already exists on disk, load skeleton from it instead of reconverting
            if (!work.convert_bundle)
            {
                const xr_string bundle_relative = BuildBundleRelativePath(*work.visual);
                fs::path bundle_path;
                if (ResolveAbsolutePath(work.output_alias, bundle_relative, bundle_path))
                {
                    OzzxBundle existing_bundle;
                    if (ReadOzzxBundle(bundle_path, existing_bundle))
                    {
                        // Load skeleton from existing bundle for motion conversion
                        work.conversion.skeleton_binary = std::move(existing_bundle.skeleton);
                        work.conversion.bone_metadata = std::move(existing_bundle.bone_metadata);

                        // Deserialize skeleton for motion conversion
                        if (!work.conversion.skeleton_binary.empty())
                        {
                            ozz::io::MemoryStream stream;
                            stream.Write(work.conversion.skeleton_binary.data(), work.conversion.skeleton_binary.size());
                            stream.Seek(0, ozz::io::Stream::kSet);
                            ozz::io::IArchive archive(&stream);

                            auto skeleton = ozz::make_unique<ozz::animation::Skeleton>();
                            archive >> *skeleton;
                            work.conversion.skeleton = std::move(skeleton);
                        }

                        // Extract bone names for motion conversion
                        if (work.conversion.skeleton)
                        {
                            const int joint_count = work.conversion.skeleton->num_joints();
                            work.conversion.bone_names.resize(joint_count);
                            for (int i = 0; i < joint_count; ++i)
                                work.conversion.bone_names[i] = work.conversion.skeleton->joint_names()[i];
                        }

                        work.success = true;
                        continue;
                    }
                }
                // If loading failed, fall through to do full conversion
                Msg("! [ozz] Failed to load existing bundle '%s', will reconvert", bundle_relative.c_str());
            }

            // Do full conversion from OGF
            auto ogf_bytes = ReadFileBytes(work.primary->location.root_alias, work.primary->location.relative_path);
            if (ogf_bytes.empty())
            {
                work.failure = VisualConversionWork::Failure::ReadFailed;
                continue;
            }

            LegacyVisualInput input;
            input.buffer = ozz::span<const std::byte>(ogf_bytes.data(), ogf_bytes.size());

            fs::path resolved_source;
            if (ResolveAbsolutePath(work.primary->location.root_alias, work.primary->location.relative_path, resolved_source))
                input.source_path = resolved_source;

            LegacyVisualConversionOptions options;
            options.build_mesh = true;
            options.build_skeleton = true;

            xr_string error;
            if (!ConvertLegacyVisualToOzzBundle(input, work.conversion, options, &error))
            {
                work.failure = VisualConversionWork::Failure::ConvertFailed;
                work.convert_error = error;
                continue;
            }

            work.success = true;

            // Report progress for converted bundles
            if (work.convert_bundle)
            {
                progress.completed_assets = ++completed_assets_atomic;
                if (progress_callback)
                    progress_callback(progress);
            }
        }
    });

    for (auto& work : visual_tasks)
    {
        const LegacyVisualAsset& visual = *work.visual;

        if (!work.success)
        {
            switch (work.failure)
            {
            case VisualConversionWork::Failure::MissingSource:
                Msg("! [ozz] Visual '%s' has no primary source", visual.normalized_identifier.c_str());
                break;
            case VisualConversionWork::Failure::ReadFailed:
                Msg("! [ozz] Failed to read legacy visual '%s:%s'",
                    work.primary->location.root_alias.c_str(), work.primary->location.relative_path.c_str());
                break;
            case VisualConversionWork::Failure::ConvertFailed:
                Msg("! [ozz] Failed to convert '%s:%s' (%s)",
                    work.primary->location.root_alias.c_str(), work.primary->location.relative_path.c_str(),
                    work.convert_error.c_str());
                break;
            case VisualConversionWork::Failure::None:
                break;
            }

            ++out_stats.failures;
            continue;
        }

        if (work.convert_bundle)
        {
            if (WriteBundleFile(work.output_alias, visual, work.conversion))
                ++out_stats.bundles_written;
            else
                ++out_stats.failures;
        }
        else
        {
            ++out_stats.bundles_skipped;
        }

        if (!work.conversion.skeleton)
        {
            Msg("! [ozz] Conversion for '%s' produced no skeleton", visual.normalized_identifier.c_str());
            ++out_stats.failures;
            continue;
        }

        for (const auto& canonical : work.motion_names)
        {
            auto mt_it = motion_tasks.find(canonical);
            if (mt_it == motion_tasks.end())
                continue;

            mt_it->second.source_conversion = &work.conversion;
        }
    }

    const auto visual_end = high_resolution_clock::now();
    out_stats.visual_conversion_time_seconds = duration_cast<duration<double>>(visual_end - visual_start).count();

    // Parallelize motion conversion with atomic counters for thread safety
    const auto motion_start = high_resolution_clock::now();

    std::atomic<size_t> motions_written_atomic{0};
    std::atomic<size_t> motions_skipped_atomic{0};
    std::atomic<size_t> motion_failures_atomic{0};
    std::atomic<size_t> total_animations_converted_atomic{0};

    // Convert map to vector for parallel processing
    xr_vector<std::pair<xr_string, MotionTask*>> motion_tasks_vec;
    motion_tasks_vec.reserve(motion_tasks.size());
    for (auto& motion_entry : motion_tasks)
        motion_tasks_vec.emplace_back(motion_entry.first, &motion_entry.second);

    xr_parallel_for(TaskRange<size_t>(0, motion_tasks_vec.size()), [&](const TaskRange<size_t>& range)
    {
        for (size_t idx = range.begin(); idx != range.end(); ++idx)
        {
            const xr_string& canonical = motion_tasks_vec[idx].first;
            MotionTask& task = *motion_tasks_vec[idx].second;

            if (!task.asset)
                continue; // already reported and accounted for

            if (!task.source_conversion || !task.source_conversion->skeleton)
            {
                Msg("! [ozz] No converted skeleton available for motion '%s'", canonical.c_str());
                ++motion_failures_atomic;
                continue;
            }

            if (!task.convert)
            {
                ++motions_skipped_atomic;
                continue;
            }

            auto omf_bytes = ReadFileBytes(task.asset->root_alias, task.asset->relative_path);
            if (omf_bytes.empty())
            {
                Msg("! [ozz] Failed to read legacy motion '%s:%s'", task.asset->root_alias.c_str(), task.asset->relative_path.c_str());
                ++motion_failures_atomic;
                continue;
            }

            xr_vector<ConvertedOmfAnimation> converted_animations;
            if (!ConvertLegacyOmf(omf_bytes.data(), omf_bytes.size(), task.source_conversion->bone_names,
                                  *task.source_conversion->skeleton, converted_animations))
            {
                Msg("! [ozz] Failed to convert legacy motion '%s'", task.asset->relative_path.c_str());
                ++motion_failures_atomic;
                continue;
            }

            if (converted_animations.empty())
            {
                Msg("! [ozz] Motion '%s' produced no Ozz animations", task.asset->relative_path.c_str());
                ++motion_failures_atomic;
                continue;
            }

            if (WriteOzzAnimations(task.output_alias, task.output_relative, converted_animations))
            {
                ++motions_written_atomic;
                total_animations_converted_atomic += converted_animations.size();

                // Report progress for converted motions
                progress.completed_assets = ++completed_assets_atomic;
                if (progress_callback)
                    progress_callback(progress);
            }
            else
            {
                ++motion_failures_atomic;
            }
        }
    });

    const auto motion_end = high_resolution_clock::now();
    out_stats.motion_conversion_time_seconds = duration_cast<duration<double>>(motion_end - motion_start).count();

    // Update stats with atomic counts
    out_stats.motions_written += motions_written_atomic.load();
    out_stats.motions_skipped += motions_skipped_atomic.load();
    out_stats.failures += motion_failures_atomic.load();

    const size_t motion_files_converted = motions_written_atomic.load();
    const size_t total_animations_converted = total_animations_converted_atomic.load();

    // Grand total
    const double total_conversion_time_s = out_stats.visual_conversion_time_seconds + out_stats.motion_conversion_time_seconds;

    Msg("[xray_to_ozz_conversion] Visual (OGF -> OZZX):  %zu bundles in %.2f seconds",
        out_stats.bundles_written, out_stats.visual_conversion_time_seconds);
    Msg("[xray_to_ozz_conversion] Motion (OMF -> OZZ):   %zu animations from %zu files in %.2f seconds",
        total_animations_converted, motion_files_converted, out_stats.motion_conversion_time_seconds);
    Msg(" ");
    Msg("[xray_to_ozz_conversion] GRAND TOTAL: %.2f seconds", total_conversion_time_s);

    // Calculate total time
    const auto end_time = high_resolution_clock::now();
    out_stats.total_time_seconds = duration_cast<duration<double>>(end_time - start_time).count();

    return out_stats.failures == 0;
}

} // namespace Animation
} // namespace XRay
