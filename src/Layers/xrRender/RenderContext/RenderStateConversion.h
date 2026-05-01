// xrRender/RenderContext/RenderStateConversion.h
#pragma once

#include "Layers/xrRender/RenderContext/PipelineState.h"

#if defined(USE_DX11)
#include <d3d11.h>
#endif

namespace xray::render {

// ══════════════════════════════════════════════════════════
//  STATE CONVERSION HELPERS
// ══════════════════════════════════════════════════════════
// Convert D3D11 render states to NVRHI/our abstraction
// Used by MaterialCache and ParticlePass to extract render state from shader passes

#if defined(USE_DX11)

// Convert D3D11 cull mode to NVRHI
inline fg::CullMode ConvertCullMode(D3D11_CULL_MODE d3dCull) {
    switch (d3dCull) {
        case D3D11_CULL_NONE: return fg::CullMode::None;
        case D3D11_CULL_FRONT: return fg::CullMode::Front;
        case D3D11_CULL_BACK: return fg::CullMode::Back;
        default: return fg::CullMode::Back;
    }
}

// Convert D3D11 fill mode to NVRHI
inline fg::FillMode ConvertFillMode(D3D11_FILL_MODE d3dFill) {
    switch (d3dFill) {
        case D3D11_FILL_WIREFRAME: return fg::FillMode::Wireframe;
        case D3D11_FILL_SOLID: return fg::FillMode::Solid;
        default: return fg::FillMode::Solid;
    }
}

// Convert D3D11 stencil op to our abstraction
inline fg::StencilOp ConvertStencilOp(D3D11_STENCIL_OP d3dOp) {
    switch (d3dOp) {
        case D3D11_STENCIL_OP_KEEP: return fg::StencilOp::Keep;
        case D3D11_STENCIL_OP_ZERO: return fg::StencilOp::Zero;
        case D3D11_STENCIL_OP_REPLACE: return fg::StencilOp::Replace;
        case D3D11_STENCIL_OP_INCR_SAT: return fg::StencilOp::IncrementSaturate;
        case D3D11_STENCIL_OP_DECR_SAT: return fg::StencilOp::DecrementSaturate;
        case D3D11_STENCIL_OP_INVERT: return fg::StencilOp::Invert;
        case D3D11_STENCIL_OP_INCR: return fg::StencilOp::Increment;
        case D3D11_STENCIL_OP_DECR: return fg::StencilOp::Decrement;
        default: return fg::StencilOp::Keep;
    }
}

// Convert D3D11 comparison func to our abstraction
inline fg::ComparisonFunc ConvertComparisonFunc(D3D11_COMPARISON_FUNC d3dFunc) {
    switch (d3dFunc) {
        case D3D11_COMPARISON_NEVER: return fg::ComparisonFunc::Never;
        case D3D11_COMPARISON_LESS: return fg::ComparisonFunc::Less;
        case D3D11_COMPARISON_EQUAL: return fg::ComparisonFunc::Equal;
        case D3D11_COMPARISON_LESS_EQUAL: return fg::ComparisonFunc::LessEqual;
        case D3D11_COMPARISON_GREATER: return fg::ComparisonFunc::Greater;
        case D3D11_COMPARISON_NOT_EQUAL: return fg::ComparisonFunc::NotEqual;
        case D3D11_COMPARISON_GREATER_EQUAL: return fg::ComparisonFunc::GreaterEqual;
        case D3D11_COMPARISON_ALWAYS: return fg::ComparisonFunc::Always;
        default: return fg::ComparisonFunc::Less;
    }
}

// Convert D3D11 blend factor to our abstraction
inline fg::BlendFactor ConvertBlendFactor(D3D11_BLEND d3dBlend) {
    switch (d3dBlend) {
        case D3D11_BLEND_ZERO: return fg::BlendFactor::Zero;
        case D3D11_BLEND_ONE: return fg::BlendFactor::One;
        case D3D11_BLEND_SRC_COLOR: return fg::BlendFactor::SrcColor;
        case D3D11_BLEND_INV_SRC_COLOR: return fg::BlendFactor::InvSrcColor;
        case D3D11_BLEND_SRC_ALPHA: return fg::BlendFactor::SrcAlpha;
        case D3D11_BLEND_INV_SRC_ALPHA: return fg::BlendFactor::InvSrcAlpha;
        case D3D11_BLEND_DEST_ALPHA: return fg::BlendFactor::DstAlpha;
        case D3D11_BLEND_INV_DEST_ALPHA: return fg::BlendFactor::InvDstAlpha;
        case D3D11_BLEND_DEST_COLOR: return fg::BlendFactor::DstColor;
        case D3D11_BLEND_INV_DEST_COLOR: return fg::BlendFactor::InvDstColor;
        case D3D11_BLEND_SRC_ALPHA_SAT: return fg::BlendFactor::SrcAlphaSat;
        case D3D11_BLEND_BLEND_FACTOR: return fg::BlendFactor::BlendFactor;
        case D3D11_BLEND_INV_BLEND_FACTOR: return fg::BlendFactor::InvBlendFactor;
        default: return fg::BlendFactor::One;
    }
}

// Convert D3D11 blend op to our abstraction
inline fg::BlendOp ConvertBlendOp(D3D11_BLEND_OP d3dOp) {
    switch (d3dOp) {
        case D3D11_BLEND_OP_ADD: return fg::BlendOp::Add;
        case D3D11_BLEND_OP_SUBTRACT: return fg::BlendOp::Subtract;
        case D3D11_BLEND_OP_REV_SUBTRACT: return fg::BlendOp::RevSubtract;
        case D3D11_BLEND_OP_MIN: return fg::BlendOp::Min;
        case D3D11_BLEND_OP_MAX: return fg::BlendOp::Max;
        default: return fg::BlendOp::Add;
    }
}

// Convert D3D11 color write mask to NVRHI
inline fg::ColorWriteMask ConvertColorWriteMask(u8 d3dMask) {
    fg::ColorWriteMask mask = fg::ColorWriteMask::None;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_RED)   mask = mask | fg::ColorWriteMask::Red;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_GREEN) mask = mask | fg::ColorWriteMask::Green;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_BLUE)  mask = mask | fg::ColorWriteMask::Blue;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_ALPHA) mask = mask | fg::ColorWriteMask::Alpha;
    return mask;
}

#endif

} // namespace xray::render
