#include "stdafx.h"
#include "Render.h"

// resources
IRender_Light::~IRender_Light() { GEnv.Render->light_destroy(this); }
IRender_Glow::~IRender_Glow() { GEnv.Render->glow_destroy(this); }
