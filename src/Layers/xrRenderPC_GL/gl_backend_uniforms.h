#pragma once

#include "xrCommon/xr_vector.h"
#include <glm/glm.hpp>
#include <glad/gl.h>

namespace xray::render::RENDER_NAMESPACE
{

struct UniformBufferObject
{
    GLuint id = GL_NONE;
    GLenum flags = GL_NONE;
    GLsizeiptr size = GL_NONE;

    void* data = nullptr;
};

struct Program
{
    GLuint id = GL_NONE;
    uint32_t blockBindingSlots = 0;
};

class CBackendUniforms
{
public:
    void set_uniforms(GLuint program, GLint location, xr_vector<glm::vec4>& uniforms);

    static void uniformBufferObjectGenerate(UniformBufferObject& ubo);
    static void uniformBufferObjectRegisterWithProgram(Program& program, std::string_view location, uint32_t blockBinding, const UniformBufferObject& ubo);

    static void uniformBufferObjectPushToDevice(const UniformBufferObject& ubo);

private:
    template<typename TYPE>
    void _set_uniforms(GLuint program, GLint location, xr_vector<TYPE>& uniforms);

};

}
