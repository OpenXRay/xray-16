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
public:
    template <typename T0>
    inline string_tupples(T0 p0) : m_count(1)
    {
        helper<0>::add_string(*this, p0);
    }

    template <typename T0, typename T1>
    inline string_tupples(T0 p0, T1 p1) : m_count(2)
    {
        helper<0>::add_string(*this, p0);
        helper<1>::add_string(*this, p1);
    }

    template <typename T0, typename T1, typename T2>
    inline string_tupples(T0 p0, T1 p1, T2 p2) : m_count(3)
    {
        helper<0>::add_string(*this, p0);
        helper<1>::add_string(*this, p1);
        helper<2>::add_string(*this, p2);
    }

    template <typename T0, typename T1, typename T2, typename T3>
    inline string_tupples(T0 p0, T1 p1, T2 p2, T3 p3) : m_count(4)
    {
        helper<0>::add_string(*this, p0);
        helper<1>::add_string(*this, p1);
        helper<2>::add_string(*this, p2);
        helper<3>::add_string(*this, p3);
    }

    template <typename T0, typename T1, typename T2, typename T3, typename T4>
    inline string_tupples(T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) : m_count(5)
    {
        helper<0>::add_string(*this, p0);
        helper<1>::add_string(*this, p1);
        helper<2>::add_string(*this, p2);
        helper<3>::add_string(*this, p3);
        helper<4>::add_string(*this, p4);
    }

    template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
    inline string_tupples(T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) : m_count(6)
    {
        helper<0>::add_string(*this, p0);
        helper<1>::add_string(*this, p1);
        helper<2>::add_string(*this, p2);
        helper<3>::add_string(*this, p3);
        helper<4>::add_string(*this, p4);
        helper<5>::add_string(*this, p5);
    }

    void error_process() const;

    inline u32 size() const
    {
        VERIFY(m_count > 0);

        u32 result = m_strings[0].second;

        for (u32 j = 1; j < m_count; ++j)
            result += m_strings[j].second;

        if (result > max_concat_result_size)
        {
            error_process();
        }

        return ((result + 1) * sizeof(*m_strings[0].first));
    }

    inline void concat(pcstr const result) const
    {
        VERIFY(m_count > 0);

        pstr i = const_cast<pstr>(result);
        memcpy(i, m_strings[0].first, m_strings[0].second * sizeof(*m_strings[0].first));
        i += m_strings[0].second;

        for (u32 j = 1; j < m_count; ++j)
        {
            memcpy(i, m_strings[j].first, m_strings[j].second * sizeof(*m_strings[j].first));
            i += m_strings[j].second;
        }

        *i = 0;
    }

private:
    enum
    {
        max_concat_result_size = u32(512 * 1024),
        max_item_count = 6,
    };

    template <u32 index>
    struct helper
    {
        static inline u32 length(pcstr string) { return (string ? (unsigned int)xr_strlen(string) : 0); }
        static inline pcstr string(pcstr string) { return (string); }
        static inline u32 length(shared_str const& string) { return (string.size()); }
        static inline pcstr string(shared_str const& string) { return (string.c_str()); }
        static inline size_t length(xr_string const& string) { return (string.size()); }
        static inline pcstr string(xr_string const& string) { return (string.c_str()); }
        template <typename T>
        static inline void add_string(string_tupples& self, T p)
        {
            static_assert(index < max_item_count, "Error invalid string index specified.");

            pcstr cstr = string(p);
            VERIFY(cstr);
            self.m_strings[index] = std::make_pair(cstr, length(p));
        }
    }; // struct helper

    using StringPair = std::pair<pcstr, u32>;

    StringPair m_strings[max_item_count];
    u32 m_count;
};

void XRCORE_API check_stack_overflow(u32 stack_increment);

} // namespace detail

} // namespace core

} // namespace xray
