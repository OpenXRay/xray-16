#pragma once

#include <array>
#include <chrono>

namespace xray::render::RENDER_NAMESPACE
{
class GlFrameProfiler
{
    struct Slot { GLuint query{}; bool pending{}; bool valid{}; };
    std::array<Slot, 6> slots{};
    int active = -1;
    bool initialized{};
    bool supported{};
    double total{}, peak{};
    unsigned samples{}, skipped{};
    std::chrono::steady_clock::time_point report = std::chrono::steady_clock::now();
public:
    void begin()
    {
        if (!initialized)
        {
            initialized = true;
            supported = GLAD_GL_EXT_disjoint_timer_query && glGenQueriesEXT &&
                glBeginQueryEXT && glEndQueryEXT && glGetQueryivEXT &&
                glGetQueryObjectuivEXT && glGetQueryObjectui64vEXT && glDeleteQueriesEXT;
            if (supported)
            {
                GLint bits = 0;
                glGetQueryivEXT(GL_TIME_ELAPSED_EXT, GL_QUERY_COUNTER_BITS_EXT, &bits);
                supported = bits != 0;
            }
            if (supported) for (auto& slot : slots) glGenQueriesEXT(1, &slot.query);
            Msg("[gpu-time] asynchronous elapsed queries %s", supported ? "enabled" : "unavailable");
        }
        if (!supported) return;
        GLint disjoint = 0;
        glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjoint);
        for (auto& slot : slots)
        {
            if (disjoint) slot.valid = false;
            if (!slot.pending) continue;
            GLuint available = 0;
            glGetQueryObjectuivEXT(slot.query, GL_QUERY_RESULT_AVAILABLE_EXT, &available);
            if (!available) continue;
            if (slot.valid)
            {
                GLuint64 ns = 0;
                glGetQueryObjectui64vEXT(slot.query, GL_QUERY_RESULT_EXT, &ns);
                const double ms = ns / 1000000.0;
                total += ms;
                peak = std::max(peak, ms);
                ++samples;
            }
            slot.pending = false;
        }
        for (unsigned i = 0; i < slots.size(); ++i)
            if (!slots[i].pending)
            {
                active = static_cast<int>(i);
                slots[i].valid = !disjoint;
                glBeginQueryEXT(GL_TIME_ELAPSED_EXT, slots[i].query);
                return;
            }
        ++skipped;
    }
    void end()
    {
        if (active >= 0)
        {
            glEndQueryEXT(GL_TIME_ELAPSED_EXT);
            slots[active].pending = true;
            active = -1;
        }
        const auto now = std::chrono::steady_clock::now();
        if (supported && now - report >= std::chrono::seconds(5))
        {
            Msg("[gpu-time] render-avg=%.2fms render-max=%.2fms samples=%u skipped=%u",
                samples ? total / samples : 0.0, peak, samples, skipped);
            total = peak = 0;
            samples = skipped = 0;
            report = now;
        }
    }
    void destroy()
    {
        if (active >= 0) glEndQueryEXT(GL_TIME_ELAPSED_EXT);
        for (auto& slot : slots) if (slot.query) glDeleteQueriesEXT(1, &slot.query);
        *this = {};
    }
};
}
