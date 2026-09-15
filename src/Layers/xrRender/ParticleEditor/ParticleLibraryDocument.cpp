#include "ParticleLibraryDocument.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace xray::render::fg
{
namespace
{
constexpr uint32_t Sprite = 1u << 0;
constexpr uint32_t Framed = 1u << 10;
constexpr uint32_t TimeLimit = 1u << 14;
constexpr uint32_t Collision = 1u << 16;
constexpr uint32_t VelocityScale = 1u << 18;

struct Chunk
{
    uint32_t id;
    size_t offset;
    size_t size;
};

class Reader
{
public:
    Reader(const std::vector<uint8_t>& bytes, size_t offset, size_t size)
        : m_bytes(bytes), m_pos(offset), m_end(offset)
    {
        if (offset > bytes.size() || size > bytes.size() - offset)
            throw std::runtime_error("Chunk extends beyond the archive");
        m_end += size;
    }

    size_t Position() const { return m_pos; }
    size_t Remaining() const { return m_end - m_pos; }

    void Skip(size_t size)
    {
        Require(size);
        m_pos += size;
    }

    uint16_t U16()
    {
        Require(2);
        const uint16_t value = uint16_t(m_bytes[m_pos]) | (uint16_t(m_bytes[m_pos + 1]) << 8);
        m_pos += 2;
        return value;
    }

    uint32_t U32()
    {
        Require(4);
        uint32_t value = 0;
        for (unsigned i = 0; i != 4; ++i)
            value |= uint32_t(m_bytes[m_pos + i]) << (i * 8);
        m_pos += 4;
        return value;
    }

    float Float()
    {
        const uint32_t bits = U32();
        float value;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        if (!std::isfinite(value))
            throw std::runtime_error("Non-finite particle parameter at byte " + std::to_string(m_pos - 4));
        return value;
    }

    std::string String()
    {
        const size_t start = m_pos;
        while (m_pos < m_end && m_bytes[m_pos] != 0)
            ++m_pos;
        if (m_pos == m_end)
            throw std::runtime_error("Unterminated string at byte " + std::to_string(start));
        constexpr size_t maximumLength = 4096 - 3 * sizeof(uint32_t) - sizeof(void*) - 2;
        if (m_pos - start > maximumLength)
            throw std::runtime_error("String exceeds the engine shared-string limit at byte " + std::to_string(start));
        std::string value(reinterpret_cast<const char*>(m_bytes.data() + start), m_pos - start);
        ++m_pos;
        return value;
    }

private:
    void Require(size_t size) const
    {
        if (size > Remaining())
            throw std::runtime_error("Truncated particle data at byte " + std::to_string(m_pos));
    }

    const std::vector<uint8_t>& m_bytes;
    size_t m_pos;
    size_t m_end;
};

std::vector<Chunk> ReadChunks(const std::vector<uint8_t>& bytes, size_t offset, size_t size)
{
    Reader reader(bytes, offset, size);
    std::vector<Chunk> chunks;
    std::unordered_set<uint32_t> ids;
    while (reader.Remaining())
    {
        const uint32_t id = reader.U32();
        const uint32_t length = reader.U32();
        if (id & 0x80000000u)
            throw std::runtime_error("Compressed chunk at byte " + std::to_string(reader.Position() - 8) + " is unsupported");
        if (!ids.insert(id).second)
            throw std::runtime_error("Duplicate chunk " + std::to_string(id) + " at byte " + std::to_string(reader.Position() - 8));
        chunks.push_back({id, reader.Position(), length});
        reader.Skip(length);
    }
    return chunks;
}

const Chunk* Find(const std::vector<Chunk>& chunks, uint32_t id)
{
    const auto found = std::find_if(chunks.begin(), chunks.end(), [id](const Chunk& chunk) { return chunk.id == id; });
    return found == chunks.end() ? nullptr : &*found;
}

const Chunk& Required(const std::vector<Chunk>& chunks, uint32_t id, size_t minimum, const char* name)
{
    const Chunk* chunk = Find(chunks, id);
    if (!chunk)
        throw std::runtime_error(std::string("Missing ") + name + " chunk (" + std::to_string(id) + ")");
    if (chunk->size < minimum)
        throw std::runtime_error(std::string("Truncated ") + name + " chunk: requires " + std::to_string(minimum) + " bytes");
    return *chunk;
}

Reader ReadChunk(const std::vector<uint8_t>& bytes, const Chunk& chunk)
{
    return Reader(bytes, chunk.offset, chunk.size);
}

void WriteU32(std::vector<uint8_t>& bytes, size_t offset, uint32_t value)
{
    for (unsigned i = 0; i != 4; ++i)
        bytes[offset + i] = static_cast<uint8_t>(value >> (i * 8));
}

void CheckFlags(uint32_t flags, uint32_t available)
{
    const struct Requirement
    {
        uint32_t flag;
        const char* name;
    } requirements[] = {
        {Sprite, "sprite requires the sprite shader/texture chunk (7)"},
        {Framed, "framed requires the 28-byte frame chunk (6)"},
        {TimeLimit, "time-limit requires the time-limit chunk (8)"},
        {Collision, "collision requires the collision chunk (33)"},
        {VelocityScale, "velocity-scale requires the velocity-scale chunk (34)"},
    };
    for (const auto& requirement : requirements)
        if ((flags & requirement.flag) && !(available & requirement.flag))
            throw std::runtime_error(std::string("Cannot enable ") + requirement.name);
}

void CheckChildFlags(const ParticleLibraryDocument::GroupEffect& effect, uint32_t flags)
{
    if ((flags & (1u << 2)) && effect.effectName.empty())
        throw std::runtime_error("Enabled group effect requires an effect name");
    if ((flags & (1u << 1)) && effect.onPlayChild.empty())
        throw std::runtime_error("On-play child requires a non-empty on-play child name");
    if ((flags & (1u << 5)) && effect.onBirthChild.empty())
        throw std::runtime_error("On-birth child requires a non-empty on-birth child name");
    if ((flags & (1u << 6)) && effect.onDeadChild.empty())
        throw std::runtime_error("On-death child requires a non-empty on-death child name");
}

std::string IOError(const char* operation, const std::string& path, int code)
{
    return std::string(operation) + " '" + path + "': " +
        (code ? std::error_code(code, std::generic_category()).message() : "I/O operation failed");
}

struct TemporaryFile
{
    std::filesystem::path directory;
    std::filesystem::path file;

    ~TemporaryFile()
    {
        std::error_code ignored;
        if (!file.empty())
            std::filesystem::remove(file, ignored);
        if (!directory.empty())
            std::filesystem::remove(directory, ignored);
    }
};
}

bool ParticleLibraryDocument::Open(const std::string& path, std::string& error)
{
    error.clear();
    try
    {
        if (path.empty() || path.find('\0') != std::string::npos)
            throw std::runtime_error("Open requires a non-empty path without embedded null characters");
        std::error_code ec;
        const auto resolved = std::filesystem::canonical(path, ec);
        if (ec)
            throw std::runtime_error("Cannot resolve '" + path + "': " + ec.message());
        if (!std::filesystem::is_regular_file(resolved, ec))
            throw std::runtime_error("Cannot open '" + path + "': " + (ec ? ec.message() : "not a regular file"));
        const auto size = std::filesystem::file_size(resolved, ec);
        if (ec)
            throw std::runtime_error("Cannot read size of '" + path + "': " + ec.message());
        if (size > static_cast<uintmax_t>(std::numeric_limits<int32_t>::max()))
            throw std::runtime_error("Archive exceeds the engine reader's supported size");
        errno = 0;
        std::unique_ptr<std::FILE, decltype(&std::fclose)> file(std::fopen(resolved.string().c_str(), "rb"), &std::fclose);
        if (!file)
            throw std::runtime_error(IOError("Cannot open", path, errno));
        ParticleLibraryDocument document;
        document.m_bytes.resize(static_cast<size_t>(size));
        errno = 0;
        if (size && std::fread(document.m_bytes.data(), 1, document.m_bytes.size(), file.get()) != size)
            throw std::runtime_error(IOError("Cannot read complete archive", path, errno));
        if (std::fgetc(file.get()) != EOF)
            throw std::runtime_error("Archive changed size while reading '" + path + "'");
        if (std::ferror(file.get()))
            throw std::runtime_error(IOError("Cannot read archive", path, errno));
        document.Parse();
        document.m_path = path;
        document.m_sourcePath = resolved.string();
        *this = std::move(document);
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}

void ParticleLibraryDocument::Parse()
{
    const auto chunks = ReadChunks(m_bytes, 0, m_bytes.size());
    auto version = ReadChunk(m_bytes, Required(chunks, 1, 2, "library version"));
    if (version.U16() != 1)
        throw std::runtime_error("Unsupported particle library version; expected version 1");
    if (const Chunk* firstGeneration = Find(chunks, 2); firstGeneration && firstGeneration->size)
        throw std::runtime_error("First-generation particle libraries are unsupported");
    for (const auto kind : {Kind::Effect, Kind::Group})
    {
        const Chunk* container = Find(chunks, kind == Kind::Effect ? 3 : 4);
        if (!container)
            continue;
        if (kind == Kind::Effect)
            m_effectsSizeOffset = container->offset - 4;
        auto entries = ReadChunks(m_bytes, container->offset, container->size);
        std::sort(entries.begin(), entries.end(), [](const Chunk& left, const Chunk& right) { return left.id < right.id; });
        std::unordered_set<std::string> names;
        for (size_t i = 0; i != entries.size(); ++i)
        {
            if (entries[i].id != i)
                throw std::runtime_error("Particle entry chunks must be numbered consecutively from zero");
            try
            {
                ParseEntry(entries[i].offset, entries[i].size, kind);
                if (!names.insert(m_entries.back().name).second)
                    throw std::runtime_error("Duplicate particle name '" + m_entries.back().name + "'");
            }
            catch (const std::exception& exception)
            {
                throw std::runtime_error(std::string(kind == Kind::Effect ? "Effect " : "Group ") +
                    std::to_string(i) + ": " + exception.what());
            }
        }
    }
}

void ParticleLibraryDocument::ParseEntry(size_t offset, size_t size, Kind kind)
{
    const auto chunks = ReadChunks(m_bytes, offset, size);
    auto version = ReadChunk(m_bytes, Required(chunks, 1, 2, "entry version"));
    const uint16_t value = version.U16();
    if (value != (kind == Kind::Effect ? 1 : 3))
        throw std::runtime_error("Unsupported entry version " + std::to_string(value));
    Entry entry;
    Binding binding;
    binding.entryOffset = offset;
    binding.entrySize = size;
    entry.kind = kind;
    entry.name = ReadChunk(m_bytes, Required(chunks, 2, 1, "name")).String();
    if (entry.name.empty())
        throw std::runtime_error("Particle name cannot be empty");
    const Chunk& flags = Required(chunks, kind == Kind::Effect ? 5 : 3, 4, "flags");
    if (flags.size != 4)
        throw std::runtime_error("Flags chunk must contain exactly four bytes");
    entry.flags = ReadChunk(m_bytes, flags).U32();
    binding.flags = {flags.offset, entry.flags};
    if (kind == Kind::Effect)
    {
        entry.maxParticles = ReadChunk(m_bytes, Required(chunks, 3, 4, "effect data")).U32();
        if (entry.maxParticles > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
            throw std::runtime_error("Maximum particle count exceeds the engine's supported range");
        Required(chunks, 4, 4, "action list");
        if (const Chunk* sprite = Find(chunks, 7))
        {
            auto reader = ReadChunk(m_bytes, *sprite);
            entry.shader = reader.String();
            entry.texture = reader.String();
            if (entry.shader.empty() || entry.texture.empty())
                throw std::runtime_error("Sprite chunk requires non-empty shader and texture names");
            binding.availableFlags |= Sprite;
        }
        if (const Chunk* frame = Find(chunks, 6))
        {
            auto reader = ReadChunk(m_bytes, *frame);
            reader.Float();
            reader.Float();
            reader.Skip(8);
            const uint32_t dimension = reader.U32();
            const uint32_t count = reader.U32();
            reader.Float();
            if (dimension == 0 || dimension > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) ||
                count == 0 || count > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
                throw std::runtime_error("Frame chunk requires positive frame dimension and frame count");
            binding.availableFlags |= Framed;
        }
        if (const Chunk* limit = Find(chunks, 8))
        {
            entry.timeLimit = ReadChunk(m_bytes, *limit).Float();
            binding.availableFlags |= TimeLimit;
        }
        for (const uint32_t id : {33u, 34u, 37u})
        {
            if (const Chunk* vector = Find(chunks, id))
            {
                auto reader = ReadChunk(m_bytes, *vector);
                reader.Float();
                reader.Float();
                reader.Float();
                if (id == 33)
                    binding.availableFlags |= Collision;
                if (id == 34)
                    binding.availableFlags |= VelocityScale;
            }
        }
        CheckFlags(entry.flags, binding.availableFlags);
    }
    else
    {
        if (Find(chunks, 7))
            throw std::runtime_error("Group effects chunk 7 is unsupported; expected version 3 flat effects chunk 4");
        if (const Chunk* limit = Find(chunks, 5))
            entry.timeLimit = ReadChunk(m_bytes, *limit).Float();
        const bool calculateTimeLimit = entry.timeLimit <= 0.0f;
        if (const Chunk* effects = Find(chunks, 4))
        {
            auto reader = ReadChunk(m_bytes, *effects);
            const uint32_t count = reader.U32();
            if (count > reader.Remaining() / 16)
                throw std::runtime_error("Group effect count exceeds the effects chunk");
            entry.effects.reserve(count);
            binding.effects.reserve(count);
            for (uint32_t i = 0; i != count; ++i)
            {
                GroupEffect effect;
                effect.effectName = reader.String();
                effect.onPlayChild = reader.String();
                effect.onBirthChild = reader.String();
                effect.onDeadChild = reader.String();
                effect.timeStart = reader.Float();
                effect.timeEnd = reader.Float();
                const size_t flagOffset = reader.Position();
                effect.flags = reader.U32();
                CheckChildFlags(effect, effect.flags);
                if (calculateTimeLimit)
                    entry.timeLimit = std::max(entry.timeLimit, effect.timeEnd);
                binding.effects.push_back({flagOffset, effect.flags});
                entry.effects.push_back(std::move(effect));
            }
        }
    }
    m_entries.push_back(std::move(entry));
    m_bindings.push_back(std::move(binding));
}

bool ParticleLibraryDocument::SaveAs(const std::string& path, std::string& error)
{
    error.clear();
    try
    {
        if (m_path.empty())
            throw std::runtime_error("Open a particle library before saving");
        if (path.empty() || path.find('\0') != std::string::npos)
            throw std::runtime_error("Save As requires a non-empty path without embedded null characters");
        std::error_code ec;
        const auto target = std::filesystem::weakly_canonical(path, ec);
        if (ec)
            throw std::runtime_error("Cannot resolve save destination '" + path + "': " + ec.message());
        const bool sourceTarget = target == std::filesystem::path(m_sourcePath) ||
            std::filesystem::equivalent(target, m_sourcePath, ec);
        if (sourceTarget)
            throw std::runtime_error("The original source library is protected; choose a different Save As path");
        ec.clear();
        const bool exists = std::filesystem::exists(target, ec);
        if (ec)
            throw std::runtime_error("Cannot inspect save destination '" + path + "': " + ec.message());
        if (exists && !std::filesystem::is_regular_file(target, ec))
            throw std::runtime_error("Cannot replace '" + path + "': " + (ec ? ec.message() : "not a regular file"));
        std::string savedPath = path;
        TemporaryFile temporary;
        static std::atomic<uint64_t> sequence{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; attempt != 128; ++attempt)
        {
            auto directory = target.parent_path() / (".particle-library-" + std::to_string(stamp) + "-" +
                std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)));
            if (std::filesystem::create_directory(directory, ec))
            {
                temporary.directory = std::move(directory);
                break;
            }
            if (ec && ec != std::errc::file_exists)
                throw std::runtime_error("Cannot create temporary save file near '" + path + "': " + ec.message());
        }
        if (temporary.directory.empty())
            throw std::runtime_error("Cannot allocate a unique temporary save directory");
        temporary.file = temporary.directory / "particles.xr";
        errno = 0;
        std::unique_ptr<std::FILE, decltype(&std::fclose)> file(std::fopen(temporary.file.string().c_str(), "wb"), &std::fclose);
        if (!file)
            throw std::runtime_error(IOError("Cannot create temporary save file for", path, errno));
        if (std::fwrite(m_bytes.data(), 1, m_bytes.size(), file.get()) != m_bytes.size())
            throw std::runtime_error(IOError("Cannot write", path, errno));
        if (std::fflush(file.get()) != 0)
            throw std::runtime_error(IOError("Cannot flush", path, errno));
        if (std::fclose(file.release()) != 0)
            throw std::runtime_error(IOError("Cannot close", path, errno));
        std::filesystem::rename(temporary.file, target, ec);
        if (ec)
            throw std::runtime_error("Cannot replace '" + path + "': " + ec.message());
        m_path.swap(savedPath);
        for (size_t i = 0; i != m_entries.size(); ++i)
        {
            m_bindings[i].flags.baseline = m_entries[i].flags;
            for (size_t j = 0; j != m_entries[i].effects.size(); ++j)
                m_bindings[i].effects[j].baseline = m_entries[i].effects[j].flags;
        }
        m_changedWords = 0;
        m_structureChanged = false;
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}

const std::vector<ParticleLibraryDocument::Entry>& ParticleLibraryDocument::Entries() const { return m_entries; }
const std::string& ParticleLibraryDocument::Path() const { return m_path; }
bool ParticleLibraryDocument::Dirty() const { return m_changedWords != 0 || m_structureChanged; }
const std::vector<uint8_t>& ParticleLibraryDocument::Bytes() const { return m_bytes; }

void ParticleLibraryDocument::AddCollisionChunk(size_t entry)
{
    constexpr float defaults[] = {1.f, 0.f, 0.f};
    constexpr size_t chunkSize = 8 + sizeof(defaults);
    if (m_bytes.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()) - chunkSize)
        throw std::runtime_error("Adding collision parameters would exceed the engine reader's supported size");

    Binding& target = m_bindings[entry];
    const size_t offset = target.entryOffset + target.entrySize;
    const uint32_t effectsSize = Reader(m_bytes, m_effectsSizeOffset, 4).U32();
    m_bytes.insert(m_bytes.begin() + offset, chunkSize, 0);
    WriteU32(m_bytes, offset, 33);
    WriteU32(m_bytes, offset + 4, sizeof(defaults));
    for (size_t i = 0; i != std::size(defaults); ++i)
    {
        uint32_t bits;
        std::memcpy(&bits, &defaults[i], sizeof(bits));
        WriteU32(m_bytes, offset + 8 + i * sizeof(bits), bits);
    }
    target.entrySize += chunkSize;
    WriteU32(m_bytes, target.entryOffset - 4, static_cast<uint32_t>(target.entrySize));
    WriteU32(m_bytes, m_effectsSizeOffset, effectsSize + chunkSize);
    for (auto& binding : m_bindings)
    {
        if (binding.entryOffset >= offset)
            binding.entryOffset += chunkSize;
        if (binding.flags.offset >= offset)
            binding.flags.offset += chunkSize;
        for (auto& word : binding.effects)
            if (word.offset >= offset)
                word.offset += chunkSize;
    }
    target.availableFlags |= Collision;
    m_structureChanged = true;
}

void ParticleLibraryDocument::Patch(const FlagWord& word, uint32_t oldFlags, uint32_t flags)
{
    if (oldFlags == flags)
        return;
    if (oldFlags != word.baseline)
        --m_changedWords;
    if (flags != word.baseline)
        ++m_changedWords;
    WriteU32(m_bytes, word.offset, flags);
}

bool ParticleLibraryDocument::SetFlags(size_t entry, uint32_t flags, std::string& error)
{
    error.clear();
    try
    {
        if (entry >= m_entries.size())
            throw std::runtime_error("Particle entry index is out of range");
        Entry& value = m_entries[entry];
        const Binding& binding = m_bindings[entry];
        if (value.kind == Kind::Effect)
        {
            CheckFlags(flags, binding.availableFlags | Collision);
            if ((flags & Collision) && !(binding.availableFlags & Collision))
                AddCollisionChunk(entry);
        }
        Patch(binding.flags, value.flags, flags);
        value.flags = flags;
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}

bool ParticleLibraryDocument::SetGroupEffectFlags(size_t entry, size_t effect, uint32_t flags, std::string& error)
{
    error.clear();
    try
    {
        if (entry >= m_entries.size() || m_entries[entry].kind != Kind::Group)
            throw std::runtime_error("Particle entry is not a valid group");
        if (effect >= m_entries[entry].effects.size())
            throw std::runtime_error("Group effect index is out of range");
        GroupEffect& value = m_entries[entry].effects[effect];
        CheckChildFlags(value, flags);
        Patch(m_bindings[entry].effects[effect], value.flags, flags);
        value.flags = flags;
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}
}
