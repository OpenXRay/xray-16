#include "stdafx.h"
#include "Render.h"

// resources
IRender_Light::~IRender_Light()
{
    GlobalEnv.Render->light_destroy(this);
}
IRender_Glow::~IRender_Glow()
{
    GlobalEnv.Render->glow_destroy(this);
}
