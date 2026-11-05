#include "../src/Layers/xrRenderVK/VulkanDiagnostics.h"
#include <cassert>
using namespace xray::render::vulkan;
int main()
{
    assert(parse_diagnostic_mode("-cop -vk-diagnostic=16 -nolog") == 16);
    for (auto bad : {"-vk-diagnostic=17", "-vk-diagnostic=1bad", "x-vk-diagnostic=2"}) assert(parse_diagnostic_mode(bad) == 0);
    for (int mode : {15,16})
    {
        diagnostics() = {}; diagnostic_record_request().store(false);
        assert(!update_diagnostics(mode == 15 ? "-vk-diagnostic=15" : "-vk-diagnostic=16", 1000, true));
        assert(!update_diagnostics(nullptr, 120000, true)); // No automatic recording or long wait.
        assert(diagnostics().mode == mode && !diagnostics().recording);
        diagnostic_record_request().store(true);
        for (unsigned i=0; i<20; ++i)
        {
            assert(update_diagnostics(nullptr, 120000+i*500, true));
            assert(diagnostics().recording && diagnostics().mode == mode);
            assert(!update_diagnostics(nullptr, 120001+i*500, true));
        }
        assert(!update_diagnostics(nullptr, 130000, true));
        assert(!diagnostics().recording && diagnostics().captures == 20);
        diagnostic_record_request().store(true);
        assert(update_diagnostics(nullptr, 140000, true)); // Repeat without restarting the game.
        assert(!update_diagnostics(nullptr, 140001, false));
        assert(!diagnostics().recording); // Loading/menu cancels the burst.
    }
}
