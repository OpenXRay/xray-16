#include "stdafx.h"
#include "string_concatenations.h"

namespace xray {
	
namespace core {
	
namespace detail {

namespace strconcat_error {
	
void process	(u32 const index, u32 const count, LPCSTR *strings)
{
	u32 const	max_string_size = 1024;
	LPSTR temp	= (LPSTR)_alloca((count*(max_string_size + 4) + 1)*sizeof(**strings));
	LPSTR k		= temp;
	*k++		= '[';
	for (u32 i = 0; i<count; ++i) {
		for (LPCSTR j = strings[i], e = j + max_string_size; *j && j < e; ++k, ++j)
			*k	= *j;

		*k++	= ']';
		
		if (i + 1 >= count)
			continue;

		*k++	= '[';
		*k++	= '\r';
		*k++	= '\n';
	}
	*k			= 0;

	Debug.fatal	(
		DEBUG_INFO,
		make_string(
			"buffer overflow: cannot concatenate strings(%d):\r\n%s",
			index,
			temp
		).c_str()
	);
}

template <u32 count>
static inline void process	(LPSTR& i, LPCSTR e, u32 const index, LPCSTR (&strings)[count])
{
	VERIFY		(i <= e);
	VERIFY		(index < count);

	if (i != e)
		return;

#ifndef MASTER_GOLD
	process		(index, count, strings);
#else // #ifndef MASTER_GOLD
	--i;
#endif // #ifndef MASTER_GOLD
}

} // namespace strconcat_error 

int stack_overflow_exception_filter	(int exception_code)
{
	if (exception_code == EXCEPTION_STACK_OVERFLOW)
	{
		// Do not call _resetstkoflw here, because
		// at this point, the stack is not yet unwound.
		// Instead, signal that the handler (the __except block)
		// is to be executed.
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

void   check_stack_overflow (u32 stack_increment)
{
	__try 
	{
		void* p = _alloca(stack_increment);
		p;
	} 
	__except ( xray::core::detail::stack_overflow_exception_filter(GetExceptionCode()) ) 
	{
		_resetstkoflw();
	}
}

void	string_tupples::error_process () const
{
	LPCSTR strings[6];

	u32 part_size = 0;
	u32 overrun_string_index = (u32)-1;
	for ( u32 i=0; i<m_count; ++i )
	{
		strings[i] = m_strings[i].first;

		if ( overrun_string_index == (u32)-1 )
		{
			part_size += m_strings[i].second;
			if ( part_size > max_concat_result_size )
			{
				overrun_string_index = i;
			}
		}
	}
	VERIFY(overrun_string_index != -1);

	strconcat_error::process(overrun_string_index, m_count, strings);
}

} // namespace detail

} // namespace core
 
} // namespace xray

using namespace xray::core::detail;

// dest = S1+S2
LPSTR						strconcat				( int dest_sz, char* dest, const char* S1, const char* S2)
{
	VERIFY			(dest);
	VERIFY			(S1);
	VERIFY			(S2);

	LPCSTR strings[]= { S1, S2 };

	LPSTR			i = dest;
	LPCSTR			e = dest + dest_sz;
	LPCSTR			j;
	for (j = S1; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 0, strings );

	for (j = S2; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 1, strings );

	*i				= 0;

	return			(dest);
}

// dest = S1+S2+S3
LPSTR						strconcat				( int dest_sz, char* dest, const char* S1, const char* S2, const char* S3)
{
	VERIFY			(dest);
	VERIFY			(S1);
	VERIFY			(S2);
	VERIFY			(S3);

	LPCSTR strings[]= { S1, S2, S3 };

	LPSTR			i = dest;
	LPCSTR			e = dest + dest_sz;
	LPCSTR			j;
	for (j = S1; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 0, strings );

	for (j = S2; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 1, strings );

	for (j = S3; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 2, strings );

	*i				= 0;

	return			(dest);
}

// dest = S1+S2+S3+S4
LPSTR						strconcat				( int dest_sz, char* dest, const char* S1, const char* S2, const char* S3, const char* S4)
{
	VERIFY			(dest);
	VERIFY			(S1);
	VERIFY			(S2);
	VERIFY			(S3);
	VERIFY			(S4);

	LPCSTR strings[]= { S1, S2, S3, S4 };

	LPSTR			i = dest;
	LPCSTR			e = dest + dest_sz;
	LPCSTR			j;
	for (j = S1; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 0, strings );

	for (j = S2; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 1, strings );

	for (j = S3; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 2, strings );

	for (j = S4; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 3, strings );

	*i				= 0;

	return			(dest);
}

// dest = S1+S2+S3+S4+S5
LPSTR						strconcat				( int dest_sz, char* dest, const char* S1, const char* S2, const char* S3, const char* S4, const char* S5)
{
	VERIFY			(dest);
	VERIFY			(S1);
	VERIFY			(S2);
	VERIFY			(S3);
	VERIFY			(S4);
	VERIFY			(S5);

	LPCSTR strings[]= { S1, S2, S3, S4, S5 };

	LPSTR			i = dest;
	LPCSTR			e = dest + dest_sz;
	LPCSTR			j;
	for (j = S1; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 0, strings );

	for (j = S2; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 1, strings );

	for (j = S3; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 2, strings );

	for (j = S4; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 3, strings );

	for (j = S5; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 4, strings );

	*i				= 0;

	return			(dest);
}

// dest = S1+S2+S3+S4+S5+S6
LPSTR						strconcat				( int dest_sz, char* dest, const char* S1, const char* S2, const char* S3, const char* S4, const char* S5, const char* S6)
{
	VERIFY			(dest);
	VERIFY			(S1);
	VERIFY			(S2);
	VERIFY			(S3);
	VERIFY			(S4);
	VERIFY			(S5);
	VERIFY			(S6);

	LPCSTR strings[]= { S1, S2, S3, S4, S5, S6 };

	LPSTR			i = dest;
	LPCSTR			e = dest + dest_sz;
	LPCSTR			j;
	for (j = S1; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 0, strings );

	for (j = S2; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 1, strings );

	for (j = S3; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 2, strings );

	for (j = S4; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 3, strings );

	for (j = S5; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 4, strings );

	for (j = S6; *j && i < e; ++i, ++j)
		*i			= *j;

	strconcat_error::process	( i, e, 5, strings );

	*i				= 0;

	return			(dest);
}

