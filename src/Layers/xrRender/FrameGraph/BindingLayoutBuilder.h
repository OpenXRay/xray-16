#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {

struct ExtractedReflection;

class BindingLayoutBuilder {
public:
    static nvrhi::BindingLayoutDesc Build(
        const ExtractedReflection& reflection,
        nvrhi::ShaderType visibility = nvrhi::ShaderType::All);

    static nvrhi::BindingLayoutDesc Build(
        const ExtractedReflection& vsReflection,
        const ExtractedReflection& psReflection,
        nvrhi::ShaderType visibility = nvrhi::ShaderType::All);
};

}
