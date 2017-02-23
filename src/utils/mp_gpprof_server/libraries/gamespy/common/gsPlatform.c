///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsPlatform.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Include standard network lib
#if defined(_WIN32) && !defined(_XBOX)
	#if defined(GSI_WINSOCK2)
		#pragma comment(lib, "ws2_32")
	#else
		#pragma comment(lib, "wsock32")
	#endif
	#pragma comment(lib, "advapi32")
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Floating point specific byte reversal functions
// stores the result in a 4-byte character array
unsigned char * gsiFloatSwap(unsigned char buf[4], float f)
{
	unsigned char *dst = (unsigned char *)buf;
	unsigned char *src = (unsigned char *)&f;

	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];

	return buf;
}

// unswap using char pointers
float gsiFloatUnswap(unsigned char buf[4]) 
{
	float f;
	unsigned char *src = (unsigned char *)buf;
	unsigned char *dst = (unsigned char *)&f;

	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];

	return f;
}

gsi_u16 gsiByteOrderSwap16(gsi_u16 _in)
{
	gsi_u16 t;
	const char *in = (char *)&_in;
	char *out = (char *)&t;
	out[0] = in[1];
	out[1] = in[0];
	return t;
}

gsi_u32 gsiByteOrderSwap32(gsi_u32 _in)
{
	gsi_u32 t;
	const char *in = (char *)&_in;
	char *out = (char *)&t;
	out[0] = in[3];
	out[1] = in[2];
	out[2] = in[1];
	out[3] = in[0];
	return t;
}

gsi_u64 gsiByteOrderSwap64(gsi_u64 _in)
{
	gsi_u64 t;
	const char *in = (char *)&_in;
	char *out = (char *)&t;
	out[0] = in[7];
	out[1] = in[6];
	out[2] = in[5];
	out[3] = in[4];
	out[4] = in[3];
	out[5] = in[2];
	out[6] = in[1];
	out[7] = in[0];
	return t;
}

