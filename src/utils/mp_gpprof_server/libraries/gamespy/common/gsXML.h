///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __GSXML_H__
#define __GSXML_H__


#include "gsPlatform.h"
#include "gsLargeInt.h"  // so that it can write large ints


#if defined(__cplusplus)
extern "C"
{
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// GameSpy XML parser for soap messages
//   Create the stream object and attach to an XML text buffer.  
//   The stream will not modify the buffer.
//   The buffer should not be released until after the stream is destroyed
//
//
//	Limitations:
//	  Processing instructions other than '<?xml' are not supported.
//    CDATA sections are not supported.
//    XML versions other than '1.0' are not supported.
//    Encoding types other than 'UTF-8' are not supported.
//    Element names may contain only alphanumeric characters or '_'
//    Elements may contain values OR child elements, not both.


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// XML does not have set size restrictions on many string items.  We set some
// reasonable length limitations for parsing efficiency.
#define	GS_XML_MAX_ELEMENT_NAME_LENGTH		128
#define GS_XML_MAX_ATTRIBUTE_NAME_LENGTH	128
#define GS_XML_MAX_ATTRIBUTE_VALUE_LENGTH	1024


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef void* GSXmlStreamReader;
typedef void* GSXmlStreamWriter;

struct gsLargeInt_s; // forward declare in case of header order problems


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GSXmlStreamWriter gsXmlCreateStreamWriter(const char ** namespaces, int count);
GSXmlStreamReader gsXmlCreateStreamReader();
void gsXmlFreeReader(GSXmlStreamReader stream);
void gsXmlFreeWriter(GSXmlStreamWriter stream);

void gsXmlResetReader(GSXmlStreamReader stream); // prepare reader for re-use

// Write utilities
const char * gsXmlWriterGetData       (GSXmlStreamWriter stream);
int      gsXmlWriterGetDataLength     (GSXmlStreamWriter stream);
gsi_bool gsXmlCloseWriter       (GSXmlStreamWriter stream);
gsi_bool gsXmlWriteStringElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const char * value);
gsi_bool gsXmlWriteAsciiStringElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const gsi_char * value);
gsi_bool gsXmlWriteUnicodeStringElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const unsigned short * value);
gsi_bool gsXmlWriteIntElement   (GSXmlStreamWriter stream, const char * namespaceName, const char * tag, gsi_u32 value);
gsi_bool gsXmlWriteInt64Element   (GSXmlStreamWriter stream, const char * namespaceName, const char * tag, gsi_i64 value);
gsi_bool gsXmlWriteFloatElement (GSXmlStreamWriter stream, const char * namespaceName, const char * tag, float value);
gsi_bool gsXmlWriteHexBinaryElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const gsi_u8 * data, int len);
gsi_bool gsXmlWriteBase64BinaryElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const gsi_u8 * data, int len);
gsi_bool gsXmlWriteDateTimeElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, time_t value);
gsi_bool gsXmlWriteLargeIntElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const struct gsLargeInt_s * lint);
gsi_bool gsXmlWriteOpenTag      (GSXmlStreamWriter stream, const char * namespaceName, const char * tag);
gsi_bool gsXmlWriteCloseTag     (GSXmlStreamWriter stream, const char * namespaceName, const char * tag);

// Read utilities
gsi_bool gsXmlParseBuffer       (GSXmlStreamReader stream, char * buffer, int len);
//    Move:
gsi_bool gsXmlMoveToStart       (GSXmlStreamReader stream);
gsi_bool gsXmlMoveToParent      (GSXmlStreamReader stream);
gsi_bool gsXmlMoveToNext        (GSXmlStreamReader stream, const char * matchtag);
gsi_bool gsXmlMoveToSibling     (GSXmlStreamReader stream, const char * matchtag);
gsi_bool gsXmlMoveToChild       (GSXmlStreamReader stream, const char * matchtag);
//    Read child values:  (separate read position from above)
gsi_bool gsXmlReadChildAsString   (GSXmlStreamReader stream, const char * matchtag, const char ** valueOut, int * lenOut);
gsi_bool gsXmlReadChildAsStringNT (GSXmlStreamReader stream, const char * matchtag, char valueOut[], int maxLen);
//gsi_bool gsXmlReadChildAsUnicodeString   (GSXmlStreamReader stream, const char * matchtag, gsi_char ** valueOut, int * lenOut);
gsi_bool gsXmlReadChildAsUnicodeStringNT (GSXmlStreamReader stream, const char * matchtag, gsi_char valueOut[], int maxLen);
gsi_bool gsXmlReadChildAsInt      (GSXmlStreamReader stream, const char * matchtag, int * valueOut);
gsi_bool gsXmlReadChildAsInt64    (GSXmlStreamReader stream, const char * matchtag, gsi_i64 * valueOut);
gsi_bool gsXmlReadChildAsFloat    (GSXmlStreamReader stream, const char * matchtag, float * valueOut);
gsi_bool gsXmlReadChildAsDateTimeElement(GSXmlStreamReader stream, const char * matchtag, time_t * valueOut);
gsi_bool gsXmlResetChildReadPosition(GSXmlStreamReader stream); // reset child read position to first child of current element

// NOTE:  HexStrings are BIG-endian, the valueout will also be BIG-endian.
// NOTE:  Call with NULL valueOut to get the lenOut.  Doing this will not move read position.
gsi_bool gsXmlReadChildAsHexBinary(GSXmlStreamReader stream, const char * matchtag, gsi_u8 valueOut[], int maxLen, int * lenOut);

// NOTE:  Call with NULL valueOut to get the lenOut.  Doing this will not move read position
gsi_bool gsXmlReadChildAsBase64Binary(GSXmlStreamReader stream, const char * matchtag, gsi_u8 valueOut[], int * lenOut);

// NOTE:  gsLargeInt_t is transmitted as HexString, but must be converted to little-endian.
gsi_bool gsXmlReadChildAsLargeInt(GSXmlStreamReader stream, const char * matchtag, struct gsLargeInt_s * valueOut);

//    Count:
int      gsXmlCountChildren     (GSXmlStreamReader stream, const char * matchtag);

// Unicode compatible string read/write functions
#ifdef GSI_UNICODE
	#define gsXmlWriteTStringElement(s,n,t,v)	gsXmlWriteUnicodeStringElement(s,n,t,v)
	//#define gsXmlReadChildAsTString(s,m,v,l)	gsXmlReadChildAsUnicodeString(s,m,v,l)
	#define gsXmlReadChildAsTStringNT(s,m,v,l)	gsXmlReadChildAsUnicodeStringNT(s,m,v,l)
#else
	#define gsXmlWriteTStringElement(s,n,t,v)	gsXmlWriteStringElement(s,n,t,v)
	//#define gsXmlReadChildAsTString(s,m,v,l)	gsXmlReadChildAsString(s,m,v,l)
	#define gsXmlReadChildAsTStringNT(s,m,v,l)	gsXmlReadChildAsStringNT(s,m,v,l)
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
} // extern "C"
#endif

#endif // __GSXML_H__
