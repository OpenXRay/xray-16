#pragma once

#include <cstdint>

class FramePacer
{
    std::uint64_t m_lastFrameNs{};
    bool m_started{};

public:
    std::uint64_t GetDelayNs(std::uint64_t nowNs, std::uint32_t fpsLimit) const
    {
        if (!m_started || fpsLimit == 0)
            return 0;

        const std::uint64_t intervalNs = (1'000'000'000ull + fpsLimit - 1) / fpsLimit;
        const std::uint64_t elapsedNs = nowNs - m_lastFrameNs;
        return elapsedNs < intervalNs ? intervalNs - elapsedNs : 0;
    }

    std::uint64_t StartFrame(std::uint64_t nowNs)
    {
        const std::uint64_t elapsedNs = m_started ? nowNs - m_lastFrameNs : 0;
        m_lastFrameNs = nowNs;
        m_started = true;
        return elapsedNs;
    }

    void Reset()
    {
        m_started = false;
    }
};
