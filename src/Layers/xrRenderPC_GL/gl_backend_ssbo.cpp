#include "stdafx.h"
#include "gl_backend_extension.h"

namespace xray::render::RENDER_NAMESPACE
{
void CBackendExtension::ssboGenerate(SharedStorageBufferObject& ssbo)
{
    VERIFY(ssbo.id == GL_NONE);

    CHK_GL(glGenBuffers(1, &ssbo.id));
    CHK_GL(glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo.id));
    CHK_GL(glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo.size, nullptr, ssbo.flags));

    CHK_GL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo.bindingIndex, ssbo.id));

#ifdef DEBUG
    CHK_GL(glObjectLabel(GL_BUFFER, ssbo.id, -1, ssbo.name.c_str()));
#endif
}

void CBackendExtension::ssboDispose(UniformBufferObject& ssbo)
{
    VERIFY(ssbo.id == GL_NONE);
    CHK_GL(glDeleteBuffers(1, &ssbo.id));
    ssbo.id = GL_NONE;
}

void CBackendExtension::ssboPushToDevice(const SharedStorageBufferObject& ssbo, GLsizeiptr size, void* data)
{
    // TODO keep track ?
    CHK_GL(glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo.id));
    CHK_GL(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, ssbo.flags));
//    CHK_GL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo.bindingIndex, ssbo.id));
}
}
