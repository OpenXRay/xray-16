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


#if !BOOST_PP_IS_ITERATING

#ifndef LUABIND_OPERATORS_HPP_INCLUDED
#define LUABIND_OPERATORS_HPP_INCLUDED

#include <luabind/config.hpp>
#include <boost/config.hpp>

#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/iteration.hpp>


#include <boost/mpl/if.hpp>
#include <boost/mpl/vector.hpp>

#include <luabind/detail/other.hpp>
#include <luabind/detail/operator_id.hpp>
#include <luabind/detail/signature_match.hpp>
#include <luabind/detail/policy.hpp>

namespace luabind { namespace detail {


	template<int N> struct execute_selector;

	template<class T>
	struct policies_with_storage : T
	{
		char storage;
	};

#if defined (BOOST_MSVC) && (BOOST_MSVC <= 1200)
	#define LUABIND_MSVC6_NO_TYPENAME
#else
	#define LUABIND_MSVC6_NO_TYPENAME typename
#endif

//#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
	#define LUABIND_CONVERT_PARAMETER conv_self.apply(L, LUABIND_DECORATE_TYPE(Self&), 1)
//#else
//	#define LUABIND_CONVERT_PARAMETER conv_self::template apply<Self_>(L, LUABIND_DECORATE_TYPE(Self&), 1)
//#endif


	#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/operators.hpp>, 1))
	#include BOOST_PP_ITERATE()

	#define LUABIND_UNWRAP_PARAM(z,n,_) LUABIND_MSVC6_NO_TYPENAME unwrap_other<A##n>::type

	// Signature is a constructor<...> with all the parameter types in it
	// constant is true if the application operator is const
	template<class Signature, bool Constant, class Policies = null_type>
	struct application_operator
	{
		template<class Self>
		struct apply: execute_selector<Signature::arity>::template apply<Signature, Constant, Self, Policies> {};

		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
		static inline int match_impl(
					lua_State* L,
					const constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, A)>*)
		{
			typedef constructor<BOOST_PP_ENUM(LUABIND_MAX_ARITY, LUABIND_UNWRAP_PARAM, _)> unwrapped_sig;

			object_rep* obj = is_class_object(L, 1);
			if (obj == 0) return -1;

			bool constant_obj = obj->flags() & object_rep::constant;

			if (!Constant && constant_obj) return -1;
//			if (lua_gettop(L) != Signature::arity + 1) return -1;

			int i = match_params(L, 2, static_cast<const unwrapped_sig*>(0), static_cast<const Policies*>(0));
			if (Constant && !constant_obj) i++;
			return i;
		}

		static inline int match(lua_State* L)
		{
			return match_impl(L, static_cast<const Signature*>(0));
		}
	};

	#undef LUABIND_UNWRAP_PARAM

	// this is the type that represents all the operators
	// this is returned from all operators on self_t
	template<class id, class L, class R = null_type>
	struct operator_
	{
	};
	
	struct self_t
	{
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/operators.hpp>, 2))
		#include BOOST_PP_ITERATE()

		operator_<op_tostring_tag, self_t, null_type> tostring;

		template<class R>
		inline operator_<op_le_tag, self_t, R> operator<=(const R&)
		{
			return operator_<op_le_tag, self_t, R>();
		}
	};

	struct const_self_t
	{
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/operators.hpp>, 3))
		#include BOOST_PP_ITERATE()

		operator_<op_tostring_tag, const_self_t, null_type> tostring;

		template<class R>
		inline operator_<op_le_tag, const_self_t, R> operator<=(const R&)
		{
			return operator_<op_le_tag, const_self_t, R>();
		}
	};

	// TODO: fix this. The type cannot be a value type for all cases
	template<class T, class Policy>
	inline int convert_result(lua_State* L, T v, const Policy*)
	{
		typedef typename find_conversion_policy<0, Policy>::type converter_policy;
		typename converter_policy::template generate_converter<T, cpp_to_lua>::type ret_converter;
		ret_converter.apply(L, v);
		return 1;
	}

	// this is an implementation that is specialized
	// for each operator and used by operator_
	template<class id>
	struct binary_operator
	{
		template<class Policies, class Left, class Right>
		struct impl;
	};

	template<class id>
	struct unary_operator
	{
		template<class Policies, class Left> struct impl;
	};

	template<class Policies, class id, class Self, class L, class R = null_type>
	struct operator_unwrapper :
		::boost::mpl::if_<
			::boost::is_same<detail::null_type, R>,
			// if this is true, it is a unary operator
			typename unary_operator<id>::template impl<
				Policies,
				typename ::boost::mpl::if_<
					boost::is_same<detail::self_t, L>,
					Self&,
					typename ::boost::mpl::if_<
						::boost::is_same<detail::const_self_t, L>,
						const Self&,
						typename unwrap_other<L>::type
					>::type
				>::type
			>,
			// else, if this is a binary operator	
			typename binary_operator<id>::template impl<
				Policies,
				// extract the left type and substitute self_t and const_self_t with the real self type
				// also, unwrap the type if it's wrapped in other<>
				typename ::boost::mpl::if_<
					boost::is_same<detail::self_t, L>,
					Self&,
					typename ::boost::mpl::if_<
						::boost::is_same<detail::const_self_t, L>,
						const Self&,
						typename unwrap_other<L>::type>::type
					>::type,
				// same thing but with the right type
				typename ::boost::mpl::if_<
					boost::is_same<detail::self_t, R>,
					Self&,
					typename ::boost::mpl::if_<
						::boost::is_same<detail::const_self_t, R>,
						const Self&,
						typename unwrap_other<R>::type
					>::type
				>::type
			>
		>::type 
	{};
}}

