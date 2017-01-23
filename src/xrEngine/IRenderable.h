#ifndef IRENDERABLE_H_INCLUDED
#define IRENDERABLE_H_INCLUDED

#include "Render.h"

//////////////////////////////////////////////////////////////////////////
// definition ("Renderable")

class RenderData
{
public:
    Fmatrix xform;
    IRenderVisual *visual;
    IRender_ObjectSpecific *pROS;
    BOOL pROS_Allowed;
};

class IRenderable
{
public:
    virtual ~IRenderable() = 0;
    virtual RenderData &GetRenderData() = 0;
    virtual void renderable_Render() = 0;
    virtual IRender_ObjectSpecific *renderable_ROS() = 0;
    virtual BOOL renderable_ShadowGenerate() = 0;
    virtual BOOL renderable_ShadowReceive() = 0;
};

inline IRenderable::~IRenderable() {}

class ENGINE_API RenderableBase : public virtual IRenderable
{
public:
    RenderData renderable;
public:
    RenderableBase();
    virtual ~RenderableBase();
    virtual RenderData &GetRenderData() override final { return renderable; }
    virtual IRender_ObjectSpecific *renderable_ROS() override final;
    BENCH_SEC_SCRAMBLEVTBL2
    virtual BOOL renderable_ShadowGenerate() override { return FALSE; }
    virtual BOOL renderable_ShadowReceive() override { return FALSE; }
};

#endif // IRENDERABLE_H_INCLUDED