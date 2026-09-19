#include "stdafx.h"
#include "LegacyOgfSkeleton.h"
#include "LegacyChunkIO.h"
#include "xrCore/xr_trims.h"

#include <cmath>
#include <unordered_map>

namespace XRay::Animation
{
void ReadOgfSkeleton(IReader* ogf, xr_vector<OzzBoneDesc>& bones,
    xr_vector<shared_str>& motionRefs, bool& embedded)
{
    bones.clear();
    motionRefs.clear();
    embedded = false;
    if (!ogf)
        throw std::runtime_error("missing skeleton source");
    ChunkStorage storage;
    const auto chunks = ParseChunks(static_cast<const std::byte*>(ogf->pointer()), ogf->length(), storage);
    const auto names = chunks.find(OGF_S_BONE_NAMES);
    if (names == chunks.end())
        throw std::runtime_error("missing OGF bones");
    BinaryReader reader{names->second.data, names->second.size};
    const u32 count = reader.read<u32>();
    if (!count || count > ozz::animation::Skeleton::kMaxJoints)
        throw std::runtime_error("invalid OGF bone count");
    bones.resize(count);
    xr_vector<std::string> parents(count);
    std::unordered_map<std::string, u16> indices;
    for (u32 index = 0; index < count; ++index)
    {
        const auto name = ToLowerCopy(reader.read_stringz());
        if (name.empty() || !indices.emplace(name, u16(index)).second)
            throw std::runtime_error("empty or duplicate OGF bone name");
        bones[index].name = name.c_str();
        parents[index] = ToLowerCopy(reader.read_stringz());
        reader.skip(sizeof(Fobb));
        bones[index].bind_local.identity();
    }
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected OGF bone data");
    for (u32 index = 0; index < count; ++index)
    {
        if (parents[index].empty())
            continue;
        const auto found = indices.find(parents[index]);
        if (found == indices.end() || found->second == index)
            throw std::runtime_error("invalid OGF bone parent");
        bones[index].parent = found->second;
    }
    const auto ik = chunks.find(OGF_S_IKDATA);
    if (ik == chunks.end())
        throw std::runtime_error("missing OGF bind transforms");
    BinaryReader transforms{ik->second.data, ik->second.size};
    for (auto& bone : bones)
    {
        const u32 version = transforms.read<u32>();
        transforms.read_stringz();
        transforms.skip(sizeof(SBoneShape));
        transforms.skip(sizeof(u32) + sizeof(SJointLimit) * 3 + 5 * sizeof(u32));
        if (version > 0)
            transforms.skip(sizeof(float));
        const Fvector rotation = transforms.read<Fvector>();
        const Fvector translation = transforms.read<Fvector>();
        if (!std::isfinite(rotation.x) || !std::isfinite(rotation.y) || !std::isfinite(rotation.z) ||
            !std::isfinite(translation.x) || !std::isfinite(translation.y) || !std::isfinite(translation.z))
            throw std::runtime_error("non-finite OGF bind transform");
        bone.bind_local.setXYZi(rotation);
        bone.bind_local.translate_over(translation);
        transforms.skip(sizeof(float) + sizeof(Fvector));
    }
    if (transforms.offset != transforms.size)
        throw std::runtime_error("unexpected OGF bind data");
    const auto refs = chunks.find(OGF_S_MOTION_REFS);
    const auto refs2 = chunks.find(OGF_S_MOTION_REFS2);
    if (refs != chunks.end())
    {
        BinaryReader values{refs->second.data, refs->second.size};
        const auto list = values.read_stringz();
        xr_vector<xr_string> items;
        _SequenceToList(items, list.c_str());
        for (const auto& item : items)
            motionRefs.emplace_back(item.c_str());
    }
    else if (refs2 != chunks.end())
    {
        BinaryReader values{refs2->second.data, refs2->second.size};
        const u32 referenceCount = values.read<u32>();
        if (referenceCount > values.size - values.offset)
            throw std::runtime_error("truncated OGF motion references");
        for (u32 index = 0; index < referenceCount; ++index)
            motionRefs.emplace_back(values.read_stringz().c_str());
    }
    else
        embedded = true;
}
}
