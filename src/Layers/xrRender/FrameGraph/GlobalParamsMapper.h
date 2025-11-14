#pragma once

#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"
#include "xrCore/xrstring.h"

namespace slang {
    struct ShaderReflection;
}

namespace xray::render {

namespace RENDER_NAMESPACE {
    class R_constant_table;
    class CBackend;
}

class GlobalParamsMapper {
public:
    struct UniformMember {
        shared_str name;
        u32 offset;
        u32 size;
        slang::TypeReflection::ScalarType type;
    };

    GlobalParamsMapper() = default;
    ~GlobalParamsMapper() = default;

    bool ExtractLayout(slang::ShaderReflection* reflection);

    void PopulateBuffer(
        void* bufferData,
        const RENDER_NAMESPACE::R_constant_table* constantTable,
        RENDER_NAMESPACE::CBackend& backend);

    bool HasGlobalParams() const { return !m_members.empty(); }

    u32 GetBufferSize() const { return m_bufferSize; }

    const xr_vector<UniformMember>& GetMembers() const { return m_members; }

private:
    xr_vector<UniformMember> m_members;
    u32 m_bufferSize = 0;
};

}
