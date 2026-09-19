#pragma once

#include "xrCore/_vector3d.h"
#include "xrCore/FS.h"
#include "xrCore/lzhuf.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace XRay::Animation {

struct Chunk
{
    const std::byte* data = nullptr;
    std::size_t size = 0;
};

struct BinaryReader
{
    const std::byte* data = nullptr;
    std::size_t size = 0;
    std::size_t offset = 0;

    template <class T>
    T read()
    {
        if (offset + sizeof(T) > size)
            throw std::runtime_error("unexpected end of chunk while reading typed data");

        T value;
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    template <class T>
    T read_struct()
    {
        return read<T>();
    }

    std::int8_t read_int8()
    {
        if (offset + sizeof(std::int8_t) > size)
            throw std::runtime_error("unexpected end of chunk while reading int8");
        const std::int8_t value = *reinterpret_cast<const std::int8_t*>(data + offset);
        offset += sizeof(std::int8_t);
        return value;
    }

    std::uint8_t read_uint8()
    {
        if (offset + sizeof(std::uint8_t) > size)
            throw std::runtime_error("unexpected end of chunk while reading uint8");
        const std::uint8_t value = *reinterpret_cast<const std::uint8_t*>(data + offset);
        offset += sizeof(std::uint8_t);
        return value;
    }

    std::string read_stringz()
    {
        const auto* begin = data + offset;
        const auto* end = data + size;
        const auto* cursor = begin;
        while (cursor < end && *reinterpret_cast<const char*>(cursor) != '\0')
            ++cursor;

        if (cursor == end)
            throw std::runtime_error("unterminated string in chunk");

        std::string value(reinterpret_cast<const char*>(begin),
                          static_cast<std::size_t>(cursor - begin));
        offset += static_cast<std::size_t>(cursor - begin) + 1;
        return value;
    }

    Fvector read_fvector3()
    {
        Fvector v{};
        v.x = read<float>();
        v.y = read<float>();
        v.z = read<float>();
        return v;
    }

    void skip(std::size_t count)
    {
        if (offset + count > size)
            throw std::runtime_error("attempted to skip past end of chunk");
        offset += count;
    }
};

inline std::vector<std::byte> LoadFileBytes(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        throw std::runtime_error("failed to open file: " + path.string());

    const auto size = stream.tellg();
    if (size <= 0)
        throw std::runtime_error("input file is empty: " + path.string());

    std::vector<std::byte> data(static_cast<std::size_t>(size));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(data.size()));
    if (!stream)
        throw std::runtime_error("failed to read file: " + path.string());

    return data;
}

struct ChunkStorage
{
    std::vector<std::vector<std::byte>> blocks;

    Chunk adopt(std::uint32_t id, const std::byte* data, std::size_t size)
    {
        if ((id & CFS_CompressMark) == 0)
            return Chunk{ data, size };

        std::uint8_t* dest = nullptr;
        std::size_t dest_size = 0;
        if (!_decompressLZ(&dest, &dest_size, const_cast<std::byte*>(data), size))
            throw std::runtime_error("failed to decompress chunk");

        blocks.emplace_back(reinterpret_cast<const std::byte*>(dest), reinterpret_cast<const std::byte*>(dest) + dest_size);
        xr_free(dest);
        return Chunk{ blocks.back().data(), blocks.back().size() };
    }
};

inline std::unordered_map<std::uint32_t, Chunk> ParseChunks(
    const std::byte* data, std::size_t size, ChunkStorage& storage)
{
    std::unordered_map<std::uint32_t, Chunk> chunks;
    std::size_t offset = 0;
    while (offset + sizeof(std::uint32_t) * 2 <= size)
    {
        std::uint32_t id = 0;
        std::uint32_t chunk_size = 0;
        std::memcpy(&id, data + offset, sizeof(std::uint32_t));
        offset += sizeof(std::uint32_t);
        std::memcpy(&chunk_size, data + offset, sizeof(std::uint32_t));
        offset += sizeof(std::uint32_t);
        if (offset + chunk_size > size)
            throw std::runtime_error("chunk extends past end of file");
        chunks.emplace(id & ~CFS_CompressMark, storage.adopt(id, data + offset, chunk_size));
        offset += chunk_size;
    }
    return chunks;
}

inline std::vector<std::pair<std::uint32_t, Chunk>> ParseSubchunks(const Chunk& chunk, ChunkStorage& storage)
{
    std::vector<std::pair<std::uint32_t, Chunk>> subchunks;
    std::size_t offset = 0;
    while (offset + sizeof(std::uint32_t) * 2 <= chunk.size)
    {
        std::uint32_t id = 0;
        std::uint32_t sub_size = 0;
        std::memcpy(&id, chunk.data + offset, sizeof(std::uint32_t));
        offset += sizeof(std::uint32_t);
        std::memcpy(&sub_size, chunk.data + offset, sizeof(std::uint32_t));
        offset += sizeof(std::uint32_t);
        if (offset + sub_size > chunk.size)
            throw std::runtime_error("sub-chunk extends past parent chunk");
        subchunks.emplace_back(id & ~CFS_CompressMark, storage.adopt(id, chunk.data + offset, sub_size));
        offset += sub_size;
    }
    return subchunks;
}

inline std::unordered_map<std::uint32_t, Chunk> ParseChunks(const std::byte* data, std::size_t size)
{
    ChunkStorage storage;
    auto chunks = ParseChunks(data, size, storage);
    if (!storage.blocks.empty())
        throw std::runtime_error("compressed chunk requires chunk storage");
    return chunks;
}

inline std::vector<std::pair<std::uint32_t, Chunk>> ParseSubchunks(const Chunk& chunk)
{
    ChunkStorage storage;
    auto subchunks = ParseSubchunks(chunk, storage);
    if (!storage.blocks.empty())
        throw std::runtime_error("compressed sub-chunk requires chunk storage");
    return subchunks;
}

inline std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

inline std::string ReadStringCRLF(BinaryReader& reader)
{
    std::string value;
    while (reader.offset < reader.size)
    {
        const char ch = static_cast<char>(reader.read_int8());
        if (ch == '\r')
        {
            if (reader.offset < reader.size)
            {
                const char lf = static_cast<char>(reader.read_int8());
                if (lf != '\n')
                    --reader.offset;
            }
            return value;
        }
        if (ch == '\n')
            return value;
        value.push_back(ch);
    }
    return value;
}

}
