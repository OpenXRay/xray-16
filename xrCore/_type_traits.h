#ifndef _STL_EXT_type_traits
#define _STL_EXT_type_traits
#pragma once

// 1. class or not class
template<typename T>
struct	is_class				{
	struct _yes	{	char _a[1];	};
	struct _no	{	char _a[2];	};

	template <class U> static _yes	is_class_tester(void(U::*)(void));
	template <class U> static _no	is_class_tester(...);

	enum						{	result = (sizeof(_yes)==sizeof(is_class_tester<T>(0)))	};
};

// 2. is polymorphic (class)
template<typename T>
struct	is_pm_class				{
	struct c1 : public	T		{
		char	padding	[16];
		c1();
		c1(const c1&);
		c1&operator=(const c1&);
	};
	struct c2 : public	T		{
		char	padding	[16];
		c2();
		c2(const c2&);
		c2&operator=(const c2&);
		virtual	~c2		();
	};
	enum						{	result = (sizeof(c1)==sizeof(c2))						};
};

// 3. select result based on class/not class
template<bool _is_class>
struct is_pm_classify			{
	template<typename _T>	
	struct _detail				{	enum { result = is_pm_class<_T>::result };				};
};
template<>
struct is_pm_classify<false>	{
	template<typename _T>
	struct _detail				{	enum { result = false };								};
};
template<typename T>	
struct	is_polymorphic			{
	enum						{
		result	= is_pm_classify<is_class<T>::result> :: _detail<T> :: result
	};
};

#endif
