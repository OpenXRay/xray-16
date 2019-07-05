#pragma once
#include "xrCommon/xr_string.h"

namespace Strconcat
{
class XRCORE_API CStringTupples
{
    template <size_t... Ind, typename... Args>
    void ProcessArgs(const std::tuple<Args...>& args, std::index_sequence<Ind...>)
    {
        (Helper<Ind>::add_string(*this, std::get<Ind>(args)), ...);
    }

public:
    template <typename... Args>
    CStringTupples(const Args... args) : m_count(sizeof...(Args))
    {
        ProcessArgs(std::make_tuple(args...), std::index_sequence_for<Args...>{});
    }

    void error_process() const;

    u32 size() const
    {
        VERIFY(m_count > 0);

        u32 result = m_strings[0].second;
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
    struct Helper
    {
        static u32 GetLength(pcstr string) { return (string ? (u32)xr_strlen(string) : 0); }
        static pcstr GetCString(pcstr string) { return (string); }
        static u32 GetLength(shared_str const& string) { return (string.size()); }
        static pcstr GetCString(shared_str const& string) { return (string.c_str()); }
        static size_t GetLength(xr_string const& string) { return (string.size()); }
        static pcstr GetCString(xr_string const& string) { return (string.c_str()); }

        template <typename TType>
        static void add_string(CStringTupples& self, TType p)
        {
            static_assert(index < MAX_ITEM_COUNT, "Error invalid string index specified.");

            pcstr cstr = GetCString(p);
            VERIFY(cstr);
            self.m_strings[index] = std::make_pair(cstr, GetLength(p));
        }
    }; // struct helper

    using StringPair = std::pair<pcstr, u32>;

    StringPair m_strings[MAX_ITEM_COUNT];
    u32 m_count;
};

void XRCORE_API check_stack_overflow(u32 stack_increment);

} // namespace Strconcat
