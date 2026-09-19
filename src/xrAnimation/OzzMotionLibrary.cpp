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
#include <cctype>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace XRay::Animation
{
namespace
{
constexpr u32 LibraryVersion = 1;
constexpr u32 LibraryMagic = 0x4c5a5a4f;
constexpr size_t MaxLibraryBytes = 512u * 1024u * 1024u;
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

std::filesystem::path NativePath(pcstr path)
{
    std::string native(path);
    std::replace(native.begin(), native.end(), '\\', char(std::filesystem::path::preferred_separator));
    return native;
}

xr_string OutputPath(pcstr source)
{
    xr_string path(source);
    const auto canonical = Startup::CanonicalPath(source);
    if (canonical.size() >= 4 && canonical.compare(canonical.size() - 4, 4, ".omf") == 0)
        path.replace(path.size() - 4, 4, ".ozz");
    else if (canonical.size() < 4 || canonical.compare(canonical.size() - 4, 4, ".ozz") != 0)
        path += ".ozz";
    for (size_t index = 0; index < path.size(); ++index)
        if (path[index] == ':' && !(index == 1 && std::isalpha(static_cast<unsigned char>(path[0]))))
            path[index] = '.';
    return path;
}

struct SourceStamp
{
    u32 size = 0;
    u32 modified = 0;
    u32 crc = 0;
    u64 nativeTime = 0;

    bool operator==(const SourceStamp& other) const
    {
        return size == other.size && modified == other.modified && crc == other.crc && nativeTime == other.nativeTime;
    }
};

bool ReadSourceStamp(pcstr source, SourceStamp& stamp)
{
    bool found = false;
    if (const auto* file = FS.GetFileDesc(source))
    {
        stamp.size = file->size_real;
        stamp.modified = file->modif;
        stamp.crc = file->crc;
        found = true;
    }
    const auto path = NativePath(source);
    std::error_code error;
    if (std::filesystem::is_regular_file(path, error))
    {
        const auto size = std::filesystem::file_size(path);
        if (size > MaxLibraryBytes)
            throw std::runtime_error("motion source exceeds size limit");
        stamp.size = u32(size);
        stamp.nativeTime = u64(std::filesystem::last_write_time(path).time_since_epoch().count());
        found = true;
    }
    return found;
}

Bytes ReadLibraryBytes(pcstr source)
{
    const auto canonical = Startup::CanonicalPath(source);
    if (canonical.size() < 4 || canonical.compare(canonical.size() - 4, 4, ".ozz") != 0)
        throw std::runtime_error("runtime motion input must be an .ozz library");
    const auto path = NativePath(source);
    std::error_code error;
    if (std::filesystem::is_regular_file(path, error))
    {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        const auto size = stream.tellg();
        if (!stream || size <= 0 || size > std::streamoff(MaxLibraryBytes))
            throw std::runtime_error("invalid Ozz library size " + path.string());
        Bytes bytes(static_cast<size_t>(size));
        stream.seekg(0);
        stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        if (!stream)
            throw std::runtime_error("cannot read Ozz library " + path.string());
        return bytes;
    }
    const auto close = [](IReader* reader) { FS.r_close(reader); };
    std::unique_ptr<IReader, decltype(close)> reader(FS.r_open(source), close);
    if (!reader || !reader->length() || size_t(reader->length()) > MaxLibraryBytes)
        throw std::runtime_error("cannot read Ozz library " + path.string());
    const auto* begin = static_cast<const std::byte*>(reader->pointer());
    return Bytes(begin, begin + reader->length());
}

void ValidateArchiveHeader(BinaryReader& reader, pcstr tag, u32 version)
{
    if (reader.read<u8>() != u8(ozz::GetNativeEndianness()) || reader.read_stringz() != tag || reader.read<u32>() != version)
        throw std::runtime_error("unsupported Ozz animation archive");
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

void WriteArchive(Writer& writer, const ozz::animation::Animation& object)
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

void ReadArchive(BinaryReader& reader, ozz::animation::Animation& object, size_t joints, const MotionMetadata& clip)
{
    const u32 size = reader.read<u32>();
    reader.require(size);
    BinaryReader body{reader.data + reader.offset, size};
    ValidateAnimationArchive(body, joints, clip);
    ozz::io::MemoryStream stream;
    if (stream.Write(body.data, body.size) != body.size || stream.Seek(0, ozz::io::Stream::kSet) != 0)
        throw std::runtime_error("Ozz archive input failed");
    ozz::io::IArchive archive(&stream);
    archive >> object;
    if (size_t(stream.Tell()) != body.size)
        throw std::runtime_error("Ozz archive length mismatch");
    reader.skip(size);
}

struct LibraryFile
{
    std::shared_ptr<const OzzMotionLibrary> library;
    Digest checksum{};
};

LibraryFile LoadLibrary(pcstr path, const SourceStamp* expected = nullptr)
{
    const auto bytes = ReadLibraryBytes(path);
    BinaryReader reader{bytes.data(), bytes.size()};
    if (reader.read<u32>() != LibraryMagic || reader.read<u32>() != LibraryVersion)
        throw std::runtime_error("unsupported Ozz library format");
    SourceStamp source;
    source.size = reader.read<u32>();
    source.modified = reader.read<u32>();
    source.crc = reader.read<u32>();
    source.nativeTime = reader.read<u64>();
    if (expected && !(source == *expected))
        throw std::runtime_error("stale Ozz library");
    const auto size = reader.read<u32>();
    LibraryFile result;
    for (auto& byte : result.checksum)
        byte = reader.read<u8>();
    if (!size || size != reader.size - reader.offset ||
        result.checksum != HashBytes(reader.data + reader.offset, size))
        throw std::runtime_error("corrupt Ozz library payload");
    const u32 tracks = reader.read<u32>();
    if (!tracks || tracks > ozz::animation::Skeleton::kMaxJoints)
        throw std::runtime_error("invalid Ozz library track count");
    auto library = std::make_shared<OzzMotionLibrary>();
    library->boneNames.reserve(tracks);
    std::unordered_set<std::string> names;
    for (u32 track = 0; track < tracks; ++track)
    {
        const auto name = ReadString(reader);
        if (name.empty() || !names.emplace(name).second)
            throw std::runtime_error("invalid Ozz library bone name");
        library->boneNames.emplace_back(name.c_str());
    }
    library->metadata = ReadMetadata(reader, tracks);
    library->metadata.source = path;
    library->animations.reserve(library->metadata.clips.size());
    library->firstFrame.resize(library->metadata.clips.size());
    ozz::animation::SamplingJob::Context context(tracks);
    for (size_t index = 0; index < library->metadata.clips.size(); ++index)
    {
        auto animation = ozz::make_unique<ozz::animation::Animation>();
        ReadArchive(reader, *animation, tracks, library->metadata.clips[index]);
        auto& frame = library->firstFrame[index];
        frame.resize((tracks + 3) / 4);
        context.Invalidate();
        ozz::animation::SamplingJob job;
        job.animation = animation.get();
        job.context = &context;
        job.ratio = 0.f;
        job.output = ozz::make_span(frame);
        if (!job.Run())
            throw std::runtime_error("cannot sample Ozz library first frame");
        library->animations.push_back(std::move(animation));
    }
    if (reader.offset != reader.size)
        throw std::runtime_error("unexpected Ozz library payload");
    result.library = std::move(library);
    return result;
}

void WriteLibrary(pcstr destination, const SourceStamp& source, const ConvertedOmfLibrary& library)
{
    Writer writer;
    writer.put(u32(library.boneNames.size()));
    for (const auto& name : library.boneNames)
        writer.string(name.c_str());
    WriteMetadata(writer, library.metadata);
    for (const auto& animation : library.animations)
        WriteArchive(writer, *animation);
    if (writer.bytes.empty() || writer.bytes.size() > MaxLibraryBytes - 64)
        throw std::runtime_error("Ozz library exceeds size limit");
    Writer header;
    header.put(LibraryMagic);
    header.put(LibraryVersion);
    header.put(source.size);
    header.put(source.modified);
    header.put(source.crc);
    header.put(source.nativeTime);
    header.put(u32(writer.bytes.size()));
    const auto checksum = HashBytes(writer.bytes.data(), writer.bytes.size());
    header.append(checksum.data(), checksum.size());
    const auto path = NativePath(destination);
    std::filesystem::create_directories(path.parent_path());
    auto temporary = path;
    temporary += ".tmp";
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(header.bytes.data()), header.bytes.size());
    stream.write(reinterpret_cast<const char*>(writer.bytes.data()), writer.bytes.size());
    stream.flush();
    if (!stream)
        throw std::runtime_error("cannot write Ozz library " + path.string());
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
        throw std::runtime_error("cannot publish Ozz library " + path.string() + ": " + error.message());
}

xr_vector<u16> TrackBones(const xr_vector<shared_str>& names, const OzzSkeletonMirror& skeleton)
{
    if (names.size() != skeleton.boneToJoint.size())
        throw std::runtime_error("Ozz library and model bone counts differ");
    std::unordered_map<std::string_view, u16> bones;
    for (u16 bone = 0; bone < skeleton.boneToJoint.size(); ++bone)
        bones.emplace(skeleton.skeleton.joint_names()[skeleton.boneToJoint[bone]], bone);
    xr_vector<u16> mapping;
    mapping.reserve(names.size());
    for (const auto& name : names)
    {
        const auto found = bones.find(name.c_str());
        if (found == bones.end())
            throw std::runtime_error("Ozz library bone absent from model: " + std::string(name.c_str()));
        mapping.push_back(found->second);
    }
    return mapping;
}

void BindPartition(CPartition& partition, const xr_vector<u16>& bones)
{
    for (u16 part = 0; part < MAX_PARTS; ++part)
        for (auto& bone : partition[part].bones)
            bone = bones[bone];
}

OzzMotionBinding BindLibrary(const std::shared_ptr<const OzzMotionLibrary>& library, const OzzSkeletonMirror& skeleton)
{
    const auto bones = TrackBones(library->boneNames, skeleton);
    OzzMotionBinding binding;
    bool sameBones = true;
    for (u16 track = 0; track < bones.size(); ++track)
    {
        sameBones = sameBones && bones[track] == track;
        const auto joint = skeleton.boneToJoint[bones[track]];
        if (joint != track && binding.jointToTrack.empty())
        {
            binding.jointToTrack.resize(bones.size());
            for (u16 index = 0; index < bones.size(); ++index)
                binding.jointToTrack[index] = index;
        }
        if (!binding.jointToTrack.empty())
            binding.jointToTrack[joint] = track;
    }
    if (sameBones)
        binding.metadata = std::shared_ptr<const MotionLibraryMetadata>(library, &library->metadata);
    else
    {
        auto metadata = std::make_shared<MotionLibraryMetadata>(library->metadata);
        BindPartition(metadata->partition, bones);
        for (auto& clip : metadata->clips)
            if ((clip.definition.flags & esmFX) && clip.definition.bone_or_part != BI_NONE)
                clip.definition.bone_or_part = bones[clip.definition.bone_or_part];
        binding.metadata = std::move(metadata);
    }
    return binding;
}

struct ModelEntry
{
    Startup::PreparedModel prepared;
    std::weak_ptr<const OzzModelAnimations> resident;
};
struct LibraryEntry
{
    xr_string path;
    xr_vector<shared_str> boneNames;
    CPartition partition;
    Digest checksum;
    std::weak_ptr<const OzzMotionLibrary> resident;
};
struct Catalog
{
    bool ready = false;
    bool preparing = false;
    std::unordered_map<xr_string, ModelEntry> models;
    std::map<Digest, std::weak_ptr<const OzzSkeletonMirror>> skeletons;
    std::unordered_map<xr_string, LibraryEntry> libraries;
};
Catalog catalog;
std::mutex catalogMutex;

xr_string ModelIdentity(const xr_string& source, const xr_string& level)
{
    return source + "|" + level;
}

std::shared_ptr<const OzzModelAnimations> LoadModel(const Startup::PreparedModel& prepared)
{
    auto model = std::make_shared<OzzModelAnimations>();
    model->skeleton = prepared.skeleton;
    model->partition = prepared.partition;
    model->libraries.reserve(prepared.libraries.size());
    model->bindings.reserve(prepared.libraries.size());
    for (const auto& path : prepared.libraries)
    {
        auto known = catalog.libraries.find(Startup::CanonicalPath(path.c_str()));
        if (known == catalog.libraries.end())
            throw std::runtime_error("unprepared Ozz library dependency");
        auto library = known->second.resident.lock();
        if (!library)
        {
            const auto file = LoadLibrary(path.c_str());
            if (file.checksum != known->second.checksum)
                throw std::runtime_error("Ozz library changed after startup: " + std::string(path.c_str()));
            library = file.library;
            known->second.resident = library;
        }
        model->bindings.push_back(BindLibrary(library, *model->skeleton));
        model->libraries.push_back(std::move(library));
    }
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


bool LibraryExists(const xr_string& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(NativePath(path.c_str()), error) || FS.exist(path.c_str());
}

std::shared_ptr<const OzzSkeletonMirror> PrepareSkeleton(const xr_vector<OzzBoneDesc>& bones)
{
    if (!catalog.preparing)
        throw std::runtime_error("skeleton importer outside startup");
    Writer identity;
    identity.put(u32(bones.size()));
    for (const auto& bone : bones)
    {
        identity.string(bone.name.c_str());
        identity.put(bone.parent);
        identity.append(&bone.bind_local, sizeof(bone.bind_local));
    }
    const auto key = HashBytes(identity.bytes.data(), identity.bytes.size());
    auto& known = catalog.skeletons[key];
    auto mirror = known.lock();
    if (!mirror)
    {
        mirror = BuildOzzSkeletonMirror(ozz::make_span(bones));
        known = mirror;
    }
    return mirror;
}

xr_string PrepareLibrary(pcstr source, const void* data, size_t size)
try
{
    if (!catalog.preparing)
        throw std::runtime_error("motion importer outside startup");
    const auto path = OutputPath(source);
    const auto key = CanonicalPath(path.c_str());
    const auto prepared = catalog.libraries.find(key);
    if (prepared != catalog.libraries.end())
        return prepared->second.path;
    SourceStamp stamp;
    xr_string original(source);
    bool hasSource;
    if (data)
    {
        if (!size || size > MaxLibraryBytes)
            throw std::runtime_error("invalid embedded motion source size");
        stamp.size = u32(size);
        const auto digest = HashBytes(data, size);
        std::memcpy(&stamp.crc, digest.data(), sizeof(stamp.crc));
        std::memcpy(&stamp.nativeTime, digest.data() + sizeof(stamp.crc), sizeof(stamp.nativeTime));
        hasSource = true;
    }
    else
    {
        original = path;
        original.replace(original.size() - 4, 4, ".omf");
        hasSource = ReadSourceStamp(original.c_str(), stamp);
    }
    LibraryFile file;
    try { file = LoadLibrary(path.c_str(), hasSource ? &stamp : nullptr); }
    catch (const std::exception&)
    {
        if (!hasSource)
            throw;
        const auto close = [](IReader* reader) { FS.r_close(reader); };
        std::unique_ptr<IReader, decltype(close)> reader(nullptr, close);
        if (!data)
        {
            reader.reset(FS.r_open(original.c_str()));
            if (!reader)
                throw std::runtime_error("cannot open startup motion source");
            data = reader->pointer();
            size = reader->length();
        }
        Msg("* [ozz] Converting motions: %s -> %s", source, path.c_str());
        {
            const auto converted = ConvertLegacyOmf(static_cast<const std::byte*>(data), size, path.c_str());
            WriteLibrary(path.c_str(), stamp, converted);
        }
        file = LoadLibrary(path.c_str(), &stamp);
    }
    catalog.libraries.emplace(key,
        LibraryEntry{path, file.library->boneNames, file.library->metadata.partition, file.checksum, file.library});
    return path;
}
catch (const std::exception& error)
{
    throw std::runtime_error(std::string(source) + ": " + error.what());
}

CPartition LibraryPartition(const xr_string& path, const OzzSkeletonMirror& skeleton)
{
    if (!catalog.preparing)
        throw std::runtime_error("partition preparation outside startup");
    const auto& library = catalog.libraries.at(CanonicalPath(path.c_str()));
    auto partition = library.partition;
    BindPartition(partition, TrackBones(library.boneNames, skeleton));
    return partition;
}

void RegisterModel(const xr_string& source, const xr_string& levelRoot, PreparedModel model)
{
    if (!catalog.preparing)
        throw std::runtime_error("model preparation outside startup");
    if (!model.skeleton || model.libraries.empty() || model.libraries.size() > MAX_ANIM_SLOT)
        throw std::runtime_error("invalid prepared model");
    for (const auto& path : model.libraries)
        TrackBones(catalog.libraries.at(CanonicalPath(path.c_str())).boneNames, *model.skeleton);
    if (!catalog.models.emplace(ModelIdentity(CanonicalPath(source.c_str()), CanonicalPath(levelRoot.c_str())),
        ModelEntry{std::move(model), {}}).second)
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
                model = LoadModel(found->second.prepared);
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
