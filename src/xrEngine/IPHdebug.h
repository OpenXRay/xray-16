#pragma once

#include "xrCore/_types.h"

xr_pure_interface IPhDebugRender
{
    virtual void open_cashed_draw() = 0;
    virtual void close_cashed_draw(u32 remove_time) = 0;
    virtual void draw_tri(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 c, bool solid) = 0;
};

extern ENGINE_API IPhDebugRender* ph_debug_render;
