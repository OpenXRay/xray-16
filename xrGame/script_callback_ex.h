////////////////////////////////////////////////////////////////////////////
//	Module 		: script_callback_ex.h
//	Created 	: 06.02.2004
//  Modified 	: 11.01.2005
//	Author		: Sergey Zhemeitsev and Dmitriy Iassenev
//	Description : Script callbacks with return value
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_space.h"
#include "script_engine.h"

IC	bool compare_safe(const luabind::object &o1 , const luabind::object &o2)
{
	if ((o1.type() == LUA_TNIL) && (o2.type() == LUA_TNIL))
		return						(true);

	return							(o1 == o2);
}

#define comma						,
#define concatenizer(a,b)			a##b
#define left_comment				concatenizer(/,*)
#define right_comment				concatenizer(*,/)
#define param_generator(a,b,c,d)	a##b##c d##b

#if XRAY_EXCEPTIONS
#	define process_error \
		catch(luabind::error &e) {\
			if (e.state())\
				ai().script_engine().print_output(e.state(),"",LUA_ERRRUN);\
			else\
				ai().script_engine().print_output(ai().script_engine().lua(),"",LUA_ERRRUN);\
		}
#else
#	define process_error
#endif

#define function_body_ex(_1,_2,_3,_4,_c,_5,_6) \
	_1 _3 _2\
	IC return_type operator() (_4) _c\
	{\
		try {					\
			try {				\
				if (m_functor) {\
					VERIFY		(m_functor.is_valid());\
					if (m_object.is_valid()) {\
						VERIFY	(m_object.is_valid());\
						macros_return_operator (m_functor(m_object _5 _6));\
					}\
					else\
						macros_return_operator (m_functor(_6));\
				}\
			}\
			process_error\
			catch(std::exception &) {\
				ai().script_engine().print_output(ai().script_engine().lua(),"",LUA_ERRRUN);\
			}\
		}\
		catch (...) {\
			const_cast<CScriptCallbackEx<return_type>*>(this)->clear();\
		}			\
		macros_return_operator (0);\
	}

#define function_body(_1,_2,_3,_4,_5,_6) \
	function_body_ex(_1,_2,_3,_4,const,_5,_6)\
	function_body_ex(_1,_2,_3,_4,,_5,_6)

template <typename _return_type>
class CScriptCallbackEx_ {
public:
	typedef _return_type							return_type;

private:
	typedef luabind::functor<_return_type>			functor_type;
	typedef luabind::object							object_type;
	typedef bool (CScriptCallbackEx_::*unspecified_bool_type) () const;

protected:
	functor_type					m_functor;
	object_type						m_object;

private:
	IC			bool				empty					() const;

public:
	IC								CScriptCallbackEx_		();
	IC								CScriptCallbackEx_		(const CScriptCallbackEx_ &callback);
	IC	virtual						~CScriptCallbackEx_		();
	IC			CScriptCallbackEx_	&operator=				(const CScriptCallbackEx_ &callback);
	IC			bool				operator==				(const CScriptCallbackEx_ &callback)const{return compare_safe(m_object,(callback.m_object))&&m_functor==(callback.m_functor);}
	IC			bool				operator==				(const object_type	&object)		const{return compare_safe(m_object,object);}
	IC			void				set						(const functor_type &functor);
	IC			void				set						(const functor_type &functor, const object_type &object);
	IC			void				clear					();
	IC			operator			unspecified_bool_type	() const {return (!m_functor.is_valid() ? 0 : &CScriptCallbackEx_::empty);}
};

#include "script_callback_ex_inline.h"
#include "script_callback_ex_nonvoid.h"
#include "script_callback_ex_void.h"

#undef comma
#undef concatenizer
#undef left_comment
#undef right_comment
#undef param_generator
#undef function_body_ex
#undef function_body
#undef process_error