#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace xray::render::vulkan
{
// A generation is a level or asset catalog load. The first occurrence keeps
// its full resource context in the log; retirement reports its total count.
class ResourceIssueCounter
{
public:
    bool record(const std::string& context)
    {
        return ++counts_[context] == 1;
    }

    template <typename Report>
    void report_repeats(Report report) const
    {
        for (const auto& [context, count] : counts_)
            if (count > 1) report(context, count);
    }

    void clear() { counts_.clear(); }

private:
    std::unordered_map<std::string, uint64_t> counts_;
};
}
