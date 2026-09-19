#include "stdafx.h"
#include "OzzMotionLibrary.h"
#include "StartupConversionInventory.h"
#include "LegacyOmfConverter.h"
#include "LegacyChunkIO.h"
#include "xrCore/LocatorAPI.h"
#include "xrCore/FS.h"

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/span.h>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace XRay::Animation
{
namespace
{
constexpr u32 CacheVersion = 5;
constexpr u32 CacheMagic = 0x435a5a4f;
constexpr size_t MaxCacheBytes = 512u * 1024u * 1024u;
using Bytes = xr_vector<std::byte>;
using Digest = std::array<u8, 32>;

u32 RotateRight(u32 value, unsigned bits)
{
    return (value >> bits) | (value << (32 - bits));
}

void HashBlock(std::array<u32, 8>& state, const u8* bytes)
{
    static constexpr u32 constants[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    u32 words[64];
    for (unsigned index = 0; index < 16; ++index)
        words[index] = (u32(bytes[index * 4]) << 24) | (u32(bytes[index * 4 + 1]) << 16) |
            (u32(bytes[index * 4 + 2]) << 8) | bytes[index * 4 + 3];
    for (unsigned index = 16; index < 64; ++index)
    {
        const u32 a = words[index - 15];
        const u32 b = words[index - 2];
        words[index] = words[index - 16] + (RotateRight(a, 7) ^ RotateRight(a, 18) ^ (a >> 3)) +
            words[index - 7] + (RotateRight(b, 17) ^ RotateRight(b, 19) ^ (b >> 10));
    }
    auto working = state;
    for (unsigned index = 0; index < 64; ++index)
    {
        const u32 a = working[0];
        const u32 e = working[4];
        const u32 first = working[7] + (RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25)) +
            ((e & working[5]) ^ (~e & working[6])) + constants[index] + words[index];
        const u32 second = (RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22)) +
            ((a & working[1]) ^ (a & working[2]) ^ (working[1] & working[2]));
        working = {first + second, working[0], working[1], working[2],
            working[3] + first, working[4], working[5], working[6]};
    }
    for (unsigned index = 0; index < 8; ++index)
        state[index] += working[index];
}

Digest HashBytes(const void* data, size_t size)
{
    std::array<u32, 8> state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    const auto* bytes = static_cast<const u8*>(data);
    const size_t blocks = size / 64;
    for (size_t index = 0; index < blocks; ++index)
        HashBlock(state, bytes + index * 64);
    std::array<u8, 128> tail{};
    const size_t remaining = size % 64;
    if (remaining)
        std::memcpy(tail.data(), bytes + blocks * 64, remaining);
    tail[remaining] = 0x80;
    const size_t padded = remaining < 56 ? 64 : 128;
    const u64 bits = u64(size) * 8;
    for (unsigned index = 0; index < 8; ++index)
        tail[padded - 1 - index] = u8(bits >> (index * 8));
    HashBlock(state, tail.data());
    if (padded == 128)
        HashBlock(state, tail.data() + 64);
    Digest digest;
    for (unsigned index = 0; index < 32; ++index)
        digest[index] = u8(state[index / 4] >> (24 - 8 * (index % 4)));
    return digest;
}

struct Writer
{
    Bytes bytes;
    void append(const void* data, size_t size)
    {
        const auto* begin = static_cast<const std::byte*>(data);
        bytes.insert(bytes.end(), begin, begin + size);
    }
    template <class T> void put(T value) { append(&value, sizeof(value)); }
    void string(pcstr value)
    {
        const size_t size = value ? xr_strlen(value) : 0;
        put(u32(size));
        if (size)
            append(value, size);
    }
};

std::string ReadString(BinaryReader& reader)
{
    const u32 count = reader.read<u32>();
    if (count > 65535)
        throw std::runtime_error("invalid prepared string length");
    reader.require(count);
    std::string value(reinterpret_cast<const char*>(reader.data + reader.offset), count);
    reader.skip(count);
    if (value.find('\0') != std::string::npos)
        throw std::runtime_error("embedded zero in prepared string");
    return value;
}

float ReadFloat(BinaryReader& reader)
{
    const float value = reader.read<float>();
    if (!std::isfinite(value))
        throw std::runtime_error("non-finite prepared value");
    return value;
}

void WritePartition(Writer& writer, const CPartition& partition)
{
    for (u16 index = 0; index < MAX_PARTS; ++index)
    {
        const auto& part = partition.part(index);
        writer.string(part.Name.c_str());
        writer.put(u32(part.bones.size()));
        for (u32 bone : part.bones)
            writer.put(bone);
    }
}

CPartition ReadPartition(BinaryReader& reader, size_t joints)
{
    CPartition result;
    for (u16 index = 0; index < MAX_PARTS; ++index)
    {
        auto& part = result[index];
        const auto name = ReadString(reader);
        if (!name.empty())
            part.Name = name.c_str();
        const u32 count = reader.read<u32>();
        if (count > joints)
            throw std::runtime_error("invalid prepared partition size");
        for (u32 item = 0; item < count; ++item)
        {
            const u32 bone = reader.read<u32>();
            if (bone >= joints || std::find(part.bones.begin(), part.bones.end(), bone) != part.bones.end())
                throw std::runtime_error("invalid prepared partition bone");
            part.bones.push_back(bone);
        }
    }
    return result;
}

void WriteMetadata(Writer& writer, const MotionLibraryMetadata& metadata)
{
    writer.string(metadata.source.c_str());
    WritePartition(writer, metadata.partition);
    writer.put(u32(metadata.clips.size()));
    for (const auto& clip : metadata.clips)
    {
        writer.string(clip.name.c_str());
        writer.put(clip.duration);
        const auto& definition = clip.definition;
        writer.put(definition.bone_or_part);
        writer.put(definition.motion);
        writer.put(definition.speed);
        writer.put(definition.power);
        writer.put(definition.accrue);
        writer.put(definition.falloff);
        writer.put(definition.flags);
        writer.put(u32(definition.marks.size()));
        for (const auto& mark : definition.marks)
        {
            writer.string(mark.name.c_str());
            writer.put(u32(mark.Intervals().size()));
            for (const auto& interval : mark.Intervals())
            {
                writer.put(interval.first);
                writer.put(interval.second);
            }
        }
    }
}

MotionLibraryMetadata ReadMetadata(BinaryReader& reader, size_t joints)
{
    MotionLibraryMetadata metadata;
    metadata.source = ReadString(reader).c_str();
    metadata.partition = ReadPartition(reader, joints);
    const u32 count = reader.read<u32>();
    if (!count || count >= 0x3fff || count > (reader.size - reader.offset) / 26)
        throw std::runtime_error("invalid prepared motion count");
    metadata.clips.resize(count);
    for (u16 index = 0; index < count; ++index)
    {
        auto& clip = metadata.clips[index];
        clip.name = ReadString(reader).c_str();
        clip.duration = ReadFloat(reader);
        auto& definition = clip.definition;
        definition.bone_or_part = reader.read<u16>();
        definition.motion = reader.read<u16>();
        definition.speed = reader.read<u16>();
        definition.power = reader.read<u16>();
        definition.accrue = reader.read<u16>();
        definition.falloff = reader.read<u16>();
        definition.flags = reader.read<u16>();
        if (!(clip.duration > 0.f) || definition.motion >= count ||
            (definition.bone_or_part != BI_NONE && definition.bone_or_part >=
                (definition.flags & esmFX ? joints : MAX_PARTS)))
            throw std::runtime_error("invalid prepared motion control");
        const u32 marks = reader.read<u32>();
        if (marks > (reader.size - reader.offset) / 8)
            throw std::runtime_error("invalid prepared marks count");
        definition.marks.resize(marks);
        for (auto& mark : definition.marks)
        {
            mark.name = ReadString(reader).c_str();
            const u32 count = reader.read<u32>();
            if (count > (reader.size - reader.offset) / 8)
                throw std::runtime_error("invalid prepared interval count");
            xr_vector<motion_marks::interval> intervals;
            intervals.reserve(count);
            for (u32 item = 0; item < count; ++item)
            {
                const float begin = ReadFloat(reader);
                const float end = ReadFloat(reader);
                if (end < begin)
                    throw std::runtime_error("reversed prepared mark interval");
                intervals.emplace_back(begin, end);
            }
            mark.SetIntervals(std::move(intervals));
        }
        if (!metadata.motions.emplace(clip.name, index).second)
            throw std::runtime_error("duplicate prepared motion");
        (definition.flags & esmFX ? metadata.effects : metadata.cycles).emplace(clip.name, index);
    }
    return metadata;
}

std::filesystem::path CachePath(const xr_string& key)
{
    if (key.size() != 64 || key.find_first_not_of("0123456789abcdef") != xr_string::npos)
        throw std::runtime_error("invalid prepared asset key");
    string_path root;
    FS.update_path(root, "$app_data_root$", "ozz_cache");
    std::string native(root);
    std::replace(native.begin(), native.end(), '\\', char(std::filesystem::path::preferred_separator));
    return std::filesystem::path(native) / (std::string(key.c_str()) + ".ozzcache");
}

Bytes ReadCache(const xr_string& key, u32 type)
{
    const auto path = CachePath(key);
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        throw std::runtime_error("cannot open prepared cache " + path.string());
    const std::streamoff size = stream.tellg();
    constexpr size_t headerBytes = 4 * sizeof(u32) + sizeof(Digest);
    if (size < std::streamoff(headerBytes) || size > std::streamoff(MaxCacheBytes))
        throw std::runtime_error("invalid prepared cache size " + path.string());
    stream.seekg(0);
    u32 header[4];
    Digest checksum;
    stream.read(reinterpret_cast<char*>(header), sizeof(header));
    stream.read(reinterpret_cast<char*>(checksum.data()), checksum.size());
    if (!stream || header[0] != CacheMagic || header[1] != CacheVersion || header[2] != type ||
        header[3] != size_t(size) - headerBytes || !header[3])
        throw std::runtime_error("invalid prepared cache header " + path.string());
    Bytes bytes(header[3]);
    stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!stream || checksum != HashBytes(bytes.data(), bytes.size()))
        throw std::runtime_error("corrupt prepared cache " + path.string());
    return bytes;
}

