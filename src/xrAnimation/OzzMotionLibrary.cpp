#include "stdafx.h"

#include "OzzMotionLibrary.h"

#include "LegacyOmfConverter.h"
#include "OzzSkeletonMirror.h"

#include "xrCore/LocatorAPI.h"
#include "xrCore/FS.h"

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/span.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

namespace XRay
{
namespace Animation
{
namespace
{
using AnimationVec = xr_vector<ozz::unique_ptr<ozz::animation::Animation>>;

constexpr u32 kCacheMagic = u32('O') | (u32('Z') << 8) | (u32('Z') << 16) | (u32('M') << 24);
constexpr u32 kCacheVersion = 1u;
constexpr size_t kCacheHeaderFields = 6;
constexpr size_t kCacheHeaderSize = kCacheHeaderFields * sizeof(u32);

struct CacheTarget
{
    std::filesystem::path path;
    u32 fingerprint{ 0u };
    u32 omf_size{ 0u };
    u32 omf_modif{ 0u };
};

shared_str MakeLibraryKey(const shared_str& omf_key, u32 fingerprint)
{
    string64 suffix;
    xr_sprintf(suffix, sizeof(suffix), "#%08x", fingerprint);

    xr_string composed = omf_key.c_str() ? omf_key.c_str() : "";
    composed += suffix;
    return shared_str(composed.c_str());
}

std::filesystem::path NativePath(pcstr path)
{
    std::string value(path ? path : "");
    const char preferred = char(std::filesystem::path::preferred_separator);
    if (preferred != '\\')
        std::replace(value.begin(), value.end(), '\\', preferred);
    return std::filesystem::path(value);
}

bool ResolveCacheTarget(const shared_str& omf_key, u32 fingerprint, CacheTarget& target)
{
    string_path omf_fn;
    if (!FS.exist(omf_fn, "$level$", omf_key.c_str()) && !FS.exist(omf_fn, "$game_meshes$", omf_key.c_str()))
        return false;

    const auto* desc = FS.GetFileDesc(omf_fn);
    if (!desc)
        return false;

    string_path cache_fn;
    xr_strcpy(cache_fn, sizeof(cache_fn), omf_fn);
    if (pstr ext = strext(cache_fn))
        *ext = 0;

    string64 suffix;
    xr_sprintf(suffix, sizeof(suffix), ".%08x.ozz", fingerprint);
    xr_strcat(cache_fn, sizeof(cache_fn), suffix);

    target.path = NativePath(cache_fn);
    target.fingerprint = fingerprint;
    target.omf_size = desc->size_real;
    target.omf_modif = desc->modif;
    return true;
}

bool ReadCacheHeader(std::ifstream& stream, const CacheTarget& target, u32& count)
{
    u32 header[kCacheHeaderFields];
    stream.read(reinterpret_cast<char*>(header), std::streamsize(sizeof(header)));
    if (stream.gcount() != std::streamsize(sizeof(header)))
        return false;

    if (header[0] != kCacheMagic || header[1] != kCacheVersion || header[2] != target.fingerprint ||
        header[3] != target.omf_size || header[4] != target.omf_modif)
        return false;

    count = header[5];
    return true;
}

bool CacheUpToDate(const CacheTarget& target)
{
    std::ifstream stream(target.path, std::ios::binary);
    if (!stream)
        return false;

    u32 count = 0;
    return ReadCacheHeader(stream, target, count);
}

bool LoadCache(const CacheTarget& target, AnimationVec& out)
{
    std::ifstream stream(target.path, std::ios::binary | std::ios::ate);
    if (!stream)
        return false;

    const std::streamoff total = stream.tellg();
    if (total < std::streamoff(kCacheHeaderSize))
        return false;

    stream.seekg(0, std::ios::beg);

    u32 count = 0;
    if (!ReadCacheHeader(stream, target, count))
        return false;

    const size_t bytes = size_t(total) - kCacheHeaderSize;
    xr_vector<u8> blob(bytes);
    if (bytes != 0)
    {
        stream.read(reinterpret_cast<char*>(blob.data()), std::streamsize(bytes));
        if (stream.gcount() != std::streamsize(bytes))
            return false;
    }

    ozz::io::MemoryStream memory;
    if (bytes != 0 && memory.Write(blob.data(), bytes) != bytes)
        return false;

    memory.Seek(0, ozz::io::Stream::kSet);
    ozz::io::IArchive archive(&memory);

    out.clear();
    out.reserve(count);
    for (u32 i = 0; i < count; ++i)
    {
        if (!archive.TestTag<ozz::animation::Animation>())
        {
            out.clear();
            return false;
        }
        out.emplace_back(ozz::make_unique<ozz::animation::Animation>());
        archive >> *out.back();
    }
    return true;
}

void SaveCache(const CacheTarget& target, const AnimationVec& animations)
{
    ozz::io::MemoryStream memory;
    {
        ozz::io::OArchive archive(&memory);
        for (const auto& animation : animations)
            archive << *animation;
    }

    const size_t bytes = size_t(memory.Size());
    xr_vector<u8> blob(bytes);
    memory.Seek(0, ozz::io::Stream::kSet);
    if (bytes != 0 && memory.Read(blob.data(), bytes) != bytes)
        return;

    std::filesystem::path tmp = target.path;
    tmp += ".tmp";

    {
        std::ofstream stream(tmp, std::ios::binary | std::ios::trunc);
        if (!stream)
            return;

        const u32 header[kCacheHeaderFields] = { kCacheMagic, kCacheVersion, target.fingerprint, target.omf_size,
            target.omf_modif, u32(animations.size()) };
        stream.write(reinterpret_cast<const char*>(header), std::streamsize(sizeof(header)));
        if (bytes != 0)
            stream.write(reinterpret_cast<const char*>(blob.data()), std::streamsize(bytes));

        if (!stream)
        {
            stream.close();
            std::error_code ec;
            std::filesystem::remove(tmp, ec);
            return;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmp, target.path, ec);
    if (ec)
        std::filesystem::remove(tmp, ec);
}

bool BakeLibrary(const shared_str& omf_key, IReader* omf, const OzzSkeletonMirror& mirror, AnimationVec& out)
{
    if (!omf || omf->length() == 0)
        return false;

    const ozz::span<const char* const> jointNames = mirror.skeleton.joint_names();
    xr_vector<xr_string> names;
    names.reserve(jointNames.size());
    for (size_t i = 0; i < jointNames.size(); ++i)
        names.emplace_back(jointNames[i] ? jointNames[i] : "");

    xr_vector<ConvertedOmfAnimation> converted;
    try
    {
        if (!ConvertLegacyOmf(
                static_cast<const std::byte*>(omf->begin()), omf->length(), names, mirror.skeleton, converted))
            return false;
    }
    catch (const std::exception& error)
    {
        Msg("! [ozz] can't convert motions [%s]: %s", omf_key.c_str(), error.what());
        return false;
    }

    out.clear();
    out.reserve(converted.size());
    for (auto& item : converted)
    {
        if (!item.animation)
        {
            out.clear();
            return false;
        }
        out.emplace_back(std::move(item.animation));
    }
    return true;
}
}

OzzMotionLibraryContainer* g_pOzzMotionLibraries = nullptr;

bool PrebakeMotionLibrary(const shared_str& omf_key, IReader* omf, const OzzSkeletonMirror& mirror)
{
    CacheTarget target;
    if (!ResolveCacheTarget(omf_key, mirror.fingerprint, target))
        return false;

    if (CacheUpToDate(target))
        return true;

    AnimationVec animations;
    if (!BakeLibrary(omf_key, omf, mirror, animations))
        return false;

    SaveCache(target, animations);
    return true;
}

const ozz::vector<ozz::math::SoaTransform>& FirstFrame(OzzMotionLibrary& library, u16 idx)
{
    ozz::vector<ozz::math::SoaTransform>& frame = library.firstFrame[idx];
    if (frame.empty())
    {
        const ozz::animation::Animation* animation = library.animations[idx].get();
        frame.resize(size_t(animation->num_soa_tracks()));

        ozz::animation::SamplingJob::Context context(animation->num_tracks());
        ozz::animation::SamplingJob job;
        job.animation = animation;
        job.context = &context;
        job.ratio = 0.f;
        job.output = ozz::make_span(frame);
        job.Run();
    }
    return frame;
}

OzzMotionLibraryContainer::~OzzMotionLibraryContainer() { clean(true); }

OzzMotionLibrary* OzzMotionLibraryContainer::dock(
    const shared_str& omf_key, IReader* omf, const OzzSkeletonMirror& mirror)
{
    const shared_str key = MakeLibraryKey(omf_key, mirror.fingerprint);

    const auto it = container.find(key);
    if (it != container.end())
    {
        ++it->second->refs;
        return it->second;
    }

    AnimationVec animations;
    bool ready = false;

    CacheTarget target;
    if (ResolveCacheTarget(omf_key, mirror.fingerprint, target) && PrebakeMotionLibrary(omf_key, omf, mirror))
        ready = LoadCache(target, animations);

    if (!ready && !BakeLibrary(omf_key, omf, mirror, animations))
    {
        Msg("! [ozz] can't bake motion library [%s]", omf_key.c_str());
        return nullptr;
    }

    OzzMotionLibrary* library = xr_new<OzzMotionLibrary>();
    library->key = key;
    library->fingerprint = mirror.fingerprint;
    library->animations = std::move(animations);
    library->firstFrame.resize(library->animations.size());
    library->refs = 1;

    container.insert(std::make_pair(key, library));
    return library;
}

void OzzMotionLibraryContainer::undock(OzzMotionLibrary* library)
{
    if (library && library->refs)
        --library->refs;
}

void OzzMotionLibraryContainer::clean(bool force)
{
    auto it = container.begin();
    const auto end = container.end();
    if (force)
    {
        for (; it != end; ++it)
        {
            OzzMotionLibrary* library = it->second;
            xr_delete(library);
        }
        container.clear();
        return;
    }

    while (it != end)
    {
        OzzMotionLibrary* library = it->second;
        if (0 == library->refs)
        {
            const auto current = it;
            const auto next = ++it;
            xr_delete(library);
            container.erase(current);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}
}
}
