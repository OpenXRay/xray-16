#include "stdafx.h"
#pragma hdrstop

using namespace std;

XRCORE_API	smem_container*	g_pSharedMemoryContainer	= NULL;

smem_value*			smem_container::dock			(u32 dwCRC, u32 dwLength, void* ptr)
{
	VERIFY						(dwCRC && dwLength && ptr);

	cs.Enter					();
	smem_value*		result		= 0;

	// search a place to insert
	u8				storage		[4*sizeof(u32)];
	smem_value*		value		= (smem_value*)storage;
	value->dwReference			= 0;
	value->dwCRC				= dwCRC;
	value->dwLength				= dwLength;
	cdb::iterator	it			= std::lower_bound	(container.begin(),container.end(),value,smem_search);
	cdb::iterator	saved_place	= it;
	if (container.end() != it)	{
		// supposedly found
		for (;;	it++)	{
			if (it==container.end())			break;
			if ((*it)->dwCRC	!= dwCRC)		break;
			if ((*it)->dwLength != dwLength)	break;
			if (0==memcmp((*it)->value,ptr,dwLength))
			{
				// really found
				result			= *it;
				break;
			}
		}
	}

	// if not found - create new entry
	if (0==result)
	{
		result					= (smem_value*)	Memory.mem_alloc	(4*sizeof(u32) + dwLength
#ifdef DEBUG_MEMORY_NAME
			, "storage: smem"
#endif // DEBUG_MEMORY_NAME
			);
		result->dwReference		= 0;
		result->dwCRC			= dwCRC;
		result->dwLength		= dwLength;
		CopyMemory			(result->value,ptr,dwLength);
		container.insert		(saved_place,result);
	}

	// exit
	cs.Leave					();
	return						result;
}

void				smem_container::clean			()
{
	cs.Enter		();
	cdb::iterator	it	= container.begin	();
	cdb::iterator	end	= container.end		();
	for (; it!=end; it++)	if (0==(*it)->dwReference)	xr_free	(*it);
	container.erase	(remove(container.begin(),container.end(),(smem_value*)0),container.end());
	if (container.empty())	container.clear	();
	cs.Leave		();
}

void				smem_container::dump			()
{
	cs.Enter		();
	cdb::iterator	it	= container.begin	();
	cdb::iterator	end	= container.end		();
	FILE* F			= fopen("x:\\$smem_dump$.txt","w");
	for (; it!=end; it++)
		fprintf		(F,"%4d : crc[%6x], %d bytes\n",(*it)->dwReference,(*it)->dwCRC,(*it)->dwLength);
	fclose			(F);
	cs.Leave		();
}

u32					smem_container::stat_economy	()
{
	cs.Enter		();
	cdb::iterator	it		= container.begin	();
	cdb::iterator	end		= container.end		();
	s64				counter	= 0;
	counter			-= sizeof(*this);
	counter			-= sizeof(cdb::allocator_type);
	const int		node_size = 20;
	for (; it!=end; it++)	{
		counter		-= 16;
		counter		-= node_size;
		counter		+= s64((s64((*it)->dwReference) - 1)*s64((*it)->dwLength));
	}
	cs.Leave		();

	return			u32(s64(counter)/s64(1024));
}

smem_container::~smem_container	()
{
	clean			();
}
