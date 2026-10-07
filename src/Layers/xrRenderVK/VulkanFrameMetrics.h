#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct VulkanFrameSample
{
    double frame_ms{};
    double cpu_ms{};
    std::optional<double> gpu_ms;
    uint64_t texture_bytes{};
    uint64_t geometry_bytes{};
    uint32_t draws{};
};

class VulkanFrameMetrics
{
public:
    uint64_t frame_count() const { return total_frames_; }
    struct Summary
    {
        uint64_t frames{};
        double fps{};
        double frame_p95_ms{};
        double cpu_p95_ms{};
        std::optional<double> gpu_p95_ms;
        uint64_t texture_bytes{}, geometry_bytes{};
        uint64_t peak_texture_bytes{}, peak_geometry_bytes{};
        uint64_t gpu_unavailable_frames{};
    };

    void record(VulkanFrameSample sample)
    {
        if (!std::isfinite(sample.frame_ms) || sample.frame_ms <= 0. ||
            !std::isfinite(sample.cpu_ms) || sample.cpu_ms < 0.) return;
        if (sample.gpu_ms && (!std::isfinite(*sample.gpu_ms) || *sample.gpu_ms < 0.))
            sample.gpu_ms.reset();
        if (samples_.size() == 36000) samples_.pop_front();
        samples_.push_back(sample);
        ++total_frames_;
        peak_texture_bytes_ = std::max(peak_texture_bytes_, sample.texture_bytes);
        peak_geometry_bytes_ = std::max(peak_geometry_bytes_, sample.geometry_bytes);
    }

    Summary summary() const
    {
        Summary result;
        result.frames = total_frames_;
        result.peak_texture_bytes = peak_texture_bytes_;
        result.peak_geometry_bytes = peak_geometry_bytes_;
        if (samples_.empty()) return result;
        result.texture_bytes = samples_.back().texture_bytes;
        result.geometry_bytes = samples_.back().geometry_bytes;
        std::vector<double> frame, cpu, gpu;
        double duration = 0.;
        for (const auto& sample : samples_)
        {
            duration += sample.frame_ms;
            frame.push_back(sample.frame_ms);
            cpu.push_back(sample.cpu_ms);
            if (sample.gpu_ms) gpu.push_back(*sample.gpu_ms);
            else ++result.gpu_unavailable_frames;
        }
        result.fps = 1000. * samples_.size() / duration;
        result.frame_p95_ms = percentile95(frame);
        result.cpu_p95_ms = percentile95(cpu);
        if (!gpu.empty()) result.gpu_p95_ms = percentile95(gpu);
        return result;
    }

    bool write_json(const std::string& path) const
    {
        std::ofstream out(path, std::ios::trunc);
        if (!out) return false;
        const auto stats = summary();
        out << "{\n  \"frames\": " << stats.frames << ",\n  \"fps\": " << stats.fps
            << ",\n  \"frame_p95_ms\": " << stats.frame_p95_ms
            << ",\n  \"cpu_p95_ms\": " << stats.cpu_p95_ms
            << ",\n  \"gpu_p95_ms\": ";
        if (stats.gpu_p95_ms) out << *stats.gpu_p95_ms; else out << "null";
        out << ",\n  \"gpu_unavailable_frames\": " << stats.gpu_unavailable_frames
            << ",\n  \"memory_bytes\": {\"textures\": " << stats.texture_bytes
            << ", \"geometry\": " << stats.geometry_bytes
            << ", \"peak_textures\": " << stats.peak_texture_bytes
            << ", \"peak_geometry\": " << stats.peak_geometry_bytes << "},\n  \"samples\": [\n";
        bool first = true;
        for (const auto& sample : samples_)
        {
            if (!first) out << ",\n";
            first = false;
            out << "    {\"frame_ms\": " << sample.frame_ms << ", \"cpu_ms\": " << sample.cpu_ms
                << ", \"gpu_ms\": ";
            if (sample.gpu_ms) out << *sample.gpu_ms; else out << "null";
            out << ", \"textures\": " << sample.texture_bytes
                << ", \"geometry\": " << sample.geometry_bytes
                << ", \"draws\": " << sample.draws << '}';
        }
        out << "\n  ]\n}\n";
        return out.good();
    }

private:
    static double percentile95(std::vector<double>& values)
    {
        std::sort(values.begin(), values.end());
        return values[static_cast<size_t>(std::ceil(values.size() * .95)) - 1];
    }
    std::deque<VulkanFrameSample> samples_;
    uint64_t total_frames_{}, peak_texture_bytes_{}, peak_geometry_bytes_{};
};
}
