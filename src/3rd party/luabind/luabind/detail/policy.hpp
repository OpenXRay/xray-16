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


#ifndef LUABIND_POLICY_HPP_INCLUDED
#define LUABIND_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>

#include <typeinfo>

#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/apply_if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/bind/arg.hpp>
#include <boost/limits.hpp>

#include <luabind/detail/class_registry.hpp>
#include <luabind/detail/primitives.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/typetraits.hpp>
#include <luabind/detail/class_cache.hpp>
#include <luabind/detail/debug.hpp>

#include <boost/type_traits/add_reference.hpp>

#include <luabind/detail/decorate_type.hpp>
#include <luabind/object.hpp>
#include <luabind/weak_ref.hpp>
#include <luabind/back_reference_fwd.hpp>

namespace luabind
{
	namespace detail
	{
		struct conversion_policy_base {};
	}

	template<int N, bool HasArg = true>
	struct conversion_policy : detail::conversion_policy_base
	{
		BOOST_STATIC_CONSTANT(int, index = N);
		BOOST_STATIC_CONSTANT(bool, has_arg = HasArg);
	};

	class index_map
	{
	public:

		index_map(const int* m): m_map(m) {}

		int operator[](int index) const
		{
			return m_map[index];
		}

	private:

		const int* m_map;
	};

	namespace converters
	{
		using luabind::detail::yes_t;
		using luabind::detail::no_t;
		using luabind::detail::by_value;
		using luabind::detail::by_reference;
		using luabind::detail::by_const_reference;
		using luabind::detail::by_pointer;
		using luabind::detail::by_const_pointer;

		no_t is_user_defined(...);

//		template<bool B = true> struct yes_no : yes_t { typedef yes_t type; };
//		template<> struct yes_no<false> : no_t { typedef no_t type; };
/*
		template<int N, class T>
		struct TO
		{
			BOOST_STATIC_CONSTANT(bool, is_specialized = false);

			std::pair<int,int> match(lua_State*, detail::type<T>, boost::mpl::int_<N>, int)
			{
				return make_pair(-1,-1);
			}

			template<int I>
			void convert(lua_State*, detail::type<T>, boost::mpl::int_<N>, int) {}
		};

		no_t is_implicit_conversion(...);

		template<class T>
		yes_no<TO<0,T>::is_specialized> is_implicit_conversion(by_value<T>);

		template<class T>
		yes_no<TO<0,T>::is_specialized> is_implicit_conversion(by_const_reference<T>);

		template<class T>
		yes_no<TO<0,T*>::is_specialized> is_implicit_conversion(by_pointer<T>);

		template<class T>
		yes_no<TO<0,const T*>::is_specialized> is_implicit_conversion(by_const_pointer<T>);

		#define LUABIND_IMPLICIT(index, to, from) template<> struct TO<index,to >:FROM<from > {}*/
	}

	namespace detail
	{
		template<class T>
		struct is_user_defined
		{
			BOOST_STATIC_CONSTANT(bool, value = 
				sizeof(luabind::converters::is_user_defined(LUABIND_DECORATE_TYPE(T))) == sizeof(yes_t));
		};

/*		template<class T>
		struct is_implicit_conversion
		{
			BOOST_STATIC_CONSTANT(bool, value =
					sizeof(luabind::converters::is_implicit_conversion(LUABIND_DECORATE_TYPE(T))) == sizeof(yes_t));
		};
*/
		LUABIND_API int implicit_cast(const class_rep* crep, LUABIND_TYPE_INFO const&, int& pointer_offset);
	}

	 template<class T> class functor;
	 class object;
	 class weak_ref;
}

namespace luabind { namespace detail
{
	template<class>
	struct is_primitive;

	template<class T>
	yes_t is_lua_functor_test(const functor<T>&);

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
	no_t is_lua_functor_test(...);
#else
	template<class T>
	no_t is_lua_functor_test(const T&);
#endif

	template<class T>
	struct is_lua_functor
	{
		static T t;

		BOOST_STATIC_CONSTANT(bool, value = sizeof(is_lua_functor_test(t)) == sizeof(yes_t));
	};

	namespace
	{
		static char msvc_fix[64];
	}

	template<class T>
	struct indirect_type
	{
		typedef typename
			boost::mpl::if_<is_primitive<T>
				, const type<T>&
				, typename boost::mpl::apply_if<boost::mpl::or_<boost::is_reference<T>, boost::is_pointer<T> >
					, identity<T>
					, boost::add_reference<T>
				>::type
			>::type result_type;

		static inline result_type get()
		{
			return reinterpret_cast<result_type>(msvc_fix);
		}
	};

	template<class H, class T>
	struct policy_cons
	{
		typedef H head;
		typedef T tail;

		template<class U>
		policy_cons<U, policy_cons<H,T> > operator,(policy_cons<U,detail::null_type>)
		{
			return policy_cons<U, policy_cons<H,T> >();
		}

		template<class U>
		policy_cons<U, policy_cons<H,T> > operator+(policy_cons<U,detail::null_type>)
		{
			return policy_cons<U, policy_cons<H,T> >();
		}

		template<class U>
		policy_cons<U, policy_cons<H,T> > operator|(policy_cons<U,detail::null_type>)
		{
			return policy_cons<U, policy_cons<H,T> >();
		}
	};

	struct indirection_layer
	{
		template<class T>
		indirection_layer(const T&);
	};

	yes_t is_policy_cons_test(const null_type&);
	template<class H, class T>
	yes_t is_policy_cons_test(const policy_cons<H,T>&);
	no_t is_policy_cons_test(...);

