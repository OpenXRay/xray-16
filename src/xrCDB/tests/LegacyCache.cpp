#include "TestSupport.h"
#include "LegacyOpcode12.h"

void CheckLegacyCache()
{
    auto mesh = Fixture();
    const auto path = TestPath("legacy-opcode12.cache");
    const auto stored = path + ".opcode13";
    auto* writer = FS.w_open(stored.c_str());
    writer->w(legacyOpcode12Cache.data(), legacyOpcode12Cache.size());
    FS.w_close(writer);
    {
        CDB::MODEL loaded;
        loaded.set_model_crc32(2139);
        Require(loaded.deserialize(path.c_str()), "Real OPCODE 1.2 cache could not be decoded");
        CDB::COLLIDER collider;
        collider.ray_query(0, &loaded, Vector(.25f, .25f, -1), Vector(0, 0, 1), 10);
        Require(Ids(collider, mesh) == std::set<int>{0, 1, 3, 4}, "Legacy cache changed triangle IDs or metadata");
    }
    FS.file_delete(stored.c_str());

    // The embedded cform representation omits the standalone tree checksum.
    const size_t offset = 16 + mesh.vertices.size() * sizeof(Fvector) + mesh.triangles.size() * sizeof(CDB::TRI);
    std::vector<u8> embedded(legacyOpcode12Cache.begin() + offset, legacyOpcode12Cache.begin() + offset + 12);
    embedded.insert(embedded.end(), legacyOpcode12Cache.begin() + offset + 16, legacyOpcode12Cache.end());
    for (const bool corrupt : {false, true})
    {
        auto bytes = embedded;
        if (corrupt)
            // An internal offset to its own root must trigger rebuild, not recursion.
            std::fill(bytes.begin() + 36, bytes.begin() + 44, 0);
        writer = FS.w_open(path.c_str());
        writer->w(bytes.data(), bytes.size());
        FS.w_close(writer);
        auto* reader = FS.r_open(path.c_str());
        CDB::MODEL loaded;
        loaded.load_geom(mesh.vertices.data(), u32(mesh.vertices.size()), mesh.triangles.data(), u32(mesh.triangles.size()));
        loaded.deserialize_tree(reader);
        FS.r_close(reader);
        CDB::COLLIDER collider;
        collider.ray_query(0, &loaded, Vector(.25f, .25f, -1), Vector(0, 0, 1), 10);
        Require(Ids(collider, mesh) == std::set<int>{0, 1, 3, 4}, "Embedded legacy tree load/rebuild failed");
    }
    FS.file_delete(path.c_str());
}
