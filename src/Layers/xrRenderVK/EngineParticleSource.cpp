#include "xrEngine/stdafx.h"
#include "EngineParticleSource.h"

namespace xray::render::vulkan
{
namespace
{
void particle_chunk(std::vector<uint8_t>& out, uint32_t id, const void* data, size_t size)
{
    for (unsigned shift = 0; shift < 32; shift += 8) out.push_back(uint8_t(id >> shift));
    for (unsigned shift = 0; shift < 32; shift += 8) out.push_back(uint8_t(size >> shift));
    if (size)
    {
        const auto* bytes = static_cast<const uint8_t*>(data);
        out.insert(out.end(), bytes, bytes + size);
    }
}

}

// IReader::open_chunk transparently decompresses CFS_CompressMark chunks.
// Reassemble only the particle library fields used by the portable decoder.
std::vector<uint8_t> particle_library_bytes(IReader& source)
{
    std::vector<uint8_t> library;
    if (IReader* version = source.open_chunk(1))
    {
        particle_chunk(library, 1, version->pointer(), version->length());
        version->close();
    }
    for (uint32_t kind : {3u, 4u})
    {
        IReader* container = source.open_chunk(kind);
        if (!container) continue;
        std::vector<uint8_t> definitions;
        for (uint32_t index = 0; index < 100000; ++index)
        {
            IReader* definition = container->open_chunk(index);
            if (!definition) break;
            std::vector<uint8_t> fields;
            for (uint32_t field = 1; field <= (kind == 3 ? 11u : 5u); ++field)
                if (IReader* value = definition->open_chunk(field))
                {
                    particle_chunk(fields, field, value->pointer(), value->length());
                    value->close();
                }
            particle_chunk(definitions, index, fields.data(), fields.size());
            definition->close();
        }
        particle_chunk(library, kind, definitions.data(), definitions.size());
        container->close();
    }
    return library;
}

}
