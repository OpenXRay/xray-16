#pragma once

#include "xrCore/xr_resource.h"

namespace xray::render::fg
{
class ECORE_API R_constant_table : public xr_resource_flagged
{
public:
    bool dx9compatibility{};
};
} // namespace xray::render::fg
