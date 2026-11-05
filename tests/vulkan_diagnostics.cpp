#include "../src/Layers/xrRenderVK/VulkanDiagnostics.h"
#include <cassert>
using namespace xray::render::vulkan;
int main()
{
    assert(parse_diagnostic_mode("-cop -vk-diagnostic=15 -nolog") == 15);
    assert(parse_diagnostic_mode("-vk-diagnostic=0") == 0);
    for (auto bad : {"-vk-diagnostic=99", "-vk-diagnostic=1bad", "x-vk-diagnostic=2", "-vk-diagnostic="})
        assert(parse_diagnostic_mode(bad) == 0);
    diagnostics() = {};
    assert(!update_diagnostics("", 1000, true));
    assert(!update_diagnostics("", 10000, true));
    assert(diagnostics().mode == 0);
    diagnostics() = {};
    assert(!update_diagnostics("-vk-diagnostic=15", 1000, false));
    constexpr int expected[]{0, 3, 4, 5, 6, 13, 14, 7, 8, 9, 10, 11, 12, 2};
    for (unsigned i = 0; i < 28; ++i)
    {
        const uint64_t begin = 1000 + i * 8000;
        assert(!update_diagnostics(nullptr, begin, true));
        assert(diagnostics().mode == expected[i % 14]);
        assert(!update_diagnostics(nullptr, begin + 5999, true));
        assert(update_diagnostics(nullptr, begin + 6000, true));
        assert(!update_diagnostics(nullptr, begin + 6001, true));
    }
    assert(!update_diagnostics(nullptr, 300000, false));
    assert(!update_diagnostics(nullptr, 400000, true));
    assert(diagnostics().mode == 0); // Loading/menu time never advances the cycle.
    diagnostics() = {};
    assert(!update_diagnostics("-vk-diagnostic=2", 1000, true));
    assert(update_diagnostics(nullptr, 7000, true));
    assert(!update_diagnostics(nullptr, 15000, true));
    assert(diagnostics().mode == 2); // Fixed mode saves once, not every eight seconds.
}