	template<class T>
	struct is_policy_cons
	{
		static const T& t;

		BOOST_STATIC_CONSTANT(bool, value = 
			sizeof(is_policy_cons_test(t)) == sizeof(yes_t));

		typedef boost::mpl::bool_<value> type;
	};	

	template<bool>
	struct is_string_literal
	{
		static no_t helper(indirection_layer);
		static yes_t helper(const char*);
	};

	template<>
	struct is_string_literal<false>
	{
		static no_t helper(indirection_layer);
	};
	

	template<class T>
	struct is_primitive/*: boost::mpl::bool_c<false>*/ 
	{
		static T t;

		BOOST_STATIC_CONSTANT(bool, value =
				sizeof(is_string_literal<boost::is_array<T>::value>::helper(t)) == sizeof(yes_t));
	};

#define LUABIND_INTEGER_TYPE(type) \
	template<> struct is_primitive<type> : boost::mpl::true_ {}; \
	template<> struct is_primitive<type const> : boost::mpl::true_ {}; \
	template<> struct is_primitive<type const&> : boost::mpl::true_ {}; \
	template<> struct is_primitive<unsigned type> : boost::mpl::true_ {}; \
	template<> struct is_primitive<unsigned type const> : boost::mpl::true_ {}; \
	template<> struct is_primitive<unsigned type const&> : boost::mpl::true_ {};

	LUABIND_INTEGER_TYPE(char)
	LUABIND_INTEGER_TYPE(short)
	LUABIND_INTEGER_TYPE(int)
	LUABIND_INTEGER_TYPE(long)

	template<> struct is_primitive<signed char> : boost::mpl::true_ {}; \
	template<> struct is_primitive<signed char const> : boost::mpl::true_ {}; \
	template<> struct is_primitive<signed char const&> : boost::mpl::true_ {}; \
	
#undef LUABIND_INTEGER_TYPE
	
	template<> struct is_primitive<luabind::object>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const luabind::object>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const luabind::object&>: boost::mpl::bool_<true> {};

	template<> struct is_primitive<luabind::weak_ref>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const luabind::weak_ref>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const luabind::weak_ref&>: boost::mpl::bool_<true> {};
	
	template<> struct is_primitive<float>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<double>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<long double>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<char*>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<bool>: boost::mpl::bool_<true> {};

	template<> struct is_primitive<const float>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const double>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const long double>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const char*>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const char* const>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const bool>: boost::mpl::bool_<true> {};

	// TODO: add more
	template<> struct is_primitive<const float&>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const double&>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const long double&>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const bool&>: boost::mpl::bool_<true> {};

	template<> struct is_primitive<const string_class&>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<string_class>: boost::mpl::bool_<true> {};
	template<> struct is_primitive<const string_class>: boost::mpl::bool_<true> {};


	template<class Direction> struct primitive_converter;
	
	template<>
	struct primitive_converter<cpp_to_lua>
	{
		void apply(lua_State* L, const luabind::object& v)
		{
			// if the luabind::object is uninitialized
			// treat it as nil.
			if (v.lua_state() == 0)
			{
				lua_pushnil(L);
				return;
			}
			// if you hit this assert you are trying to return a value from one state into another lua state
			assert((v.lua_state() == L) && "you cannot return an uninitilized value "
				"or a value from one lua state into another");
			v.pushvalue();
		}
		void apply(lua_State* L, int v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, short v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, char v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, long v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, unsigned int v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, unsigned short v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, unsigned char v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, unsigned long v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, float v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, double v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, long double v) { lua_pushnumber(L, (lua_Number)v); }
		void apply(lua_State* L, const char* v) { lua_pushstring(L, v); }
		void apply(lua_State* L, const string_class& v)
		{ lua_pushlstring(L, v.data(), v.size()); }
		void apply(lua_State* L, bool b) { lua_pushboolean(L, b); }
	};

	template<>
	struct primitive_converter<lua_to_cpp>
	{
		#define PRIMITIVE_CONVERTER(prim) \
			prim apply(lua_State* L, luabind::detail::by_const_reference<prim>, int index) { return apply(L, detail::by_value<prim>(), index); } \
			prim apply(lua_State* L, luabind::detail::by_value<const prim>, int index) { return apply(L, detail::by_value<prim>(), index); } \
			prim apply(lua_State* L, luabind::detail::by_value<prim>, int index)

		#define PRIMITIVE_MATCHER(prim) \
			static int match(lua_State* L, luabind::detail::by_const_reference<prim>, int index) { return match(L, detail::by_value<prim>(), index); } \
			static int match(lua_State* L, luabind::detail::by_value<const prim>, int index) { return match(L, detail::by_value<prim>(), index); } \
			static int match(lua_State* L, luabind::detail::by_value<prim>, int index)

		PRIMITIVE_CONVERTER(bool) { return lua_toboolean(L, index) == 1; }
		PRIMITIVE_MATCHER(bool) { if (lua_type(L, index) == LUA_TBOOLEAN) return 0; else return -1; }

