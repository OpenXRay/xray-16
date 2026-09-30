#pragma once

#include <string>

namespace xray::render::vulkan
{
// Tests that the system Vulkan loader can create an instance and enumerate at
// least one physical device. Surface and presentation support are checked once
// the engine has created its SDL Vulkan window.
bool probe_vulkan_loader(std::string& error);
}
