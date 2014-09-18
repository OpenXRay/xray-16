#include "pch.h"

#ifndef LUABIND_NO_EXCEPTIONS
namespace boost
{
	void throw_exception(const std::exception &){}
}
#endif // LUABIND_NO_EXCEPTIONS

#ifdef NDEBUG

namespace std {
	void terminate			()
	{
		abort				();
	}
} // namespace std

#endif // #ifdef NDEBUG