void WriteCache(const xr_string& key, u32 type, const Bytes& bytes)
{
    if (bytes.empty() || bytes.size() > MaxCacheBytes - 4 * sizeof(u32) - sizeof(Digest))
        throw std::runtime_error("prepared asset exceeds cache size limit");
    const auto path = CachePath(key);
    std::filesystem::create_directories(path.parent_path());
    auto temporary = path;
    temporary += ".tmp";
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    const u32 header[] = {CacheMagic, CacheVersion, type, u32(bytes.size())};
    const auto checksum = HashBytes(bytes.data(), bytes.size());
    stream.write(reinterpret_cast<const char*>(header), sizeof(header));
    stream.write(reinterpret_cast<const char*>(checksum.data()), checksum.size());
    stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    stream.flush();
    if (!stream)
        throw std::runtime_error("cannot write prepared cache " + path.string());
    stream.close();
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error)
    {
        std::filesystem::remove(path, error);
        error.clear();
        std::filesystem::rename(temporary, path, error);
    }
    if (error)
        throw std::runtime_error("cannot publish prepared cache " + path.string() + ": " + error.message());
}

void ValidateArchiveHeader(BinaryReader& reader, pcstr tag, u32 version)
{
    if (reader.read<u8>() != u8(ozz::GetNativeEndianness()) || reader.read_stringz() != tag || reader.read<u32>() != version)
        throw std::runtime_error("unsupported Ozz cache archive");
}

