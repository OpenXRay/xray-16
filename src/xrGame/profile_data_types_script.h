#pragma once
#include "mixed_delegate.h"

// XXX: The type store_operation_cb is VERY heavy-weight in terms of how much
// the compiler has to work. It includes lua and therefore luabind, that is literally
// chock-full of templates, and slows down compilation measurably.

// XXX: See if it's possible to make store_operation_cb a simple free-standing type,
// that in a source file forwards to the actual mixed_delegate.

namespace gamespy_profile
{
typedef mixed_delegate<void(bool, const char*), store_operation_cb_tag> store_operation_cb;
}
