#pragma once

#include "xrCommon/xr_vector.h"
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <string_view>

namespace xray::render::RENDER_NAMESPACE
{

#ifdef DEBUG
#define BUFFER_DEBUG_NAME(name) #name,
#else
#define BUFFER_DEBUG_NAME(name)
#endif

struct BufferObject
{
#ifdef DEBUG
    xr_string name;
#endif
    GLuint id = GL_NONE;
    GLenum flags = GL_NONE;
    GLsizeiptr size = GL_NONE;

    [[nodiscard]] bool valid() const
    {
        return id != GL_NONE;
    }
};

struct UniformBufferObject : BufferObject
{
};

struct SharedStorageBufferObject : BufferObject
{
    GLuint bindingIndex = GL_INVALID_INDEX;
};

class CBackendExtension
{
protected:
    static void initializeUniforms();
public:

    void setUniforms(GLuint program, GLint location, xr_vector<glm::vec4>& uniforms);

    static void uboGenerate(UniformBufferObject& ubo);
    static void uboDispose(UniformBufferObject& ubo);
    static void uboRegisterWithProgram(ref_vs& program, std::string_view location, uint32_t blockBinding, const UniformBufferObject& ubo);
    static void uboPushToDevice(const UniformBufferObject& ubo, GLsizeiptr size, void* data);
    static void uboBindRange(const UniformBufferObject &ubo, GLuint blockBinding, GLintptr offset, GLsizeiptr size);

    static void ssboGenerate(SharedStorageBufferObject& ssbo);
    static void ssboPushToDevice(const SharedStorageBufferObject& ssbo, GLsizeiptr size, void* data);
    static void ssboDispose(UniformBufferObject& ubo);

    [[nodiscard]]
    static uint calculateMaxUniformVertexSize(size_t sizeOfElement, uint max)
    {
        return std::min(static_cast<uint>(maxUniformVertexSize / std::max(sizeOfElement, sizeof(float))), max);
    }

    /**
     * @param offset
     * @return Offset + alignment padding
     */
    [[nodiscard]]
    static size_t calculateSsboAlignedOffset(const size_t offset)
    {
        return offset + calculateSsboAlignmentPadding(offset);
    }

    /**
     *
     * @param offset
     * @return Padding to fulfilling the SSBO ALIGNMENT
     */
    [[nodiscard]]
    static size_t calculateSsboAlignmentPadding(const size_t offset)
    {
        return (ssboAlignment - (offset % ssboAlignment)) % ssboAlignment;
    }

private:
    static void bindBuffer(const UniformBufferObject& ubo);

    static GLuint uboCurrentId;

    static GLint maxUniformVertexSize;
    static GLint maxUniformBlockSize;
    static GLint ssboAlignment;

    template<typename TYPE>
    void _setUniforms(GLuint program, GLint location, xr_vector<TYPE>& uniforms);
};
}