void ValidateSkeletonArchive(BinaryReader reader, size_t joints)
{
    ValidateArchiveHeader(reader, "ozz-skeleton", 2);
    if (reader.read<u32>() != joints)
        throw std::runtime_error("prepared skeleton joint count mismatch");
    const u32 namesBytes = reader.read<u32>();
    reader.require(namesBytes);
    BinaryReader names{reader.data + reader.offset, namesBytes};
    for (size_t joint = 0; joint < joints; ++joint)
        if (names.read_stringz().empty())
            throw std::runtime_error("empty prepared joint name");
    if (names.offset != names.size)
        throw std::runtime_error("prepared skeleton name count mismatch");
    reader.skip(namesBytes);
    for (size_t joint = 0; joint < joints; ++joint)
    {
        const s16 parent = reader.read<s16>();
        if (parent < -1 || parent >= int(joint))
            throw std::runtime_error("invalid prepared joint parent");
    }
    const size_t floats = ((joints + 3) / 4) * 40;
    for (size_t value = 0; value < floats; ++value)
        ReadFloat(reader);
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected prepared skeleton bytes");
}

void ValidateAnimationArchive(BinaryReader reader, size_t joints, const MotionMetadata& clip)
{
    ValidateArchiveHeader(reader, "ozz-animation", 7);
    if (ReadFloat(reader) != clip.duration || reader.read<u32>() != joints)
        throw std::runtime_error("prepared animation dimensions mismatch");
    const u32 nameBytes = reader.read<u32>();
    const u32 timeCount = reader.read<u32>();
    const u32 counts[] = {reader.read<u32>(), reader.read<u32>(), reader.read<u32>()};
    for (unsigned index = 0; index < 6; ++index)
        if (reader.read<u32>() != 0)
            throw std::runtime_error("unsupported prepared animation iframes");
    reader.require(nameBytes);
    if (nameBytes != clip.name.size() || std::memcmp(reader.data + reader.offset, clip.name.c_str(), nameBytes))
        throw std::runtime_error("prepared animation name mismatch");
    reader.skip(nameBytes);
    if (timeCount < 2 || timeCount > 65535 || timeCount > (reader.size - reader.offset) / sizeof(float))
        throw std::runtime_error("invalid prepared timepoint count");
    const float minEndRatio = std::nextafter(1.f, 0.f);
    const float maxEndRatio = std::nextafter(1.f, 2.f);
    xr_vector<float> times(timeCount);
    for (u32 index = 0; index < timeCount; ++index)
    {
        times[index] = ReadFloat(reader);
        if ((index && times[index] <= times[index - 1]) || times[index] < 0.f || times[index] > maxEndRatio)
            throw std::runtime_error("invalid prepared timepoint");
    }
    if (times.front() != 0.f || times.back() < minEndRatio)
        throw std::runtime_error("incomplete prepared time range");
    const u32 padded = u32((joints + 3) & ~size_t(3));
    for (unsigned component = 0; component < 3; ++component)
    {
        const u32 count = counts[component];
        const size_t ratioSize = timeCount <= 255 ? 1 : 2;
        if (count < padded * 2 || count > (reader.size - reader.offset) / (ratioSize + 8))
            throw std::runtime_error("invalid prepared key count");
        xr_vector<u16> ratios(count);
        for (auto& ratio : ratios)
        {
            ratio = ratioSize == 1 ? reader.read<u8>() : reader.read<u16>();
            if (ratio >= timeCount)
                throw std::runtime_error("invalid prepared key time");
        }
        xr_vector<u32> owner(count);
        xr_vector<u32> last(padded);
        float previousTime = 0.f;
        for (u32 index = 0; index < count; ++index)
        {
            const u16 distance = reader.read<u16>();
            if (index < padded)
            {
                if (distance || ratios[index])
                    throw std::runtime_error("invalid prepared initial key");
                owner[index] = index;
                last[index] = index;
                continue;
            }
            if (!distance || distance > index)
                throw std::runtime_error("invalid prepared key link");
            const u32 previous = index - distance;
            const u32 track = owner[previous];
            const float time = times[ratios[previous]];
            if (last[track] != previous || ratios[index] <= ratios[previous] || time < previousTime ||
                (index < padded * 2 && previous != index - padded))
                throw std::runtime_error("invalid prepared key chain");
            previousTime = time;
            owner[index] = track;
            last[track] = index;
        }
        for (u32 index : last)
            if (ratios[index] != timeCount - 1)
                throw std::runtime_error("incomplete prepared track");
        ReadFloat(reader);
        for (size_t index = 0; index < size_t(count) * 3; ++index)
        {
            const u16 packed = reader.read<u16>();
            if (component == 2 && (packed & 0x7c00) == 0x7c00)
                throw std::runtime_error("non-finite prepared scale key");
        }
    }
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected prepared animation bytes");
}

