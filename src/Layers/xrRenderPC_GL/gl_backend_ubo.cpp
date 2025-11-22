#include "stdafx.h"
#include "gl_backend_extension.h"

#include <glm/glm.hpp>

namespace xray::render::RENDER_NAMESPACE
{
GLuint CBackendExtension::uboCurrentId = GL_INVALID_INDEX;

GLint CBackendExtension::maxUniformVertexSize = GL_NONE;
GLint CBackendExtension::maxUniformBlockSize = GL_NONE;
GLint CBackendExtension::ssboAlignment = GL_NONE;

void CBackendExtension::bindBuffer(const UniformBufferObject& ubo)
{
    VERIFY(ubo.id != GL_NONE);
    if (uboCurrentId != ubo.id)
    {
        uboCurrentId = ubo.id;
        CHK_GL(glBindBuffer(GL_UNIFORM_BUFFER, ubo.id));
    }
}

void CBackendExtension::uboGenerate(UniformBufferObject& ubo)
{
    VERIFY(ubo.id == GL_NONE);

    CHK_GL(glGenBuffers(1, &ubo.id));
    bindBuffer(ubo);
    CHK_GL(glBufferData(GL_UNIFORM_BUFFER, ubo.size, nullptr, ubo.flags));

#ifdef DEBUG
    CHK_GL(glObjectLabel(GL_BUFFER, ubo.id, -1, ubo.name.c_str()));
#endif
}

void CBackendExtension::uboDispose(UniformBufferObject& ubo)
{
    VERIFY(ubo.id != GL_NONE);
    CHK_GL(glDeleteBuffers(1, &ubo.id));
    ubo.id = GL_NONE;
}

void CBackendExtension::uboRegisterWithProgram(ref_vs& program, const std::string_view location, const uint32_t blockBinding, const UniformBufferObject& ubo)
{
    VERIFY(program);
    VERIFY(program->sh);

    // Only single bit is 1
    //VERIFY(blockBinding == 0 || (blockBinding & (blockBinding - 1)) == 0);
    //VERIFY(program.bindingSlots & 1 << blockBinding == 0);

    if (program->bindingSlots & 1 << blockBinding)
    {
        // UBO already registered with this program
        return;
    }

    bindBuffer(ubo);
    const GLuint blockIndex = glGetUniformBlockIndex(program->sh, location.data());
    VERIFY(blockIndex != GL_INVALID_INDEX);
    CHK_GL(glUniformBlockBinding(program->sh, blockIndex, blockBinding));
    CHK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, blockBinding, ubo.id, 0,ubo.size));

    program->bindingSlots |= 1 << blockBinding;
}

void CBackendExtension::uboPushToDevice(const UniformBufferObject& ubo, GLsizeiptr size, void* data)
{
    VERIFY(ubo.id != GL_NONE);

    bindBuffer(ubo);
    CHK_GL(glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data));
}

void CBackendExtension::uboBindRange(const UniformBufferObject &ubo, GLuint blockBinding, GLintptr offset, GLsizeiptr size)
{
    VERIFY(ubo.id != GL_NONE);

    CHK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, blockBinding, ubo.id, offset, size));
}

void CBackendExtension::setUniforms(GLuint program, GLint location, xr_vector<glm::vec4>& uniforms)
{
    _setUniforms<glm::vec4>(program, location, uniforms);
}

template<typename TYPE>
void CBackendExtension::_setUniforms(GLuint program, GLint location, xr_vector<TYPE>& uniforms)
{
    if constexpr (std::is_same_v<TYPE, glm::vec4>)
    {
        CHK_GL(glProgramUniform4fv(program, location, uniforms.size(), reinterpret_cast<GLfloat*>(uniforms.data())));
    }
}

void CBackendExtension::initializeUniforms()
{
    CHK_GL(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxUniformVertexSize));
    CHK_GL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize));
    CHK_GL(glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &ssboAlignment));

    Msg("maxUniformVertexSize=%d maxUniformBlockSize=%d ssboAlignment=%d", maxUniformVertexSize, maxUniformBlockSize, ssboAlignment);
}
}
