////////////////////////////////////////////////////////////////////////////
//	Module 		: object_comparer.h
//	Created 	: 13.07.2004
//  Modified 	: 13.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Object equality checker
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename P>
struct CComparer {
	template <typename T>
	struct CHelper {
		template <bool a>
		IC	static bool compare(const T &_1, const T &_2, const P &p)
		{
			return			(p(_1,_2));
		}

		template <>
		IC	static bool compare<true>(const T &_1, const T &_2, const P &p)
		{
			return			(CComparer::compare(*_1,*_2,p));
		}
	};

	IC	static bool compare(LPCSTR _1, LPCSTR _2, const P &p)
	{
		return						(p(_1,_2));
	}

	IC	static bool compare(LPSTR _1, LPSTR _2, const P &p)
	{
		return						(p(_1,_2));
	}

	IC	static bool compare(const shared_str &_1, const shared_str &_2, const P &p)
	{
		return						(p(_1,_2));
	}

	template <typename T1, typename T2>
	IC	static bool compare(const std::pair<T1,T2> &_1, const std::pair<T1,T2> &_2, const P &p)
	{
		return						(
			compare(_1.first,_2.first,p)
			&&
			compare(_1.second,_2.second,p)
		);
	}

	template <typename T, int size>
	IC	static bool compare(const svector<T,size> &_1, const svector<T,size> &_2, const P &p)
	{
		if (_1.size() != _2.size())
			return					(p());
		
		svector<T,size>::const_iterator	I = _1.begin(), J = _2.begin();
		svector<T,size>::const_iterator	E = _1.end();
		for ( ; I != E; ++I, ++J)
			if (!compare(*I,*J,p))
				return				(false);
		return						(true);
	}

	template <typename T1, typename T2>
	IC	static bool compare(const std::queue<T1,T2> &__1, const std::queue<T1,T2> &__2, const P &p)
	{
		std::queue<T1,T2>			_1 = __1;
		std::queue<T1,T2>			_2 = __2;
		
		if (_1.size() != _2.size())
			return					(p());

		for ( ; !_1.empty(); _1.pop(), _2.pop())
			if (!compare(_1.front(),_2.front(),p))
				return				(false);
		return						(true);
	}

	template <template <typename _1, typename _2> class T1, typename T2, typename T3>
	IC	static bool compare(const T1<T2,T3> &__1, const T1<T2,T3> &__2, const P &p, bool)
	{
		T1<T2,T3>					_1 = __1;
		T1<T2,T3>					_2 = __2;

		if (_1.size() != _2.size())
			return					(p());

		for ( ; !_1.empty(); _1.pop(), _2.pop())
			if (!compare(_1.top(),_2.top(),p))
				return				(false);
		return						(true);
	}

	template <template <typename _1, typename _2, typename _3> class T1, typename T2, typename T3, typename T4>
	IC	static bool compare(const T1<T2,T3,T4> &__1, const T1<T2,T3,T4> &__2, const P &p, bool)
	{
		T1<T2,T3,T4>				_1 = __1;
		T1<T2,T3,T4>				_2 = __2;

		if (_1.size() != _2.size())
			return					(p());

		for ( ; !_1.empty(); _1.pop(), _2.pop())
			if (!compare(_1.top(),_2.top(),p))
				return				(false);
		return						(true);
	}

	template <typename T1, typename T2>
	IC	static bool compare(const xr_stack<T1,T2> &_1, const xr_stack<T1,T2> &_2, const P &p)
	{
		return					(compare(_1,_2,p,true));
	}

	template <typename T1, typename T2, typename T3>
	IC	static bool compare(const std::priority_queue<T1,T2,T3> &_1, const std::priority_queue<T1,T2,T3> &_2, const P &p)
	{
		return					(compare(_1,_2,p,true));
	}

	struct CHelper3 {
		template <typename T>
		IC	static bool compare(const T &_1, const T &_2, const P &p)
		{
			if (_1.size() != _2.size())
				return					(p());

			T::const_iterator			I = _1.begin(), J = _2.begin();
			T::const_iterator			E = _1.end();
			for ( ; I != E; ++I, ++J)
				if (!CComparer::compare(*I,*J,p))
					return				(false);
			return						(true);
		}
	};

	template <typename T>
	struct CHelper4 {
		template <bool a>
		IC	static bool compare(const T &_1, const T &_2, const P &p)
		{
			return(CHelper<T>::compare<object_type_traits::is_pointer<T>::value>(_1,_2,p));
		}

		template <>
		IC	static bool compare<true>(const T &_1, const T &_2, const P &p)
		{
			return(CHelper3::compare(_1,_2,p));
		}
	};

	template <typename T>
	IC	static bool compare(const T &_1, const T &_2, const P &p)
	{
		return						(CHelper4<T>::compare<object_type_traits::is_stl_container<T>::value>(_1,_2,p));
	}
};

template <typename P>
IC	bool compare(LPCSTR p0, LPSTR p1, const P &p)
{
	return			(p(p0,p1));
}

template <typename P>
IC	bool compare(LPSTR p0, LPCSTR p1, const P &p)
{
	return			(p(p0,p1));
}

template <typename T, typename P>
IC	bool compare(const T &p0, const T &p1, const P &p)
{
	return			(CComparer<P>::compare(p0,p1,p));
}

namespace object_comparer {
	namespace detail {
		template <template <typename _1> class P>
		struct comparer {
			template <typename T>
			IC	bool operator() (const T &_1, const T &_2)	const	{return(P<T>()		(_1,_2));}
			IC	bool operator() ()							const	{return(P<bool>()	(false,true));}
			IC	bool operator() (LPCSTR _1, LPCSTR _2)		const	{return(P<int>()	(xr_strcmp(_1,_2),0));}
			IC	bool operator() (LPSTR _1, LPSTR _2)		const	{return(P<int>()	(xr_strcmp(_1,_2),0));}
			IC	bool operator() (LPCSTR _1, LPSTR _2)		const	{return(P<int>()	(xr_strcmp(_1,_2),0));}
			IC	bool operator() (LPSTR _1, LPCSTR _2)		const	{return(P<int>()	(xr_strcmp(_1,_2),0));}
		};
	};
};

#define declare_comparer(a,b) \
	template <typename T1, typename T2>\
	IC	bool a(const T1 &p0, const T2 &p1)\
	{\
		return			(compare(p0,p1,object_comparer::detail::comparer<b>()));\
	}

declare_comparer(equal,			std::equal_to);
declare_comparer(greater_equal,	std::greater_equal);
declare_comparer(greater,		std::greater);
declare_comparer(less_equal,	std::less_equal);
declare_comparer(less,			std::less);
declare_comparer(not_equal,		std::not_equal_to);
declare_comparer(logical_and,	std::logical_and);
declare_comparer(logical_or,	std::logical_or);
