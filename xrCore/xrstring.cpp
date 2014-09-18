#include "stdafx.h"
#pragma hdrstop

#include "xrstring.h"

#include "FS_impl.h"

XRCORE_API	extern		str_container*	g_pStringContainer	= NULL;
#define		HEADER		16			// ref + len + crc + next

#if 1

struct str_container_impl
{
	static const u32 buffer_size = 1024*256;
	str_value*		 buffer[buffer_size];
	int              num_docs;

	str_container_impl ()
	{
		num_docs = 0;
		ZeroMemory(buffer, sizeof(buffer));
	}

	str_value*       find   (str_value* value, const char* str)
	{
		str_value* candidate = buffer[ value->dwCRC % buffer_size ];
		while ( candidate )
		{
			if ( candidate->dwCRC == value->dwCRC &&
				 candidate->dwLength == value->dwLength &&
				 !memcmp(candidate->value, str, value->dwLength) )
			{
				return candidate;				
			}

			candidate = candidate->next;
		}

		return NULL;
	}

	void			 insert (str_value* value)
	{
		str_value** element = &buffer[ value->dwCRC % buffer_size ];
		value->next = *element;
		*element = value;
	}

	void			 clean ()
	{
		for ( u32 i=0; i<buffer_size; ++i )
		{
			str_value** current = &buffer[i];

			while ( *current != NULL )
			{
				str_value* value = *current;
				if ( !value->dwReference )
				{
					*current = value->next;
					xr_free(value);
				}
				else
				{
					current = &value->next;
				}
			}
		}
	}

	void			 verify ()
	{
		Msg			("strings verify started");
		for ( u32 i=0; i<buffer_size; ++i )
		{
			str_value* value = buffer[i];
			while ( value )
			{
				u32			crc		= crc32	(value->value, value->dwLength);
				string32	crc_str;
				R_ASSERT3	(crc==value->dwCRC, "CorePanic: read-only memory corruption (shared_strings)", itoa(value->dwCRC,crc_str,16));
				R_ASSERT3	(value->dwLength == xr_strlen(value->value), "CorePanic: read-only memory corruption (shared_strings, internal structures)", value->value);
				value = value->next;
			}
		}
		Msg			("strings verify completed");
	}

	void			dump (FILE* f) const
	{
		for ( u32 i=0; i<buffer_size; ++i )
		{
			str_value* value = buffer[i];
			while ( value )
			{
				fprintf	(f,"ref[%4d]-len[%3d]-crc[%8X] : %s\n",value->dwReference,value->dwLength,value->dwCRC,value->value);
				value = value->next;
			}
		}
	}

	void			dump (IWriter* f) const
	{
		for ( u32 i=0; i<buffer_size; ++i )
		{
			str_value* value = buffer[i];
			string4096		temp;
			while ( value )
			{
				xr_sprintf	(temp, sizeof(temp), "ref[%4d]-len[%3d]-crc[%8X] : %s\n", value->dwReference, value->dwLength, value->dwCRC, value->value);
				f->w_string	(temp);
				value		= value->next;
			}
		}
	}

	int				stat_economy ()
	{
		int				counter	  = 0;
		for ( u32 i=0; i<buffer_size; ++i )
		{
			str_value* value = buffer[i];
			while ( value )
			{
				counter -= sizeof(str_value);
				counter += (value->dwReference-1)*(value->dwLength+1);
				value = value->next;
			}
		}

		return counter;
	}
};

str_container::str_container ()
{
	impl = xr_new<str_container_impl>();
}