template <class T> void WriteArchive(Writer& writer, const T& object)
{
    ozz::io::MemoryStream stream;
    { ozz::io::OArchive archive(&stream); archive << object; }
    const size_t size = stream.Size();
    writer.put(u32(size));
    const size_t offset = writer.bytes.size();
    writer.bytes.resize(offset + size);
    stream.Seek(0, ozz::io::Stream::kSet);
    if (stream.Read(writer.bytes.data() + offset, size) != size)
        throw std::runtime_error("Ozz archive serialization failed");
}

template <class T> void ReadArchive(BinaryReader& reader, T& object, size_t joints, const MotionMetadata* clip = nullptr)
{
    const u32 size = reader.read<u32>();
    reader.require(size);
    BinaryReader body{reader.data + reader.offset, size};
    if (clip)
        ValidateAnimationArchive(body, joints, *clip);
    else
        ValidateSkeletonArchive(body, joints);
    ozz::io::MemoryStream stream;
    if (stream.Write(body.data, body.size) != body.size || stream.Seek(0, ozz::io::Stream::kSet) != 0)
        throw std::runtime_error("Ozz archive input failed");
    ozz::io::IArchive archive(&stream);
    archive >> object;
    if (size_t(stream.Tell()) != body.size)
        throw std::runtime_error("Ozz archive length mismatch");
    reader.skip(size);
}

