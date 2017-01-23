///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __STRINGUTIL_H__
#define __STRINGUTIL_H__


// String utilities used by the SDKs

#ifdef __cplusplus
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef _PS2
#define ALIGNED	__attribute__ ((aligned(16)))
#else
#define ALIGNED
#endif

#define UCS2Char        unsigned short
#define UCS2String      unsigned short*     // null terminated
#define UTF8ByteType    unsigned char       // For type casting
#define UTF8String      char*               // may not be NULL terminated when treated as a single character

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define UTF8_FOLLOW_BYTE_TAG     0x80    //:1000 0000    // Identifies 2nd or 3rd byte of UTF8String
#define UTF8_TWO_BYTE_TAG        0xC0	//:1100 0000    // Identifies start of Two-byte UTF8String
#define UTF8_THREE_BYTE_TAG      0xE0	//:1110 0000    // Identifies start of Three-byte UTF8String
#define UTF8_FOUR_BYTE_TAG       0xF0	//:1111 0000    // Unsupported tag, need USC4 to store this

#define UTF8_FOLLOW_BYTE_MASK    0x3F    //:0011 1111    // The value bits in a follow byte
#define UTF8_TWO_BYTE_MASK       0x1F    //:0001 1111    // The value bits in a two byte tag
#define UTF8_THREE_BYTE_MASK     0x0F    //:0000 1111    // The value bits in a three byte tag

#define UTF8_IS_THREE_BYTE(a)    (((UTF8ByteType)a & UTF8_FOUR_BYTE_TAG)==UTF8_THREE_BYTE_TAG)	
#define UTF8_IS_TWO_BYTE(a)	     (((UTF8ByteType)a & UTF8_THREE_BYTE_TAG)==UTF8_TWO_BYTE_TAG)
#define UTF8_IS_FOLLOW_BYTE(a)   (((UTF8ByteType)a & UTF8_TWO_BYTE_TAG)==UTF8_FOLLOW_BYTE_TAG)
#define UTF8_IS_SINGLE_BYTE(a)   ((UTF8ByteType)a <= 0x7F)	// 0-127

#define	REPLACE_INVALID_CHAR     '?' 	// Replace invalid UTF8 chars with this

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Prototypes
//		'_' denotes internal use functions
int _ReadUCS2CharFromUTF8String (const UTF8String  theUTF8String, UCS2Char* theUnicodeChar, int theMaxLength);
int _UCS2CharToUTF8String       (UCS2Char          theUCS2Char,   UTF8String theUTF8String);
int _UCS2ToUTF8ConversionLengthOnly (const UCS2String theUCS2String);
int _UTF8ToUCS2ConversionLengthOnly (const UTF8String theUTF8String);

// Convert string types
int AsciiToUTF8String(const char*      theAsciiString, UTF8String theUTF8String );
int UTF8ToAsciiString(const UTF8String theUTF8String,  char*      theAsciiString);
int UCS2ToUTF8String (const UCS2String theUCS2String,  UTF8String theUTF8String );
int UTF8ToUCS2String (const UTF8String theUTF8String,  UCS2String theUCS2String );
int UCS2ToAsciiString(const UCS2String theUCS2String,  char*      theAsciiString);
int AsciiToUCS2String(const char*      theAsciiString, UCS2String theUCS2String );

// Convert with maximum buffer length
// similar to strncpy
//int UCS2ToUTF8StringLength(const UCS2String theUCS2String, UTF8String theUTF8String, int theMaxLength);
int UTF8ToUCS2StringLen(const UTF8String theUTF8String, UCS2String theUCS2String, int theMaxLength);
    
// Convert a string, allocate space for the new string
UTF8String UCS2ToUTF8StringAlloc(const UCS2String theUCS2String);
UCS2String UTF8ToUCS2StringAlloc(const UTF8String theUTF8String);

// Convert an array of strings, allocate space for the new strings
UTF8String* UCS2ToUTF8StringArrayAlloc(const UCS2String* theUCS2StringArray, int theNumStrings);
UCS2String* UTF8ToUCS2StringArrayAlloc(const UTF8String* theUTF8StringArray, int theNumStrings);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __STRINGUTIL_H__

