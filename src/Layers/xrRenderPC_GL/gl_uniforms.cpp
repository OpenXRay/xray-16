#include "stdafx.h"
#include <glm/glm.hpp>
#include "xrCommon/xr_vector.h"

namespace xray::render::RENDER_NAMESPACE
{
void CBackend::set_uniforms(GLuint program, GLint location, xr_vector<glm::vec4>& uniforms)
{
    _set_uniforms<glm::vec4>(program, location, uniforms);
}

template<typename TYPE>
void CBackend::_set_uniforms(GLuint program, GLint location, xr_vector<TYPE>& uniforms)
{
    if constexpr (std::is_same_v<TYPE, glm::vec4>)
    {
        CHK_GL(glProgramUniform4fv(program, location, uniforms.size(), reinterpret_cast<GLfloat*>(uniforms.data())));
    }
}
}
