#include "stdafx.h"
#include "render.h"

// resources
IRender_Light::~IRender_Light()
{
    ::Render->light_destroy(this);
}
IRender_Glow::~IRender_Glow()
{
    ::Render->glow_destroy(this);
}
