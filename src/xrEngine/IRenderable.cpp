#include "stdafx.h"

#include "IRenderable.h"
#include "Render.h"

#include "xrCDB/ISpatial.h"

#include <atomic>

static std::atomic<u64> g_renderableLifetimeCounter{1};

RenderableBase::RenderableBase()
{
    renderable.xform.identity();
    renderable.visual = NULL;
    renderable.pROS = NULL;
    renderable.lifetimeSerial = g_renderableLifetimeCounter.fetch_add(1, std::memory_order_relaxed);
    renderable.pROS_Allowed = true;
    renderable.invisible = false;
    renderable.hud = false;
    ISpatial* self = dynamic_cast<ISpatial*>(this);
    if (self)
        self->GetSpatialData().type |= STYPE_RENDERABLE;
}

extern ENGINE_API bool g_bRendering;
RenderableBase::~RenderableBase()
{
    VERIFY(!g_bRendering);
    GEnv.Render->model_Delete(renderable.visual);
    if (renderable.pROS)
        GEnv.Render->ros_destroy(renderable.pROS);
    renderable.visual = NULL;
    renderable.pROS = NULL;
}

IRender_ObjectSpecific* RenderableBase::renderable_ROS()
{
    if (0 == renderable.pROS && renderable.pROS_Allowed)
        renderable.pROS = GEnv.Render->ros_create(this);
    return renderable.pROS;
}
