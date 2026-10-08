#pragma once
#include <cstdint>
#include <vector>
class IReader;
namespace xray::render::vulkan
{
std::vector<uint8_t> particle_library_bytes(IReader& source);
}