namespace luabind
{
	namespace
	{
		LUABIND_ANONYMOUS_FIX detail::self_t self;
		LUABIND_ANONYMOUS_FIX detail::const_self_t const_self;
	}

#define LUABIND_BINARY_OPERATOR(id, op)\
	namespace detail {\
 		template<>\
		struct binary_operator<op_##id##_tag>\
 		{\
 			template<class Policies, class Left, class Right>\
 			struct impl\
 			{\
				typedef typename unwrap_other<Left>::type left_t; \
				typedef typename unwrap_other<Right>::type right_t; \
				static inline operator_id get_id() { return op_##id; } \
 				static inline int execute(lua_State* L)\
 				{\
					typedef typename find_conversion_policy<1, Policies>::type converter_policy_left; \
					typename converter_policy_left::template generate_converter<left_t, lua_to_cpp>::type converter_left; \
					typedef typename find_conversion_policy<2, Policies>::type converter_policy_right; \
					typename converter_policy_right::template generate_converter<right_t, lua_to_cpp>::type converter_right; \
					int ret = convert_result(L, converter_left.apply(L, LUABIND_DECORATE_TYPE(left_t), 1) op converter_right.apply(L, LUABIND_DECORATE_TYPE(right_t), 2), static_cast<const Policies*>(0));\
					return ret;\
 				}\
 				static int match(lua_State* L)\
 				{\
					return match_params(L, 1, static_cast<constructor<left_t, right_t>*>(0), static_cast<Policies*>(0));\
				}\
			};\
 		};\
	}\
	\
	namespace detail\
	{\
	inline detail::operator_<detail::op_##id##_tag, detail::self_t, detail::self_t> operator op(const detail::self_t&, const detail::self_t&)\
	{\
		return detail::operator_<detail::op_##id##_tag, detail::self_t, detail::self_t>();\
	}\
	template<class L>\
	inline detail::operator_<detail::op_##id##_tag, L, detail::self_t> operator op(const L&, const detail::self_t&)\
	{\
		return detail::operator_<detail::op_##id##_tag, L, detail::self_t>();\
	}\
	template<class R>\
	inline detail::operator_<detail::op_##id##_tag, detail::self_t, R> operator op(const detail::self_t&, const R&)\
	{\
		return detail::operator_<detail::op_##id##_tag, detail::self_t, R>();\
	}\
	inline detail::operator_<detail::op_##id##_tag, detail::const_self_t, detail::const_self_t> operator op(const detail::const_self_t&, const detail::const_self_t&)\
	{\
		return detail::operator_<detail::op_##id##_tag, detail::const_self_t, detail::const_self_t>();\
	}\
	template<class L>\
	inline detail::operator_<detail::op_##id##_tag, L, detail::const_self_t> operator op(const L&, const detail::const_self_t&)\
	{\
		return detail::operator_<detail::op_##id##_tag, L, detail::const_self_t>();\
	}\
	template<class R>\
	inline detail::operator_<detail::op_##id##_tag, detail::const_self_t, R> operator op(const detail::const_self_t&, const R&)\
	{\
		return detail::operator_<detail::op_##id##_tag, detail::const_self_t, R>();\
	}\
	}

