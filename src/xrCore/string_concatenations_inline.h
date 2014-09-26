
namespace xray {

namespace core {

namespace detail {

class XRCORE_API string_tupples {
public:

	template < typename T0 >
	inline						string_tupples	( T0 p0 ) :
			m_count				(1)
	{
		helper<0>::add_string	(*this, p0);
	}

	template < typename T0, typename T1 >
	inline		string_tupples	( T0 p0, T1 p1 ) :
		m_count					(2)
	{
		helper<0>::add_string	(*this, p0);
		helper<1>::add_string	(*this, p1);
	}

	template < typename T0, typename T1, typename T2 >
	inline		string_tupples	( T0 p0, T1 p1, T2 p2 ) :
		m_count					(3)
	{
		helper<0>::add_string	(*this, p0);
		helper<1>::add_string	(*this, p1);
		helper<2>::add_string	(*this, p2);
	}

	template < typename T0, typename T1, typename T2, typename T3 >
	inline		string_tupples	( T0 p0, T1 p1, T2 p2, T3 p3 ) :
		m_count					(4)
	{
		helper<0>::add_string	(*this, p0);
		helper<1>::add_string	(*this, p1);
		helper<2>::add_string	(*this, p2);
		helper<3>::add_string	(*this, p3);
	}

	template < typename T0, typename T1, typename T2, typename T3, typename T4 >
	inline		string_tupples	( T0 p0, T1 p1, T2 p2, T3 p3, T4 p4 ) :
		m_count					(5)
	{
		helper<0>::add_string	(*this, p0);
		helper<1>::add_string	(*this, p1);
		helper<2>::add_string	(*this, p2);
		helper<3>::add_string	(*this, p3);
		helper<4>::add_string	(*this, p4);
	}

	template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5 >
	inline		string_tupples	( T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5 ) :
		m_count					(6)
	{
		helper<0>::add_string	(*this, p0);
		helper<1>::add_string	(*this, p1);
		helper<2>::add_string	(*this, p2);
		helper<3>::add_string	(*this, p3);
		helper<4>::add_string	(*this, p4);
		helper<5>::add_string	(*this, p5);
	}

			void	error_process	() const;

	inline	u32		size			() const
	{
		VERIFY			(m_count > 0);

		u32				result = m_strings[0].second;
		
		for (u32 j = 1; j < m_count; ++j)
			result		+= m_strings[j].second;

		if ( result > max_concat_result_size )
		{
			error_process();
		}
		
		return			((result + 1)*sizeof(*m_strings[0].first));
	}

 	inline	void	concat			(LPCSTR const result) const
 	{
 		VERIFY			(m_count > 0);
 
 		LPSTR			i = const_cast<LPSTR>(result);
		Memory.mem_copy	(i, m_strings[0].first, m_strings[0].second*sizeof(*m_strings[0].first));
		i				+= m_strings[0].second;

		for (u32 j = 1; j < m_count; ++j) {
			Memory.mem_copy	(i, m_strings[j].first, m_strings[j].second*sizeof(*m_strings[j].first));
			i			+= m_strings[j].second;
		}
 
 		*i				= 0;
 	}

private:
	enum {
		max_concat_result_size	= u32(512*1024),
		max_item_count			= 6,
	};

private:
	template <u32 index>
	struct helper {

		static inline	u32		length  (LPCSTR string)
		{
			return		(string ? (unsigned int)xr_strlen(string) : 0);
		}

		static inline	LPCSTR	string  (LPCSTR string)
		{
			return		(string);
		}

		static inline	u32		length	(shared_str const& string)
		{
			return		(string.size());
		}

		static inline	LPCSTR	string	(shared_str const& string)
		{
			return		(string.c_str());
		}

		static inline	u32		length	(xr_string const& string)
		{
			return		(string.size());
		}

		static inline	LPCSTR	string	(xr_string const& string)
		{
			return		(string.c_str());
		}

		template <typename T>
		static inline	void	add_string		(string_tupples& self, T p)
		{
			STATIC_CHECK			(index < max_item_count, Error_invalid_string_index_specified);

			LPCSTR cstr				= string(p);
			VERIFY					(cstr);
			self.m_strings[index]	= std::make_pair(cstr, length(p));
		}
	}; // struct helper

private:
	typedef std::pair<LPCSTR, u32>	StringPair;

private:
	StringPair			m_strings[max_item_count];
	u32 				m_count;
};

void XRCORE_API check_stack_overflow (u32 stack_increment);

} // namespace detail

} // namespace core

} // namespace xray
