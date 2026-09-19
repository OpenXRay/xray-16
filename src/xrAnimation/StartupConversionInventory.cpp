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

struct VisualEntry
{
    pcstr alias{ nullptr };
    xr_string relative;
};

struct VisualResult
{
    MirrorPtr mirror;
    xr_vector<shared_str> keys;
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
            visuals.emplace_back(std::move(entry));
        }
    }

    return visuals;
}

void InspectVisual(const VisualEntry& entry, VisualResult& result)
{
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

    xr_vector<VisualResult> results(visuals.size());
    xr_parallel_for(TaskRange<size_t>(0, visuals.size()), [&](const TaskRange<size_t>& range)
    {
        for (size_t idx = range.begin(); idx != range.end(); ++idx)
            InspectVisual(visuals[idx], results[idx]);
    });

    xr_vector<PrebakeJob> jobs;
    std::unordered_set<xr_string> queued;

    for (const VisualResult& result : results)
    {
        if (result.failed)
            ++out_stats.failed;
        if (!result.mirror)
            continue;

        for (const shared_str& key : result.keys)
        {
            string64 suffix;
            xr_sprintf(suffix, sizeof(suffix), "#%08x", result.mirror->fingerprint);

            xr_string unique(key.c_str() ? key.c_str() : "");
            unique += suffix;

            if (!queued.insert(std::move(unique)).second)
            {
                ++out_stats.skipped;
                continue;
            }

            jobs.emplace_back(PrebakeJob{ key, result.mirror });
        }
    }

    std::atomic<size_t> baked{ 0 };
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

            const bool ok = PrebakeMotionLibrary(job.key, reader, *job.mirror);
            FS.r_close(reader);

            if (ok)
            {
                ++baked;
            }
            else
            {
                ++failed;
                Msg("! [ozz] can't prebake motion library [%s]", job.key.c_str());
            }
        }
    });

    out_stats.baked = baked.load();
    out_stats.failed += failed.load();
    out_stats.total_time_seconds = duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count();
}
}
}
