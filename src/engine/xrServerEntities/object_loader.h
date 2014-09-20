////////////////////////////////////////////////////////////////////////////
//	Module 		: object_loader.h
//	Created 	: 21.01.2003
//  Modified 	: 09.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Object loader
////////////////////////////////////////////////////////////////////////////

#pragma once

template <class M, typename P>
struct CLoader {
	
	template <typename T>
	struct CHelper1 {
		template <bool a>
		IC	static void load_data(T &data, M &stream, const P &p)
		{
			STATIC_CHECK				(!is_polymorphic<T>::result,Cannot_load_polymorphic_classes_as_binary_data);
			stream.r					(&data,sizeof(T));
		}

		template <>
		IC	static void load_data<true>(T &data, M &stream, const P &p)
		{
			T* data1 = const_cast<T*>(&data);
			data1->load	(stream);
		}
	};

	template <typename T>
	struct CHelper {

		template <bool pointer>
		IC	static void load_data(T &data, M &stream, const P &p)
		{
			CHelper1<T>::load_data<
				object_type_traits::is_base_and_derived_or_same_from_template<
					IPureLoadableObject,
					T
				>::value
			>(data,stream,p);
		}

		template <>
		IC	static void load_data<true>(T &data, M &stream, const P &p)
		{
			CLoader<M,P>::load_data	(*(data = xr_new<object_type_traits::remove_pointer<T>::type>()),stream,p);
		}
	};

	struct CHelper3 {
		template <typename T>
		struct has_value_compare {
		template <typename _P> static object_type_traits::detail::yes	select(object_type_traits::detail::other<typename _P::value_compare>*);
			template <typename _P> static object_type_traits::detail::no		select(...);
			enum { value = sizeof(object_type_traits::detail::yes) == sizeof(select<T>(0)) };
		};

		template <typename T>
		struct is_tree_structure {
			enum { 
				value = 
					has_value_compare<T>::value
			};
		};

		template <typename T1, typename T2>
		struct add_helper {
			template <bool>
			IC	static void add(T1 &data, T2 &value)
			{
				data.push_back	(value);
			}

			template <>
			IC	static void add<true>(T1 &data, T2 &value)
			{
				data.insert		(value);
			}
		};

		template <typename T1, typename T2>
		IC	static void add(T1 &data, T2 &value)
		{
			add_helper<T1,T2>::add<is_tree_structure<T1>::value>(data,value);
		}

		template <typename T>
		IC	static void load_data(T &data, M &stream, const P &p)
		{
			if (p.can_clear())
				data.clear();
			u32								count = stream.r_u32();
			for (u32 i=0; i<count; ++i) {
				T::value_type				temp;
				CLoader<M,P>::load_data		(temp,stream,p);
				if (p(data,temp))
					add						(data,temp);
			}
		}
	};

	template <typename T>
	struct CHelper4 {
		template <bool a>
		IC	static void load_data(T &data, M &stream, const P &p)
		{
			CHelper<T>::load_data<object_type_traits::is_pointer<T>::value>	(data,stream,p);
		}

		template <>
		IC	static void load_data<true>(T &data, M &stream, const P &p)
		{
			CHelper3::load_data			(data,stream,p);
		}
	};

	IC	static void load_data(LPCSTR &data, M &stream, const P &p)
	{
		NODEFAULT;
	}

	IC	static void load_data(LPSTR &data, M &stream, const P &p)
	{
		shared_str						S;
		stream.r_stringZ				(S);
		data							= xr_strdup(*S);
	}

	IC	static void load_data(shared_str &data, M &stream, const P &p)
	{
		stream.r_stringZ				(data);
	}

	IC	static void load_data(xr_string &data, M &stream, const P &p)
	{
		shared_str						S;
		stream.r_stringZ				(S);
		data							= *S;
	}

	template <typename T1, typename T2>
	IC	static void load_data(std::pair<T1,T2> &data, M &stream, const P &p)
	{
		if (p(data,const_cast<object_type_traits::remove_const<T1>::type&>(data.first),true)) {
			const bool					value = object_type_traits::is_same<T1,LPCSTR>::value;
			VERIFY						(!value);
			load_data					(const_cast<object_type_traits::remove_const<T1>::type&>(data.first),stream,p);
		}
		if (p(data,data.second,false))
			load_data					(data.second,stream,p);
		p.after_load					(data,stream);
	}

