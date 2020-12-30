#pragma once

// TODO: Get rid of D3D types.
#if defined(XR_PLATFORM_WINDOWS)
#include <d3d9types.h>
#endif

class glState;

typedef enum D3D_CLEAR_FLAG {
    D3D_CLEAR_DEPTH = 0x1L,
    D3D_CLEAR_STENCIL = 0x2L
} D3D_CLEAR_FLAG;

typedef enum D3D_COMPARISON_FUNC {
    D3D_COMPARISON_NEVER = GL_NEVER,
    D3D_COMPARISON_LESS = GL_LESS,
    D3D_COMPARISON_EQUAL = GL_EQUAL,
    D3D_COMPARISON_LESS_EQUAL = GL_LEQUAL,
    D3D_COMPARISON_GREATER = GL_GREATER,
    D3D_COMPARISON_NOT_EQUAL = GL_NOTEQUAL,
    D3D_COMPARISON_GREATER_EQUAL = GL_GEQUAL,
    D3D_COMPARISON_ALWAYS = GL_ALWAYS
} D3D_COMPARISON_FUNC;

struct XR_GL_VIEWPORT
{
    GLint TopLeftX, TopLeftY;
    GLsizei Width, Height;
    GLclampf MinDepth, MaxDepth;
};

struct D3D_VIEWPORT : XR_GL_VIEWPORT
{
    using XR_GL_VIEWPORT::XR_GL_VIEWPORT;

    // Needed to suppress warnings
    template <typename TopLeftCoords, typename Dimensions>
    D3D_VIEWPORT(TopLeftCoords x, TopLeftCoords y, Dimensions w, Dimensions h, float minZ, float maxZ)
        : XR_GL_VIEWPORT{
            static_cast<GLint>(x), static_cast<GLint>(y),
            static_cast<GLsizei>(w), static_cast<GLsizei>(h),
            static_cast<GLclampf>(minZ), static_cast<GLclampf>(maxZ),
        }
    {}
};

using D3D_QUERY = enum XR_GL_QUERY
{
    D3D_QUERY_EVENT,
    D3D_QUERY_OCCLUSION
};

using ID3DState = glState;

#define DX10_ONLY(expr)			do {} while (0)

using unused_t = int[0];

using IndexBufferHandle = GLuint;
using VertexBufferHandle = GLuint;
using ConstantBufferHandle = GLuint;
using HostBufferHandle = void*;

using VertexElement = D3DVERTEXELEMENT9;
using InputElementDesc = unused_t;