	LUABIND_BINARY_OPERATOR(add,+)
	LUABIND_BINARY_OPERATOR(sub,-)
	LUABIND_BINARY_OPERATOR(div,/)
	LUABIND_BINARY_OPERATOR(mul,*)
	LUABIND_BINARY_OPERATOR(pow,^)
	LUABIND_BINARY_OPERATOR(lt, <)
	LUABIND_BINARY_OPERATOR(le, <=)
	LUABIND_BINARY_OPERATOR(gt, >)
	LUABIND_BINARY_OPERATOR(ge, >=)
	LUABIND_BINARY_OPERATOR(eq, ==)

#undef LUABIND_BINARY_OPERATOR

	namespace detail {
 		template<>
		struct binary_operator<op_le_tag>
 		{
 			template<class Policies, class Left, class Right>
 			struct impl
 			{
				typedef typename unwrap_other<Left>::type left_t;
				typedef typename unwrap_other<Right>::type right_t;
				static inline operator_id get_id() { return op_le; }
 				static inline int execute(lua_State* L)
 				{
					typedef typename find_conversion_policy<1, Policies>::type converter_policy_left;
					typename converter_policy_left::template generate_converter<left_t, lua_to_cpp>::type converter_left;
					typedef typename find_conversion_policy<2, Policies>::type converter_policy_right; 
					typename converter_policy_right::template generate_converter<right_t, lua_to_cpp>::type converter_right; 
					int ret = convert_result(L, converter_left.apply(L, LUABIND_DECORATE_TYPE(left_t), 1) <= converter_right.apply(L, LUABIND_DECORATE_TYPE(right_t), 2), static_cast<const Policies*>(0));
					return ret;
 				}
 				static int match(lua_State* L)
 				{
					return match_params(L, 1, static_cast<constructor<left_t, right_t>*>(0), static_cast<Policies*>(0));
				}
			};
 		};
	}
	
	namespace detail
	{
	inline detail::operator_<detail::op_le_tag, detail::self_t, detail::self_t> operator <=(const detail::self_t&, const detail::self_t&)
	{
		return detail::operator_<detail::op_le_tag, detail::self_t, detail::self_t>();
	}
	template<class L>
	inline detail::operator_<detail::op_le_tag, L, detail::self_t> operator <=(const L&, const detail::self_t&)
	{
		return detail::operator_<detail::op_le_tag, L, detail::self_t>();
	}

	inline detail::operator_<detail::op_le_tag, detail::const_self_t, detail::const_self_t> operator <=(const detail::const_self_t&, const detail::const_self_t&)
	{
		return detail::operator_<detail::op_le_tag, detail::const_self_t, detail::const_self_t>();
	}
	template<class L>
	inline detail::operator_<detail::op_le_tag, L, detail::const_self_t> operator <=(const L&, const detail::const_self_t&)
	{
		return detail::operator_<detail::op_le_tag, L, detail::const_self_t>();
	}
	}

#define LUABIND_UNARY_OPERATOR(id, op)\
	namespace detail\
	{\
		template<>\
		struct unary_operator<op_##id##_tag>\
		{\
			template<class Policies, class Left>\
			struct impl\
			{\
				typedef typename unwrap_other<Left>::type left_t;\
				typedef detail::null_type right_t;\
				static inline operator_id get_id() { return op_##id; }\
				static inline int execute(lua_State* L)\
				{\
					typedef typename find_conversion_policy<1, Policies>::type converter_policy_left; \
					typename converter_policy_left::template generate_converter<left_t, lua_to_cpp>::type converter_left;\
					return convert_result(L, op converter_left.apply(L, LUABIND_DECORATE_TYPE(left_t), 1), static_cast<const Policies*>(0));\
				}\
				static inline int match(lua_State* L)\
				{\
					return match_params(L, 1, static_cast<constructor<left_t>*>(0), static_cast<Policies*>(0));\
				}\
			};\
		};\
	}\
	\
	inline detail::operator_<detail::op_##id##_tag, detail::self_t, detail::null_type> operator op(const detail::self_t&)\
	{\
		return detail::operator_<detail::op_##id##_tag, detail::self_t, detail::null_type>();\
	}\
	inline detail::operator_<detail::op_##id##_tag, detail::const_self_t, detail::null_type> operator op(const detail::const_self_t&)\
	{\
		return detail::operator_<detail::op_##id##_tag, detail::const_self_t, detail::null_type>();\
	}

	LUABIND_UNARY_OPERATOR(unm,-);

