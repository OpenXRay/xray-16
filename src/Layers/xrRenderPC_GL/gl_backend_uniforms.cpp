#include "stdafx.h"
#include "gl_backend_uniforms.h"

#include <glm/glm.hpp>


namespace xray::render::RENDER_NAMESPACE
{
void CBackendUniforms::uniformBufferObjectGenerate(UniformBufferObject& info)
{
    VERIFY(info.id == GL_NONE);

    CHK_GL(glGenBuffers(1, &info.id));
    CHK_GL(glBindBuffer(GL_UNIFORM_BUFFER, info.id));
    CHK_GL(glBufferData(GL_UNIFORM_BUFFER, info.size, nullptr, info.flags));

    CHK_GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void CBackendUniforms::uniformBufferObjectRegisterWithProgram(Program& program, const std::string_view location, const uint32_t blockBinding, const UniformBufferObject& ubo)
{
    // Only single bit is 1
    VERIFY(blockBinding != 0 && (blockBinding & (blockBinding - 1)) == 0);
    //VERIFY(program.blockBindingSlots & 1 << blockBinding == 0);

    if (program.blockBindingSlots & 1 << blockBinding)
    {
        // UBO already registered with this program
        return;
    }

    const GLuint blockIndex = glGetUniformBlockIndex(program.id, location.data());
    VERIFY(blockIndex != GL_INVALID_INDEX);
    CHK_GL(glUniformBlockBinding(program.id, blockIndex, blockBinding));
    CHK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, blockBinding, ubo.id, 0,ubo.size));

    program.blockBindingSlots |= 1 << blockBinding;

    CHK_GL(glBindBuffer(GL_UNIFORM_BUFFER, ubo.id));
}

void CBackendUniforms::uniformBufferObjectPushToDevice(const UniformBufferObject& ubo)
{
    CHK_GL(glBindBuffer(GL_UNIFORM_BUFFER, ubo.id));
    CHK_GL(glBufferSubData(GL_UNIFORM_BUFFER, 0, ubo.size, ubo.data));
    //CHK_GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void CBackendUniforms::set_uniforms(GLuint program, GLint location, xr_vector<glm::vec4>& uniforms)
{
    _set_uniforms<glm::vec4>(program, location, uniforms);
}

template<typename TYPE>
void CBackendUniforms::_set_uniforms(GLuint program, GLint location, xr_vector<TYPE>& uniforms)
{
    if constexpr (std::is_same_v<TYPE, glm::vec4>)
    {
        CHK_GL(glProgramUniform4fv(program, location, uniforms.size(), reinterpret_cast<GLfloat*>(uniforms.data())));
    }
}
}
