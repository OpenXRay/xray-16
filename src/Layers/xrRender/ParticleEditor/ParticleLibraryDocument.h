#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xray::render::fg
{
class ParticleLibraryDocument
{
public:
    enum class Kind
    {
        Effect,
        Group
    };

    struct GroupEffect
    {
        std::string effectName;
        std::string onPlayChild;
        std::string onBirthChild;
        std::string onDeadChild;
        float timeStart = 0.0f;
        float timeEnd = 0.0f;
        uint32_t flags = 0;
    };

    struct Entry
    {
        Kind kind = Kind::Effect;
        std::string name;
        std::string shader;
        std::string texture;
        uint32_t flags = 0;
        uint32_t maxParticles = 0;
        float timeLimit = 0.0f;
        std::vector<GroupEffect> effects;
    };

    bool Open(const std::string& path, std::string& error);
    bool SaveAs(const std::string& path, std::string& error);
    const std::vector<Entry>& Entries() const;
    const std::string& Path() const;
    bool Dirty() const;
    bool SetFlags(size_t entry, uint32_t flags, std::string& error);
    bool SetGroupEffectFlags(size_t entry, size_t effect, uint32_t flags, std::string& error);
    const std::vector<uint8_t>& Bytes() const;

private:
    struct FlagWord
    {
        size_t offset = 0;
        uint32_t baseline = 0;
    };

    struct Binding
    {
        size_t entryOffset = 0;
        size_t entrySize = 0;
        FlagWord flags;
        uint32_t availableFlags = 0;
        std::vector<FlagWord> effects;
    };

    void Parse();
    void ParseEntry(size_t offset, size_t size, Kind kind);
    void AddChunk(size_t entry, uint32_t id, const std::vector<uint8_t>& payload, uint32_t flag);
    void EnsureChunks(size_t entry, uint32_t flags);
    void Patch(const FlagWord& word, uint32_t oldFlags, uint32_t flags);

    std::vector<uint8_t> m_bytes;
    std::vector<Entry> m_entries;
    std::vector<Binding> m_bindings;
    std::string m_path;
    std::string m_sourcePath;
    size_t m_changedWords = 0;
    size_t m_effectsSizeOffset = 0;
    bool m_structureChanged = false;
};
}
