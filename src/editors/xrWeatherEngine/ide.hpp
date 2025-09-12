////////////////////////////////////////////////////////////////////////////
// Module : ide.hpp
// Created : 11.12.2007
// Modified : 29.12.2007
// Author : Dmitriy Iassenev
// Description : editor ide function
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Include/editor/ide.hpp"
#include "xrEngine/device.h"

namespace XRay
{
namespace Editor
{
//class ide_base;
} // namespace Editor
} // namespace XRay

inline XRay::Editor::ide_base& ide()
{
    VERIFY(Device.editor());
    return (*Device.editor());
}
