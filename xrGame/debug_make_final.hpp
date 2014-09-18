////////////////////////////////////////////////////////////////////////////
//	Module 		: debug_make_final.hpp
//	Created 	: 03.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : make_final class for debug purposes
////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_MAKE_FINAL_HPP_INCLUDED
#define DEBUG_MAKE_FINAL_HPP_INCLUDED

#include <boost/noncopyable.hpp>

namespace debug {

#ifdef DEBUG
	namespace detail {

		template <typename T1, typename T2>
		class make_final {
				make_final () {}
			friend T1;
			friend T2;
		}; // class make_final

	} // namespace detail

	template <typename T>
	struct make_final : 
		private virtual
			detail::make_final<
				T,
				make_final<T>
			>
	{
	}; // class make_final
#else // DEBUG
	template <typename T>
	class make_final {};
#endif // DEBUG

} // namespace debug

#endif // DEBUG_MAKE_FINAL_HPP_INCLUDED