#pragma once

namespace xray::render::fg
{
constexpr u32 occq_size_base = 768;
constexpr u32 occq_size = 2 * occq_size_base * R__NUM_PARALLEL_CONTEXTS;

class R_occlusion
{
public:
    typedef u64 occq_result;

    void occq_create(u32) {}
    void occq_destroy() {}
    u32 occq_begin(u32& ID) { ID = 0xFFFFFFFF; return 0; }
    void occq_end(u32&) {}
    occq_result occq_get(u32&) { return 0xffffffff; }
};
}
