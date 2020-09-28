#include "stdafx.h"

#include "net_task_callback.h"

bool net_task_callback::test_connection()
{
    VERIFY(_session != u32(-1));
#ifdef NET_CMP
    _agent.TestConnection(_session);
#else
    if (!break_all() && _agent.TestConnection(_session) == S_FALSE)
        _beak_count--;
#endif
    return !break_all();
}
