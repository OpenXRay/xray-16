#pragma once

namespace xray::render::RENDER_NAMESPACE
{
void xrStripify(xr_vector<u16>& indices, xr_vector<u16>& perturb, int iCacheSize, int iMinStripLength);
int xrSimulate(xr_vector<u16>& indices, int iCacheSize);
} // namespace xray::render::RENDER_NAMESPACE
