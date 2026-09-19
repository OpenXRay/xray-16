#pragma once

#include "xrCore/FS.h"
#include "xrCore/lzhuf.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace XRay::Animation
{
struct Chunk
{
    const std::byte* data = nullptr;
    size_t size = 0;
};

struct BinaryReader
{
    const std::byte* data = nullptr;
    size_t size = 0;
    size_t offset = 0;

    void require(size_t count) const
    {
        if (offset > size || count > size - offset)
            throw std::runtime_error("truncated animation data");
    }

    template <class T> T read()
    {
        require(sizeof(T));
        T value;
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    void skip(size_t count)
    {
        require(count);
        offset += count;
    }

    std::string read_stringz()
    {
        require(1);
        const auto* begin = reinterpret_cast<const char*>(data + offset);
        const auto* end = static_cast<const char*>(std::memchr(begin, 0, size - offset));
        if (!end)
            throw std::runtime_error("unterminated animation string");
        offset += size_t(end - begin) + 1;
        return std::string(begin, end);
    }
};

struct ChunkStorage
{
    std::vector<std::vector<std::byte>> blocks;

    Chunk adopt(u32 id, const std::byte* data, size_t size)
    {
        if (!(id & CFS_CompressMark))
            return {data, size};
        if (size < sizeof(u32))
            throw std::runtime_error("truncated compressed animation chunk");
        u32 expanded;
        std::memcpy(&expanded, data, sizeof(expanded));
        if (!expanded || expanded > 512u * 1024u * 1024u)
            throw std::runtime_error("invalid compressed animation chunk size");
        u8* destination = nullptr;
        size_t destinationSize = 0;
        if (!_decompressLZ(&destination, &destinationSize, const_cast<std::byte*>(data), size))
            throw std::runtime_error("animation chunk decompression failed");
        blocks.emplace_back(reinterpret_cast<const std::byte*>(destination),
            reinterpret_cast<const std::byte*>(destination) + destinationSize);
        xr_free(destination);
        return {blocks.back().data(), blocks.back().size()};
    }
};

inline std::unordered_map<u32, Chunk> ParseChunks(const std::byte* data, size_t size, ChunkStorage& storage)
{
    std::unordered_map<u32, Chunk> chunks;
    BinaryReader reader{data, size};
    while (reader.offset < reader.size)
    {
        const u32 id = reader.read<u32>();
        const u32 bytes = reader.read<u32>();
        reader.require(bytes);
        if (!chunks.emplace(id & ~CFS_CompressMark, storage.adopt(id, data + reader.offset, bytes)).second)
            throw std::runtime_error("duplicate animation chunk");
        reader.skip(bytes);
    }
    return chunks;
}

inline std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char value) { return char(std::tolower(value)); });
    return value;
}

inline std::string ReadStringCRLF(BinaryReader& reader)
{
    const size_t begin = reader.offset;
    while (reader.offset < reader.size)
    {
        const char value = reader.read<char>();
        if (value == '\r' || value == '\n')
        {
            const size_t end = reader.offset - 1;
            if (value == '\r' && reader.offset < reader.size && reader.data[reader.offset] == std::byte{'\n'})
                reader.skip(1);
            return std::string(reinterpret_cast<const char*>(reader.data + begin), end - begin);
        }
    }
    throw std::runtime_error("unterminated animation mark name");
}
}
