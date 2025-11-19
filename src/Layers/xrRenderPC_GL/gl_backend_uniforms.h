#pragma once

#include "xrCommon/xr_vector.h"
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <string_view>

#include "gl_program.h"

namespace xray::render::RENDER_NAMESPACE
{

struct UniformBufferObject
{
    GLuint id = GL_NONE;
    GLenum flags = GL_NONE;
    GLsizeiptr size = GL_NONE;

    void* data = nullptr;
};


class CBackendUniforms
{
protected:
    void initializeUniforms();
public:

    void set_uniforms(GLuint program, GLint location, xr_vector<glm::vec4>& uniforms);

    static void uniformBufferObjectGenerate(UniformBufferObject& ubo);
    static void uniformBufferObjectRegisterWithProgram(Program& program, std::string_view location, uint32_t blockBinding, const UniformBufferObject& ubo);
    static void uniformBufferObjectRegisterWithProgram(ref_vs& program, std::string_view location, uint32_t blockBinding, const UniformBufferObject& ubo);

    static void uniformBufferObjectPushToDevice(const UniformBufferObject& ubo)
    {
        uniformBufferObjectPushToDevice(ubo, ubo.size, ubo.data);
    }
    static void uniformBufferObjectPushToDevice(const UniformBufferObject& ubo, GLsizeiptr size, void* data);

    [[nodiscard]] uint calculateMaxUniformVertexSize(size_t sizeOfElement, uint max) const
    {
        return std::min(static_cast<uint>(maxUniformVertexSize / std::max(sizeOfElement, sizeof(float))), max);
    }

private:
    GLint maxUniformVertexSize = GL_NONE;
    GLint maxUniformBlockSize = GL_NONE;

    template<typename TYPE>
    void _set_uniforms(GLuint program, GLint location, xr_vector<TYPE>& uniforms);
};

}
