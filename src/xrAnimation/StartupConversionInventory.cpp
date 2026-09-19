#include "stdafx.h"

#include "StartupConversionInventory.h"

#include "LegacyOgfSkeleton.h"
#include "OzzMotionLibrary.h"
#include "OzzSkeletonMirror.h"

#include "xrCore/FMesh.hpp"
#include "xrCore/FS.h"
#include "xrCore/LocatorAPI.h"
#include "xrCore/log.h"
#include "xrCore/Threading/ParallelFor.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <memory>
#include <unordered_set>
#include <utility>

namespace XRay
{
namespace Animation
{
namespace
{
using MirrorPtr = std::shared_ptr<const OzzSkeletonMirror>;

constexpr u32 kInventoryVersion = 1u;

struct VisualEntry
{
    pcstr alias{ nullptr };
    xr_string relative;
    u32 size{ 0u };
    u32 modif{ 0u };
};

struct VisualResult
{
    MirrorPtr mirror;
    xr_vector<shared_str> keys;
    u32 fingerprint{ 0u };
    bool cached{ false };
    bool failed{ false };
};

struct PrebakeJob
{
    shared_str key;
    MirrorPtr mirror;
};

xr_string CanonicalizeRelativePath(pcstr value)
{
    xr_string result(value ? value : "");
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char ch) { return char(std::tolower(ch)); });
    std::replace(result.begin(), result.end(), '/', '\\');
    while (!result.empty() && result.front() == '\\')
        result.erase(result.begin());
    return result;
}

bool OpenByKey(pcstr key, IReader*& reader)
{
    string_path fn;
    if (!FS.exist(fn, "$level$", key) && !FS.exist(fn, "$game_meshes$", key))
        return false;

    reader = FS.r_open(fn);
    return reader != nullptr;
}

xr_vector<VisualEntry> ScanVisuals()
{
    static pcstr const roots[] = { "$level$", "$game_meshes$" };

    xr_vector<VisualEntry> visuals;
    std::unordered_set<xr_string> seen;

    for (pcstr root : roots)
    {
        FS_FileSet files;
        FS.file_list(files, root, FS_ListFiles, "*.ogf");

        for (const auto& file : files)
        {
            xr_string relative = CanonicalizeRelativePath(file.name.c_str());
            if (relative.empty())
                continue;
            if (!seen.insert(relative).second)
                continue;

            VisualEntry entry;
            entry.alias = root;
            entry.relative = std::move(relative);
            entry.size = file.size;
            entry.modif = u32(file.time_write);
            visuals.emplace_back(std::move(entry));
        }
    }

    return visuals;
}

struct InventoryCache
{
    struct Record
    {
        u32 size{ 0u };
        u32 modif{ 0u };
        u32 fingerprint{ 0u };
        xr_vector<xr_string> keys;
    };

    std::unordered_map<xr_string, Record> records;
    bool dirty{ false };
};

string_path& InventoryCachePath()
{
    static string_path path;
    if (path[0] == 0)
        FS.update_path(path, "$app_data_root$", "ozz_inventory.cache");
    return path;
}

void LoadInventoryCache(InventoryCache& cache)
{
    IReader* reader = FS.r_open(InventoryCachePath());
    if (!reader)
        return;

    if (reader->length() >= int(sizeof(u32)) && reader->r_u32() == kInventoryVersion)
    {
        while (reader->elapsed() > int(sizeof(u32) * 4))
        {
            string_path relative;
            reader->r_stringZ(relative, sizeof(relative));

            InventoryCache::Record record;
            record.size = reader->r_u32();
            record.modif = reader->r_u32();
            record.fingerprint = reader->r_u32();

            const u32 count = reader->r_u32();
            record.keys.reserve(count);
            for (u32 i = 0; i < count; ++i)
            {
                string_path key;
                reader->r_stringZ(key, sizeof(key));
                record.keys.emplace_back(key);
            }
            cache.records.emplace(xr_string(relative), std::move(record));
        }
    }

    FS.r_close(reader);
}

void SaveInventoryCache(const InventoryCache& cache)
{
    if (!cache.dirty)
        return;

    IWriter* writer = FS.w_open(InventoryCachePath());
    if (!writer)
        return;

    writer->w_u32(kInventoryVersion);
    for (const auto& item : cache.records)
    {
        writer->w_stringZ(item.first.c_str());
        writer->w_u32(item.second.size);
        writer->w_u32(item.second.modif);
        writer->w_u32(item.second.fingerprint);
        writer->w_u32(u32(item.second.keys.size()));
        for (const xr_string& key : item.second.keys)
            writer->w_stringZ(key.c_str());
    }
    FS.w_close(writer);
}

void InspectVisual(const VisualEntry& entry, const InventoryCache& cache, VisualResult& result)
{
    const auto known = cache.records.find(entry.relative);
    if (known != cache.records.end() && known->second.size == entry.size && known->second.modif == entry.modif)
    {
        bool complete = true;
        for (const xr_string& key : known->second.keys)
            complete = complete && MotionCachePresent(shared_str(key.c_str()), known->second.fingerprint);

        if (complete)
        {
            result.fingerprint = known->second.fingerprint;
            result.cached = true;
            for (const xr_string& key : known->second.keys)
                result.keys.emplace_back(key.c_str());
            return;
        }
    }

    IReader* ogf = FS.r_open(entry.alias, entry.relative.c_str());
    if (!ogf)
    {
        result.failed = true;
        Msg("! [ozz] can't open visual [%s]", entry.relative.c_str());
        return;
    }

    ogf_header header{};
    const bool has_header = ogf->r_chunk_safe(OGF_HEADER, &header, sizeof(header)) != 0;
    if (!has_header || header.type != MT_SKELETON_ANIM)
    {
        FS.r_close(ogf);
        return;
    }

    xr_vector<OzzBoneDesc> bones;
    xr_vector<shared_str> refs;
    bool embedded = false;
    const bool parsed = ReadOgfSkeleton(ogf, bones, refs, embedded);
    FS.r_close(ogf);

    if (!parsed || bones.empty())
    {
        result.failed = true;
        Msg("! [ozz] can't read skeleton of [%s]", entry.relative.c_str());
        return;
    }

    result.mirror = BuildOzzSkeletonMirror(ozz::span<const OzzBoneDesc>(bones.data(), bones.size()));
    if (!result.mirror)
    {
        result.failed = true;
        Msg("! [ozz] can't build skeleton mirror for [%s]", entry.relative.c_str());
        return;
    }
    result.fingerprint = result.mirror->fingerprint;
    if (embedded)
        result.keys.emplace_back(entry.relative.c_str());
    else
        result.keys = std::move(refs);
}
}

