#pragma once

#include "SkeletonMotions.h"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct MotionKey
{
    std::array<float, 4> rotation{0.f, 0.f, 0.f, 1.f};
    std::array<float, 3> translation{};
};

// Mutable playback state belongs to a visual instance; the decoded tracks
// are immutable and can be shared by every copy of that visual.
class MotionPlayback
{
public:
    struct Handle { size_t slot{}, definition{}; };
    using Finished = std::function<void()>;

    explicit MotionPlayback(std::shared_ptr<const std::vector<MotionSlot>> slots,
        std::vector<uint16_t> parents = {});
    bool find(const std::string& name, bool fx, Handle& result) const;
    bool play(Handle handle, bool fx, bool mixing = true, float power = 1.f,
        Finished finished = {}, float accrue = -1.f, float falloff = -1.f,
        float speed = -1.f, bool noloop = false, uint8_t channel = 0,
        uint16_t part_override = UINT16_MAX);
    void set_channel_factor(uint8_t channel, float factor);
    void stop_cycles(uint16_t part = UINT16_MAX, uint8_t channel_mask = 0xff);
    void advance(float seconds);
    bool sample(uint16_t bone, MotionKey& key) const;
    void reset();
    void clear_callbacks();
    size_t active_count() const { return active_.size(); }

private:
    struct Active
    {
        Handle handle;
        float time{}, weight{}, power{}, accrue{}, falloff{}, speed{};
        bool fx{}, stopping{}, stop_at_end{};
        uint8_t channel{};
        uint16_t part{UINT16_MAX};
        Finished finished;
    };
    bool affects(const Active& active, uint16_t bone) const;
    std::shared_ptr<const std::vector<MotionSlot>> slots_;
    std::vector<uint16_t> parents_;
    std::vector<Active> active_;
    std::array<float, 4> channel_factors_{1.f, 1.f, 1.f, 1.f};
};

bool sample_motion_key(const MotionClip& clip, uint16_t bone, float seconds, MotionKey& key);
} // namespace xray::render::vulkan
