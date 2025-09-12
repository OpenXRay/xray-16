///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Conversion Utility for ASCII, UTF8 and USC2 (Unicode) character sets
//
// See RFC2279 for reference
//
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsCommon.h"
#include "gsStringUtil.h"

#ifdef __cplusplus
extern "C" {
#endif	

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Reads UCS2 character from UTF8String
//
// [in]		theUTF8String	:	UTF8String, doesn't need to be null terminated
// [out]	theUCS2Char		:	The 2 byte UCS2 equivalent
// [in]     theMaxLength    :   Maximum number of *bytes* to read (not UTF8 characters)
//
// return value				:	The number of bytes read from theUTF8String
//                              0 = error when parsing
//
//  Remarks:
//		If theUTF8String is invalid, theUnicodeChar will be set to '?'
//		Function is designed for convenient parsing of UTF8 data streams
//
//	Security Concern:
//		Because data is routed through an ASCII stream prior to this function being
//		called, embedded NULLs are stripped and hence, this function does not check for them
//		For example, the UTF-8 byte :1000 0000, would convert to a UCS2 NULL character
//		If this appeared in the middle of a stream, it could cause undesired operation
int _ReadUCS2CharFromUTF8String(const UTF8String theUTF8String,  UCS2Char* theUnicodeChar, int theMaxLength)
{
#ifndef _PS2
	assert(theUnicodeChar != NULL);
#endif

	if (theMaxLength == 0)
	{
		// assert?
		*theUnicodeChar = (UCS2Char)REPLACE_INVALID_CHAR;
		return 0; // not enough data
	}

	// Check for normal ascii range (includes NULL terminator)
	if (UTF8_IS_SINGLE_BYTE(theUTF8String[0]))
	{
		// ASCII, just copy the value
		*theUnicodeChar = (UCS2Char)theUTF8String[0];
		return 1;
	}

	// Check for 2 byte UTF8
	else if (UTF8_IS_TWO_BYTE(theUTF8String[0]))
	{
		if (theMaxLength < 2)
		{
			*theUnicodeChar = (UCS2Char)REPLACE_INVALID_CHAR;
			return 0; // not enough data
		}

		// Make sure the second byte is valid 
		if (UTF8_IS_FOLLOW_BYTE(theUTF8String[1]))
		{
			// Construct 11 bit unicode character
			//		5 value bits from first UTF8Byte			(:000ABCDE)
			//		plus 6 value bits from the second UTF8Byte	(:00FGHIJK)
			//	Store as (:0000 0ABC DEFG HIJK)
			*theUnicodeChar  =	(unsigned short)(((theUTF8String[0] & UTF8_TWO_BYTE_MASK) << 6) +
								((theUTF8String[1] & UTF8_FOLLOW_BYTE_MASK)));
			return 2;
		}
	}

	// Check for 3 byte UTF8
	else if (UTF8_IS_THREE_BYTE(theUTF8String[0]))
	{
		if (theMaxLength < 3)
		{
			*theUnicodeChar = (UCS2Char)REPLACE_INVALID_CHAR;
			return 0; // not enough data
		}

		// Make sure the second and third bytes are valid
		if (UTF8_IS_FOLLOW_BYTE(theUTF8String[1]) &&
			UTF8_IS_FOLLOW_BYTE(theUTF8String[2]))
		{
			// Construct 16 bit unicode character
			//		4 value bits from first UTF8Byte			(:0000ABCD)
			//		plus 6 value bits from the second UTF8Byte	(:00EFGHIJ)
			//		plus 6 value bits from the third  UTF8Byte	(:00KLMNOP)
			//	Store as (:ABCD EFGH IJKL MNOP)
			*theUnicodeChar  =	(unsigned short)(((theUTF8String[0] & UTF8_THREE_BYTE_MASK) << 12) +
								((theUTF8String[1] & UTF8_FOLLOW_BYTE_MASK) << 6) +
								((theUTF8String[2] & UTF8_FOLLOW_BYTE_MASK)));
			return 3;	
		}
	}

	// Invalid character, replace with '?' and return false
	*theUnicodeChar = (UCS2Char)REPLACE_INVALID_CHAR;

	// The second byte on could have been the start of a new valid UTF8 character
	// so we can only safely discard one invalid character
	return 1; 
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Converts UCS2 (Unicode) character into UTF8String 
//
// [in]		theUCS2Char		:	The 2 byte character to convert
// [out]	theUTF8String	:	The 1-3 byte UTF8 equivalent
//
// return value				:	The length of theUTF8String in bytes
//
//  Remarks:
//		theUTF8String may be up to 3 bytes, caller is responsible for allocating memory
//		theUTF8String is NOT NULL terminated, 
int _UCS2CharToUTF8String(UCS2Char theUCS2Char, UTF8String theUTF8String)
{
#ifndef _PS2
	assert(theUTF8String != NULL);
#endif

	// Screen out simple ascii (includes NULL terminator)
	if (theUCS2Char <= 0x7F)
	{
		// 0-7 bit unicode, copy stright over
		theUTF8String[0] = (char)(UTF8ByteType)theUCS2Char;
		return 1;
	}
	else if (theUCS2Char <= 0x07FF)
	{
		// 8-11 bits unicode, store as two byte UTF8
		// :00000ABC DEFGHIJK
		// :110ABCDE 10FGHIJK
		theUTF8String[0] = (char)(UTF8ByteType)(UTF8_TWO_BYTE_TAG | (theUCS2Char >> 6));				// Store the upper 5/11 bits as 0x110xxxxx
		theUTF8String[1] = (char)(UTF8ByteType)(UTF8_FOLLOW_BYTE_TAG | (theUCS2Char & UTF8_FOLLOW_BYTE_MASK));	// Store the lower 6 bits as 0x10xxxxxx
		return 2;
	}
	else
	{
		// 12-16 bits unicode, store as three byte UTF8
		// :ABCDEFGH IJKLMNOP
		// :1110ABCD 10EFGHIJ 10KLMNOP
		theUTF8String[0] = (char)(UTF8ByteType)(UTF8_THREE_BYTE_TAG |  (theUCS2Char >> 12));					// Store the upper 4/16 bits as 0x1110xxxx
		theUTF8String[1] = (char)(UTF8ByteType)(UTF8_FOLLOW_BYTE_TAG | ((theUCS2Char >> 6) & UTF8_FOLLOW_BYTE_MASK));	// Store the 5th-10th bits as 0x10xxxxxx
		theUTF8String[2] = (char)(UTF8ByteType)(UTF8_FOLLOW_BYTE_TAG | ((theUCS2Char) & UTF8_FOLLOW_BYTE_MASK));			// Store the last 6 bits as 0x10xxxxxx
		return 3;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert an ASCII string to UTF8
//
//  Since an ASCII string IS a valid UTF8 string, just copy and return
//
//  [in]	theAsciiString, NULL terminated c-string
//  [out]	theUTF8String, NULL terminated UTF8String
//
//  returns the length of theUTF8String
int AsciiToUTF8String(const char* theAsciiString, UTF8String theUTF8String)
{
	// Allow for NULL here since SDKs allow for NULL string arrays
	if (theAsciiString == NULL)
	{
		*theUTF8String = 0x00;
		return 1;
	}
	else
	{
		// Copy the string, keeping track of length
		unsigned int aLength = 0;
		while (*theAsciiString != '\0')
		{
			*(theUTF8String++) = *(theAsciiString++);
			aLength++;
		} 

		// Append the null
		*theUTF8String = '\0';
		aLength++;

		return (int)aLength;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UTF8String to it's ASCII equivalent
//
//  [in]	theUTF8String, NULL terminated UTF8String
//	[out]	theAsciiString, NULL terminated c-string
//
//  returns the length of theAsciiString
//
//	  Remarks:
//		Unvalid ASCII characters are replaced with '?'
//		Memory allocated for theAsciiString may need to be as large as the UTF8String
//		UTF8String will be NULL terminated
int UTF8ToAsciiString(const UTF8String theUTF8String, char* theAsciiString)
{
	// Strip non-ascii characters and replace with REPLACE_INVALID_CHAR
	const unsigned char* anInStream = (const unsigned char*)theUTF8String;
	unsigned int   aNumBytesWritten = 0;

	// Allow for NULL here since SDKs allow for NULL string arrays
	if (theUTF8String == NULL)
	{
		*theAsciiString = 0x00;
		return 1;
	}

	// Keep extracting characters until we get a '\0'
	while (*anInStream != '\0')
	{
		if (UTF8_IS_SINGLE_BYTE(*anInStream))
			theAsciiString[aNumBytesWritten++] = (char)*anInStream;
		else
			theAsciiString[aNumBytesWritten++] = REPLACE_INVALID_CHAR;

		// move to next character
		anInStream++;
	}

	// Append the '\0'
	theAsciiString[aNumBytesWritten++] = '\0';
	return (int)aNumBytesWritten;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UCS2 (Unicode) string to it's UTF8 equivalent
//
//  [in]	theUCS2String, double NULL terminated UTF8String
//	[out]	theUTF8String, NULL terminated c-string
//
//  returns the length of theUTF8String
//
//	  Remarks:
//		Memory allocated for theUTF8String may need to be up to 1.5* the size of theUCS2String
//      This means that for each UCS2 character, 3 UTF8 characters may be generated
int UCS2ToUTF8String(const UCS2String theUCS2String, UTF8String theUTF8String)
{
	unsigned int	aTotalBytesWritten	= 0;
	unsigned int	aUTF8CharLength		= 0;
	const UCS2Char*	anInStream			= theUCS2String;
	unsigned char*	anOutStream			= (unsigned char*)theUTF8String;

	// Allow for NULL here since SDKs allow for NULL string parameters
	if (theUCS2String == NULL)
	{
		*anOutStream = 0x00;
		return 1;
	}

	// Loop until we reach a NULL terminator
	while(*anInStream != 0)
	{
		aUTF8CharLength = (unsigned int)_UCS2CharToUTF8String(*anInStream, (UTF8String)anOutStream);

		// Move out stream to next character position
		anOutStream += aUTF8CharLength;

		// Move to next UCS2 character
		anInStream++;

		// Record number of bytes written
		aTotalBytesWritten += aUTF8CharLength;
	}
	
	// Copy over the null terminator
	*anOutStream = '\0';
	aTotalBytesWritten++;

	return (int)aTotalBytesWritten;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UTF8 string to it's UCS2 (Unicode) equivalent
//
//  [in]	theUTF8String, NULL terminated UTF8String
//	[out]	theUCS2String, NULL terminated c-string
//
//  returns the length of theUCS2String
//
//	  Remarks:
//		Unvalid UTF8 characters are replaced with '?'
//		Memory allocated for theAsciiString may need to be as large as the UTF8String
//		UTF8String will be NULL terminated
int UTF8ToUCS2String(const UTF8String theUTF8String, UCS2String theUCS2String)
{
	return UTF8ToUCS2StringLen(theUTF8String, theUCS2String, (gsi_i32)strlen(theUTF8String));
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Calculate the size needed to convert a UTF8String to a UCS2String
//
//  [in]	theUTF8String, NULL terminated UTF8String
//
//  returns the length (in UCS2 characters) of theUCS2String that would be created
//
//	  Remarks:
//		Unvalid UTF8 characters are treated as 1 byte
int _UTF8ToUCS2ConversionLengthOnly(const UTF8String theUTF8String)
{
	int length = 0;
	const UTF8String theReadPos = theUTF8String;

	assert(theUTF8String != NULL);
	if (theUTF8String == NULL)
		return 0;

	while (*theReadPos != '\0')
	{
		// Check for valid two byte string
		if (UTF8_IS_TWO_BYTE(theReadPos[0]) && UTF8_IS_FOLLOW_BYTE(theReadPos[1]))
			theReadPos += 2;

		// Check for valid three byte string
		else if (UTF8_IS_THREE_BYTE(theReadPos[0]) && 
				 UTF8_IS_FOLLOW_BYTE(theReadPos[1]) &&
				 UTF8_IS_FOLLOW_BYTE(theReadPos[2]))
		{
			theReadPos += 3;
		}
		// Anything else means one UTF8 character read from the buffer
		else
			theReadPos++;

		// Increment the length of the UCS2 string
		length++;
	}

	// don't count the null as a character, this conforms
	// with ANSI strlen functions
	return length;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Calculate the size needed to convert a UCS2String to a UTF8String
//
//  [in]	theUCS2String, NULL terminated UCS2String
//
//  returns the length of theUTF8String that would be created
//
//	  Remarks:
//		Unvalid UTF8 characters are treated as 1 byte
int _UCS2ToUTF8ConversionLengthOnly(const UCS2String theUCS2String)
{
	int length = 0;
	const UCS2String theReadPos = theUCS2String;
	assert(theUCS2String != NULL);
	while (*theReadPos != 0x0000)
	{
		// Values <= 0x7F are single byte ascii
		if (*theReadPos <= 0x7F)
			length++;
		// Values > 0x7F and <= 0x07FF are two bytes in UTF8
		else if (*theReadPos <= 0x07FF) 
			length += 2;
		// Anything else is 3 bytes of UTF8
		else
			length += 3;

		// Set read pos to right spot (1 more UCS2 Character = 2 bytes)
		theReadPos++;
	}

	// don't count the null as a character, this conforms
	// with ANSI strlen functions
	return length;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UTF8String to a UCS2String, allocate space for the UCS2String
//
//  [in]	theUTF8String, NULL terminated UTF8String
//
//  returns the newly allocated UCS2String 
//
//	  Remarks:
//		The callee is responsible for freeing the allocated memory block
UCS2String UTF8ToUCS2StringAlloc(const UTF8String theUTF8String)
{
	// Allow for NULL here since SDKs allow for NULL string parameters
	if (theUTF8String == NULL)
		return NULL;
	else
	{
		// Find the length of the UCS2 string and allocate a block
		int newLength = _UTF8ToUCS2ConversionLengthOnly(theUTF8String);
		UCS2String aUCS2String = (UCS2String)gsimalloc(sizeof(UCS2Char)*(newLength + 1));

		// Do the conversion
		UTF8ToUCS2String(theUTF8String, aUCS2String);

		// Return the allocated string
		return aUCS2String;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UCS2String to a UTF8String, allocate space for the UTF8String
//
//  [in]	UCS2String, NULL terminated UCS2String
//
//  returns the newly allocated UTF8String 
//
//	  Remarks:
//		The callee is responsible for freeing the allocated memory block
UTF8String UCS2ToUTF8StringAlloc(const UCS2String theUCS2String)
{
	// Allow for NULL here since SDKs allow for NULL string parameters
	if (theUCS2String == NULL)
		return NULL;
	else
	{
		// Find the length of the UCS2 string and allocate a block
		int newLength			= _UCS2ToUTF8ConversionLengthOnly(theUCS2String);
		UTF8String aUTF8String	= (UTF8String)gsimalloc(sizeof(char)*(newLength + 1));

		// Do the conversion
		UCS2ToUTF8String(theUCS2String, aUTF8String);

		// Return the allocated string
		return aUTF8String;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UTF8StringArray to a UCS2StringArray, allocate space for the UCS2Strings
//
//  [in]	UTF8StringArray, array of NULL terminated UTF8Strings
//  [in]	theNumStrings, how many strings are in the array
//
//  returns the newly allocated UCS2StringArray
//
//	  Remarks:
//		The callee is responsible for freeing the allocated memory block(s)
UCS2String* UTF8ToUCS2StringArrayAlloc(const UTF8String* theUTF8StringArray, int theNumStrings)
{
	// Allow for NULL here since SDKs allow for NULL string arrays
	if(theUTF8StringArray == NULL || theNumStrings == 0)
		return NULL;
	else
	{
		UCS2String* aUCS2StringArray = (UCS2String*)gsimalloc(sizeof(UCS2String)*theNumStrings);
		int stringNum = 0;
		while(stringNum < theNumStrings)
		{
			aUCS2StringArray[stringNum] = UTF8ToUCS2StringAlloc(theUTF8StringArray[stringNum]);
			stringNum++;
		}

		return aUCS2StringArray;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UCS2StringArray to a UTF8StringArray, allocate space for the UTF8Strings
//
//  [in]	UCS2StringArray, array of NULL terminated UCS2Strings
//  [in]	theNumStrings, how many strings are in the array
//
//  returns the newly allocated UTF8StringArray
//
//	  Remarks:
//		The callee is responsible for freeing the allocated memory block
UTF8String* UCS2ToUTF8StringArrayAlloc(const UCS2String* theUCS2StringArray, int theNumStrings)
{
	// Allow for NULL here since SDKs allow for NULL string arrays
	if (theUCS2StringArray == NULL || theNumStrings == 0)
		return NULL;
	else
	{
		UTF8String* aUTF8StringArray = (UTF8String*)gsimalloc(sizeof(UTF8String)*theNumStrings);
		int stringNum = 0;
		while(stringNum < theNumStrings)
		{
			aUTF8StringArray[stringNum] = UCS2ToUTF8StringAlloc(theUCS2StringArray[stringNum]);
			stringNum++;
		}

		return aUTF8StringArray;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UCS2String to an AsciiString
//
//  [in]		UCS2StringArray, NULL terminated UCS2String
//  [in/out]	theAsciiString, ascii representation
//
//  returns the length of the Ascii string
//
//	  Remarks:
//		callee is responsible for allocating memory for theAsciiString
//		Invalid ASCII characters are truncated
//		The ASCII buffer must be at least 1/2 the size of the UCS2String
int UCS2ToAsciiString(const UCS2String theUCS2String, char* theAsciiString)
{
	int length = 0;
	const UCS2String aReadPos = theUCS2String;
	char* aWritePos = theAsciiString;

	assert(theAsciiString != NULL);

	// Allow for NULL here since SDKs allow for NULL string arrays
	if (theUCS2String == NULL)
	{
		*theAsciiString = '\0';
		return 1;
	}

	// Convert each character until a '\0' is reached
	while(*aReadPos != '\0')
	{
		(*aWritePos++) = (char)(0x00FF & (*aReadPos++));
		length++;
	}

	// append the NULL
	*aWritePos = '\0';
	length++;

	return length;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert an ASCII string to a UCS2String
//
//  [in]		theAsciiString, NULL terminated ASCII string
//  [in/out]	theUCS2String, UCS2String to be filled with the converted ASCII
//
//  returns the length of the unicode string
//
//	  Remarks:
//		The callee is responsible for allocating memory for theUCS2String
//		the size returned should always be 2x the size passed in
int AsciiToUCS2String(const char* theAsciiString, UCS2String theUCS2String)
{
	int length = 0;
	const char* aReadPos = theAsciiString;
	UCS2String aWritePos = theUCS2String;

	assert(theUCS2String != NULL);

	// Allow for NULL here since SDKs allow for NULL string arrays
	if (theAsciiString == NULL)
	{
		*theUCS2String = 0x0000;
		return 1;
	}

	// Convert each character until a '\0' is reached
	while(*aReadPos != '\0')
	{
		(*aWritePos++) = (unsigned short)(0x00FF & (*aReadPos++)); // copy and strip extra byte
		length++;
	}

	// append a NULL terminator to the UCS2String
	*aWritePos = '\0';
	length++;

	return length;
}

/*
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UCS2String to a UTF8String with a maximum length
//
//  [in]     theUCS2String, NULL terminated UCS2String
//  [in/out] theUTF8String, The UTF8 equivilent of theUCS2String
//  [in]     theMaxLength, maximum number of UTF8 characters to write
//
//  returns the length of the UTF8String 
//
//	  Remarks:
//		The length of theUTF8String will not exceed theMaxLength supplied.
int UCS2ToUTF8StringLength(const UCS2String theUCS2String, UTF8String theUTF8String, int theMaxLength)
{
	return 0;
}
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Convert a UTF8String to a UCS2String with a maximum length
//
//  [in]     theUTF8String, NULL terminated UTF8String
//  [in/out] theUCS2String, The UCS2 equivilent of theUTF8String
//  [in]     theMaxLength, maximum number of UTF8 characters to write
//
//  returns the length of the UCS2String 
//
//	  Remarks:
//		The length of theUCS2String will not exceed theMaxLength supplied.
int UTF8ToUCS2StringLen(const UTF8String theUTF8String, UCS2String theUCS2String, int theMaxLength)
{
	int aNumCharsWritten	= 0;
	int aNumBytesRead		= 0;
	int aTotalBytesRead     = 0;
	const unsigned char* anInStream	= (const unsigned char*)theUTF8String;
	UCS2Char*            anOutStream= theUCS2String;

	// Allow for NULL here since SDKs allow for NULL string arrays
	if (theUTF8String == NULL)
	{
		*anOutStream = 0x0000;
		return 1;
	}

	// Loop until we find the NULL terminator
	while (*anInStream != '\0' && theMaxLength > aTotalBytesRead)
	{
		// Convert one character
		aNumBytesRead = _ReadUCS2CharFromUTF8String((UTF8String)anInStream, anOutStream, theMaxLength-aTotalBytesRead);
		if (aNumBytesRead == 0)
		{
			// Error, read past end of buffer
			theUCS2String[0] = 0x0000;
			return 0;
		}
		aTotalBytesRead += aNumBytesRead;

		// Move InStream position to new data
		anInStream += aNumBytesRead;

		// Keep track of characters written
		aNumCharsWritten++;

		// Move OutStream to next write position
		anOutStream++;
	}

	// NULL terminate the UCS2String
	*anOutStream = 0x0000;
	aNumCharsWritten++;
	
	return aNumCharsWritten;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} //extern "C"
#endif	



