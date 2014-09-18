//TODO: Address Byte order & Byte alignment issues
#ifndef _GT_ENCODE_H
#define _GT_ENCODE_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(UNDER_CE) || defined(__mips64) || defined(_PSP)
#define ALIGNED_COPY
//the aligned copy code all needs to be optimized
#endif




// Used to identify the type of message so you can look up the correct format string
// and pass the correct parameters
// You should use 1 msgType for each unique format string/parameter combination
////////////////////////////////////////////////////////
typedef unsigned short GTMessageType;

// Encode a message into outBuffer
// Returns the length of the encoded message, or -1 to indicate insufficient space
// You must make sure the number of arguments match the fmtString list
////////////////////////////////////////////////////////
int gtEncode(GTMessageType msgType, const char *fmtString, char *outBuffer, int outLength, ...);
int gtEncodeV(GTMessageType msgType, const char *fmtString, char *outBuffer, int outLength, va_list *args);
int gtEncodeNoType(const char *fmtString, char *outBuffer, int outLength, ...);
int gtEncodeNoTypeV(const char *fmtString, char *outBuffer, int outLength, va_list *args);

// Decode the message from inBuffer into the vars provided
// Returns -1 if there was a problem with the buffer
// Vars should all be pointers (as if using scanf)
// You must make sure the number of arguments match the fmtString list
////////////////////////////////////////////////////////
int gtDecode(const char *fmtString, char *inBuffer, int inLength, ...);
int gtDecodeV(const char *fmtString, char *inBuffer, int inLength, 	va_list *args);
int gtDecodeNoType(const char *fmtString, char *inBuffer, int inLength, ...);
int gtDecodeNoTypeV(const char *fmtString, char *inBuffer, int inLength, va_list *args);

// Retrieve the message type for an encoded message
////////////////////////////////////////////////////////
GTMessageType	gtEncodedMessageType	(char *inBuffer);
// change the message type for an encoded message
void			gtEncodedMessageTypeSet	(char *inBuffer, GTMessageType newtype);

// This handles alignment issues and endianess
/////////////////////////////////////////////////////////
void gt2MemCopy16(char *out, char const *in);
void gt2MemCopy32(char *out, char const *in);
void gt2MemCopy64(char *out, char const *in);
void gt2MemCopy(char *out, char const *in, int size);

/****************************
Types that can be sent using the encode/decode functions
Most are self-explanatory, but the following require some clarification:

GT_CSTR: This is a NUL terminated C-string. The length is determined automatically.
The NUL character is restored in decode. You simply pass the char * string as
the arguement for Encode. Note that in the pointer passed to Decode must have enough
memory allocated to it for the max string that will be encoded - otherwise
the destination may get trashed. If you cannot guarantee them max length of the
string, you should use GT_RAW (see below).

GT_DBSTR: Same as a GT_CSTR, except with 2-byte characters instead of single byte.
String must be terminated with a double NUL character.

GT_RAW: Use raw to send data blocks, structures, arrays, etc - although you must 
make sure they're the same on all platforms!
Requires you to pass both a buffer and a length arguement to Encode and Decode.
You should pass the buffer first, then the length.
For Decode, you must initialize the length pointer to the max length of the buffer.
If the decoded data exceeds this length, the Decode function will return -1, and the
length pointer will be set to the actual length. You can then call Decode again with
a buffer that is at least the required length.
Example:
gtEncode(0, "r", buf, buflen, "somerawdata",10);
char *rawbuffer = malloc(5);
char rawlen = 5;
ret = gtDecode(0, "r", buf, buflen, rawbuffer, &rawlen);
//gtDecode will return -1, and rawlen will be set to 10
if (ret == -1)
{
	rawbuffer = realloc(rawbuffer, rawlen);
	gtDecode(0, "r", buf, buflen, rawbuffer, &rawlen);
	//gtDecode will now succeed
}

GT_CSTR_PTR, GT_DBSTR_PTR, GT_RAW_PTR: For Decode, instead of copying the data from
input buffer into the pointer provided, the pointer is simply set to the
offset of the data in the input buffer. This removes the need to allocate
memory for the pointers before hand, and elimantes an extra memory copy.
You will need to pass in double pointers (e.g. char **) so that the pointer
can be changed.
However, you must make sure you don't try to use the pointers after the
input buffer is freed/changed.
Note that the buffer passed in gtReceivedCallback must be copied off if
you want to continue using it after you return from the callback (or, you
can use the non-PTR versions that copy off into the buffers you provide 
automatically)
You can pass the _PTR versions to Encode and they will behave exactly as
the regular versions.

GT_CSTR_PTR, GT_CSTR_ARRAY_PTR: Same as GT_CSTR and GT_CSTR_PTR, but uses
an array of strings instead of a single string.  The array of strings is
terminated by an empty string (a single NUL character).

GT_BIT: If you pass all your bits together in the format string, they will be
packed together to save space. So, the format string "zzzzzzzz" will only take
1 byte for the data (+2 bytes for the message type).
Note that if you have other types between the bits, the packing will NOT occur,
e.g.: "ziz" will use 6 bytes (2 for the bits, 4 for the int), whereas "zzi" would use
only 5 bytes (1 for the bits, 4 for the int)
The argument type for bits is char for Encode and char * for Decode - 
If the char is 0, the bit will not be set, if it's non-zero, the bit will be set.
Note that in Decode, the set bit will always be returned as 1 (not the non-zero value
you set)
*/