	IC	static void load_data(xr_vector<bool> &data, M &stream, const P &p)
	{
		if (p.can_clear())
			data.clear();
		u32								prev_count = data.size();
		data.resize						(prev_count + stream.r_u32());
		xr_vector<bool>::iterator		I = data.begin() + prev_count;
		xr_vector<bool>::iterator		E = data.end();
		u32								mask = 0;
		for (int j=32; I != E; ++I, ++j) {
			if (j >= 32) {
				mask					= stream.r_u32();
				j						= 0;
			}
			*I							= !!(mask & (u32(1) << j));
		}
	};

	template <typename T, int size>
	IC	static void load_data(svector<T,size> &data, M &stream, const P &p)
	{
		if (p.can_clear())
			data.clear();
		u32								count = stream.r_u32();
		for (u32 i=0; i<count; ++i) {
			svector<T,size>::value_type	temp;
			CLoader<M,P>::load_data		(temp,stream,p);
			if (p(data,temp))
				data.push_back			(temp);
		}
	}

	template <typename T1, typename T2>
	IC	static void load_data(std::queue<T1,T2> &data, M &stream, const P &p)
	{
		if (p.can_clear()) {
			while (!data.empty())
				data.pop();
		}
		std::queue<T1,T2>				temp;
		u32								count = stream.r_u32();
		for (u32 i=0; i<count; ++i) {
			std::queue<T1,T2>::value_type	t;
			CLoader<M,P>::load_data		(t,stream,p);
			if (p(temp,t))
				temp.push				(t);
		}
		for ( ; !temp.empty(); temp.pop())
			data.push					(temp.front());
	}

	template <template <typename _1, typename _2> class T1, typename T2, typename T3>
	IC	static void load_data(T1<T2,T3> &data, M &stream, const P &p, bool)
	{
		if (p.can_clear()) {
			while (!data.empty())
				data.pop();
		}
		T1<T2,T3>						temp;
		u32								count = stream.r_u32();
		for (u32 i=0; i<count; ++i) {
			T1<T2,T3>::value_type		t;
			CLoader<M,P>::load_data		(t,stream,p);
			if (p(temp,t))
				temp.push				(t);
		}
		for ( ; !temp.empty(); temp.pop())
			data.push					(temp.top());
	}

	template <template <typename _1, typename _2, typename _3> class T1, typename T2, typename T3, typename T4>
	IC	static void load_data(T1<T2,T3,T4> &data, M &stream, const P &p, bool)
	{
		if (p.can_clear()) {
			while (!data.empty())
				data.pop();
		}
		T1<T2,T3,T4>					temp;
		u32								count = stream.r_u32();
		for (u32 i=0; i<count; ++i) {
			T1<T2,T3,T4>::value_type	t;
			CLoader<M,P>::load_data		(t,stream,p);
			if (p(temp,t))
				temp.push				(t);
		}
		for ( ; !temp.empty(); temp.pop())
			data.push					(temp.top());
	}

	template <typename T1, typename T2>
	IC	static void load_data(xr_stack<T1,T2> &data, M &stream, const P &p)
	{
		load_data						(data,stream,p,true);
	}

	template <typename T1, typename T2, typename T3>
	IC	static void load_data(std::priority_queue<T1,T2,T3> &data, M &stream, const P &p)
	{
		load_data						(data,stream,p,true);
	}

	template <typename T>
	IC	static void load_data(T &data, M &stream, const P &p)
	{
		CHelper4<T>::load_data<object_type_traits::is_stl_container<T>::value>	(data,stream,p);
	}
};

namespace object_loader {
	namespace detail {
		struct CEmptyPredicate {
			template <typename T1, typename T2>
			IC	void after_load	(T1 &data, T2 &stream) const {}
			template <typename T1, typename T2>
			IC	bool operator()	(T1 &data, const T2 &value) const {return(true);}
			template <typename T1, typename T2>
			IC	bool operator()	(T1 &data, const T2 &value, bool) const {return(true);}
			IC	bool can_clear() const {return(true);}
			IC	bool can_add() const {return(true);}
		};
	};
};

template <typename T, typename M, typename P>
IC	void load_data(const T &data, M &stream, const P &p)
{
	T						*temp = const_cast<T*>(&data);
	CLoader<M,P>::load_data	(*temp,stream,p);
}

template <typename T, typename M>
IC	void load_data(const T &data, M &stream)
{
	load_data				(data,stream,object_loader::detail::CEmptyPredicate());
}
