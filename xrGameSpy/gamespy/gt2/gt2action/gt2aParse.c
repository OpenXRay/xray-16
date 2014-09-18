/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include <string.h>
#include "gt2aParse.h"

static char buffer[256];

static char * StripKeyValue
(
	char * input
)
{
	int i;
	int c;

	// Get the key.
	///////////////
	for(i = 0 ; i < (sizeof(buffer) - 1) ; i++)
	{
		c = *input++;
		if((c == '\\') || (c == '\0'))
			break;
		buffer[i] = (char)c;
	}
	buffer[i] = '\0';

	return buffer;
}

char * ParseKeyValue
(
	char * input,
	const char * key
)
{
	char * str;
	int len;

	if(!input)
		return NULL;

	// Are we looking for the first key?
	////////////////////////////////////
	if(!key)
	{
		// Does it start with a '\'?
		////////////////////////////
		if(*input++ != '\\')
			return NULL;

		// Get the key.
		///////////////
		return StripKeyValue(input);
	}

	// Find the key.
	////////////////
	buffer[0] = '\\';
	len = strlen(key);
	memcpy(buffer + 1, key, len);
	buffer[++len] = '\\';
	buffer[++len] = '\0';
	str = strstr(input, buffer);
	if(!str)
		return NULL;
	str += strlen(buffer);

	// Get the key.
	///////////////
	return StripKeyValue(str);
}