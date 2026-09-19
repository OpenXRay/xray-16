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

#include <cstddef>
#include <exception>
#include <optional>
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

shared_str MakeLibraryKey(const shared_str& omf_key, u32 fingerprint)
{
    string64 suffix;
    xr_sprintf(suffix, sizeof(suffix), "#%08x", fingerprint);

    xr_string composed = omf_key.c_str() ? omf_key.c_str() : "";
    composed += suffix;
    return shared_str(composed.c_str());
}

bool ResolveOmfPath(const shared_str& omf_key, string_path& fn)
{
    if (FS.exist(fn, "$level$", omf_key.c_str()))
        return true;
    if (FS.exist(fn, "$game_meshes$", omf_key.c_str()))
        return true;
    return false;
}

void MakeCachePath(pcstr omf_fn, u32 fingerprint, string_path& out)
{
    xr_strcpy(out, sizeof(string_path), omf_fn);
    if (pstr ext = strext(out))
        *ext = 0;

    string64 suffix;
    xr_sprintf(suffix, sizeof(suffix), ".%08x.ozz", fingerprint);
    xr_strcat(out, sizeof(string_path), suffix);
}

bool LoadCache(pcstr path, u32 fingerprint, u32 omf_size, u32 omf_modif, AnimationVec& out)
{
    if (!FS.exist(path, FSType::Any))
        return false;

    IReader* R = FS.r_open(path);
    if (!R)
        return false;

    bool ok = false;
    if (R->length() >= 6 * sizeof(u32))
    {
        const u32 magic = R->r_u32();
        const u32 version = R->r_u32();
        const u32 fp = R->r_u32();
        const u32 size_real = R->r_u32();
        const u32 modif = R->r_u32();
        const u32 count = R->r_u32();

        if (magic == kCacheMagic && version == kCacheVersion && fp == fingerprint && size_real == omf_size &&
            modif == omf_modif)
        {
            const size_t bytes = R->length() - R->tell();
            ozz::io::MemoryStream stream;
            if (bytes == 0 || stream.Write(R->pointer(), bytes) == bytes)
            {
                stream.Seek(0, ozz::io::Stream::kSet);
                ozz::io::IArchive archive(&stream);

                out.clear();
                out.reserve(count);
                ok = true;
                for (u32 i = 0; i < count; ++i)
                {
                    if (!archive.TestTag<ozz::animation::Animation>())
                    {
                        ok = false;
                        break;
                    }
                    out.emplace_back(ozz::make_unique<ozz::animation::Animation>());
                    archive >> *out.back();
                }
                if (!ok)
                    out.clear();
            }
        }
    }

    FS.r_close(R);
    return ok;
}

void SaveCache(pcstr path, u32 fingerprint, u32 omf_size, u32 omf_modif, const AnimationVec& animations)
{
    ozz::io::MemoryStream stream;
    {
        ozz::io::OArchive archive(&stream);
        for (const auto& animation : animations)
            archive << *animation;
    }

    const size_t bytes = stream.Size();
    xr_vector<u8> blob(bytes);
    stream.Seek(0, ozz::io::Stream::kSet);
    if (bytes != 0 && stream.Read(blob.data(), bytes) != bytes)
        return;

    string_path tmp;
    xr_strcpy(tmp, sizeof(tmp), path);
    xr_strcat(tmp, sizeof(tmp), ".tmp");

    IWriter* W = FS.w_open(tmp);
    if (!W)
        return;

    if (!W->valid())
    {
        FS.w_close(W);
        return;
    }

    W->w_u32(kCacheMagic);
    W->w_u32(kCacheVersion);
    W->w_u32(fingerprint);
    W->w_u32(omf_size);
    W->w_u32(omf_modif);
    W->w_u32(u32(animations.size()));
    if (bytes != 0)
        W->w(blob.data(), bytes);
    FS.w_close(W);

    FS.file_rename(tmp, path, true);
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
        if (!ConvertLegacyOmf(static_cast<const std::byte*>(omf->begin()), omf->length(), names, mirror.skeleton,
                converted, std::nullopt, false))
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

    string_path omf_fn;
    const bool resolved = ResolveOmfPath(omf_key, omf_fn);

    u32 omf_size = omf ? u32(omf->length()) : 0u;
    u32 omf_modif = 0u;
    if (resolved)
    {
        if (const auto* desc = FS.GetFileDesc(omf_fn))
        {
            omf_size = desc->size_real;
            omf_modif = desc->modif;
        }
    }

    string_path cache_fn;
    if (resolved)
        MakeCachePath(omf_fn, mirror.fingerprint, cache_fn);

    AnimationVec animations;
    if (!resolved || !LoadCache(cache_fn, mirror.fingerprint, omf_size, omf_modif, animations))
    {
        if (!BakeLibrary(omf_key, omf, mirror, animations))
        {
            Msg("! [ozz] can't bake motion library [%s]", omf_key.c_str());
            return nullptr;
        }
        if (resolved)
            SaveCache(cache_fn, mirror.fingerprint, omf_size, omf_modif, animations);
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
