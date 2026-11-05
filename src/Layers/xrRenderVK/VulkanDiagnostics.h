#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string_view>

namespace xray::render::vulkan
{
// Render-thread state. No polling, file access or shader work in normal mode.
struct VulkanDiagnostics
{
    int requested = 0, mode = 0, previous = -1;
    uint64_t start = 0, slot = 0, captured_slot = UINT64_MAX;
    uint32_t captures = 0;
    bool initialized = false, recording = false;
    uint64_t recording_start = 0;
};
inline VulkanDiagnostics& diagnostics() { static VulkanDiagnostics state; return state; }
inline std::atomic<bool>& diagnostic_record_request() { static std::atomic<bool> request{false}; return request; }
inline int parse_diagnostic_mode(std::string_view args)
{
    constexpr std::string_view prefix = "-vk-diagnostic=";
    while (!args.empty())
    {
        const auto end = args.find(' ');
        const auto token = args.substr(0, end);
        if (token.substr(0, prefix.size()) == prefix)
        {
            const auto value = token.substr(prefix.size());
            if (value.size() == 1 && value[0] >= '0' && value[0] <= '9') return value[0] - '0';
            if (value.size() == 2 && value[0] == '1' && value[1] >= '0' && value[1] <= '6') return 10 + value[1] - '0';
            return 0;
        }
        if (end == std::string_view::npos) break;
        args.remove_prefix(end + 1);
    }
    return 0;
}
inline const char* diagnostic_name(int mode)
{
    constexpr const char* names[]{"baseline", "profile", "particle-quads", "normals", "albedo", "baked-light",
        "surface", "no-local-lights", "no-shadows", "no-distortion", "full-trees", "tree-lod",
        "no-transparency", "sun-only", "indirect-only", "motion-lighting", "motion-water"};
    return mode >= 0 && mode <= 16 ? names[mode] : "invalid";
}
inline bool update_diagnostics(const char* args, uint64_t now, bool world)
{
    auto& state = diagnostics();
    if (!state.initialized)
    {
        state.requested = parse_diagnostic_mode(args ? args : "");
        state.initialized = true;
    }
    if (!state.requested) return false;
    if (!world) { state.recording = false; diagnostic_record_request().store(false); state.start = 0; state.previous = -1; state.mode = 0; state.captured_slot = UINT64_MAX; return false; }
    if (state.requested >= 15)
    {
        state.mode = state.requested;
        if (diagnostic_record_request().exchange(false))
        {
            state.recording = true; state.recording_start = now; state.captured_slot = UINT64_MAX;
        }
        if (!state.recording) return false;
        const uint64_t duration = now >= state.recording_start ? now - state.recording_start : 0;
        if (duration >= 10000) { state.recording = false; return false; }
        // Twenty samples over ten seconds. The live view updates every frame;
        // image readback is limited to 2 Hz and begins immediately on request.
        state.slot = duration / 500;
        if (state.captured_slot == state.slot) return false;
        state.captured_slot = state.slot; ++state.captures; return true;
    }
    if (!state.start) state.start = now ? now : 1;
    const uint64_t elapsed = now >= state.start ? now - state.start : 0;
    state.slot = elapsed / 8000;
    state.mode = state.requested;
    // One snapshot after six seconds of settling, never during menu/loading.
    const uint64_t capture_slot = 0;
    if (state.captures >= 112 || elapsed % 8000 < 6000 || state.captured_slot == capture_slot) return false;
    state.captured_slot = capture_slot;
    ++state.captures;
    return true;
}
}
