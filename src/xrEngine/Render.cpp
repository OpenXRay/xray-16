#include "stdafx.h"
#include "Render.h"

// resources
IRender_Light::~IRender_Light() { GEnv.Render->light_destroy(this); }
IRender_Glow::~IRender_Glow() { GEnv.Render->glow_destroy(this); }

IRender::ScopedContext::ScopedContext(RenderContext context)
{
    previousContext = GEnv.Render->GetCurrentContext();
    GEnv.Render->MakeContextCurrent(context);
}

IRender::ScopedContext::~ScopedContext()
{
    GEnv.Render->MakeContextCurrent(previousContext);
}
