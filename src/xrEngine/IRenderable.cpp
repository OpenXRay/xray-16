#include "stdafx.h"
#include "xrCDB/ISpatial.h"
#include "IRenderable.h"
// XXX: rename this file to RenderableBase.cpp
RenderableBase::RenderableBase()
{
    renderable.xform.identity();
    renderable.visual = NULL;
    renderable.pROS = NULL;
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
