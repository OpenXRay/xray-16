#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render {

struct VariantPartitionConfig {
    nvrhi::IBuffer* countBuffer = nullptr;
    nvrhi::IBuffer* drawArgsBuffer = nullptr;
    nvrhi::IBuffer* batchIndicesBuffer = nullptr;
    nvrhi::IBuffer* materialIDsBuffer = nullptr;
    nvrhi::IBuffer* drawIndexBuffer = nullptr;
    u32 binCapacity = 0;
    u32 variantCount = 0;

    bool Enabled() const { return variantCount > 1 && drawArgsBuffer && countBuffer; }
};

} // namespace xray::render