std::shared_ptr<const OzzSkeletonMirror> LoadSkeleton(const xr_string& key)
{
    const auto bytes = ReadCache(key, 1);
    BinaryReader reader{bytes.data(), bytes.size()};
    auto mirror = std::make_shared<OzzSkeletonMirror>();
    mirror->fingerprint = reader.read<u32>();
    const u32 count = reader.read<u32>();
    if (!count || count > ozz::animation::Skeleton::kMaxJoints)
        throw std::runtime_error("invalid prepared skeleton size");
    mirror->boneToJoint.resize(count);
    mirror->jointToBone.assign(count, BI_NONE);
    for (u16 bone = 0; bone < count; ++bone)
    {
        const u16 joint = reader.read<u16>();
        if (joint >= count || mirror->jointToBone[joint] != BI_NONE)
            throw std::runtime_error("invalid prepared skeleton mapping");
        mirror->boneToJoint[bone] = joint;
        mirror->jointToBone[joint] = bone;
    }
    ReadArchive(reader, mirror->skeleton, count);
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected prepared skeleton payload");
    return mirror;
}

std::shared_ptr<const OzzMotionLibrary> LoadLibrary(const xr_string& key, const xr_string& skeletonKey,
    const OzzSkeletonMirror& skeleton)
{
    const auto bytes = ReadCache(key, 2);
    BinaryReader reader{bytes.data(), bytes.size()};
    if (ReadString(reader) != skeletonKey.c_str())
        throw std::runtime_error("prepared library skeleton mismatch");
    auto library = std::make_shared<OzzMotionLibrary>();
    library->metadata = ReadMetadata(reader, skeleton.boneToJoint.size());
    library->animations.reserve(library->metadata.clips.size());
    library->firstFrame.resize(library->metadata.clips.size());
    ozz::animation::SamplingJob::Context context(skeleton.skeleton.num_joints());
    for (size_t index = 0; index < library->metadata.clips.size(); ++index)
    {
        auto animation = ozz::make_unique<ozz::animation::Animation>();
        ReadArchive(reader, *animation, skeleton.boneToJoint.size(), &library->metadata.clips[index]);
        auto& frame = library->firstFrame[index];
        frame.resize(skeleton.skeleton.num_soa_joints());
        context.Invalidate();
        ozz::animation::SamplingJob job;
        job.animation = animation.get();
        job.context = &context;
        job.ratio = 0.f;
        job.output = ozz::make_span(frame);
        if (!job.Run())
            throw std::runtime_error("cannot sample prepared first frame");
        library->animations.push_back(std::move(animation));
    }
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected prepared library payload");
    return library;
}

