/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "gt2Buffer.h"
#include <stdlib.h>

GT2Bool gti2AllocateBuffer(GTI2Buffer * buffer, int size)
{
	buffer->buffer = (GT2Byte *)gsimalloc((unsigned int)size);
	if(!buffer->buffer)
		return GT2False;
	buffer->size = size;

	return GT2True;
}

int gti2GetBufferFreeSpace(const GTI2Buffer * buffer)
{
	return (buffer->size - buffer->len);
}

void gti2BufferWriteByte(GTI2Buffer * buffer, GT2Byte b)
{
	assert(buffer->len < buffer->size);
#if 0
	if(buffer->len >= buffer->size)
		return;
#endif

	buffer->buffer[buffer->len++] = b;
}

void gti2BufferWriteUShort(GTI2Buffer * buffer, unsigned short s)
{
	assert((buffer->len + 2) <= buffer->size);
#if 0
	if((buffer->len + 2) > buffer->size)
		return;
#endif

	buffer->buffer[buffer->len++] = (GT2Byte)((s >> 8) & 0xFF);
	buffer->buffer[buffer->len++] = (GT2Byte)(s & 0xFF);
}

void gti2BufferWriteData(GTI2Buffer * buffer, const GT2Byte * data, int len)
{
	if(!data || !len)
		return;

	if(len == -1)
		len = (int)strlen((const char *)data);

	assert((buffer->len + len) <= buffer->size);
#if 0
	if(buffer->len >= buffer->size)
		return;
#endif

	memcpy(buffer->buffer + buffer->len, data, (unsigned int)len);
	buffer->len += len;
}

void gti2BufferShorten(GTI2Buffer * buffer, int start, int shortenBy)
{
	if(start == -1)
		start = (buffer->len - shortenBy);

	assert(start <= buffer->len);
	assert(shortenBy <= (buffer->len - start));

	memmove(buffer->buffer + start, buffer->buffer + start + shortenBy, (unsigned int)(buffer->len - start - shortenBy));
	buffer->len -= shortenBy;
}