str_value*	str_container::dock		(str_c value)
{
	if (0==value)				return 0;

	cs.Enter					();

#ifdef DEBUG_MEMORY_MANAGER
	Memory.stat_strdock			++	;
#endif // DEBUG_MEMORY_MANAGER

	str_value*	result			= 0	;

	// calc len
	u32		s_len				= xr_strlen(value);
	u32		s_len_with_zero		= (u32)s_len+1;
	VERIFY	(HEADER+s_len_with_zero < 4096);

	// setup find structure
	char	header[sizeof(str_value)];
	str_value*	sv				= (str_value*)header;
	sv->dwReference				= 0;
	sv->dwLength				= s_len;
	sv->dwCRC					= crc32	(value,s_len);

	// search
	result						= impl->find	(sv, value);
	
#ifdef DEBUG
	bool is_leaked_string = !xr_strcmp(value, "enter leaked string here");
#endif //DEBUG

	// it may be the case, string is not found or has "non-exact" match
	if (0==result 
#ifdef DEBUG
		|| is_leaked_string
#endif //DEBUG
		) {

		result					= (str_value*)Memory.mem_alloc(HEADER+s_len_with_zero
#ifdef DEBUG_MEMORY_NAME
			, "storage: sstring"
#endif // DEBUG_MEMORY_NAME
			);

#ifdef DEBUG
		static int num_leaked_string = 0;
		if ( is_leaked_string )
		{
			++num_leaked_string;
			Msg("leaked_string: %d 0x%08x", num_leaked_string, result);
		}
#endif // DEBUG

		result->dwReference		= 0;
		result->dwLength		= sv->dwLength;
		result->dwCRC			= sv->dwCRC;
		CopyMemory				(result->value,value,s_len_with_zero);

		impl->insert (result);
	}
	cs.Leave					();

	return	result;
}

void		str_container::clean	()
{
	cs.Enter	();
	impl->clean ();
	cs.Leave	();
}

void		str_container::verify	()
{
	cs.Enter	();
	impl->verify();
	cs.Leave	();
}

void		str_container::dump	()
{
 	cs.Enter	();
 	FILE* F		= fopen("d:\\$str_dump$.txt","w");
 	impl->dump  (F);
 	fclose		(F);
 	cs.Leave	();
}

void		str_container::dump	(IWriter* W)
{
 	cs.Enter	();
 	impl->dump  (W);
 	cs.Leave	();
}

u32			str_container::stat_economy		()
{
 	cs.Enter	();
 	int				counter	= 0;
 	counter			-= sizeof(*this);
	counter			+= impl->stat_economy();
 	cs.Leave		(); 
 	return			u32(counter);
}

str_container::~str_container		()
{
	clean ();
	//dump ();
	xr_delete(impl);
}

#else

struct str_container_impl
{
	typedef xr_multiset<str_value*, str_value_cmp>	cdb;
	int num_docs;
	str_container_impl () {num_docs=0;}


	cdb	container;
};

str_container::str_container ()
{
	impl = xr_new<str_container_impl>();
}

str_value*	str_container::dock		(str_c value)
{
	if (0==value)				return 0;

	cs.Enter					();

// 	++impl->num_docs;
// 	if ( impl->num_docs == 10000000 )
// 	{
// 		Msg("shared_strings");
// 		g_find_chunk_counter.flush();
// 	}
// 
// 	//#ifdef FIND_CHUNK_BENCHMARK_ENABLE
// 	find_chunk_auto_timer timer;
// 	//#endif // FIND_CHUNK_BENCHMARK_ENABLE

#ifdef DEBUG_MEMORY_MANAGER
	Memory.stat_strdock			++	;
#endif // DEBUG_MEMORY_MANAGER

	str_value*	result			= 0	;

	// calc len
	u32		s_len				= xr_strlen(value);
	u32		s_len_with_zero		= (u32)s_len+1;
	VERIFY	(HEADER+s_len_with_zero < 4096);

	// setup find structure
	char	header[sizeof(str_value)];
	str_value*	sv				= (str_value*)header;
	sv->dwReference				= 0;
	sv->dwLength				= s_len;
	sv->dwCRC					= crc32	(value,s_len);
	sv->next					= NULL;
	
	// search
	str_container_impl::cdb::iterator	I			= impl->container.find	(sv);	// only integer compares :)
	if (I!=impl->container.end())		{
		// something found - verify, it is exactly our string
		str_container_impl::cdb::iterator	save	= I;
		for (; I!=impl->container.end() && (*I)->dwCRC == sv->dwCRC; ++I)	{
			str_value*	V		= (*I);
			if	(V->dwLength!=sv->dwLength)			continue;
			if	(0!=memcmp(V->value,value,s_len))	continue;
			result				= V;				// found
			break;
		}
	}

	bool is_leaked_string = !xr_strcmp(value, "enter leaked string here");

	// it may be the case, string is not found or has "non-exact" match
	if (0==result || is_leaked_string)				{
		// Insert string
//		DUMP_PHASE;

		result					= (str_value*)Memory.mem_alloc(HEADER+s_len_with_zero
#ifdef DEBUG_MEMORY_NAME
			, "storage: sstring"
#endif // DEBUG_MEMORY_NAME
			);

		static int num11 = 0;
		
		if ( is_leaked_string )
		{
			++num11;
			Msg("leaked_string: %d 0x%08x", num11, result);
		}

//		DUMP_PHASE;

		result->dwReference		= 0;
		result->dwLength		= sv->dwLength;
		result->dwCRC			= sv->dwCRC;
		result->next			= NULL;

		CopyMemory				(result->value,value,s_len_with_zero);

		impl->container.insert (result);
	}

	cs.Leave					();

	return	result;
}

