#include "stdafx.h"
#include "GlobalParamsMapper.h"
#include "Layers/xrRender/r_constants.h"
#include <algorithm>

namespace xray::render {

bool GlobalParamsMapper::ExtractLayout(slang::ShaderReflection* reflection)
{
    if (!reflection)
        return false;

    SlangReflectionWrapper wrapper(reflection);
    auto members = wrapper.GetConstantBufferMembers("globalParams_0");

    if (members.empty())
    {
        return false;
    }

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

    if (!m_members.empty())
    {
        u32 maxEnd = 0;
        for (const auto& m : m_members)
        {
            maxEnd = std::max(maxEnd, m.offset + m.size);
        }

        m_bufferSize = (maxEnd + 15) & ~15;
    }

    Msg("  [GlobalParamsMapper] Extracted layout: %u members, %u bytes",
        (u32)m_members.size(), m_bufferSize);

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

    std::memset(bufferData, 0, m_bufferSize);

    u8* buffer = static_cast<u8*>(bufferData);

    for (const auto& member : m_members)
    {
        auto constant = constantTable->get(member.name.c_str());

        if (constant && constant->handler)
        {

            Msg("    [GlobalParamsMapper] Found handler for '%s' (TODO: extract value)",
                member.name.c_str());
        }
        else
        {
        }
    }
}

}
