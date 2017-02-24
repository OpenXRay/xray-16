/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GT2_BUFFER_H_
#define _GT2_BUFFER_H_

#include "gt2Main.h"

GT2Bool gti2AllocateBuffer(GTI2Buffer * buffer, int size);

int gti2GetBufferFreeSpace(const GTI2Buffer * buffer);

void gti2BufferWriteByte(GTI2Buffer * buffer, GT2Byte b);
void gti2BufferWriteUShort(GTI2Buffer * buffer, unsigned short s);
void gti2BufferWriteData(GTI2Buffer * buffer, const GT2Byte * data, int len);

// shortens the buffer by "shortenBy" (length, not size)
void gti2BufferShorten(GTI2Buffer * buffer, int start, int shortenBy);

#endif
