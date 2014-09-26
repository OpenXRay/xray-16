#include "stdafx.h"
#pragma hdrstop

#include "FS2.h"

// memory
CMemoryStream::~CMemoryStream()
{
	xr_free(data);
}

void CMemoryStream::write(const void* ptr, u32 count)
{
	if (position+count > mem_size) {
		// reallocate
		if (mem_size==0)	mem_size=128;
		while (mem_size <= (position+count)) mem_size*=2;
		data = (u8*)xr_realloc(data,mem_size);
	}
	CopyMemory(data+position,ptr,count);
	position+=count;
	if (position>file_size) file_size=position;
}
