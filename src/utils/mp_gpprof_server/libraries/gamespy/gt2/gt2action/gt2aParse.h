/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2APARSE_H_
#define _GT2APARSE_H_

// Search the input for "\key\".
// Returns the value following the key,
// or NULL if the key wasn't found.
// If key is NULL, returns the first key
// in input, or NULL if there's an error.
/////////////////////////////////////////
char * ParseKeyValue
(
	char * input,
	const char * key
);

#endif