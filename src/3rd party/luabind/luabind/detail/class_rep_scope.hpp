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


#ifndef LUABIND_CLASS_REP_HPP_INCLUDED
#define LUABIND_CLASS_REP_HPP_INCLUDED

//#include <cstdlib>

#include <boost/limits.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>

#include <luabind/config.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/construct_rep.hpp>
#include <luabind/detail/method_rep.hpp>
#include <luabind/detail/garbage_collector.hpp>
#include <luabind/detail/operator_id.hpp>
#include <luabind/detail/signature_match.hpp>
#include <luabind/detail/class_registry.hpp>
#include <luabind/detail/find_best_match.hpp>
#include <luabind/detail/get_overload_signature.hpp>
#include <luabind/error.hpp>

namespace luabind
{

	template<BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(LUABIND_MAX_BASES, class A, detail::null_type)>
	struct bases {};
	typedef bases<detail::null_type> no_bases;

	struct class_base;

}

namespace luabind { namespace detail
{
	string_class stack_content_by_name(lua_State* L, int start_index);
	int construct_lua_class_callback(lua_State* L);

	// this is class-specific information, poor man's vtable
	// this is allocated statically (removed by the compiler)
	// a pointer to this structure is stored in the lua tables'
	// metatable with the name __classrep
	// it is used when matching parameters to function calls
	// to determine possible implicit casts
	// it is also used when finding the best match for overloaded
	// methods
	class class_rep
	{
	friend struct luabind::class_base;
	friend int super_callback(lua_State*);
//TODO: avoid the lua-prefix
	friend int lua_class_gettable(lua_State*);
	friend int lua_class_settable(lua_State*);
	friend int static_class_gettable(lua_State*);
	public:

		enum class_type
		{
			cpp_class = 0,
			lua_class = 1
		};

		// destructor is a lua callback function that is hooked as garbage collector event on every instance
		// of this class (including those that is not owned by lua). It gets an object_rep as argument
		// on the lua stack. It should delete the object pointed to by object_rep::ptr if object_pre::flags
		// is object_rep::owner (which means that lua owns the object)

		// EXPECTS THE TOP VALUE ON THE LUA STACK TO
		// BE THE USER DATA WHERE THIS CLASS IS BEING
		// INSTANTIATED!
		class_rep(LUABIND_TYPE_INFO t, const char* name, lua_State* L, void(*destructor)(void*), LUABIND_TYPE_INFO held_t, void*(*extractor)(void*))
			: m_type(t)
			, m_held_type(held_t)
			, m_extract_underlying_fun(extractor)
			, m_name(name)
			, m_class_type(cpp_class)
			, m_destructor(destructor)
		{

			// TODO: don't we need to copy the name?
			class_registry* r = class_registry::get_registry(L);
			assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()"); // you must call luabind::open()

			detail::getref(L, r->cpp_class());
			lua_setmetatable(L, -2);

			lua_pushvalue(L, -1); // duplicate our user data
			m_self_ref = detail::ref(L); // pop one of them

			m_instance_metatable = r->cpp_instance();
		}

		// used when creating a lua class
		// EXPECTS THE TOP VALUE ON THE LUA STACK TO
		// BE THE USER DATA WHERE THIS CLASS IS BEING
		// INSTANTIATED!
		class_rep(lua_State* L, const char* name)
			: m_type(LUABIND_TYPEID(int))
			, m_held_type(0)
			, m_extract_underlying_fun(0)
			, m_name(name)
			, m_class_type(lua_class)
		{
			// TODO: don't we need to copy the name?
			lua_newtable(L);
			m_table_ref = detail::ref(L);

			class_registry* r = class_registry::get_registry(L);
			assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()"); // you must call luabind::open()

			detail::getref(L, r->lua_class());
			lua_setmetatable(L, -2);
			lua_pushvalue(L, -1); // duplicate our user data
			m_self_ref = detail::ref(L); // pop one of them

			m_instance_metatable = r->lua_instance();
		}

		~class_rep()
		{
#ifndef LUABIND_DONT_COPY_STRINGS
			for (vector_class<char*>::iterator i = m_strings.begin(); i != m_strings.end(); ++i)
			{
				delete[] *i;
			}
#endif
		}


		// called from the metamethod for __index
		// the object pointer is passed on the lua stack
		int gettable(lua_State* L)
		{
			if (lua_isnil(L, 2))
			{
				lua_pushnil(L);
				return 1;
			}

			// we have to ignore the first argument since this may point to
			// a method that is not present in this class (but in a subclass)
			const char* key = lua_tostring(L, 2);
			map_class<const char*, method_rep, ltstr>::iterator i = m_methods.find(key);
			if (i != m_methods.end())
			{
				// the name is a method, return it
				lua_pushlightuserdata(L, &i->second);
				lua_pushcclosure(L, function_dispatcher, 1);
				return 1;
			}

			map_class<const char*, callback, ltstr>::iterator j = m_getters.find(key);
			if (j != m_getters.end())
			{
				// the name is a data member
				return j->second.func(L, j->second.pointer_offset);
			}

			lua_pushnil(L);
			return 1;
		}