#define GT_INT		'i'
#define GT_INT_		"i"
#define GT_INT_TYPE	int
#define GT_UINT		'u'
#define GT_UINT_	"u"
#define GT_UINT_TYPE	unsigned int
#define GT_SHORT	'o'
#define GT_SHORT_	"o"
#define GT_SHORT_TYPE	short
#define GT_USHORT	'p'
#define GT_USHORT_	"p"
#define GT_USHORT_TYPE	unsigned short
#define GT_CHAR		'c'
#define GT_CHAR_	"c"
#define GT_CHAR_TYPE	signed char
#define GT_UCHAR	'b'
#define GT_UCHAR_	"b"
#define GT_UCHAR_TYPE	unsigned char
#define GT_FLOAT	'f'
#define GT_FLOAT_	"f"
#define GT_FLOAT_TYPE	float
#define GT_DOUBLE	'd'
#define GT_DOUBLE_	"d"
#define GT_DOUBLE_TYPE	double
#define GT_CSTR		's'
#define GT_CSTR_	"s"
#define GT_CSTR_TYPE	char *
#define GT_CSTR_PTR 'S'
#define GT_CSTR_PTR_ "S"
#define GT_CSTR_PTR_TYPE	char **
#define GT_DBSTR	'w'
#define GT_DBSTR_	"w"
#define GT_DBSTR_TYPE	short *
#define GT_DBSTR_PTR 'W'
#define GT_DBSTR_PTR_ "W"
#define GT_DBSTR_PTR_TYPE	short **
#define GT_CSTR_ARRAY 'a'
#define GT_CSTR_ARRAY_ "a"
#define GT_CSTR_ARRAY_TYPE  char *
#define GT_CSTR_ARRAY_PTR        'A'
#define GT_CSTR_ARRAY_PTR_       "A"
#define GT_CSTR_ARRAY_PTR_TYPE   char **
#define GT_RAW		'r' //two parameters! (data, then length)
#define GT_RAW_		"r" //two parameters!
#define GT_RAW_TYPE		char *
#define GT_RAW_PTR	'R'
#define GT_RAW_PTR_	"R"
#define GT_RAW_PTR_TYPE		char **
#define GT_BIT		'z'
#define GT_BIT_		"z"
#define GT_BIT_TYPE		unsigned char


#ifdef __cplusplus
}
#endif

#endif
