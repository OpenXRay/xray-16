#pragma once
#include "../../../sdk/include/glad/gl.h"
#include "xrCommon/xr_string.h"

struct Program
{
    GLuint id = GL_NONE;
    uint32_t bindingSlots = 0;

#ifdef DEBUG
    xr_string name;
#endif
};
