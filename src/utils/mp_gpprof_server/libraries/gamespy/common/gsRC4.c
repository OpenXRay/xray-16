///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsCommon.h"
#include "gsRC4.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void swap_byte (unsigned char *a, unsigned char *b)
{
    unsigned char swapByte;

    swapByte = *a;
    *a = *b;
    *b = swapByte;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RC4Init(RC4Context *context, const unsigned char *key, int len)
{
	int i=0;
	unsigned char stateIndex = 0;
	unsigned char keyIndex = 0;

	// must supply a key
	assert(key != NULL && len != 0);
	if (key == NULL || len == 0)
		return;

	context->x = 0;
	context->y = 0;

	for (i=0; i<256; i++)
		context->state[i] = (unsigned char)i;
	
	for (i=0; i<256; i++)
	{
		stateIndex = (unsigned char)(stateIndex + context->state[i] + key[keyIndex]);
		swap_byte(&context->state[i], &context->state[stateIndex]);
		keyIndex = (unsigned char)((keyIndex+1)%len);
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RC4Encrypt(RC4Context *context, const unsigned char *src, unsigned char *dest, int len)
{
    int i = 0;
	for (i=0; i<len; i++)
	{
		context->x = (unsigned char)(context->x + 1); // ok to wrap around from overflow
		context->y = (unsigned char)(context->y + context->state[context->x]); // ditto
		swap_byte(&context->state[context->x], &context->state[context->y]);
		dest[i] = (unsigned char)(src[i] ^ context->state[(unsigned char)(context->state[context->x]+context->state[context->y])]);
	}
}