struct ModelEntry
{
    xr_string key;
    std::weak_ptr<const OzzModelAnimations> resident;
};
struct LibraryEntry
{
    CPartition partition;
    std::weak_ptr<const OzzMotionLibrary> resident;
};
struct Catalog
{
    bool ready = false;
    bool preparing = false;
    std::unordered_map<xr_string, ModelEntry> models;
    std::unordered_map<xr_string, std::weak_ptr<const OzzSkeletonMirror>> skeletons;
    std::unordered_map<xr_string, LibraryEntry> libraries;
    std::unordered_set<xr_string> validatedModels;
    std::unordered_map<xr_string, xr_string> sources;
};
Catalog catalog;
std::mutex catalogMutex;

xr_string ModelIdentity(const xr_string& source, const xr_string& level)
{
    return source + "|" + level;
}

std::shared_ptr<const OzzModelAnimations> LoadModel(const xr_string& key)
{
    const auto bytes = ReadCache(key, 3);
    BinaryReader reader{bytes.data(), bytes.size()};
    const xr_string skeletonKey(ReadString(reader).c_str());
    auto knownSkeleton = catalog.skeletons.find(skeletonKey);
    if (knownSkeleton == catalog.skeletons.end())
        throw std::runtime_error("unprepared skeleton dependency");
    auto model = std::make_shared<OzzModelAnimations>();
    model->skeleton = knownSkeleton->second.lock();
    if (!model->skeleton)
    {
        model->skeleton = LoadSkeleton(skeletonKey);
        knownSkeleton->second = model->skeleton;
    }
    model->partition = ReadPartition(reader, model->skeleton->boneToJoint.size());
    const u32 count = reader.read<u32>();
    if (!count || count > MAX_ANIM_SLOT)
        throw std::runtime_error("invalid prepared model slot count");
    for (u32 index = 0; index < count; ++index)
    {
        const xr_string libraryKey(ReadString(reader).c_str());
        auto knownLibrary = catalog.libraries.find(libraryKey);
        if (knownLibrary == catalog.libraries.end())
            throw std::runtime_error("unprepared library dependency");
        auto library = knownLibrary->second.resident.lock();
        if (!library)
        {
            library = LoadLibrary(libraryKey, skeletonKey, *model->skeleton);
            knownLibrary->second.resident = library;
        }
        model->libraries.push_back(std::move(library));
    }
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected prepared model payload");
    return model;
}
}