void		str_container::clean	()
{
	cs.Enter	();
	str_container_impl::cdb::iterator	it	= impl->container.begin	();
	str_container_impl::cdb::iterator	end	= impl->container.end		();
	for (; it!=end; )	{
		str_value*	sv = *it;
		if (0==sv->dwReference)	
		{
			str_container_impl::cdb::iterator	i_current	= it;
			str_container_impl::cdb::iterator	i_next		= ++it;
			xr_free			(sv);
			impl->container.erase	(i_current);
			it							= i_next;
		} else {
			it++;
		}
	}
	if (impl->container.empty())	impl->container.clear	();
	cs.Leave	();
}

void		str_container::verify	()
{
	cs.Enter	();
	str_container_impl::cdb::iterator	it	= impl->container.begin	();
	str_container_impl::cdb::iterator	end	= impl->container.end		();
	for (; it!=end; ++it)	{
		str_value*	sv		= *it;
		u32			crc		= crc32	(sv->value,sv->dwLength);
		string32	crc_str;
		R_ASSERT3	(crc==sv->dwCRC, "CorePanic: read-only memory corruption (shared_strings)", itoa(sv->dwCRC,crc_str,16));
		R_ASSERT3	(sv->dwLength == xr_strlen(sv->value), "CorePanic: read-only memory corruption (shared_strings, internal structures)", sv->value);
	}
	cs.Leave	();
}

void		str_container::dump	()
{
	cs.Enter	();
	str_container_impl::cdb::iterator	it	= impl->container.begin	();
	str_container_impl::cdb::iterator	end	= impl->container.end		();
	FILE* F		= fopen("d:\\$str_dump$.txt","w");
	for (; it!=end; it++)
		fprintf		(F,"ref[%4d]-len[%3d]-crc[%8X] : %s\n",(*it)->dwReference,(*it)->dwLength,(*it)->dwCRC,(*it)->value);
	fclose		(F);
	cs.Leave	();
}

u32			str_container::stat_economy		()
{
	cs.Enter	();
	str_container_impl::cdb::iterator	it		= impl->container.begin	();
	str_container_impl::cdb::iterator	end		= impl->container.end		();
	int				counter	= 0;
	counter			-= sizeof(*this);
	counter			-= sizeof(str_container_impl::cdb::allocator_type);
	const int		node_size = 20;
	for (; it!=end; it++)	{
		counter		-= HEADER;
		counter		-= node_size;
		counter		+= int((int((*it)->dwReference) - 1)*int((*it)->dwLength + 1));
	}
	cs.Leave		();

	return			u32(counter);
}

str_container::~str_container		()
{
	clean	();
	//dump ();
	xr_delete(impl);
	//R_ASSERT(impl->container.empty());
}
#endif