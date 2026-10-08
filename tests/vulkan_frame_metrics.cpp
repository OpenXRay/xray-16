#include "src/Layers/xrRenderVK/VulkanFrameMetrics.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

using namespace xray::render::vulkan;

int main()
{
    VulkanFrameMetrics metrics;
    for (int i = 0; i < 100; ++i)
        metrics.record({40., i < 94 ? 10. : 60., i % 2 ? std::optional<double>{5.} : std::nullopt,
            static_cast<uint64_t>(i + 1), 200, 3});
    const auto summary = metrics.summary();
    assert(summary.frames == 100 && summary.fps == 25.);
    assert(summary.cpu_p95_ms == 60. && summary.gpu_p95_ms.value() == 5.);
    assert(summary.gpu_unavailable_frames == 50);
    assert(summary.texture_bytes == 100 && summary.peak_texture_bytes == 100);
    assert(metrics.write_json("vulkan-frame-metrics-test.json"));
    std::ifstream input("vulkan-frame-metrics-test.json");
    const std::string json((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    assert(json.find("\"gpu_unavailable_frames\": 50") != std::string::npos);
    std::remove("vulkan-frame-metrics-test.json");

    VulkanFrameMetrics unsupported;
    unsupported.record({33., 5., std::nullopt, 0, 0, 0});
    assert(!unsupported.summary().gpu_p95_ms);
    assert(unsupported.summary().gpu_unavailable_frames == 1);
}