namespace Startup
{
xr_string CanonicalPath(pcstr path)
{
    std::string value = ToLowerCopy(path ? path : "");
    std::replace(value.begin(), value.end(), '/', '\\');
    while (value.size() >= 2 && value.compare(0, 2, ".\\") == 0)
        value.erase(0, 2);
    return value.c_str();
}

xr_string ContentKey(const void* data, size_t size)
{
    if ((!data && size) || size > std::numeric_limits<u32>::max())
        throw std::runtime_error("invalid animation digest input");
    const auto digest = HashBytes(data, size);
    xr_string value;
    value.reserve(digest.size() * 2);
    constexpr char digits[] = "0123456789abcdef";
    for (u8 byte : digest)
    {
        value.push_back(digits[byte >> 4]);
        value.push_back(digits[byte & 15]);
    }
    return value;
}

xr_string PrepareSkeleton(const xr_vector<OzzBoneDesc>& bones, std::shared_ptr<const OzzSkeletonMirror>& mirror)
{
    if (!catalog.preparing)
        throw std::runtime_error("skeleton importer outside startup");
    Writer identity;
    identity.put(CacheVersion);
    identity.put(u32(bones.size()));
    for (const auto& bone : bones)
    {
        identity.string(bone.name.c_str());
        identity.put(bone.parent);
        identity.append(&bone.bind_local, sizeof(bone.bind_local));
    }
    const auto key = ContentKey(identity.bytes.data(), identity.bytes.size());
    try { mirror = LoadSkeleton(key); }
    catch (const std::exception&)
    {
        mirror = BuildOzzSkeletonMirror(ozz::make_span(bones));
        Writer writer;
        writer.put(mirror->fingerprint);
        writer.put(u32(mirror->boneToJoint.size()));
        for (u16 joint : mirror->boneToJoint)
            writer.put(joint);
        WriteArchive(writer, mirror->skeleton);
        WriteCache(key, 1, writer.bytes);
        mirror = LoadSkeleton(key);
    }
    catalog.skeletons.emplace(key, mirror);
    return key;
}

xr_string PrepareLibrary(pcstr source, const void* data, size_t size,
    const xr_string& skeletonKey, const OzzSkeletonMirror& mirror, CPartition& partition)
try
{
    if (!catalog.preparing)
        throw std::runtime_error("motion importer outside startup");
    const xr_string sourceIdentity = CanonicalPath(source) + "|" + skeletonKey;
    const auto prepared = catalog.sources.find(sourceIdentity);
    if (prepared != catalog.sources.end())
    {
        partition = catalog.libraries.at(prepared->second).partition;
        return prepared->second;
    }
    const auto close = [](IReader* reader) { FS.r_close(reader); };
    std::unique_ptr<IReader, decltype(close)> reader(nullptr, close);
    if (!data)
    {
        reader.reset(FS.r_open(source));
        if (!reader)
            throw std::runtime_error("cannot open motion source");
        data = reader->pointer();
        size = reader->length();
    }
    const xr_string identity = sourceIdentity + "|" + ContentKey(data, size);
    const auto key = ContentKey(identity.data(), identity.size());
    const auto known = catalog.libraries.find(key);
    if (known != catalog.libraries.end())
    {
        partition = known->second.partition;
        catalog.sources.emplace(sourceIdentity, key);
        return key;
    }
    std::shared_ptr<const OzzMotionLibrary> library;
    try { library = LoadLibrary(key, skeletonKey, mirror); }
    catch (const std::exception&)
    {
        Msg("* [ozz] Converting motions: %s", source);
        auto converted = ConvertLegacyOmf(static_cast<const std::byte*>(data), size, source, mirror);
        Writer writer;
        writer.string(skeletonKey.c_str());
        WriteMetadata(writer, converted.metadata);
        for (const auto& animation : converted.animations)
            WriteArchive(writer, *animation);
        WriteCache(key, 2, writer.bytes);
        converted.animations.clear();
        library = LoadLibrary(key, skeletonKey, mirror);
    }
    partition = library->metadata.partition;
    catalog.libraries.emplace(key, LibraryEntry{partition, {}});
    catalog.sources.emplace(sourceIdentity, key);
    return key;
}
catch (const std::exception& error)
{
    throw std::runtime_error(std::string(source) + ": " + error.what());
}

void RegisterModel(const xr_string& source, const xr_string& levelRoot, PreparedModel model)
{
    if (!catalog.preparing)
        throw std::runtime_error("model preparation outside startup");
    Writer writer;
    writer.string(model.skeleton.c_str());
    WritePartition(writer, model.partition);
    writer.put(u32(model.libraries.size()));
    for (const auto& key : model.libraries)
        writer.string(key.c_str());
    const auto key = ContentKey(writer.bytes.data(), writer.bytes.size());
    if (catalog.validatedModels.insert(key).second)
    {
        try
        {
            if (ReadCache(key, 3) != writer.bytes)
                throw std::runtime_error("model cache identity mismatch");
        }
        catch (const std::exception&) { WriteCache(key, 3, writer.bytes); }
        LoadModel(key);
    }
    if (!catalog.models.emplace(ModelIdentity(CanonicalPath(source.c_str()), CanonicalPath(levelRoot.c_str())),
        ModelEntry{key, {}}).second)
        throw std::runtime_error("duplicate prepared model identity: " + std::string(source.c_str()));
}
}

