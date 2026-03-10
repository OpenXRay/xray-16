#pragma once

namespace xray::render::framegraph {

enum class ResourceShape : u8 {
    Texture,
    StructuredBuffer,
    RawBuffer,
    TypedBuffer,
    AccelStruct,
    Unknown
};

}
