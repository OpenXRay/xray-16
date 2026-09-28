#pragma once
#include <array>
#include <chrono>

namespace xray::render
{
class FramePhaseProfile
{
    using Clock = std::chrono::steady_clock;
    struct Totals
    {
        std::array<double, 7> sum{};
        std::array<double, 7> peak{};
        unsigned frames{};
        Clock::time_point report = Clock::now();
    };
    static Totals& totals() { static Totals value; return value; }
    Clock::time_point mark = Clock::now();
public:
    void end(unsigned phase)
    {
        auto now = Clock::now();
        const double ms = std::chrono::duration<double, std::milli>(now - mark).count();
        auto& t = totals();
        t.sum[phase] += ms;
        t.peak[phase] = std::max(t.peak[phase], ms);
        mark = now;
    }
    ~FramePhaseProfile()
    {
        end(6);
        auto& t = totals();
        ++t.frames;
        if (mark - t.report < std::chrono::seconds(5)) return;
        Msg("[render-phases] cpu-ms frames=%u setup=%.2f/%.2f sync=%.2f/%.2f geometry=%.2f/%.2f "
            "visibility=%.2f/%.2f sun-rain=%.2f/%.2f lights=%.2f/%.2f combine=%.2f/%.2f (avg/max)",
            t.frames, t.sum[0]/t.frames, t.peak[0], t.sum[1]/t.frames, t.peak[1],
            t.sum[2]/t.frames, t.peak[2], t.sum[3]/t.frames, t.peak[3],
            t.sum[4]/t.frames, t.peak[4], t.sum[5]/t.frames, t.peak[5], t.sum[6]/t.frames, t.peak[6]);
        t = {};
    }
};
}
