#pragma once

struct ID3D11Resource;

namespace xray::render::fg
{
ID3D11Resource* texture_load(pcstr fname, u32& msize);
}
