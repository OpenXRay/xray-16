#include "MotionPlayback.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
namespace
{
std::array<float, 4> quaternion(const std::array<int16_t, 4>& packed)
{
    std::array<float, 4> result;
    float length = 0.f;
    for (size_t i = 0; i < 4; ++i)
    {
        result[i] = packed[i] / 32767.f;
        length += result[i] * result[i];
    }
    if (length <= 0.f) return {0.f, 0.f, 0.f, 1.f};
    for (auto& component : result) component /= std::sqrt(length);
    return result;
}

std::array<float, 4> interpolate(std::array<float, 4> a, std::array<float, 4> b, float fraction)
{
    float dot = 0.f;
    for (size_t i = 0; i < 4; ++i) dot += a[i] * b[i];
    if (dot < 0.f)
        for (auto& component : b) component = -component;
    std::array<float, 4> out;
    float length = 0.f;
    for (size_t i = 0; i < 4; ++i)
    {
        out[i] = a[i] * (1.f - fraction) + b[i] * fraction;
        length += out[i] * out[i];
    }
    if (length <= 0.f) return {0.f, 0.f, 0.f, 1.f};
    for (auto& component : out) component /= std::sqrt(length);
    return out;
}
}

bool sample_motion_key(const MotionClip& clip, uint16_t bone, float seconds, MotionKey& key)
{
    if (bone >= clip.bones.size() || !clip.frames || !std::isfinite(seconds) || seconds < 0.f)
        return false;
    const auto& track = clip.bones[bone];
    if (track.rotations.size() != 1 && track.rotations.size() != clip.frames) return false;
    const float frame = std::fmod(seconds * 30.f, float(clip.frames));
    if (!std::isfinite(frame)) return false;
    const size_t first = static_cast<size_t>(std::floor(frame));
    const size_t next = (first + 1) % clip.frames;
    const float fraction = frame - std::floor(frame);
    const auto rotation = [&](size_t at)
    {
        return quaternion(track.rotations[track.rotations.size() == 1 ? 0 : at]);
    };
    MotionKey sampled;
    sampled.rotation = interpolate(rotation(first), rotation(next), fraction);
    for (size_t axis = 0; axis < 3; ++axis)
    {
        float position = track.translation_init[axis];
        if (track.flags & 1)
        {
            const auto value = [&](size_t at) -> float
            {
                return (track.flags & 4 ? float(track.translations16[at][axis]) :
                    float(track.translations8[at][axis])) * track.translation_size[axis] + position;
            };
            if ((track.flags & 4 ? track.translations16.size() : track.translations8.size()) != clip.frames)
                return false;
            sampled.translation[axis] = value(first) * (1.f - fraction) + value(next) * fraction;
        }
        else sampled.translation[axis] = position;
    }
    key = sampled;
    return true;
}

MotionPlayback::MotionPlayback(std::shared_ptr<const std::vector<MotionSlot>> slots,
    std::vector<uint16_t> parents)
    : slots_(std::move(slots)), parents_(std::move(parents)) {}

bool MotionPlayback::find(const std::string& name, bool fx, Handle& result) const
{
    if (!slots_) return false;
    for (size_t s = slots_->size(); s > 0; --s)
        for (size_t d = 0; d < (*slots_)[s - 1].definitions.size(); ++d)
        {
            const auto& def = (*slots_)[s - 1].definitions[d];
            if (def.name == name && bool(def.flags & 1) == fx)
            {
                result = {s - 1, d};
                return true;
            }
        }
    return false;
}

bool MotionPlayback::play(Handle handle, bool fx, bool mixing, float power, Finished finished,
    float accrue, float falloff, float speed, bool noloop, uint8_t channel,
    uint16_t part_override)
{
    if (!slots_ || handle.slot >= slots_->size() ||
        handle.definition >= (*slots_)[handle.slot].definitions.size() ||
        !std::isfinite(power) || power < 0.f || channel >= channel_factors_.size()) return false;
    const auto& def = (*slots_)[handle.slot].definitions[handle.definition];
    const auto& slot = (*slots_)[handle.slot];
    if (bool(def.flags & 1) != fx || def.motion >= slot.clips.size()) return false;
    const uint16_t part = part_override == UINT16_MAX ? def.bone_or_part : part_override;
    if (!fx && part != UINT16_MAX && part >= slot.partitions.size()) return false;
    const float replacement_falloff = falloff >= 0.f ? falloff : def.parameters[3];
    if (!fx)
        for (auto& prior : active_)
            if (!prior.fx && prior.channel == channel &&
                prior.part == part)
            {
                if (!mixing || replacement_falloff <= 0.f) prior.weight = 0.f;
                else prior.falloff = replacement_falloff;
                prior.stopping = true;
            }
    Active state;
    state.handle = handle;
    state.fx = fx;
    state.channel = channel;
    state.part = part;
    state.power = power * def.parameters[1];
    state.accrue = accrue >= 0.f ? accrue : def.parameters[2];
    state.falloff = falloff >= 0.f ? falloff : def.parameters[3];
    state.speed = speed >= 0.f ? speed : def.parameters[0];
    state.stop_at_end = fx || noloop || (def.flags & 2);
    state.weight = mixing ? 0.f : state.power;
    state.finished = std::move(finished);
    active_.push_back(std::move(state));
    return true;
}

