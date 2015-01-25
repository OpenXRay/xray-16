// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef LUABIND_COM_POLICY_HPP_INCLUDED
#define LUABIND_COM_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>
#include <luabind/detail/policy.hpp>
#include <luabind/detail/implicit_cast.hpp>

namespace luabind { namespace detail 
{
	template<class Direction = lua_to_cpp>
	struct COM_ptr_converter;

	template<class T>
	struct COM_release
	{
		static void release(void* ptr)
		{
			T* obj = static_cast<T*>(ptr);
			obj->Release();
		}
	};

	template<>
	struct COM_ptr_converter<cpp_to_lua>
	{
		template<class T>
		void apply(lua_State* L, T* ptr)
		{
			if (ptr == 0)
			{
				lua_pushnil(L);
				return;
			}

			class_registry* registry = class_registry::get_registry(L);
			class_rep* crep = registry->find_class(LUABIND_TYPEID(T));

			// if you get caught in this assert you are trying
			// to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			// create the struct to hold the object
			void* obj = lua_newuserdata(L, sizeof(object_rep));
			new(obj) object_rep(ptr, crep, object_rep::owner, COM_release<T>::release);

			ptr->AddRef();

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);
		}
	};

	template<int N>
	struct COM_policy : conversion_policy<N>
	{
		static void precall(lua_State*, const index_map&) {}
		static void postcall(lua_State*, const index_map&) {}

		template<class T, class Direction>
		struct generate_converter
		{
			typedef COM_ptr_converter<cpp_to_lua> type;
		};
	};

}}

namespace luabind
{
	template<int N>
	detail::policy_cons<detail::COM_policy<N>, detail::null_type> 
	COM_managed(boost::arg<N>) { return detail::policy_cons<detail::COM_policy<N>, detail::null_type>(); }
}

#endif // LUABIND_COM_POLICY_HPP_INCLUDED


function(L, "createCar", &createCar, COM_managed(result));