	namespace detail
	{
		template<>
		struct unary_operator<op_tostring_tag>
		{
			template<class Policies, class Left>
			struct impl
			{
				typedef typename unwrap_other<Left>::type left_t;
				typedef detail::null_type right_t;
				static inline operator_id get_id() { return op_tostring; }
				static inline int execute(lua_State* L)
				{
					// TODO: Should policies apply to this operator? shouldn't the string returntype be enforced?
					typedef typename find_conversion_policy<1, Policies> :: type converter_policy_left;
					typename converter_policy_left :: template generate_converter<left_t, lua_to_cpp> :: type converter_left;

#ifdef BOOST_NO_STRINGSTREAM
					strstream_class s;
					s << converter_left.apply(L, LUABIND_DECORATE_TYPE(left_t), 1) << std::ends;
#else
					stringstream_class s;
					s << converter_left.apply(L, LUABIND_DECORATE_TYPE(left_t), 1);
#endif
					return convert_result(L, s.str(), static_cast<const Policies*>(0));
				}
				static inline int match(lua_State* L)
				{
					return match_params(L, 1, static_cast<constructor<left_t>*>(0), static_cast<Policies*>(0));
				}
			};
		};
	}

	inline detail::operator_<detail::op_tostring_tag, detail::self_t, detail::null_type> tostring(const detail::self_t&)
	{
		return detail::operator_<detail::op_tostring_tag, detail::self_t, detail::null_type>();
	}
	inline detail::operator_<detail::op_tostring_tag, detail::const_self_t, detail::null_type> tostring(const detail::const_self_t&)
	{
		return detail::operator_<detail::op_tostring_tag, detail::const_self_t, detail::null_type>();
	}


#undef LUABIND_UNARY_OPERATOR
#undef LUABIND_MSVC6_NO_TYPENAME
}

#endif // LUABIND_OPERATORS_HPP_INCLUDED


#elif BOOST_PP_ITERATION_FLAGS() == 1

#define LUABIND_DECL(z,n,_) typedef typename find_conversion_policy<n + 1, Policies>::type BOOST_PP_CAT(converter_policy,n); \
		typedef typename unwrap_other<A##n>::type unwrapped_a##n; \
		typename BOOST_PP_CAT(converter_policy,n)::template generate_converter<unwrapped_a##n, lua_to_cpp>::type BOOST_PP_CAT(c,n);

#define LUABIND_PARAM(z,n,_) BOOST_PP_CAT(c,n).apply(L, LUABIND_DECORATE_TYPE(unwrapped_a##n), n+2)

	template<>
	struct execute_selector<BOOST_PP_ITERATION()>
	{
		template<class Signature, bool Constant, class Self_, class Policies>
		struct apply
		{
			static inline int execute(lua_State* L)
			{
				return execute_impl(L, static_cast<const Signature*>(0));
			}

			template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
			static inline int execute_impl(
						lua_State* L,
						const constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, A)>*)
			{
				// TODO: use policies here, instead of default_policy, or shouldn't we?
				typename boost::mpl::apply_if_c<Constant,
					LUABIND_MSVC6_NO_TYPENAME default_policy::template generate_converter<const Self_&, lua_to_cpp>,
					LUABIND_MSVC6_NO_TYPENAME default_policy::template generate_converter<Self_&, lua_to_cpp>
				>::type conv_self;

				typedef typename boost::mpl::if_c<Constant,
					const Self_,
					Self_
				>::type Self;

				BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)

				return convert_result(L, conv_self.apply(L, LUABIND_DECORATE_TYPE(Self&), 1)
					(
						BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAM, _)
					), static_cast<Policies*>(0));
			}
		};
	};

#undef LUABIND_DECL
#undef LUABIND_PARAM

#elif BOOST_PP_ITERATION_FLAGS() == 2 // self_t

#define LUABIND_UNWRAP_PARAM(z,n,_) A##n

#if BOOST_PP_ITERATION() > 0
template<BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
#endif
application_operator< constructor<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_UNWRAP_PARAM, _)>, false>*
operator()(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)) { return 0; }

#undef LUABIND_UNWRAP_PARAM

#elif BOOST_PP_ITERATION_FLAGS() == 3 // const_self_t

#define LUABIND_UNWRAP_PARAM(z,n,_) A##n

#if BOOST_PP_ITERATION() > 0
template<BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
#endif
application_operator< constructor<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_UNWRAP_PARAM, _)>, true>*
operator()(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)) { return 0; }

#undef LUABIND_UNWRAP_PARAM

#endif

