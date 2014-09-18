///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "sciSerialize.h"



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeInt8(gsi_u8* theCursor, gsi_i8 theValue)
{
	// Copy int8 value to possibly misaligned destination
	gsi_u8* dst = (gsi_u8*)theCursor;
	
	*dst++ = (gsi_u8)theValue;

	return (gsi_u8*)dst;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeInt16(gsi_u8* theCursor, gsi_i16 theValue)
{
	// Convert to network byte order
	gsi_i16 netValue = (gsi_i16)htons(theValue);

	// Copy int16 value to possibly misaligned destination
	gsi_u8* dst = (gsi_u8*)theCursor;
	gsi_u8* src = (gsi_u8*)&netValue;
	
	*dst++ = *src++;
	*dst++ = *src++;

	return (gsi_u8*)dst;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeInt32(gsi_u8* theCursor, gsi_i32 theValue)
{
	// Convert to network byte order
	gsi_i32 netValue = (gsi_i32)htonl(theValue);

	// Copy int32 value to possibly misaligned destination
	gsi_u8* dst = (gsi_u8*)theCursor;
	gsi_u8* src = (gsi_u8*)&netValue;
	
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;

	return (gsi_u8*)dst;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeInt64(gsi_u8* theCursor, gsi_i64 theValue)
{
	// Convert to network byte order
	//gsi_i32 netValue = (gsi_i32)htonl(theValue);

	// Copy int32 value to possibly misaligned destination
	gsi_u8* dst = (gsi_u8*)theCursor;
	gsi_u8* src = (gsi_u8*)&theValue;

#ifdef GSI_BIG_ENDIAN
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];

	dst[4] = src[4];
	dst[5] = src[5];
	dst[6] = src[6];
	dst[7] = src[7];
#else
	dst[0] = src[7];
	dst[1] = src[6];
	dst[2] = src[5];
	dst[3] = src[4];

	dst[4] = src[3];
	dst[5] = src[2];
	dst[6] = src[1];
	dst[7] = src[0];
#endif 

	return (gsi_u8*)dst;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeFloat(gsi_u8* theCursor, float theValue)
{
	unsigned char netValue[4];
	gsi_u8 *dst, *src;

#if defined(GSI_BIG_ENDIAN)
	//swap byte ordering for floats to little endian - htonl doesn't work
	gsiFloatSwap(netValue, theValue);
#else
	memcpy(netValue, &theValue, 4);
#endif

	dst = (gsi_u8*)theCursor;
	src = (gsi_u8*)&netValue;
	
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;

	return (gsi_u8*)dst;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeKey(gsi_u8* theCursor, gsi_u16 theKey)
{
	return sciSerializeInt16(theCursor, (gsi_i16)theKey);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeDataLength(gsi_u8* theCursor, gsi_u32 theDataLength)
{
	return sciSerializeInt32(theCursor, (gsi_i32)theDataLength);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u8* sciSerializeGUID(gsi_u8* theCursor, const gsi_u8 theGUID[SC_CONNECTION_GUID_SIZE])
{
	// a GUID is a string comprised of an int, two shorts and eight bytes
	// 6B29FC40-CA47-1067-B31D-00DD010662DA
	gsi_u32 anInt = 0;
	gsi_u32 aShort1 = 0;
	gsi_u32 aShort2 = 0;
	gsi_u32 aIntArray[8];
	gsi_u8 aByteArray[8]; 
	int i=0;

	sscanf((char *)theGUID, "%8x", &anInt);
	sscanf((char *)theGUID+9, "%4x", &aShort1);
	sscanf((char *)theGUID+14, "%4x", &aShort2);

	sscanf((char *)theGUID+19, "%2x", &aIntArray[0]);
	sscanf((char *)theGUID+21, "%2x", &aIntArray[1]);

	sscanf((char *)theGUID+24, "%2x%2x%2x%2x%2x%2x", &aIntArray[2],
		&aIntArray[3], &aIntArray[4], &aIntArray[5],
		&aIntArray[6], &aIntArray[7]);

	for (i=0; i < 8; i++)
		aByteArray[i] = (gsi_u8)aIntArray[i];

	theCursor = sciSerializeInt32(theCursor, (gsi_i32)anInt);
	theCursor = sciSerializeInt16(theCursor, (gsi_i16)aShort1);
	theCursor = sciSerializeInt16(theCursor, (gsi_i16)aShort2);
	memcpy(theCursor, aByteArray, 8);
	theCursor += 8;

	return theCursor;
}
