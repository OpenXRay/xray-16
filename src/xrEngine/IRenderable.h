#ifndef IRENDERABLE_H_INCLUDED
#define IRENDERABLE_H_INCLUDED

#include "Render.h"

//////////////////////////////////////////////////////////////////////////
// definition ("Renderable")

class RenderData
{
public:
    Fmatrix xform;
    IRenderVisual* visual;
    IRender_ObjectSpecific* pROS;
    bool pROS_Allowed;
    bool invisible; // object should be invisible on the scene graph
    bool hud; // At the current moment, object is being rendered on HUD
};

class XR_NOVTABLE IRenderable
{
public:
    virtual ~IRenderable() = 0;
    virtual RenderData& GetRenderData() = 0;
    virtual void renderable_Render(u32 context_id, IRenderable* root) = 0;
    virtual IRender_ObjectSpecific* renderable_ROS() = 0;
    virtual bool renderable_ShadowGenerate() = 0;
    virtual bool renderable_ShadowReceive() = 0;
    virtual bool renderable_Invisible() = 0;
    virtual void renderable_Invisible(bool value) = 0;
    virtual bool renderable_HUD() = 0;
    virtual void renderable_HUD(bool value) = 0;
};

inline IRenderable::~IRenderable() = default;

// XXX: can't be NOVTABLE because of dynamic_cast in the constructor.. Fix some day
class ENGINE_API /*XR_NOVTABLE*/ RenderableBase : public virtual IRenderable
{
public:
    RenderData renderable;

public:
    RenderableBase();
    virtual ~RenderableBase();
    virtual RenderData& GetRenderData() override final { return renderable; }
    virtual IRender_ObjectSpecific* renderable_ROS() override final;
    virtual bool renderable_ShadowGenerate() override { return false; }
    virtual bool renderable_ShadowReceive() override { return false; }
    bool renderable_Invisible() override { return renderable.invisible; }
    void renderable_Invisible(bool value) override { renderable.invisible = value; }
    bool renderable_HUD() override { return renderable.hud; }
    void renderable_HUD(bool value) override { renderable.hud = value; }
};

#endif // IRENDERABLE_H_INCLUDED