void PrebakeLegacyMotionLibraries(StartupConversionStats& out_stats)
{
    using namespace std::chrono;
    const auto start_time = high_resolution_clock::now();

    out_stats = {};

    const xr_vector<VisualEntry> visuals = ScanVisuals();
    if (visuals.empty())
        return;

    InventoryCache inventory;
    LoadInventoryCache(inventory);

    xr_vector<VisualResult> results(visuals.size());
    xr_parallel_for(TaskRange<size_t>(0, visuals.size()), [&](const TaskRange<size_t>& range)
    {
        for (size_t idx = range.begin(); idx != range.end(); ++idx)
            InspectVisual(visuals[idx], inventory, results[idx]);
    });

    for (size_t idx = 0; idx < visuals.size(); ++idx)
    {
        const VisualResult& result = results[idx];
        if (result.cached || result.failed || !result.mirror)
            continue;

        InventoryCache::Record record;
        record.size = visuals[idx].size;
        record.modif = visuals[idx].modif;
        record.fingerprint = result.fingerprint;
        record.keys.reserve(result.keys.size());
        for (const shared_str& key : result.keys)
            record.keys.emplace_back(key.c_str());

        inventory.records[visuals[idx].relative] = std::move(record);
        inventory.dirty = true;
    }

    xr_vector<PrebakeJob> jobs;
    std::unordered_set<xr_string> queued;

    for (const VisualResult& result : results)
    {
        if (result.failed)
            ++out_stats.failed;
        if (!result.mirror && !result.cached)
            continue;

        for (const shared_str& key : result.keys)
        {
            string64 suffix;
            xr_sprintf(suffix, sizeof(suffix), "#%08x", result.fingerprint);

            xr_string unique(key.c_str() ? key.c_str() : "");
            unique += suffix;

            if (!queued.insert(std::move(unique)).second)
            {
                ++out_stats.skipped;
                continue;
            }

            if (result.cached)
            {
                ++out_stats.cache_hits;
                continue;
            }

            jobs.emplace_back(PrebakeJob{ key, result.mirror });
        }
    }

    std::atomic<size_t> baked{ 0 };
    std::atomic<size_t> cache_hits{ 0 };
    std::atomic<size_t> write_failed{ 0 };
    std::atomic<size_t> failed{ 0 };

    xr_parallel_for(TaskRange<size_t>(0, jobs.size()), [&](const TaskRange<size_t>& range)
    {
        for (size_t idx = range.begin(); idx != range.end(); ++idx)
        {
            const PrebakeJob& job = jobs[idx];

            IReader* reader = nullptr;
            if (!OpenByKey(job.key.c_str(), reader))
            {
                ++failed;
                Msg("! [ozz] can't open motion source [%s]", job.key.c_str());
                continue;
            }

            const PrebakeResult result = PrebakeMotionLibrary(job.key, reader, *job.mirror);
            FS.r_close(reader);

            switch (result)
            {
            case PrebakeResult::CacheHit: ++cache_hits; break;
            case PrebakeResult::Baked: ++baked; break;
            case PrebakeResult::WriteFailed:
                ++write_failed;
                Msg("! [ozz] can't store motion library cache [%s]", job.key.c_str());
                break;
            case PrebakeResult::Failed:
                ++failed;
                Msg("! [ozz] can't prebake motion library [%s]", job.key.c_str());
                break;
            }
        }
    });

    out_stats.baked = baked.load();
    out_stats.cache_hits += cache_hits.load();
    out_stats.write_failed = write_failed.load();
    SaveInventoryCache(inventory);
    out_stats.failed += failed.load();
    out_stats.total_time_seconds = duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count();
}
}
}
