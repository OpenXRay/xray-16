#pragma once
#include "xrCommon/xr_string.h"

namespace xray
{
namespace core
{
namespace detail
{
class XRCORE_API string_tupples
{
    template <size_t... Ind, typename... Args>
    void process_args(const std::tuple<Args...>& args, std::index_sequence<Ind...>)
    {
        (helper<Ind>::add_string(*this, std::get<Ind>(args)), ...);
    }

public:
    template <typename... Args>
    string_tupples(const Args... args) : m_count(sizeof...(Args))
    {
        process_args(std::make_tuple(args...), std::index_sequence_for<Args...>{});
    }

    void error_process() const;

    size_t size() const
    {
        VERIFY(m_count > 0);

        size_t result = m_strings[0].second;
        for (size_t j = 1; j < m_count; ++j)
            result += m_strings[j].second;

        if (result > MAX_CONCAT_RESULT_SIZE)
        {
            error_process();
        }

        return ((result + 1) * sizeof(*m_strings[0].first));
    }

    void concat(pcstr const result) const
    {
        VERIFY(m_count > 0);

        pstr i = const_cast<pstr>(result);
        memcpy(i, m_strings[0].first, m_strings[0].second * sizeof(*m_strings[0].first));
        i += m_strings[0].second;

        for (size_t j = 1; j < m_count; ++j)
        {
            memcpy(i, m_strings[j].first, m_strings[j].second * sizeof(*m_strings[j].first));
            i += m_strings[j].second;
        }

        *i = 0;
    }

private:
    static const u32 MAX_CONCAT_RESULT_SIZE = static_cast<u32>(512 * 1024);
    static const u32 MAX_ITEM_COUNT = static_cast<u32>(6);

    template <u32 index>
    struct helper
    {
        static size_t get_length(pcstr string) { return string ? xr_strlen(string) : 0; }
        static pcstr get_cstr(pcstr string) { return string; }
        static size_t get_length(shared_str const& string) { return string.size(); }
        static pcstr get_cstr(shared_str const& string) { return string.c_str(); }
        static size_t get_length(xr_string const& string) { return string.size(); }
        static pcstr get_cstr(xr_string const& string) { return string.c_str(); }

        template <typename TType>
        static void add_string(string_tupples& self, TType p)
        {
            static_assert(index < MAX_ITEM_COUNT, "Error invalid string index specified.");

            pcstr cstr = get_cstr(p);
            VERIFY(cstr);
            self.m_strings[index] = std::make_pair(cstr, get_length(p));
        }
    };

    using StringPair = std::pair<pcstr, size_t>;

    StringPair m_strings[MAX_ITEM_COUNT];
    u32 m_count;
};

void XRCORE_API check_stack_overflow(u32 stack_increment);

} // namespace detail
} // namespace core
} // namespace xray