		// called from the metamethod for __newindex
		// the object pointer is passed on the lua stack
		bool settable(lua_State* L)
		{
			if (lua_isnil(L, 2))
			{
				return false;
			}

			// we have to ignore the first argument since this may point to
			// a method that is not present in this class (but in a subclass)
			const char* key = lua_tostring(L, 2);
			map_class<const char*, callback, ltstr>::iterator j = m_setters.find(key);
			if (j != m_setters.end())
			{
				// the name is a data member
				j->second.func(L, j->second.pointer_offset);
				return true;
			}

			return false; // false means that we don't have a member with the given name
		}

		// this is called as __index metamethod on every instance of this class
		static int gettable_dispatcher(lua_State* L)
		{
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
			return obj->crep()->gettable(L);
		}

		// this is called as __newindex metamethod on every instance of this class
		static int settable_dispatcher(lua_State* L)
		{
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));

			bool success = obj->crep()->settable(L);

#ifndef LUABIND_NO_ERROR_CHECKING

			if (!success)
			{
				// this block is needed to make sure the string_class is destructed before
				// lua_error() is called
#ifdef BOOST_MSVC
				{
					// msvc has a bug which deletes the string twice, that's
					// why we have to create it on the heap
					string_class msg = string_class("cannot set attribute '");
					msg += obj->crep()->m_name;
					msg += ".";
					msg += lua_tostring(L, -2);
					msg += "'";
					lua_pushstring(L, msg.c_str());
				}
#else
				{
					string_class msg = "cannot set attribute '";
					msg += obj->crep()->m_name;
					msg += ".";
					msg += lua_tostring(L, -2);
					msg += "'";
					lua_pushstring(L, msg.c_str());
				}
#endif
				lua_error(L);
			}

#endif

			return 0;
		}

