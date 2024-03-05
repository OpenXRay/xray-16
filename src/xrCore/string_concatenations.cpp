#include "stdafx.h"
#include "string_concatenations.h"

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE) // XXX: remove or cleanup
int _cdecl _resetstkoflw(void)
{
    return 0;
}
#endif

namespace xray
{
namespace core
{
namespace detail
{
namespace strconcat_error
{
void process(size_t const index, size_t const count, pcstr* strings)
{
    constexpr size_t max_string_size = 1024;
    // XXX: Why stack allocation?
    pstr temp = (pstr)xr_alloca((count * (max_string_size + 4) + 1) * sizeof(**strings));
    pstr k = temp;
    *k++ = '[';
    for (size_t i = 0; i < count; ++i)
    {
        for (pcstr j = strings[i], e = j + max_string_size; *j && j < e; ++k, ++j)
            *k = *j;

        *k++ = ']';

        if (i + 1 >= count)
            continue;

        *k++ = '[';
        *k++ = '\r';
        *k++ = '\n';
    }
    *k = 0;

    xrDebug::Fatal(
        DEBUG_INFO, make_string("buffer overflow: cannot concatenate strings(%d):\r\n%s", index, temp).c_str());
}

template <size_t count>
static inline void process(pstr& i, pcstr e, size_t const index, pcstr (&strings)[count])
{
    VERIFY(i <= e);
    VERIFY(index < count);

    if (i != e)
        return;

#ifndef MASTER_GOLD
    process(index, count, strings);
#else // #ifndef MASTER_GOLD
    --i;
#endif // #ifndef MASTER_GOLD
}

} // namespace strconcat_error

int stack_overflow_exception_filter(int exception_code)
{
    if (exception_code == static_cast<int>(EXCEPTION_STACK_OVERFLOW))
    {
        // Do not call _resetstkoflw here, because
        // at this point, the stack is not yet unwound.
        // Instead, signal that the handler (the __except block)
        // is to be executed.
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

void check_stack_overflow(u32 stack_increment)
{
#if defined(XR_PLATFORM_WINDOWS)
    __try
    {
        void* p = xr_alloca(stack_increment);
        p;
    }
    __except (xray::core::detail::stack_overflow_exception_filter(GetExceptionCode()))
    {
        _resetstkoflw();
    }
#endif
}

void string_tupples::error_process() const
{
    constexpr auto npos = std::numeric_limits<size_t>::max();

    pcstr strings[MAX_ITEM_COUNT];
    size_t part_size = 0;
    size_t overrun_string_index = npos;

    for (size_t i = 0; i < m_count; ++i)
    {
        const auto [str, size] = m_strings[i];

        strings[i] = str;
        part_size += size;

        if (part_size > MAX_CONCAT_RESULT_SIZE)
        {
            overrun_string_index = i;
            // No need to continue, index has been found
            break;
        }
    }

    VERIFY(overrun_string_index != npos);
    strconcat_error::process(overrun_string_index, m_count, strings);
}

} // namespace detail
} // namespace core
} // namespace xray
