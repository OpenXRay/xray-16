#pragma once

// TODO: Get rid of D3D types.
#if defined(WINDOWS)
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

using D3D_VIEWPORT = struct XR_GL_VIEWPORT
{
    GLint TopLeftX, TopLeftY;
    GLsizei Width, Height;
    GLclampf MinDepth, MaxDepth;
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