		static int operator_dispatcher(lua_State* L)
		{
			int id = static_cast<int>(lua_tonumber(L, lua_upvalueindex(1)));

			int operand_id = 0;

			object_rep* operand[2];
			for (int i = 0; i < 2; ++i)
				operand[i] = detail::is_class_object(L, i + 1);

			if (operand[0] && operand[1])
				if (LUABIND_TYPE_INFO_EQUAL(operand[0]->crep()->type(), operand[1]->crep()->type())) operand[1] = 0;

			vector_class<operator_callback>* overloads[2];
			for (int i = 0; i < 2; ++i)
				if (operand[i]) overloads[i] = &operand[i]->crep()->m_operators[id]; else overloads[i] = 0;

			std::size_t num_overloads[2];
			for (int i = 0; i < 2; ++i)
				if (overloads[i]) num_overloads[i] = overloads[i]->size(); else num_overloads[i] = 0;

			bool ambiguous = false;
			int match_index = -1;
			int min_match = std::numeric_limits<int>::max();

//			std::cout << "operator_dispatcher\n";
//			std::cout << "num overloads: " << num_overloads[0] + num_overloads[1] << "\n";
//			std::cout << "operator: " << id << "\n";

#ifdef LUABIND_NO_ERROR_CHECKING

			if (num_overloads[0] == 1 && num_overloads[1] == 0)
			{
				operand_id = 0;
				match_index = 0;
			}
			else if (num_overloads[0] == 0 && num_overloads[1] == 1)
			{
				operand_id = 1;
				match_index = 0;
			}
			else
			{

#endif

				int num_params = lua_gettop(L);
				if (overloads[0])
				{
					if (find_best_match(L, overloads[0]->begin(), overloads[0]->end(), ambiguous, min_match, match_index, num_params))
						operand_id = 0;
				}

				// have look at the right operand.
				// if the right operand is a class and
				// not the same class as this, we have to
				// try to match it's operators too	

				if (overloads[1])
				{
					if(find_best_match(L, overloads[1]->begin(), overloads[1]->end(), ambiguous, min_match, match_index, num_params))
						operand_id = 1;
				}

#ifdef LUABIND_NO_ERROR_CHECKING

			}

#else

			if (match_index == -1)
			{
				// this block is needed to make sure the string_class is destructed before
				// lua_error() is called
				{
					string_class msg = "no operator ";
					msg += get_operator_symbol(id);
					msg += " matched the arguments (";
					msg += stack_content_by_name(L, 1);
					msg += ")\ncandidates are:\n";

					if (overloads[0])
						msg += get_overload_signatures(L, overloads[0]->begin(), overloads[0]->end(), get_operator_symbol(id));

					if (overloads[1])
						msg += get_overload_signatures(L, overloads[1]->begin(), overloads[1]->end(), get_operator_symbol(id));
					
					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}
			else if (ambiguous)
			{
				// this block is needed to make sure the string_class is destructed before
				// lua_error() is called
				{
					string_class msg = "call of overloaded operator ";
					msg += get_operator_symbol(id);
					msg += " (";
					msg += stack_content_by_name(L, 1);
					msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

					vector_class<operator_callback> candidates;
					if (overloads[0])
						find_exact_match(L, overloads[0]->begin(), overloads[0]->end(), min_match, num_params, candidates);

					if (overloads[1])
						find_exact_match(L, overloads[1]->begin(), overloads[1]->end(), min_match, num_params, candidates);

					msg += get_overload_signatures(L, candidates.begin(), candidates.end(), get_operator_symbol(id));


					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}

#endif

			return (*overloads[operand_id])[match_index].call(L);
		}


		// this is called as metamethod __call on the class_rep.
		static int constructor_dispatcher(lua_State* L)
		{
			class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));
			construct_rep* rep = &crep->m_constructor;

			bool ambiguous = false;
			int match_index = -1;
			int min_match = std::numeric_limits<int>::max();
			bool found;

#ifdef LUABIND_NO_ERROR_CHECKING

			if (rep->overloads.size() == 1)
			{
				match_index = 0;
			}
			else
			{

#endif

				int num_params = lua_gettop(L) - 1;
				found = find_best_match(L, rep->overloads.begin(), rep->overloads.end(), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING

			}

#else

			if (!found)
			{
				// this block is needed to make sure the string_class is destructed before
				// lua_error() is called
				{
					string_class msg = "no constructor of '";
					msg += crep->name();
					msg += "' matched the arguments (";
					msg += stack_content_by_name(L, 2);
					msg += ")\n candidates are:\n";

					msg += get_overload_signatures(L, rep->overloads.begin(), rep->overloads.end(), crep->name());

					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}
			else if (ambiguous)
			{
				// this block is needed to make sure the string_class is destructed before
				// lua_error() is called
				{
					string_class msg = "call of overloaded constructor '";
					msg += crep->m_name;
					msg +=  "(";
					msg += stack_content_by_name(L, 2);
					msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

					vector_class<construct_rep::overload_t> candidates;
					find_exact_match(L, rep->overloads.begin(), rep->overloads.end(), min_match, num_params, candidates);
					msg += get_overload_signatures(L, candidates.begin(), candidates.end(), crep->name());

					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}

#endif

#ifndef LUABIND_NO_EXCEPTIONS

			try
			{

#endif

				void* object_ptr = rep->overloads[match_index].construct(L);

				void* obj_rep = lua_newuserdata(L, sizeof(object_rep));
				new(obj_rep) object_rep(object_ptr, crep, object_rep::owner, crep->destructor());

				detail::getref(L, crep->m_instance_metatable);
				lua_setmetatable(L, -2);
				return 1;

#ifndef LUABIND_NO_EXCEPTIONS

			}

			catch(const std::exception& e)
			{
				lua_pushstring(L, e.what());
			}
			catch(const char* s)
			{
				lua_pushstring(L, s);
			}
			catch(...)
			{
				{
					string_class msg = crep->name();
					msg += "() threw an exception";
					lua_pushstring(L, msg.c_str());
				}
			}

			// we can only reach this line if an exception was thrown
			lua_error(L);
			return 0; // will never be reached

#endif

		}

		static int implicit_cast(const class_rep* from, const class_rep* to, int& pointer_offset)
		{
			int offset = 0;
			if (LUABIND_TYPE_INFO_EQUAL(from->type(), to->type())) return 0;

			for (vector_class<class_rep::base_info>::const_iterator i = from->bases().begin(); i != from->bases().end(); ++i)
			{
				int steps = implicit_cast(i->base, to, offset);
				pointer_offset = offset + i->pointer_offset;
				if (steps >= 0) return steps + 2;
			}
			return -1;
		}

		// the functions dispatcher assumes the following:
		// there is one upvalue that points to the method_rep that this dispatcher is to call
		// the first parameter on the lua stack is an object_rep that points to the object the
		// call is being made on
		static int function_dispatcher(lua_State* L)
		{
			method_rep* rep = static_cast<method_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
			object_rep* obj = reinterpret_cast<object_rep*>(lua_touserdata(L, 1));

#ifndef LUABIND_NO_ERROR_CHECKING

			if (is_class_object(L, 1) == 0)
			{
				{
					string_class msg = "No self reference given as first parameter to member function '";
					msg += rep->crep->name();
					msg += ":";
					msg += rep->name;
					msg += "'. Have you used '.' instead of ':'?";

					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}

			int p;
			if (implicit_cast(obj->crep(), rep->crep, p) < 0)
			{
				{
					string_class msg = "invalid self reference given to '";
					msg += rep->crep->name();
					msg += ":";
					msg += rep->name;
					msg += "'";
					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}

#endif

			bool ambiguous = false;
			int match_index = -1;
			int min_match = std::numeric_limits<int>::max();
			bool found;

#ifdef LUABIND_NO_ERROR_CHECKING
			if (rep->overloads().size() == 1)
			{
				match_index = 0;
			}
			else
			{
#endif

				int num_params = lua_gettop(L) - 1;
				found = find_best_match(L, rep->overloads().begin(), rep->overloads().end(), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING

			}

#else

			if (!found)
			{
				{
					string_class msg = "no overload of  '";
					msg += rep->crep->name();
					msg += ":";
					msg += rep->name;
					msg += "' matched the arguments (";
					msg += stack_content_by_name(L, 2);
					msg += ")\ncandidates are:\n";

					string_class function_name;
					function_name += rep->crep->name();
					function_name += ":";
					function_name += rep->name;

					msg += get_overload_signatures(L, rep->overloads().begin(), rep->overloads().end(), function_name);

					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}
			else if (ambiguous)
			{
				{
					string_class msg = "call of overloaded  '";
					msg += rep->crep->name();
					msg += ":";
					msg += rep->name;
					msg += "(";
					msg += stack_content_by_name(L, 2);
					msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

					vector_class<overload_rep> candidates;
					find_exact_match(L, rep->overloads().begin(), rep->overloads().end(), min_match, num_params, candidates);

					string_class function_name;
					function_name += rep->crep->name();
					function_name += ":";
					function_name += rep->name;

					msg += get_overload_signatures(L, candidates.begin(), candidates.end(), function_name);

					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}

#endif

#ifndef LUABIND_NO_EXCEPTIONS

			try
			{

#endif

				const overload_rep& o = rep->overloads()[match_index];
				return o.call(L, *obj);

#ifndef LUABIND_NO_EXCEPTIONS

			}
			catch(const std::exception& e)
			{
				lua_pushstring(L, e.what());
			}
			catch (const char* s)
			{
				lua_pushstring(L, s);
			}
			catch(...)
			{
				string_class msg = rep->crep->name();
				msg += ":";
				msg += rep->name;
				msg += "() threw an exception";
				lua_pushstring(L, msg.c_str());
			}
			// we can only reach this line if an exception was thrown
			lua_error(L);
			return 0; // will never be reached

#endif
			
		}






		struct base_info
		{
			int pointer_offset; // the offset added to the pointer to obtain a basepointer (due to multiple-inheritance)
			class_rep* base;
		};


		inline void add_base_class(const base_info& binfo)
		{
			// If you hit this assert you are deriving from a type that is not registered
			// in lua. That is, in the class_<> you are giving a baseclass that isn't registered.
			// Please note that if you don't need to have access to the base class or the
			// conversion from the derived class to the base class, you don't need
			// to tell luabind that it derives.
			assert(binfo.base && "You cannot derive from an unregistered type");

			class_rep* bcrep = binfo.base;

			// import all functions from the base
			for (map_class<const char*, method_rep, ltstr>::const_iterator i = bcrep->m_methods.begin(); i != bcrep->m_methods.end(); ++i)
			{
#ifndef LUABIND_DONT_COPY_STRINGS
				m_strings.push_back(dup_string(i->first));
				method_rep& m = m_methods[m_strings.back()];
#else
				method_rep& m = m_methods[i->first];
#endif
				m.name = i->first;
				m.crep = this;

				for (vector_class<overload_rep>::const_iterator j = i->second.overloads().begin(); j != i->second.overloads().end(); ++j)
				{
					overload_rep o = *j;
					o.add_offset(binfo.pointer_offset);
					m.add_overload(o);
				}
			}

			// import all getters from the base
			for (map_class<const char*, callback, ltstr>::const_iterator i = bcrep->m_getters.begin(); i != bcrep->m_getters.end(); ++i)
			{
#ifndef LUABIND_DONT_COPY_STRINGS
				m_strings.push_back(dup_string(i->first));
				callback& m = m_getters[m_strings.back()];
#else
				callback& m = m_getters[i->first];
#endif
				m.pointer_offset = i->second.pointer_offset + binfo.pointer_offset;
				m.func = i->second.func;
			}

			// import all setters from the base
			for (map_class<const char*, callback, ltstr>::const_iterator i = bcrep->m_setters.begin(); i != bcrep->m_setters.end(); ++i)
			{
#ifndef LUABIND_DONT_COPY_STRINGS
				// TODO: optimize this by not copying the string if it already exists in m_setters.
				// This goes for m_getters, m_static_constants and m_functions too. Both here
				// in add_base() and in the add_function(), add_getter() ... functions.
				m_strings.push_back(dup_string(i->first));
				callback& m = m_setters[m_strings.back()];
#else
				callback& m = m_setters[i->first];
#endif
				m.pointer_offset = i->second.pointer_offset + binfo.pointer_offset;
				m.func = i->second.func;
			}

			// import all static constants
			for (map_class<const char*, int, ltstr>::const_iterator i = bcrep->m_static_constants.begin(); i != bcrep->m_static_constants.end(); ++i)
			{
#ifndef LUABIND_DONT_COPY_STRINGS
				m_strings.push_back(dup_string(i->first));
				int& v = m_static_constants[m_strings.back()];
#else
				int& v = m_static_constants[i->first];
#endif
				v = i->second;
			}

			// import all operators
			for (int i = 0; i < number_of_operators; ++i)
			{
				for (vector_class<operator_callback>::const_iterator j = bcrep->m_operators[i].begin(); j != bcrep->m_operators[i].end(); ++j)
					m_operators[i].push_back(*j);
			}

			// also, save the baseclass info to be used for typecasts
			m_bases.push_back(binfo);
		}


		inline const vector_class<base_info>& bases() const throw() { return m_bases; }
		inline LUABIND_TYPE_INFO type() const throw() { return m_type; }
		inline void set_type(LUABIND_TYPE_INFO t) { m_type = t; }

		inline void add_function(const char* name, const overload_rep& o)
		{

#ifdef LUABIND_DONT_COPY_STRINGS
			detail::method_rep& method = m_methods[name];
			method.name = name;
#else
			m_strings.push_back(dup_string(name));
			detail::method_rep& method = m_methods[m_strings.back()];
			method.name = m_strings.back();
#endif

			method.add_overload(o);
			method.crep = this;
		}

		inline void add_constructor(const detail::construct_rep::overload_t& o)
		{
			m_constructor.overloads.push_back(o);
		}

		inline void add_wrapped_constructor(const detail::construct_rep::overload_t& o)
		{
			m_wrapped_constructor.overloads.push_back(o);
		}

		inline const char* name() const throw()
		{
#ifdef LUABIND_DONT_COPY_STRINGS
			return m_name;
#else
			return m_name.c_str();
#endif
		}

		inline void add_getter(const char* name, const boost::function2<int, lua_State*, int, luabind::memory_allocator<boost::function_base> >& g)
		{
			callback c;
			c.func = g;
			c.pointer_offset = 0;
#ifndef LUABIND_DONT_COPY_STRINGS
			m_strings.push_back(dup_string(name));
			m_getters[m_strings.back()] = c;
#else
			m_getters[name] = c;
#endif
		}

		inline void add_setter(const char* name, const boost::function2<int, lua_State*, int, luabind::memory_allocator<boost::function_base> >& s)
		{
			callback c;
			c.func = s;
			c.pointer_offset = 0;
#ifndef LUABIND_DONT_COPY_STRINGS
			m_strings.push_back(dup_string(name));
			m_setters[m_strings.back()] = c;
#else
			m_setters[name] = c;
#endif
		}

#ifndef LUABIND_NO_ERROR_CHECKING
		inline void add_operator(lua_State*, int op_id,  int(*func)(lua_State*), int(*matcher)(lua_State*), void(*sig)(lua_State*, string_class&), int arity)
#else
		inline void add_operator(lua_State*, int op_id,  int(*func)(lua_State*), int(*matcher)(lua_State*), int arity)
#endif
		{
			operator_callback o;
			o.set_fun(func);
			o.set_match_fun(matcher);
			o.set_arity(arity);

#ifndef LUABIND_NO_ERROR_CHECKING

			o.set_sig_fun(sig);

#endif
			m_operators[op_id].push_back(o);
		}
		
		// the lua reference to this class_rep
		inline int self_ref() const throw() { return m_self_ref; }

		// the lua reference to the metatable for this class' instances
		inline int metatable_ref() const throw() { return m_instance_metatable; }

		inline int table_ref() const { return m_table_ref; }

		inline void(*destructor() const)(void*) { return m_destructor; }

		inline class_type get_class_type() const { return m_class_type; }

		void add_static_constant(const char* name, int val)
		{
#ifndef LUABIND_DONT_COPY_STRINGS
			m_strings.push_back(dup_string(name));
			m_static_constants[m_strings.back()] = val;
#else
			m_static_constants[name] = val;
#endif
		}

	static inline int super_callback(lua_State* L)
	{
		int args = lua_gettop(L);
		
		object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, lua_upvalueindex(2)));
		class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
		class_rep* base = crep->bases()[0].base;

//		std::cout << "__super of " << base->name() << "\n";

		if (base->get_class_type() == class_rep::lua_class)
		{
			if (base->bases().empty())
			{
				obj->set_flags(obj->flags() & ~object_rep::call_super);

				lua_pushstring(L, "super");
				lua_pushnil(L);
				lua_settable(L, LUA_GLOBALSINDEX);
			}
			else
			{
				lua_pushstring(L, "super");
				lua_pushlightuserdata(L, base);
				lua_pushvalue(L, lua_upvalueindex(2));
				lua_pushcclosure(L, super_callback, 2);
				lua_settable(L, LUA_GLOBALSINDEX);
			}

//			std::cout << "  lua_class\n";
			detail::getref(L, base->table_ref());
			lua_pushstring(L, "__init");
			lua_gettable(L, -2);
			lua_insert(L, 1);
			lua_pop(L, 1);

			lua_pushvalue(L, lua_upvalueindex(2));
			lua_insert(L, 2);

			lua_call(L, args + 1, 0);

			// TODO: instead of clearing the global variable "super"
			// store it temporarily in the registry. maybe we should
			// have some kind of warning if the super global is used?
			lua_pushstring(L, "super");
			lua_pushnil(L);
			lua_settable(L, LUA_GLOBALSINDEX);
		}
		else
		{
			obj->set_flags(obj->flags() & ~object_rep::call_super);
//			std::cout << "  cpp_class\n";

			// we need to push some garbage at index 1 to make the construction work
			lua_pushboolean(L, 1);
			lua_insert(L, 1);

			construct_rep* rep = &base->m_constructor;

			bool ambiguous = false;
			int match_index = -1;
			int min_match = std::numeric_limits<int>::max();
			bool found;
			
#ifdef LUABIND_NO_ERROR_CHECKING

			if (rep->overloads.size() == 1)
			{
				match_index = 0;
			}
			else
			{

#endif

				int num_params = lua_gettop(L) - 1;
				found = find_best_match(L, rep->overloads.begin(), rep->overloads.end(), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING

			}

#else
				
			if (!found)
			{
				{
					string_class msg = "no constructor of '";
					msg += base->m_name;
					msg += "' matched the arguments (";
					msg += stack_content_by_name(L, 2);
					msg += ")";
					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}
			else if (ambiguous)
			{
				{
					string_class msg = "call of overloaded constructor '";
					msg += base->m_name;
					msg +=  "(";
					msg += stack_content_by_name(L, 2);
					msg += ")' is ambiguous";
					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}

			// TODO: should this be a warning or something?
/*
			// since the derived class is a lua class
			// it may have reimplemented virtual functions
			// therefore, we have to instantiate the Basewrapper
			// if there is no basewrapper, throw a run-time error
			if (!rep->overloads[match_index].has_wrapped_construct())
			{
				{
					string_class msg = "Cannot derive from C++ class '";
					msg += base->name();
					msg += "'. It does not have a wrapped type";
					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}
*/
#endif

#ifndef LUABIND_NO_EXCEPTIONS

			try
			{

#endif

				if (!rep->overloads[match_index].has_wrapped_construct())
				{
					// if the type doesn't have a wrapped type, use the ordinary constructor
					obj->set_object(rep->overloads[match_index].construct(L));
				}
				else
				{
					// get reference to lua object
					lua_pushvalue(L, lua_upvalueindex(2));
					int ref = detail::ref(L);
					obj->set_object(rep->overloads[match_index].construct_wrapped(L, ref));
				}
				// TODO: is the wrapped type destructed correctly?
				// it should, since the destructor is either the wrapped type's
				// destructor or the base type's destructor, depending on wether
				// the type has a wrapped type or not.
				obj->set_destructor(base->destructor());
				return 0;

#ifndef LUABIND_NO_EXCEPTIONS

			}
			catch(const std::exception& e)
			{
				lua_pushstring(L, e.what());
			}
			catch(const char* s)
			{
				lua_pushstring(L, s);
			}
			catch(...)
			{
				string_class msg = base->m_name;
				msg += "() threw an exception";
				lua_pushstring(L, msg.c_str());
			}
			// can only be reached if an exception was thrown
			lua_error(L);
#endif
		}

		return 0;

	}

	static int lua_settable_dispatcher(lua_State* L)
	{
		class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));
		detail::getref(L, crep->m_table_ref);
		lua_replace(L, 1);
		lua_rawset(L, -3);
		return 0;
	}

	static int construct_lua_class_callback(lua_State* L)
	{
		class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));

		int args = lua_gettop(L);

		// lua stack: crep <arguments>

		lua_newtable(L);
		int ref = detail::ref(L);

		bool has_bases = !crep->bases().empty();
		
		if (has_bases)
		{
			lua_pushstring(L, "super");
			lua_pushvalue(L, 1); // crep
		}

		// lua stack: crep <arguments> "super" crep
		// or
		// lua stack: crep <arguments>


		// if we have a baseclass we set the flag to say that the super has not yet been called
		// we will use this flag later to check that it actually was called from __init()
		int flags = object_rep::lua_class | object_rep::owner | (has_bases ? object_rep::call_super : 0);

		void* obj_ptr = lua_newuserdata(L, sizeof(object_rep));
		new(obj_ptr) object_rep(crep, flags, ref);

		detail::getref(L, crep->metatable_ref());
		lua_setmetatable(L, -2);

		// lua stack: crep <arguments> "super" crep obj_ptr
		// or
		// lua stack: crep <arguments> obj_ptr

		if (has_bases)	lua_pushvalue(L, -1); // obj_ptr
		lua_replace(L, 1); // obj_ptr

		// lua stack: obj_ptr <arguments> "super" crep obj_ptr
		// or
		// lua stack: obj_ptr <arguments>

		if (has_bases)
		{
			lua_pushcclosure(L, super_callback, 2);
			// lua stack: crep <arguments> "super" function
			lua_settable(L, LUA_GLOBALSINDEX);
		}

		// lua stack: crep <arguments>

		lua_pushvalue(L, 1);
		lua_insert(L, 1);

		detail::getref(L, crep->table_ref());
		lua_pushstring(L, "__init");
		lua_gettable(L, -2);

#ifndef LUABIND_NO_ERROR_CHECKING

		// TODO: should this be a run-time error?
		// maybe the default behavior should be to just call
		// the base calss' constructor. We should register
		// the super callback funktion as __init
		if (!lua_isfunction(L, -1))
		{
			{
				string_class msg = crep->name();
				msg += ":__init is not defined";
				lua_pushstring(L, msg.c_str());
			}
			lua_error(L);
		}

#endif

		lua_insert(L, 2); // function first on stack
		lua_pop(L, 1);
		// TODO: lua_call may invoke longjump! make sure we don't have any memory leaks!
		// we don't have any stack objects here
		lua_call(L, args, 0);

#ifndef LUABIND_NO_ERROR_CHECKING

		object_rep* obj = static_cast<object_rep*>(obj_ptr);
		if (obj->flags() & object_rep::call_super)
		{
			lua_pushstring(L, "derived class must call super on base");
			lua_error(L);
		}

#endif

		return 1;
	}

	// called from the metamethod for __index
	// obj is the object pointer
	static int lua_class_gettable(lua_State* L)
	{
		object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
		class_rep* crep = obj->crep();

#ifndef LUABIND_NO_ERROR_CHECKING

		if (obj->flags() & object_rep::call_super)
		{
			lua_pushstring(L, "derived class must call super on base");
			lua_error(L);
		}

#endif

		detail::getref(L, obj->lua_table_ref());
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);

		if (!lua_isnil(L, -1)) return 1;

		lua_pop(L, 2);

		detail::getref(L, crep->table_ref());
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);

		if (!lua_isnil(L, -1)) return 1;

		lua_pop(L, 2);


		if (lua_isnil(L, 2))
		{
			lua_pushnil(L);
			return 1;
		}

		// we have to ignore the first argument since this may point to
		// a method that is not present in this class (but in a subclass)
		const char* key = lua_tostring(L, 2);

		map_class<const char*, method_rep, ltstr>::iterator i = crep->m_methods.find(key);
		if (i != crep->m_methods.end())
		{
			// the name is a method, return it
			lua_pushlightuserdata(L, &i->second);
			lua_pushcclosure(L, class_rep::function_dispatcher, 1);
			return 1;
		}

		map_class<const char*, class_rep::callback, ltstr>::iterator j = crep->m_getters.find(key);
		if (j != crep->m_getters.end())
		{
			// the name is a data member
			return j->second.func(L, j->second.pointer_offset);
		}

		lua_pushnil(L);
		return 1;
	}

	// called from the metamethod for __newindex
	// obj is the object pointer
	static int lua_class_settable(lua_State* L)
	{
		object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
		class_rep* crep = obj->crep();

#ifndef LUABIND_NO_ERROR_CHECKING

		if (obj->flags() & object_rep::call_super)
		{
			// this block makes sure the string_class is destructed
			// before lua_error is called
			{
				string_class msg = "derived class '";
				msg += crep->name();
				msg += "'must call super on base";
				lua_pushstring(L, msg.c_str());
			}
			lua_error(L);
		}

#endif

		// we have to ignore the first argument since this may point to
		// a method that is not present in this class (but in a subclass)
		const char* key = lua_tostring(L, 2);
		map_class<const char*, class_rep::callback, ltstr>::iterator j = crep->m_setters.find(key);

		if (j == crep->m_setters.end())
		{
			map_class<const char*, class_rep::callback, ltstr>::iterator k = crep->m_getters.find(key);

#ifndef LUABIND_NO_ERROR_CHECKING

			if (k != crep->m_getters.end())
			{
				{
					string_class msg = "cannot set property '";
					msg += crep->name();
					msg += ".";
					msg += key;
					msg += "', because it's read only";
					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}

#endif

			detail::getref(L, obj->lua_table_ref());
			lua_replace(L, 1);
			lua_settable(L, 1);
		}
		else
		{
			// the name is a data member
			j->second.func(L, j->second.pointer_offset);
		}

		return 0;
	}

	static int static_class_gettable(lua_State* L)
	{
		class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));

		if (crep->get_class_type() == class_rep::lua_class)
		{
			detail::getref(L, crep->m_table_ref);
			lua_pushvalue(L, 2);
			lua_gettable(L, -2);
			if (!lua_isnil(L, -1)) return 1;
			else lua_pop(L, 2);
		}

		const char* key = lua_tostring(L, 2);

		map_class<const char*, method_rep, ltstr>::iterator i = crep->m_methods.find(key);
		if (i != crep->m_methods.end())
		{
			// the name is a method, return it
			lua_pushlightuserdata(L, &i->second);
			lua_pushcclosure(L, class_rep::function_dispatcher, 1);
			return 1;
		}

#ifndef LUABIND_NO_ERROR_CHECKING

		map_class<const char*, int, ltstr>::const_iterator j = crep->m_static_constants.find(key);

		if (j != crep->m_static_constants.end())
		{
			lua_pushnumber(L, j->second);
			return 1;
		}

		{
			string_class msg = "no static '";
			msg += key;
			msg += "' in class '";
			msg += crep->name();
			msg += "'";
			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);

#endif

		return 0;
	}

	private:

		// this is a pointer to the type_info structure for
		// this type
		// warning: this may be a problem when using dll:s, since
		// typeid() may actually return different pointers for the same
		// type.
		LUABIND_TYPE_INFO m_type;
		LUABIND_TYPE_INFO m_held_type;

		typedef void*(*extract_ptr_t)(void*);
		extract_ptr_t m_extract_underlying_fun;

		// a list of info for every class this class derives from
		// the information stored here is sufficient to do
		// type casts to the base classes
		vector_class<base_info> m_bases;

		// the class' name (as given when registered to lua with class_)
#ifdef LUABIND_DONT_COPY_STRINGS
		const char* m_name;
#else
		string_class m_name;
#endif

		// contains signatures for all constructors
		construct_rep m_constructor;
		construct_rep m_wrapped_constructor;

		// a reference to this structure itself. Since this struct
		// is kept inside lua (to let lua collect it when lua_close()
		// is called) we need to lock it to prevent collection.
		// the actual reference is not currently used.
		int m_self_ref;

		// a reference to the lua table that represents this class
		// (only used if it is a lua class)
		int m_table_ref;

		// the type of this class.. determines if it's written in c++ or lua
		class_type m_class_type;

		// this is a lua reference that points to the lua table
		// that is to be used as meta table for all instances
		// of this class.
		int m_instance_metatable;

		// ***** the maps below contains all members in this class *****

		// maps method names to a structure with more
		// information about that method.
		// that struct contains the function-signatures
		// for every overload
		map_class<const char*, method_rep, ltstr> m_methods;

#ifndef LUABIND_DONT_COPY_STRINGS
		// this is where the strings that the maps contains
		// pointer to are kept. To make sure they are destructed.
		vector_class<char*> m_strings;
#endif

		struct callback
		{
			boost::function2<int, lua_State*, int, luabind::memory_allocator<boost::function_base> > func;
			int pointer_offset;
		};

		// datamembers, some members may be readonly, and
		// only have a getter function
		map_class<const char*, callback, ltstr> m_getters;
		map_class<const char*, callback, ltstr> m_setters;

		struct operator_callback: public overload_rep_base
		{
			inline void set_fun(int (*f)(lua_State*)) { func = f; }
			inline int call(lua_State* L) { return func(L); }
			inline void set_arity(int arity) { m_arity = arity; }

		private:

			int(*func)(lua_State*);
		};

		vector_class<operator_callback> m_operators[number_of_operators]; // the operators in lua

		void(*m_destructor)(void*);

		map_class<const char*, int, ltstr> m_static_constants;
	};


	inline bool is_class_rep(lua_State* L, int index)
	{
		if (lua_getmetatable(L, index) == 0) return false;

		lua_pushstring(L, "__luabind_classrep");
		lua_gettable(L, -2);
		if (lua_toboolean(L, -1))
		{
			lua_pop(L, 2);
			return true;
		}

		lua_pop(L, 2);
		return false;

	}

}}

#endif // LUABIND_CLASS_REP_HPP_INCLUDED