		PRIMITIVE_CONVERTER(int) { return static_cast<int>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(int) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(unsigned int) { return static_cast<unsigned int>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(unsigned int) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(char) { return static_cast<char>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(char) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(signed char) { return static_cast<char>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(signed char) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }
		
		PRIMITIVE_CONVERTER(unsigned char) { return static_cast<unsigned char>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(unsigned char) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(short) { return static_cast<short>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(short) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(unsigned short) { return static_cast<unsigned short>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(unsigned short) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(long) { return static_cast<long>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(long) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(unsigned long) { return static_cast<unsigned long>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(unsigned long) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(float) { return static_cast<float>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(float) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(double) { return static_cast<double>(lua_tonumber(L, index)); }
		PRIMITIVE_MATCHER(double) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1; }

		PRIMITIVE_CONVERTER(string_class)
		{ return string_class(lua_tostring(L, index), lua_strlen(L, index)); }
		PRIMITIVE_MATCHER(string_class) { if (lua_type(L, index) == LUA_TSTRING) return 0; else return -1; }

		PRIMITIVE_CONVERTER(luabind::object)
		{
			LUABIND_CHECK_STACK(L);

			lua_pushvalue(L, index);
			detail::lua_reference ref;
			ref.set(L);
			return luabind::object(L, ref, true);
		}

		PRIMITIVE_MATCHER(luabind::object)
		{
			(void)index;
			(void)L;
			return std::numeric_limits<int>::max() / LUABIND_MAX_ARITY;
		}

		PRIMITIVE_CONVERTER(luabind::weak_ref)
		{
			LUABIND_CHECK_STACK(L);
			return luabind::weak_ref(L, index);
		}

		PRIMITIVE_MATCHER(luabind::weak_ref) { (void)index; (void)L; return std::numeric_limits<int>::max() - 1; }
		
		const char* apply(lua_State* L, detail::by_const_pointer<char>, int index) { return static_cast<const char*>(lua_tostring(L, index)); }
		const char* apply(lua_State* L, detail::by_const_pointer<const char>, int index) { return static_cast<const char*>(lua_tostring(L, index)); }
		static int match(lua_State* L, by_const_pointer<char>, int index) { if (lua_type(L, index) == LUA_TSTRING) return 0; else return -1;}
		static int match(lua_State* L, by_const_pointer<const char>, int index) { if (lua_type(L, index) == LUA_TSTRING) return 0; else return -1;}

/*		template<class T>
		T apply(lua_State* L, luabind::detail::by_const_reference<T>, int index) { return apply(L, detail::by_value<T>(), index); }

		// TODO: add more
		bool apply(lua_State* L, detail::by_value<bool>, int index) { return lua_toboolean(L, index) == 1; }
		float apply(lua_State* L, detail::by_value<float>, int index) { return static_cast<float>(lua_tonumber(L, index)); }
		double apply(lua_State* L, detail::by_value<double>, int index) { return static_cast<double>(lua_tonumber(L, index)); }
		long double apply(lua_State* L, detail::by_value<long double>, int index) { return static_cast<long double>(lua_tonumber(L, index)); }
		int apply(lua_State* L, detail::by_value<int>, int index) { return static_cast<int>(lua_tonumber(L, index)); }
		short apply(lua_State* L, detail::by_value<short>, int index) { return static_cast<short>(lua_tonumber(L, index)); }
		char apply(lua_State* L, detail::by_value<char>, int index) { return static_cast<char>(lua_tonumber(L, index)); }
		long apply(lua_State* L, detail::by_value<long>, int index) { return static_cast<long>(lua_tonumber(L, index)); }
		unsigned int apply(lua_State* L, detail::by_value<unsigned int>, int index) { return static_cast<unsigned int>(lua_tonumber(L, index)); }
		unsigned short apply(lua_State* L, detail::by_value<unsigned short>, int index) { return static_cast<short>(lua_tonumber(L, index)); }
		unsigned char apply(lua_State* L, detail::by_value<unsigned char>, int index) { return static_cast<char>(lua_tonumber(L, index)); }
		unsigned long apply(lua_State* L, detail::by_value<unsigned long>, int index) { return static_cast<long>(lua_tonumber(L, index)); }
		
		float apply(lua_State* L, detail::by_value<const float>, int index) { return static_cast<float>(lua_tonumber(L, index)); }
		double apply(lua_State* L, detail::by_value<const double>, int index) { return static_cast<double>(lua_tonumber(L, index)); }
		long double apply(lua_State* L, detail::by_value<const long double>, int index) {return  static_cast<long double>(lua_tonumber(L, index)); }
		int apply(lua_State* L, detail::by_value<const int>, int index) { return static_cast<int>(lua_tonumber(L, index)); }
		short apply(lua_State* L, detail::by_value<const short>, int index) { return static_cast<short>(lua_tonumber(L, index)); }
		char apply(lua_State* L, detail::by_value<const char>, int index) { return static_cast<char>(lua_tonumber(L, index)); }
		long apply(lua_State* L, detail::by_value<const long>, int index) { return static_cast<long>(lua_tonumber(L, index)); }

		unsigned int apply(lua_State* L, detail::by_value<const unsigned int>, int index) { return static_cast<int>(lua_tonumber(L, index)); }
		unsigned short apply(lua_State* L, detail::by_value<const unsigned short>, int index) { return static_cast<short>(lua_tonumber(L, index)); }
		unsigned char apply(lua_State* L, detail::by_value<const unsigned char>, int index) { return static_cast<char>(lua_tonumber(L, index)); }
		unsigned long apply(lua_State* L, detail::by_value<const unsigned long>, int index) { return static_cast<long>(lua_tonumber(L, index)); }
		
//		string_class apply(lua_State* L, detail::by_value<string_class>, int index) { return static_cast<const char*>(lua_tostring(L, index)); }
//		const string_class apply(lua_State* L, detail::by_value<const string_class>, int index) { return static_cast<const char*>(lua_tostring(L, index)); }
//		const string_class apply(lua_State* L, detail::by_const_reference<string_class>, int index) { return static_cast<const char*>(lua_tostring(L, index)); }
		PRIMITIVE_CONVERTER(string_class) { return static_cast<const char*>(lua_tostring(L, index)); }

		luabind::object apply(lua_State* L, detail::by_value<luabind::object>, int index)
		{
			lua_pushvalue(L, index);
			return luabind::object(L, detail::ref(L), true);
		}

		const luabind::object apply(lua_State* L, detail::by_value<const luabind::object>, int index)
		{
			lua_pushvalue(L, index);
			return luabind::object(L, detail::ref(L), true);
		}

		// TODO: add more

		const char* apply(lua_State* L, detail::by_const_pointer<char>, int index) { return static_cast<const char*>(lua_tostring(L, index)); }

		// matchers
		static int match(lua_State* L, detail::by_value<bool>, int index) { if (lua_type(L, index) == LUA_TBOOLEAN) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<float>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<double>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<long double>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<int>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<short>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<char>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<long>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<unsigned int>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<unsigned short>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<unsigned char>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<unsigned long>, int index) { if (lua_type(L, index) == LUA_TNUMBER) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<string_class>, int index) { if (lua_type(L, index) == LUA_TSTRING) return 0; else return -1;}
		static int match(lua_State* L, detail::by_value<const string_class>, int index) { if (lua_type(L, index) == LUA_TSTRING) return 0; else return -1;}
		static int match(lua_State*, detail::by_value<luabind::object>, int) { return std::numeric_limits<int>::max() - 1; }
		static int match(lua_State*, detail::by_value<const luabind::object>, int) { return std::numeric_limits<int>::max() - 1; }

		static int match(lua_State* L, by_const_pointer<char>, int index) { if (lua_type(L, index) == LUA_TSTRING) return 0; else return -1;}
		static int match(lua_State* L, by_const_pointer<const char>, int index) { if (lua_type(L, index) == LUA_TSTRING) return 0; else return -1;}

		template<class T>
		static int match(lua_State* L, detail::by_const_reference<T>, int index) { return match(L, detail::by_value<T>(), index); }
*/

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
		
		#undef PRIMITIVE_MATCHER
		#undef PRIMITIVE_CONVERTER
	};



// *********** default converters ***************

/*	template<class> struct implicit_converter;

	template<>
	struct implicit_converter<lua_to_cpp>
	{
		int converter_index;

		template<class T>
		T apply(lua_State* L, detail::by_value<T>, int index)
		{
			return converters::TO<T>::convert(L, detail::type<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, detail::by_value<T>, int index)
		{
			return converters::TO<T>::match(L, detail::type<T>(), index);
		}

		template<class T>
		T apply(lua_State* L, detail::by_const_reference<T>, int index)
		{
			return converters::TO<T>::convert(L, detail::type<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, detail::by_const_reference<T>, int index)
		{
			return converters::TO<T>::match(L, detail::type<T>(), index);
		}

		template<class T>
		T* apply(lua_State* L, detail::by_pointer<T>, int index)
		{
			return converters::TO<T*>::convert(L, detail::type<T*>(), index);
		}

		template<class T>
		static int match(lua_State* L, detail::by_pointer<T>, int index)
		{
			return converters::TO<T*>::match(L, detail::type<T*>(), index);
		}

		template<class T>
		const T* apply(lua_State* L, detail::by_const_pointer<T>, int index)
		{
			return converters::TO<const T*>::convert(L, detail::type<const T*>(), index);
		}

		template<class T>
		static int match(lua_State* L, detail::by_const_pointer<T>, int index)
		{
			return converters::TO<const T*>::match(L, detail::type<const T*>(), index);
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};
*/

// ********** user defined converter ***********

	template<class Direction> struct user_defined_converter;
	
	template<>
	struct user_defined_converter<lua_to_cpp>
	{
		template<class T>
		T apply(lua_State* L, detail::by_value<T>, int index) 
		{ 
//			std::cerr << "user_defined_converter\n";
			return converters::convert_lua_to_cpp(L, detail::by_value<T>(), index);
		}

		template<class T>
		T apply(lua_State* L, detail::by_reference<T>, int index) 
		{ 
//			std::cerr << "user_defined_converter\n";
			return converters::convert_lua_to_cpp(L, detail::by_reference<T>(), index);
		}

		template<class T>
		T apply(lua_State* L, detail::by_const_reference<T>, int index) 
		{ 
//			std::cerr << "user_defined_converter\n";
			return converters::convert_lua_to_cpp(L, detail::by_const_reference<T>(), index);
		}

		template<class T>
		T* apply(lua_State* L, detail::by_pointer<T>, int index) 
		{ 
//			std::cerr << "user_defined_converter\n";
			return converters::convert_lua_to_cpp(L, detail::by_pointer<T>(), index);
		}

		template<class T>
		const T* apply(lua_State* L, detail::by_const_pointer<T>, int index) 
		{ 
//			std::cerr << "user_defined_converter\n";
			return converters::convert_lua_to_cpp(L, detail::by_pointer<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, T, int index)
		{
			return converters::match_lua_to_cpp(L, T(), index);
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

	template<>
	struct user_defined_converter<cpp_to_lua>
	{
			template<class T>
			void apply(lua_State* L, const T& v) 
			{ 
				converters::convert_cpp_to_lua(L, v);
			}
	};

// ********** pointer converter ***********


	template<class Direction> struct pointer_converter;

	template<>
	struct pointer_converter<cpp_to_lua>
	{
		template<class T>
		void apply(lua_State* L, T* ptr)
		{
			if (ptr == 0) 
			{
				lua_pushnil(L);
				return;
			}

			if (back_reference<T>::extract(L, ptr))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			// create the struct to hold the object
			void* obj = lua_newuserdata(L, sizeof(object_rep));
			//new(obj) object_rep(ptr, crep, object_rep::owner, destructor_s<T>::apply);
			new(obj) object_rep(ptr, crep, 0, 0);

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);

//			make_instance(L, ptr, (pointer_holder<T, T*>*)0);
		}
	};

	template<class T> struct make_pointer { typedef T* type; };
	template<>
	struct pointer_converter<lua_to_cpp>
	{
		// TODO: does the pointer converter need this?!
		char target[32];
		void (*destructor)(void *);

		pointer_converter(): destructor(0) {}

		template<class T>
		typename make_pointer<T>::type apply(lua_State* L, by_pointer<T>, int index)
		{
			// preconditions:
			//	lua_isuserdata(L, index);
			// getmetatable().__lua_class is true
			// object_rep->flags() & object_rep::constant == 0

			if (lua_isnil(L, index)) return 0;
			
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, index));
			assert((obj != 0) && "internal error, please report"); // internal error
			const class_rep* crep = obj->crep();

			T* ptr = reinterpret_cast<T*>(crep->convert_to(LUABIND_TYPEID(T), obj, target));

			if ((void*)ptr == (char*)target) destructor = detail::destruct_only_s<T>::apply;
			assert(!destructor || sizeof(T) <= 32);

			return ptr;
		}

		template<class T>
		static int match(lua_State* L, by_pointer<T>, int index)
		{
			if (lua_isnil(L, index)) return 0;
			object_rep* obj = is_class_object(L, index);
			if (obj == 0) return -1;
			// cannot cast a constant object to nonconst
			if (obj->flags() & object_rep::constant) return -1;

			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?-1:0;
			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->const_holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?0:-1;

			int d;
			return implicit_cast(obj->crep(), LUABIND_TYPEID(T), d);
		}

		~pointer_converter()
		{
			if (destructor) destructor(target);
		}

		template<class T>
		void converter_postcall(lua_State*, by_pointer<T>, int) 
		{}
	};

// ******* value converter *******

	template<class Direction> struct value_converter;

	template<>
	struct value_converter<cpp_to_lua>
	{
		template<class T>
		void apply(lua_State* L, const T& ref)
		{
			if (back_reference<T>::extract(L, &ref))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			void* obj_rep;
			void* held;

			boost::tie(obj_rep,held) = crep->allocate(L);

			void* object_ptr;
			void(*destructor)(void*);
			destructor = crep->destructor();
			int flags = object_rep::owner;
			if (crep->has_holder())
			{
				new(held) T(ref);
				object_ptr = held;
				if (LUABIND_TYPE_INFO_EQUAL(LUABIND_TYPEID(T), crep->const_holder_type()))
				{
					flags |= object_rep::constant;
					destructor = crep->const_holder_destructor();
				}
			}
			else
			{
				object_ptr = luabind_new<T>(ref);
			}
			new(obj_rep) object_rep(object_ptr, crep, flags, destructor);

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);
		}
	};


	template<class T> struct make_const_reference { typedef const T& type; };

	template<class T>
	struct destruct_guard
	{
		T* ptr;
		bool dismiss;
		destruct_guard(T* p): ptr(p), dismiss(false) {}

		~destruct_guard()
		{
			if (!dismiss)
				ptr->~T();
		}
	};

	template<>
	struct value_converter<lua_to_cpp>
	{
		template<class T>
		/*typename make_const_reference<T>::type*/T apply(lua_State* L, by_value<T>, int index)
		{
			// preconditions:
			//	lua_isuserdata(L, index);
			// getmetatable().__lua_class is true
			// object_rep->flags() & object_rep::constant == 0

			object_rep* obj = 0;
			const class_rep* crep = 0;

			// special case if we get nil in, try to convert the holder type
			if (lua_isnil(L, index))
			{
				crep = get_class_rep<T>(L);
				assert(crep);
			}
			else
			{
				obj = static_cast<object_rep*>(lua_touserdata(L, index));
				assert((obj != 0) && "internal error, please report"); // internal error
				crep = obj->crep();
			}
			assert(crep);

			// TODO: align!
			char target[sizeof(T)];
			T* ptr = reinterpret_cast<T*>(crep->convert_to(LUABIND_TYPEID(T), obj, target));

			destruct_guard<T> guard(ptr);
			if ((void*)ptr != (void*)target) guard.dismiss = true;

			return *ptr;
		}

		template<class T>
		static int match(lua_State* L, by_value<T>, int index)
		{
			// special case if we get nil in, try to match the holder type
			if (lua_isnil(L, index))
			{
				class_rep* crep = get_class_rep<T>(L);
				if (crep == 0) return -1;
				if ((LUABIND_TYPE_INFO_EQUAL(crep->holder_type(), LUABIND_TYPEID(T))))
					return 0;
				if ((LUABIND_TYPE_INFO_EQUAL(crep->const_holder_type(), LUABIND_TYPEID(T))))
					return 0;
				return -1;
			}

			object_rep* obj = is_class_object(L, index);
			if (obj == 0) return -1;
			int d;

			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?-1:0;
			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->const_holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?0:1;

			return implicit_cast(obj->crep(), LUABIND_TYPEID(T), d);	
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

// ******* const pointer converter *******

	template<class Direction> struct const_pointer_converter;

	template<>
	struct const_pointer_converter<cpp_to_lua>
	{
		template<class T>
		void apply(lua_State* L, const T* ptr)
		{
			if (ptr == 0) 
			{
				lua_pushnil(L);
				return;
			}

			if (back_reference<T>::extract(L, ptr))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			// create the struct to hold the object
			void* obj = lua_newuserdata(L, sizeof(object_rep));
			assert(obj && "internal error, please report");
			// we send 0 as destructor since we know it will never be called
			new(obj) object_rep(const_cast<T*>(ptr), crep, object_rep::constant, 0);

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);
		}
	};


	template<class T> struct make_const_pointer { typedef const T* type; };
	template<>
	struct const_pointer_converter<lua_to_cpp>
		: private pointer_converter<lua_to_cpp>
	{
		template<class T>
		typename make_const_pointer<T>::type apply(lua_State* L, by_const_pointer<T>, int index)
		{
//			std::cerr << "const_pointer_converter\n";
			return pointer_converter<lua_to_cpp>::apply(L, by_pointer<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, by_const_pointer<T>, int index)
		{
			if (lua_isnil(L, index)) return 0;
			object_rep* obj = is_class_object(L, index);
			if (obj == 0) return -1; // if the type is not one of our own registered types, classify it as a non-match

			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?-1:0;
			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->const_holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?0:1;

            bool const_ = obj->flags() & object_rep::constant;
			int d;
			return implicit_cast(obj->crep(), LUABIND_TYPEID(T), d) + !const_;
		}

		template<class T>
		void converter_postcall(lua_State* L, by_const_pointer<T>, int index) 
		{
			pointer_converter<lua_to_cpp>::converter_postcall(L, by_pointer<T>(), index);
		}
	};

// ******* reference converter *******

	template<class Direction> struct ref_converter;

	template<>
	struct ref_converter<cpp_to_lua>
	{
		template<class T>
		void apply(lua_State* L, T& ref)
		{
			if (back_reference<T>::extract(L, &ref))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			T* ptr = &ref;

			// create the struct to hold the object
			void* obj = lua_newuserdata(L, sizeof(object_rep));
			assert(obj && "internal error, please report");
			new(obj) object_rep(ptr, crep, 0, 0);

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);
		}
	};

	template<class T> struct make_reference { typedef T& type; };
	template<>
	struct ref_converter<lua_to_cpp>
	{
		template<class T>
		typename make_reference<T>::type apply(lua_State* L, by_reference<T>, int index)
		{
			assert(!lua_isnil(L, index));
			return *pointer_converter<lua_to_cpp>().apply(L, by_pointer<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, by_reference<T>, int index)
		{
			if (lua_isnil(L, index)) return -1;
			return pointer_converter<lua_to_cpp>::match(L, by_pointer<T>(), index);
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

// ******** const reference converter *********

	template<class Direction> struct const_ref_converter;

	template<>
	struct const_ref_converter<cpp_to_lua>
	{
		template<class T>
		void apply(lua_State* L, const T& ref)
		{
			if (back_reference<T>::extract(L, &ref))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

#if 0
			void* obj_rep;
			void* held;

			boost::tie(obj_rep,held) = crep->allocate(L);

			void* object_ptr;
			void(*destructor)(void*);
			destructor = crep->destructor();
			int flags = 0;
			if (crep->has_holder())
			{
				flags = object_rep::owner;
				new(held) T(ref);
				object_ptr = held;
				if (LUABIND_TYPE_INFO_EQUAL(LUABIND_TYPEID(T), crep->const_holder_type()))
				{
					flags |= object_rep::constant;
					destructor = crep->const_holder_destructor();
				}
			}
			else
			{
				object_ptr = luabind_new<T>(ref);
			}
			new(obj_rep) object_rep(object_ptr, crep, flags, destructor);
#else
			T const* ptr = &ref;

			// create the struct to hold the object
			void* obj = lua_newuserdata(L, sizeof(object_rep));
			assert(obj && "internal error, please report");
			new(obj) object_rep(const_cast<T*>(ptr), crep, object_rep::constant, 0);
#endif

			// set the meta table
			detail::getref(L, crep->metatable_ref());
			lua_setmetatable(L, -2);
		}
	};

	template<>
	struct const_ref_converter<lua_to_cpp>
	{
		// TODO: align!
		char target[32];
		void (*destructor)(void*);

		const_ref_converter(): destructor(0) {}

		template<class T>
		typename make_const_reference<T>::type apply(lua_State* L, by_const_reference<T>, int index)
		{
			object_rep* obj = 0;
			class_rep const * crep = 0;

			// special case if we get nil in, try to convert the holder type
			if (lua_isnil(L, index))
			{
				crep = get_class_rep<T>(L);
				assert(crep);
			}
			else
			{
				obj = static_cast<object_rep*>(lua_touserdata(L, index));
				assert((obj != 0) && "internal error, please report"); // internal error
				crep = obj->crep();
			}
			assert(crep);

			T* ptr = reinterpret_cast<T*>(crep->convert_to(LUABIND_TYPEID(T), obj, target));
			// if the pointer returned points into the converter storage,
			// we need to destruct it once the converter destructs
			if ((void*)ptr == (void*)target) destructor = detail::destruct_only_s<T>::apply;
			assert(!destructor || sizeof(T) <= 32);

			return *ptr;
		}

		template<class T>
		static int match(lua_State* L, by_const_reference<T>, int index)
		{
			// special case if we get nil in, try to match the holder type
			if (lua_isnil(L, index))
			{
				class_rep* crep = get_class_rep<T>(L);
				if (crep == 0) return -1;
				if ((LUABIND_TYPE_INFO_EQUAL(crep->holder_type(), LUABIND_TYPEID(T))))
					return 0;
				if ((LUABIND_TYPE_INFO_EQUAL(crep->const_holder_type(), LUABIND_TYPEID(T))))
					return 0;
				return -1;
			}

			object_rep* obj = is_class_object(L, index);
			if (obj == 0) return -1; // if the type is not one of our own registered types, classify it as a non-match

			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?-1:0;
			if ((LUABIND_TYPE_INFO_EQUAL(obj->crep()->const_holder_type(), LUABIND_TYPEID(T))))
				return (obj->flags() & object_rep::constant)?0:1;

            bool const_ = obj->flags() & object_rep::constant;
			int d;
			return implicit_cast(obj->crep(), LUABIND_TYPEID(T), d) + !const_;
		}

		~const_ref_converter()
		{
			if (destructor) destructor(target);
		}

		template<class T>
		void converter_postcall(lua_State* L, by_const_reference<T>, int index) 
		{
		}
	};

	// ****** enum converter ********

	template<class Direction = cpp_to_lua>
	struct enum_converter
	{
		void apply(lua_State* L, int val)
		{
			lua_pushnumber(L, (lua_Number)val);
		}
	};

	template<>
	struct enum_converter<lua_to_cpp>
	{
		template<class T>
		T apply(lua_State* L, by_value<T>, int index)
		{
//			std::cerr << "enum_converter\n";
			return static_cast<T>(static_cast<int>(lua_tonumber(L, index)));
		}
		
		template<class T>
		static int match(lua_State* L, by_value<T>, int index)
		{
			if (lua_isnumber(L, index)) return 0; else return -1;
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

	// ****** functor converter ********

	template<class Direction> struct functor_converter;

	template<>
	struct functor_converter<lua_to_cpp>
	{
		template<class T>
		functor<T> apply(lua_State* L, by_const_reference<functor<T> >, int index)
		{
			if (lua_isnil(L, index))
				return functor<T>();

			lua_pushvalue(L, index);
			detail::lua_reference ref;
			ref.set(L);
			return functor<T>(L, ref);
		}

		template<class T>
		functor<T> apply(lua_State* L, by_value<functor<T> >, int index)
		{
			if (lua_isnil(L, index))
				return functor<T>();

			lua_pushvalue(L, index);
			detail::lua_reference ref;
			ref.set(L);
			return functor<T>(L, ref);
		}

		template<class T>
		static int match(lua_State* L, by_const_reference<functor<T> >, int index)
		{
			if (lua_isfunction(L, index) || lua_isnil(L, index)) return 0; else return -1;
		}

		template<class T>
		static int match(lua_State* L, by_value<functor<T> >, int index)
		{
			if (lua_isfunction(L, index) || lua_isnil(L, index)) return 0; else return -1;
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};





// *********** default_policy *****************



	struct default_policy : converter_policy_tag
	{
		BOOST_STATIC_CONSTANT(bool, has_arg = true);

		template<class T>
		static void precall(lua_State*, T, int) {}

//		template<class T>
//		static void postcall(lua_State*, T, int) {}

		template<class T, class Direction>
		struct generate_converter
		{
			typedef typename boost::mpl::if_<is_user_defined<T>
						, user_defined_converter<Direction>
//						, typename boost::mpl::if_<is_implicit_conversion<T>
//							, implicit_converter<Direction>
							, typename boost::mpl::if_<is_primitive<T>
								, primitive_converter<Direction>
								, typename boost::mpl::if_<is_lua_functor<T>
									, functor_converter<Direction>
									, typename boost::mpl::if_<boost::is_enum<T>
										, enum_converter<Direction>
										, typename boost::mpl::if_<is_nonconst_pointer<T>
											, pointer_converter<Direction>
											, typename boost::mpl::if_<is_const_pointer<T>
												, const_pointer_converter<Direction>
												, typename boost::mpl::if_<is_nonconst_reference<T>
													, ref_converter<Direction>
													, typename boost::mpl::if_<is_const_reference<T>
														, const_ref_converter<Direction>
														, value_converter<Direction>
			>::type>::type>::type>::type>::type>::type>::type>::type type;
		};	
	};

// ********** get policy **********

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
	template<int N, class T>
	struct get_policy_list_impl
	{
		template<class U>
		struct inner
		{
			typedef typename U::head head;
			typedef typename U::tail tail;
			
			typedef typename boost::mpl::if_<boost::mpl::equal_to<boost::mpl::integral_c<int, head::index>, boost::mpl::integral_c<int, N> >
				,	policy_cons<head, typename get_policy_list_impl<N, tail>::type>
				,	typename get_policy_list_impl<N, tail>::type
			>::type type;
		};

		template<>
		struct inner<null_type>
		{
			typedef null_type type;
		};

		typedef typename inner<T>::type type;
	};
#else
	template<class List>
	struct get_policy_list_impl
	{
		template<int N>
		struct apply
		{
			typedef typename List::head head;
			typedef typename List::tail tail;

			typedef typename boost::mpl::if_<boost::mpl::equal_to<boost::mpl::integral_c<int, head::index>, boost::mpl::integral_c<int, N> >
				,	policy_cons<head, typename get_policy_list_impl<tail>::template apply<N>::type>
				,	typename get_policy_list_impl<tail>::template apply<N>::type
			>::type type;
		};
	};

	template<>
	struct get_policy_list_impl<detail::null_type>
	{
		template<int N>
		struct apply
		{
			typedef null_type type;
		};
	};
#endif

	template<int N, class T>
	struct get_policy_list
	{
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
		typedef typename get_policy_list_impl<N, T>::type type;
#else
		typedef typename get_policy_list_impl<T>::template apply<N>::type type;
#endif
	};

// ============== new policy system =================

	template<int, class> struct find_conversion_policy;

	template<bool IsConverter = false>
	struct find_conversion_impl
	{
		template<int N, class Policies>
		struct apply
		{
			typedef typename find_conversion_policy<N, typename Policies::tail>::type type;
		};
	};

	template<>
	struct find_conversion_impl<true>
	{
		template<int N, class Policies>
		struct apply
		{
			typedef typename Policies::head head;
			typedef typename Policies::tail tail;

			BOOST_STATIC_CONSTANT(bool, found = (N == head::index));

			typedef typename
				boost::mpl::if_c<found
					, head
					, typename find_conversion_policy<N, tail>::type
				>::type type;
		};
	};

	template<class Policies>
	struct find_conversion_impl2
	{
		template<int N>
		struct apply
			: find_conversion_impl<
				boost::is_base_and_derived<conversion_policy_base, typename Policies::head>::value
			>::template apply<N, Policies>
		{
		};
	};

	template<>
	struct find_conversion_impl2<detail::null_type>
	{
		template<int N>
		struct apply
		{
			typedef default_policy type;
		};
	};

	template<int N, class Policies>
	struct find_conversion_policy : find_conversion_impl2<Policies>::template apply<N>
	{
	};

	template<class List>
	struct policy_list_postcall
	{
		typedef typename List::head head;
		typedef typename List::tail tail;

		static void apply(lua_State* L, const index_map& i)
		{
			head::postcall(L, i);
			policy_list_postcall<tail>::apply(L, i);
		}
	};

	template<>
	struct policy_list_postcall<detail::null_type>
	{
		static void apply(lua_State*, const index_map&) {}
	};

/*	template<int N>
	struct find_conversion_policy<N, detail::null_type>
	{
		typedef default_policy type;
	};*/

// ==================================================

// ************** precall and postcall on policy_cons *********************


	template<class List>
	struct policy_precall
	{
		typedef typename List::head head;
		typedef typename List::tail tail;

		static void apply(lua_State* L, int index)
		{
			head::precall(L, index);
			policy_precall<tail>::apply(L, index);
		}
	};

	template<>
	struct policy_precall<detail::null_type>
	{
		static void apply(lua_State*, int) {}
	};

	template<class List>
	struct policy_postcall
	{
		typedef typename List::head head;
		typedef typename List::tail tail;

		static void apply(lua_State* L, int index)
		{
			head::postcall(L, index);
			policy_postcall<tail>::apply(L, index);
		}
	};

	template<>
	struct policy_postcall<detail::null_type>
	{
		static void apply(lua_State*, int) {}
	};

/* 
	struct pointer_only_converter
	{
		template<class T>
		static const T* apply(lua_State* L, type<const T*>, int index)
		{
			int a = index;
		}
	};
*/
	struct only_one_converter_policy_can_be_used_per_index {};

	template<class List, class T> struct assert_converter_policy_impl;

	template<class List>
	struct assert_converter_policy
	{
		template<class T>
		struct apply
		{
			typedef typename boost::mpl::if_<boost::is_base_and_derived<converter_policy_tag, typename List::head>
				, only_one_converter_policy_can_be_used_per_index
				, typename assert_converter_policy_impl<typename List::tail, T>::type
			>::type type;
		};
	};

	template<>
	struct assert_converter_policy<detail::null_type>
	{
		template<class T>
		struct apply
		{
			typedef T type;
		};
	};

	template<class List, class T> 
	struct assert_converter_policy_impl
	{
		typedef typename assert_converter_policy<List>::template apply<T>::type type;
	};

	template<class List>
	struct find_converter_policy_impl
	{
		typedef typename List::head head;
		typedef typename List::tail tail;

		typedef typename boost::mpl::if_<boost::is_base_and_derived<converter_policy_tag, head>
			, typename assert_converter_policy_impl<tail, head>::type
			, typename find_converter_policy_impl<tail>::type
		>::type type;
	};

	template<>
	struct find_converter_policy_impl<detail::null_type>
	{
		typedef default_policy type;
	};

}
/*
	namespace converters
	{
		template<class T>
		struct FROM
		{
			BOOST_STATIC_CONSTANT(bool, is_specialized = true);

			template<class U, int N>
			static U convert(lua_State* L, boost::mpl::int_<N>, detail::type<U>, int index)
			{
				typename luabind::detail::default_policy
					::generate_converter<T, detail::lua_to_cpp>::type c;
				return static_cast<U>(c.apply(L,
							LUABIND_DECORATE_TYPE(T), index));
			}

			template<class U>
			static std::pair<int,int> match(lua_State* L, boost::mpl::int_<N>, detail::type<U>, int index)
			{
				typedef typename luabind::detail::default_policy
					::generate_converter<T, detail::lua_to_cpp>::type c;

				int my_match = c::match(L, LUABIND_DECORATE_TYPE(T), index);

				std::pair<int,int> result = TO<N + 1, U>
					::match(L, boost::mpl::int_<N + 1>(), detail::type<U>(), index);

				if (my_match < result.first() && my_match != -1)
					return make_pair(my_match, N);
				else
					return result;
			}
		};
	}
*/
}


namespace luabind {	 namespace
{
	LUABIND_ANONYMOUS_FIX boost::arg<0> return_value;
	LUABIND_ANONYMOUS_FIX boost::arg<0> result;
}}

#include <luabind/detail/object_funs.hpp>

#endif // LUABIND_POLICY_HPP_INCLUDED

