#include "stdafx.h"
#include "string_concatenations.h"

#if defined(LINUX) || defined(FREEBSD)
int _cdecl _resetstkoflw(void)
{
    int stack_addr;

    return 0;
}
#endif

namespace Strconcat
{
namespace strconcat_error
{
void process(u32 const index, u32 const count, pcstr* strings)
{
    u32 const max_string_size = 1024;
    pstr temp = (pstr)_alloca((count * (max_string_size + 4) + 1) * sizeof(**strings));
    pstr k = temp;
    *k++ = '[';
    for (u32 i = 0; i < count; ++i)
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

template <u32 count>
static inline void process(pstr& i, pcstr e, u32 const index, pcstr (&strings)[count])
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
    if (exception_code == EXCEPTION_STACK_OVERFLOW)
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
    __try
    {
        void* p = _alloca(stack_increment);
        p;
    }
    __except (Strconcat::stack_overflow_exception_filter(GetExceptionCode()))
    {
        _resetstkoflw();
    }
}

void CStringTupples::error_process() const
{
    pcstr strings[MAX_ITEM_COUNT];

    u32 part_size = 0;
    u32 overrun_string_index = (u32)-1;
    for (u32 i = 0; i < m_count; ++i)
    {
        strings[i] = m_strings[i].first;

        if (overrun_string_index == (u32)-1)
        {
            part_size += m_strings[i].second;
            if (part_size > MAX_CONCAT_RESULT_SIZE)
            {
                overrun_string_index = i;
            }
        }
    }

    strconcat_error::process(overrun_string_index, m_count, strings);
}

} // namespace Strconcat
