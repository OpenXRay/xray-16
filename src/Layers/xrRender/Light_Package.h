#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class light;
class light_Package
{
public:
    xr_vector<light*> v_point;
    xr_vector<light*> v_spot;
    xr_vector<light*> v_shadowed;

public:
    void clear();
    void sort();
};
} // namespace xray::render::RENDER_NAMESPACE