void PrepareOzzAnimationInventory()
{
    std::lock_guard<std::mutex> guard(catalogMutex);
    if (catalog.ready)
        return;
    catalog = {};
    catalog.preparing = true;
    try
    {
        Msg("* [ozz] Preparing skeletons and motions from mesh inventory");
        Startup::BuildInventory();
        for (auto& entry : catalog.libraries)
            entry.second.partition = CPartition{};
        catalog.validatedModels.clear();
        catalog.validatedModels.rehash(0);
        catalog.sources.clear();
        catalog.sources.rehash(0);
        catalog.preparing = false;
        catalog.ready = true;
        Msg("* [ozz] Prepared %zu model contexts, %zu skeletons, %zu motion libraries",
            catalog.models.size(), catalog.skeletons.size(), catalog.libraries.size());
    }
    catch (const std::exception& error)
    {
        catalog = {};
        xrDebug::Fatal(DEBUG_INFO, "Ozz startup preparation failed: %s", error.what());
        throw;
    }
}

std::shared_ptr<const OzzModelAnimations> LoadOzzModelAnimations(pcstr modelName)
{
    std::lock_guard<std::mutex> guard(catalogMutex);
    try
    {
        if (!catalog.ready)
            throw std::runtime_error("animation inventory is not ready");
        if (!modelName || !*modelName)
            throw std::runtime_error("model has no prepared identity");
        string_path levelPath;
        FS.update_path(levelPath, "$level$", "");
        const auto level = Startup::CanonicalPath(levelPath);
        xr_string name = Startup::CanonicalPath(modelName);
        const auto child = name.find(':', name.size() > 1 && name[1] == ':' ? 2 : 0);
        const bool virtualName = name.compare(0, 13, "@level_visual") == 0;
        xr_string suffix;
        if (!virtualName && child != xr_string::npos)
        {
            suffix = name.substr(child);
            name.resize(child);
        }
        if (!virtualName && (name.size() < 4 || name.compare(name.size() - 4, 4, ".ogf") != 0))
            name += ".ogf";
        xr_vector<xr_string> candidates;
        candidates.push_back(name + suffix);
        candidates.push_back(level + name + suffix);
        string_path meshPath;
        FS.update_path(meshPath, "$game_meshes$", "");
        candidates.push_back(Startup::CanonicalPath(meshPath) + name + suffix);
        for (const auto& source : candidates)
        {
            auto found = catalog.models.find(ModelIdentity(source, level));
            if (found == catalog.models.end())
                found = catalog.models.find(ModelIdentity(source, ""));
            if (found == catalog.models.end())
                continue;
            auto model = found->second.resident.lock();
            if (!model)
            {
                model = LoadModel(found->second.key);
                found->second.resident = model;
            }
            return model;
        }
        throw std::runtime_error("model absent from prepared inventory");
    }
    catch (const std::exception& error)
    {
        xrDebug::Fatal(DEBUG_INFO, "Cannot load prepared Ozz model '%s': %s", modelName ? modelName : "<null>", error.what());
        throw;
    }
}

void ShutdownOzzAnimations()
{
    std::lock_guard<std::mutex> guard(catalogMutex);
    catalog = {};
}

void DumpOzzAnimationStats()
{
    std::lock_guard<std::mutex> guard(catalogMutex);
    size_t resident = 0;
    size_t bytes = 0;
    for (const auto& entry : catalog.libraries)
        if (const auto library = entry.second.resident.lock())
        {
            ++resident;
            for (const auto& animation : library->animations)
                bytes += animation->size();
            for (const auto& frame : library->firstFrame)
                bytes += frame.size() * sizeof(ozz::math::SoaTransform);
        }
    Msg("* [ozz] %zu prepared models, %zu prepared libraries, %zu resident libraries, %zu pose/animation bytes",
        catalog.models.size(), catalog.libraries.size(), resident, bytes);
}
}
