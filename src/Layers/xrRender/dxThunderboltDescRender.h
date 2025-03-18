#pragma once

#include "Include/xrRender/ThunderboltDescRender.h"

namespace xray::render::RENDER_NAMESPACE
{
class IRender_DetailModel;

class dxThunderboltDescRender : public IThunderboltDescRender
{
public:
    virtual void Copy(IThunderboltDescRender& _in);

    virtual void CreateModel(LPCSTR m_name);
    virtual void DestroyModel();
    // private:
public:
    IRender_DetailModel* l_model;
};
} // namespace xray::render::RENDER_NAMESPACE
