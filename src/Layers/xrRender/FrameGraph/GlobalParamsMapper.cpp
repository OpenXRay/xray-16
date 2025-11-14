// xrRender/FrameGraph/GlobalParamsMapper.cpp
#include "stdafx.h"
#include "GlobalParamsMapper.h"
#include "Layers/xrRender/r_constants.h"
#include <algorithm>

namespace xray::render {

bool GlobalParamsMapper::ExtractLayout(slang::ShaderReflection* reflection)
{
    if (!reflection)
        return false;

    // Use SlangReflectionWrapper to query globalParams_0 members
    SlangReflectionWrapper wrapper(reflection);
    auto members = wrapper.GetConstantBufferMembers("globalParams_0");

    if (members.empty())
    {
        // No globalParams_0 in this shader (it's optional)
        return false;
    }

    // Convert to our format
    m_members.clear();
    for (const auto& member : members)
    {
        UniformMember uniform;
        uniform.name = member.name;
        uniform.offset = member.offset;
        uniform.size = member.size;
        uniform.type = member.scalarType;

        m_members.push_back(uniform);
    }

    // Calculate total buffer size
    if (!m_members.empty())
    {
        // Find max (offset + size)
        u32 maxEnd = 0;
        for (const auto& m : m_members)
        {
            maxEnd = std::max(maxEnd, m.offset + m.size);
        }

        // Round up to 16-byte alignment (D3D11 constant buffer requirement)
        m_bufferSize = (maxEnd + 15) & ~15;
    }

    Msg("  [GlobalParamsMapper] Extracted layout: %u members, %u bytes",
        (u32)m_members.size(), m_bufferSize);

    // Log each member for debugging
    for (const auto& m : m_members)
    {
        Msg("    - %s: offset=%u, size=%u", m.name.c_str(), m.offset, m.size);
    }

    return true;
}

void GlobalParamsMapper::PopulateBuffer(
    void* bufferData,
    const R_constant_table* constantTable,
    RENDER_NAMESPACE::CBackend& backend)
{
    VERIFY(bufferData);
    VERIFY(constantTable);

    // Zero-fill buffer first
    std::memset(bufferData, 0, m_bufferSize);

    u8* buffer = static_cast<u8*>(bufferData);

    // For each member in globalParams_0, try to find it in the constant table
    for (const auto& member : m_members)
    {
        // Query X-Ray's constant table for this uniform
        auto constant = constantTable->get(member.name.c_str());

        if (constant && constant->handler)
        {
            // TODO (Phase 2): Extract value from backend after calling setup()
            // For now, we'll implement value extraction in a follow-up
            //
            // The proper approach is:
            // 1. Call constant->handler->setup(backend, constant.get())
            // 2. Extract the value that was set (from backend's cache)
            // 3. Copy it to our buffer at the correct offset
            //
            // This requires understanding CBackend's internal value storage,
            // which we'll implement next.

            Msg("    [GlobalParamsMapper] Found handler for '%s' (TODO: extract value)",
                member.name.c_str());
        }
        else
        {
            // Uniform not found in constant table - leave as zero
            // This is normal for optional uniforms or engine-specific shaders
        }
    }
}

} // namespace xray::render
