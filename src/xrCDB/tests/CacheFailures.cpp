#include <cstring>
#include <limits>
#include "TestSupport.h"

static void WriteMetadata(IWriter& writer)
{
    writer.w_u32(0x2139);
}

static bool ReadMetadata(IReader& reader)
{
    return reader.elapsed() >= 4 && reader.r_u32() == 0x2139;
}

static void WriteBytes(const std::string& name, const std::vector<u8>& bytes)
{
    auto* writer = FS.w_open(name.c_str());
    Require(writer != nullptr, "Could not create test cache");
    writer->w(bytes.data(), bytes.size());
    FS.w_close(writer);
}

static std::vector<u8> ReadBytes(const std::string& name)
{
    auto* reader = FS.r_open(name.c_str());
    Require(reader != nullptr, "Could not read test cache");
    std::vector<u8> bytes(reader->length());
    reader->r(bytes.data(), bytes.size());
    FS.r_close(reader);
    return bytes;
}

void CheckCacheFailures()
{
    auto mesh = Fixture();
    CDB::MODEL model;
    mesh.Build(model);
    model.set_model_crc32(2139);
    const auto path = TestPath("cdb-failures.cache");
    auto stored = path;
    size_t versionBytes = 0;
#ifdef XRAY_USE_JOLT_CDB
    stored += ".jolt";
    versionBytes = 4;
    const std::vector<u8> legacy{ 1, 2, 3, 4, 5 };
    WriteBytes(path, legacy);
#endif
    Require(model.serialize(path.c_str(), WriteMetadata), "Could not serialize fixture");
    const auto valid = ReadBytes(stored);
#ifdef XRAY_USE_JOLT_CDB
    Require(ReadBytes(path) == legacy, "Jolt overwrote OPCODE cache");
    FS.file_delete(path.c_str());
#endif
    const size_t checksum = versionBytes + 8, counts = checksum + 4, geometry = counts + 8;
    const size_t verticesEnd = geometry + mesh.vertices.size() * sizeof(Fvector);
    const size_t geometryEnd = verticesEnd + mesh.triangles.size() * sizeof(CDB::TRI);
    auto put = [](std::vector<u8>& bytes, size_t offset, u32 value)
    {
        std::memcpy(bytes.data() + offset, &value, 4);
    };
    auto reject = [&](const std::vector<u8>& bytes, bool skip = false)
    {
        WriteBytes(stored, bytes);
        CDB::MODEL rejected;
        rejected.set_model_crc32(2139);
        Require(!rejected.deserialize(path.c_str(), skip, ReadMetadata), "Invalid cache accepted");
        mesh.Build(rejected);
        CDB::COLLIDER collider;
        collider.ray_query(0, &rejected, Vector(.25f, .25f, -1), Vector(0, 0, 1), 10);
        Require(Ids(collider, mesh) == std::set<int>{ 0, 1, 3, 4 }, "Rejected cache prevented rebuild");
    };
    for (const size_t length : { versionBytes, versionBytes + 4, checksum, counts, geometry - 1, verticesEnd - 1, geometryEnd - 1 })
        reject(std::vector<u8>(valid.begin(), valid.begin() + length));
    for (const auto offset : { versionBytes, versionBytes + 4 })
    {
        auto bytes = valid;
        put(bytes, offset, 999);
        reject(bytes);
        reject(bytes, true);
    }
#ifdef XRAY_USE_JOLT_CDB
    {
        auto bytes = valid;
        put(bytes, 0, 2);
        reject(bytes);
    }
#endif
    for (const auto offset : { counts, counts + 4 })
        for (const u32 count : { 0u, 1u, std::numeric_limits<u32>::max() })
        {
            auto bytes = valid;
            put(bytes, offset, count);
            reject(bytes, true);
        }
    {
        auto bytes = valid;
        bytes[geometry] ^= 1;
        reject(bytes);
        WriteBytes(stored, bytes);
        CDB::MODEL unchecked;
        unchecked.set_model_crc32(2139);
        Require(unchecked.deserialize(path.c_str(), true, ReadMetadata), "Explicit CRC bypass ignored");
    }
#ifdef XRAY_USE_JOLT_CDB
    for (const auto offset : { geometry, verticesEnd })
    {
        auto bytes = valid;
        put(bytes, offset, 0x7fc00000);
        put(bytes, checksum, crc32(bytes.data() + counts, geometryEnd - counts));
        reject(bytes);
    }
#endif
    FS.file_delete(stored.c_str());
    CDB::MODEL absent;
    absent.set_model_crc32(2139);
    Require(!absent.deserialize(path.c_str(), false, ReadMetadata), "Missing cache accepted");
}
