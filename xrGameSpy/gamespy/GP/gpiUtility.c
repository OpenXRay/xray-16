/*
gpiUtility.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4244) //lines: 333
#pragma warning(disable: 4267) //lines: 142
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS


//INCLUDES
//////////
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "gpi.h"

//DEFINES
/////////
#define OUTPUT_MAX_COL     100

// Disable compiler warnings for issues that are unavoidable.
/////////////////////////////////////////////////////////////
#if defined(_MSC_VER) // DevStudio
// Level4, "conditional expression is constant". 
// Occurs with use of the MS provided macro FD_SET
#pragma warning ( disable: 4127 )
#endif // _MSC_VER

//FUNCTIONS
///////////
void
strzcpy(
  char * dest,
  const char * src,
  size_t len
)
{
	assert(dest != NULL);
	assert(src != NULL);

	strncpy(dest, src, len);
	dest[len - 1] = '\0';
}

GPIBool
gpiCheckForError(
  GPConnection * connection,
  const char * input,
  GPIBool callErrorCallback
)
{
	char buffer[16];
	GPIConnection * iconnection = (GPIConnection*)*connection;
	
	if(strncmp(input, "\\error\\", 7) == 0)
	{
		// Get the err code.
		////////////////////
		if(gpiValueForKey(input, "\\err\\", buffer, sizeof(buffer)))
			iconnection->errorCode = (GPErrorCode)atoi(buffer);
		
		// Get the error string.
		////////////////////////
		if(!gpiValueForKey(input, "\\errmsg\\", iconnection->errorString, sizeof(iconnection->errorString)))
			iconnection->errorString[0] = '\0';

#ifdef GSI_UNICODE
		// Update the UNICODE version
		UTF8ToUCS2String(iconnection->errorString, iconnection->errorString_W);
#endif
		// Call the error callback?
		///////////////////////////
		if(callErrorCallback)
		{
			GPIBool fatal = (GPIBool)(strstr(input, "\\fatal\\") != NULL);
			gpiCallErrorCallback(connection, GP_SERVER_ERROR, fatal ? GP_FATAL : GP_NON_FATAL);
		}
		
		return GPITrue;
	}

	return GPIFalse;
}

GPIBool
gpiValueForKeyWithIndex(
  const char * command,
  const char * key,
  int * index,
  char * value,
  int len
)
{
    char delimiter;
    const char * start;
    int i;
    char c;

    // Check for NULL.
    //////////////////
    assert(command != NULL);
    assert(key != NULL);
    assert(value != NULL);
    assert(len > 0);

    // Find which char is the delimiter.
    ////////////////////////////////////
    delimiter = key[0];

    // Find the key - first navigate to the index
    /////////////////////////////////////////////
    command += *index;
    start = strstr(command, key);
    if(start == NULL)
        return GPIFalse;

    // Get to the start of the value.
    /////////////////////////////////
    start += strlen(key);

    // Copy in the value.
    /////////////////////
    len--;
    for(i = 0 ; (i < len) && ((c = start[i]) != '\0') && (c != delimiter) ; i++)
    {
        value[i] = c;
    }
    value[i] = '\0';

    // Copy back current end point for index
    ////////////////////////////////////////
    *index += ((start - command) + strlen(value));

    return GPITrue;
}

GPIBool
gpiValueForKey(
  const char * command,
  const char * key,
  char * value,
  int len
)
{
	char delimiter;
	const char * start;
	int i;
	char c;

	// Check for NULL.
	//////////////////
	assert(command != NULL);
	assert(key != NULL);
	assert(value != NULL);
	assert(len > 0);

	// Find which char is the delimiter.
	////////////////////////////////////
	delimiter = key[0];

	// Find the key.
	////////////////
	start = strstr(command, key);
	if(start == NULL)
		return GPIFalse;

	// Get to the start of the value.
	/////////////////////////////////
	start += strlen(key);

	// Copy in the value.
	/////////////////////
	len--;
	for(i = 0 ; (i < len) && ((c = start[i]) != '\0') && (c != delimiter) ; i++)
	{
		value[i] = c;
	}
	value[i] = '\0';

	return GPITrue;
}

char *
gpiValueForKeyAlloc(
  const char * command,
  const char * key
)
{
	char delimiter;
	const char * start;
	char c;
	char * value;
	int len;

	// Check for NULL.
	//////////////////
	assert(command != NULL);
	assert(key != NULL);

	// Find which char is the delimiter.
	////////////////////////////////////
	delimiter = key[0];

	// Find the key.
	////////////////
	start = strstr(command, key);
	if(start == NULL)
		return NULL;

	// Get to the start of the value.
	/////////////////////////////////
	start += strlen(key);

	// Find the key length.
	///////////////////////
	for(len = 0 ; ((c = start[len]) != '\0') && (c != delimiter) ; len++)  { };

	// Allocate the value.
	//////////////////////
	value = (char *)gsimalloc((unsigned int)len + 1);
	if(!value)
		return NULL;

	// Copy in the value.
	/////////////////////
	memcpy(value, start, (unsigned int)len);
	value[len] = '\0';

	return value;
}

GPResult
gpiCheckSocketConnect(
  GPConnection * connection,
  SOCKET sock,
  int * state
)
{
	int aWriteFlag  = 0;
	int aExceptFlag = 0;
	int aReturnCode = 0;

	// Check if the connect is completed.
	/////////////////////////////////////
	aReturnCode = GSISocketSelect(sock, NULL, &aWriteFlag, &aExceptFlag);
	if ( gsiSocketIsError(aReturnCode))
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError,
			"Error connecting\n");
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error checking for a completed connection.");
	}

	if (aReturnCode > 0)
	{
		// Check for a failed attempt.
		//////////////////////////////
		if(aExceptFlag)
		{
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError,
				"Connection rejected\n");
			*state = GPI_DISCONNECTED;
			return GP_NO_ERROR;
		}

		// Check for a successful attempt.
		//////////////////////////////////
		if(aWriteFlag)
		{
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice,
				"Connection accepted\n");
			*state = GPI_CONNECTED;
			return GP_NO_ERROR;
		}
	}

	// Not connected yet.
	/////////////////////
	*state = GPI_NOT_CONNECTED;
	return GP_NO_ERROR;
}

GPResult
gpiReadKeyAndValue(
  GPConnection * connection,
  const char * buffer,
  int * index,
  char key[512],
  char value[512]
)
{
	int c;
	int i;
	char * start;

	assert(buffer != NULL);
	assert(key != NULL);
	assert(value != NULL);

	buffer += *index;
	start = (char *)buffer;

	if(*buffer++ != '\\')
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Parse Error.");

	for(i = 0 ; (c = *buffer++) != '\\' ; i++)
	{
		if(c == '\0')
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Parse Error.");
		if(i == 511)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Parse Error.");
		*key++ = (char)c;
	}
	*key = '\0';

	for(i = 0 ; ((c = *buffer++) != '\\') && (c != '\0') ; i++)
	{
		if(i == 511)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Parse Error.");
		*value++ = (char)c;
	}
	*value = '\0';

	*index += (buffer - start - 1);

	return GP_NO_ERROR;
}

void
gpiSetError(
  GPConnection * connection,
  GPErrorCode errorCode,
  const char * errorString
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	
	// Copy the string.
	///////////////////
	strzcpy(iconnection->errorString, errorString, GP_ERROR_STRING_LEN);

#ifdef GSI_UNICODE
	// Update the unicode version
	UTF8ToUCS2StringLen(iconnection->errorString, iconnection->errorString_W, GP_ERROR_STRING_LEN);
#endif

	// Set the code.
	////////////////
	iconnection->errorCode = errorCode;
}

void
gpiSetErrorString(
  GPConnection * connection,
  const char * errorString
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	
	// Copy the string.
	///////////////////
	strzcpy(iconnection->errorString, errorString, GP_ERROR_STRING_LEN);

#ifdef GSI_UNICODE
	// Update the unicode version
	UTF8ToUCS2StringLen(iconnection->errorString, iconnection->errorString_W, GP_ERROR_STRING_LEN);
#endif

}

void
gpiEncodeString(
  const char * unencodedString,
  char * encodedString
)
{
	size_t i;
	const int useAlternateEncoding = 1;

	// Encrypt the password (xor with random values)
	char passwordxor[GP_PASSWORD_LEN];
	size_t passwordlen = strlen(unencodedString);
	
	Util_RandSeed((unsigned long)GP_XOR_SEED);
	for (i=0; i < passwordlen; i++)
	{
		// XOR each character with the next rand
		char aRand = (char)Util_RandInt(0, 0xFF);
		passwordxor[i] = (char)(unencodedString[i] ^ aRand);
	}
	passwordxor[i] = '\0';

	// Base 64 it (printable chars only)
	B64Encode(passwordxor, encodedString, (int)passwordlen, useAlternateEncoding);
}


// Re-enable previously disabled compiler warnings
///////////////////////////////////////////////////
#if defined(_MSC_VER)
#pragma warning ( default: 4127 )
#endif // _MSC_VER