void MotionPlayback::stop_cycles(uint16_t part, uint8_t channel_mask)
{
    for (auto& active : active_)
    {
        if (active.fx || !(channel_mask & (1u << active.channel))) continue;
        if (part == UINT16_MAX || active.part == part) active.stopping = true;
    }
}

void MotionPlayback::advance(float seconds)
{
    if (!std::isfinite(seconds) || seconds < 0.f) return;
    for (auto& active : active_)
    {
        const auto& slot = (*slots_)[active.handle.slot];
        const auto& def = slot.definitions[active.handle.definition];
        const float duration = slot.clips[def.motion].frames / 30.f;
        active.time += seconds * active.speed;
        if (active.time >= duration && active.stop_at_end)
        {
            active.time = duration > 1.f / 30.f ? duration - 1.f / 30.f : 0.f;
            if (!active.stopping && active.finished) active.finished();
            active.stopping = true;
        }
        else if (duration > 0.f && active.time >= duration)
            active.time = std::fmod(active.time, duration);
        if (active.stopping)
            active.weight = std::max(0.f, active.weight - seconds * active.falloff * active.power);
        else
            active.weight = std::min(active.power, active.weight + seconds * active.accrue * active.power);
    }
    active_.erase(std::remove_if(active_.begin(), active_.end(),
        [](const Active& active) { return active.stopping && active.weight <= 0.f; }), active_.end());
}

bool MotionPlayback::affects(const Active& active, uint16_t bone) const
{
    const auto& slot = (*slots_)[active.handle.slot];
    const auto& def = slot.definitions[active.handle.definition];
    if (active.fx)
    {
        if (def.bone_or_part == UINT16_MAX) return true;
        uint16_t current = bone;
        for (size_t depth = 0; depth <= parents_.size(); ++depth)
        {
            if (current == def.bone_or_part) return true;
            if (current >= parents_.size()) break;
            current = parents_[current];
        }
        return false;
    }
    if (active.part == UINT16_MAX) return true;
    if (active.part >= slot.partitions.size()) return false;
    const auto& part = slot.partitions[active.part];
    return std::find(part.begin(), part.end(), bone) != part.end();
}

bool MotionPlayback::sample(uint16_t bone, MotionKey& key) const
{
    MotionKey accumulated;
    accumulated.rotation = {0.f, 0.f, 0.f, 0.f};
    float weight = 0.f;
    for (const auto& active : active_)
    {
        const float effective_weight = active.weight * channel_factors_[active.channel];
        if (effective_weight <= 0.f || !affects(active, bone)) continue;
        const auto& slot = (*slots_)[active.handle.slot];
        MotionKey current;
        if (!sample_motion_key(slot.clips[slot.definitions[active.handle.definition].motion],
                bone, active.time, current)) continue;
        float sign = 1.f;
        if (weight > 0.f)
        {
            float dot = 0.f;
            for (size_t axis = 0; axis < 4; ++axis)
                dot += accumulated.rotation[axis] * current.rotation[axis];
            if (dot < 0.f) sign = -1.f;
        }
        for (size_t axis = 0; axis < 4; ++axis)
            accumulated.rotation[axis] += current.rotation[axis] * effective_weight * sign;
        for (size_t axis = 0; axis < 3; ++axis)
            accumulated.translation[axis] += current.translation[axis] * effective_weight;
        weight += effective_weight;
    }
    if (weight <= 0.f) return false;
    float magnitude = 0.f;
    for (float component : accumulated.rotation) magnitude += component * component;
    if (magnitude <= 0.f) return false;
    for (auto& component : accumulated.rotation) component /= std::sqrt(magnitude);
    for (auto& component : accumulated.translation) component /= weight;
    key = accumulated;
    return true;
}

void MotionPlayback::reset() { active_.clear(); }
void MotionPlayback::set_channel_factor(uint8_t channel, float factor)
{
    if (channel < channel_factors_.size() && std::isfinite(factor))
        channel_factors_[channel] = std::clamp(factor, 0.f, 1.f);
}
void MotionPlayback::clear_callbacks()
{
    for (auto& active : active_) active.finished = {};
}
} // namespace xray::render::vulkan
