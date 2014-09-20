///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsPlatform.h"
#include "gsMemory.h"
#include "gsXML.h"
#include "gsStringUtil.h"
#include "../darray.h"

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4267) //lines: 155, 156, 161, 162
#pragma warning(disable: 4244) //lines: 128, 720, 752
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define GS_XML_INITIAL_ELEMENT_ARRAY_COUNT	    32
#define GS_XML_INITIAL_ATTRIBUTE_ARRAY_COUNT    16

#define GS_XML_WHITESPACE    "\x20\x09\x0D\x0A"

#define GS_XML_CHECK(a)  { if (gsi_is_false(a)) return gsi_false; }

#define GS_XML_BASE64_ENCODING_TYPE             0

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define GS_XML_SOAP_BUFFER_INITIAL_SIZE	    (1 * 1024)
#define GS_XML_SOAP_BUFFER_INCREMENT_SIZE   (1 * 1024)
#define GS_XML_SOAP_INITIAL_NAMESPACE_COUNT 6

#define GS_XML_SOAP_HEADER     "<?xml version=\"1.0\" encoding=\"UTF-8\"?><SOAP-ENV:Envelope"
#define GS_XML_SOAP_BODY_TAG   "<SOAP-ENV:Body>"
#define GS_XML_SOAP_FOOTER     "</SOAP-ENV:Body></SOAP-ENV:Envelope>"
#define GS_XML_SOAP_NAMESPACE_PREFIX "xmlns:"

#define GS_XML_SOAP_DEFAULT_NAMESPACE_COUNT 4
const char * GS_XML_SOAP_DEFAULT_NAMESPACES[GS_XML_SOAP_DEFAULT_NAMESPACE_COUNT] =
{
	"SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"",
	"SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\"",
	"xsi=\"http://www.w3.org/2001/XMLSchema-instance\"",
	"xsd=\"http://www.w3.org/2001/XMLSchema\""
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Warning: Do not store pointers to other GSXml* objects within a GSXml object
//          Pointers to elements may be invalidate if the DArray's grow
//          Instead, store the array index and require that indexes never change
typedef struct GSIXmlString
{
	const gsi_u8 * mData;
	int mLen;
} GSIXmlString;

typedef struct GSIXmlAttribute
{
	GSIXmlString mName;
	GSIXmlString mValue;

	int mIndex;
	int mParentIndex;
} GSIXmlAttribute;

typedef struct GSIXmlElement
{
	GSIXmlString mName;
	GSIXmlString mValue; // most do not have a value

	int mIndex;
	int mParentIndex;
} GSIXmlElement;

typedef struct GSIXmlStreamReader
{
	DArray mElementArray;
	DArray mAttributeArray;

	int mElemReadIndex;   // current index
	int mValueReadIndex;  // current child parsing index
} GSIXmlStreamReader;

typedef struct GSIXmlStreamWriter
{
	char * mBuffer;
	int mLen;
	int mCapacity;
	gsi_bool mClosed; // footer has been written, append not allowed
} GSIXmlStreamWriter;



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilSkipWhiteSpace(GSIXmlStreamReader * stream, const char * buffer, int len, int * pos);
static gsi_bool gsiXmlUtilParseName     (GSIXmlStreamReader * stream, const char * buffer, int len, int * pos, GSIXmlString * strOut);
static gsi_bool gsiXmlUtilParseString   (GSIXmlStreamReader * stream, char * buffer, int len, int * pos, GSIXmlString * strOut);
static gsi_bool gsiXmlUtilParseValue    (GSIXmlStreamReader * stream, char * buffer, int len, int * pos, GSIXmlString * strOut);
static gsi_bool gsiXmlUtilParseElement  (GSIXmlStreamReader * stream, char * buffer, int len, int * pos, int parentIndex);
static gsi_bool gsiXmlUtilTagMatches(const char * matchtag, GSIXmlString * xmlstr);

// Note: Writes decoded form back into buffer
static gsi_bool gsiXmlUtilDecodeString(char * buffer, int * len);

static void gsiXmlUtilElementFree(void * elem);
static void gsiXmlUtilAttributeFree(void * elem);

static gsi_bool gsiXmlUtilWriteChar(GSIXmlStreamWriter * stream, char ch);
static gsi_bool gsiXmlUtilWriteString(GSIXmlStreamWriter * stream, const char * str);
static gsi_bool gsiXmlUtilWriteXmlSafeString(GSIXmlStreamWriter * stream, const char * str);
static gsi_bool gsiXmlUtilGrowBuffer(GSIXmlStreamWriter * stream);
#ifdef GSI_UNICODE
static gsi_bool gsiXmlUtilWriteAsciiString(GSIXmlStreamWriter * stream, const gsi_char * str);
#endif
static gsi_bool gsiXmlUtilWriteUnicodeString(GSIXmlStreamWriter * stream, const unsigned short * str);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int gsUnicodeStringLen(const unsigned short * str)
{
	const unsigned short * end = str;
	while(*end++)
		{}
	return (end - str - 1);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GSXmlStreamWriter gsXmlCreateStreamWriter(const char ** namespaces, int count)
{
	GSIXmlStreamWriter * newStream = NULL;
	int initialCapacity = GS_XML_SOAP_BUFFER_INCREMENT_SIZE;
	int namespaceLen = 0;
	int i=0;

	GS_ASSERT((namespaces == NULL && count == 0) || (namespaces != NULL && count != 0));

	newStream = (GSIXmlStreamWriter*)gsimalloc(sizeof(GSIXmlStreamWriter));
	if (newStream == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
			"Out of memory in gsXmlCreateStreamWriter, needed %d bytes", sizeof(GSIXmlStreamWriter));
		return NULL; // OOM
	}

	// do this to prevent an immediately reallocation due to long namespace string
	for (i=0; i < GS_XML_SOAP_DEFAULT_NAMESPACE_COUNT; i++)
	{
		GS_ASSERT(GS_XML_SOAP_DEFAULT_NAMESPACES[i] != NULL);
		namespaceLen += strlen(GS_XML_SOAP_NAMESPACE_PREFIX)+1; // +1 for space
		namespaceLen += strlen(GS_XML_SOAP_DEFAULT_NAMESPACES[i]);
	}
	for (i=0; i < count; i++)
	{
		GS_ASSERT(namespaces[i] != NULL);
		namespaceLen += strlen(GS_XML_SOAP_NAMESPACE_PREFIX)+1; // +1 for space
		namespaceLen += strlen(namespaces[i]);
	}
	while (initialCapacity < namespaceLen)
		initialCapacity += GS_XML_SOAP_BUFFER_INCREMENT_SIZE;

	// allocate write buffer
	newStream->mBuffer = (char*)gsimalloc((size_t)initialCapacity);
	if (newStream->mBuffer == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
			"Out of memory in gsXmlCreateStreamWriter, needed %d bytes", initialCapacity);
		return NULL; // OOM
	}
	newStream->mCapacity = initialCapacity;
	newStream->mLen = 0;
	newStream->mBuffer[0] = '\0';
	newStream->mClosed = gsi_false;

	// write the XML header
	if (gsi_is_false(gsiXmlUtilWriteString(newStream, GS_XML_SOAP_HEADER)))
	{
		gsifree(newStream->mBuffer);
		gsifree(newStream);
		return NULL; // OOM
	}
	for (i=0; i < GS_XML_SOAP_DEFAULT_NAMESPACE_COUNT; i++)
	{
		if (gsi_is_false(gsiXmlUtilWriteChar(newStream, ' ')) ||
			gsi_is_false(gsiXmlUtilWriteString(newStream, GS_XML_SOAP_NAMESPACE_PREFIX)) ||
			gsi_is_false(gsiXmlUtilWriteString(newStream, GS_XML_SOAP_DEFAULT_NAMESPACES[i])))
		{
			gsifree(newStream->mBuffer);
			gsifree(newStream);
			return NULL; // OOM
		}
	}
	for (i=0; i < count; i++)
	{
		if (gsi_is_false(gsiXmlUtilWriteChar(newStream, ' ')) ||
			gsi_is_false(gsiXmlUtilWriteString(newStream, GS_XML_SOAP_NAMESPACE_PREFIX)) ||
			gsi_is_false(gsiXmlUtilWriteString(newStream, namespaces[i])) )
		{
			gsifree(newStream->mBuffer);
			gsifree(newStream);
			return NULL; // OOM
		}
	}
	if (gsi_is_false(gsiXmlUtilWriteChar(newStream, '>')) ||
		gsi_is_false(gsiXmlUtilWriteString(newStream, GS_XML_SOAP_BODY_TAG)) )
	{
		gsifree(newStream->mBuffer);
		gsifree(newStream);
		return NULL; // OOM
	}

	return (GSXmlStreamWriter)newStream;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GSXmlStreamReader gsXmlCreateStreamReader()
{
	GSIXmlStreamReader * newStream = NULL;

	newStream = (GSIXmlStreamReader*)gsimalloc(sizeof(GSIXmlStreamReader));
	if (newStream == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
			"Out of memory in gsXmlCreateStream, needed %d bytes", sizeof(GSIXmlStreamReader));
		return NULL; // OOM
	}
	
	newStream->mElementArray = ArrayNew(sizeof(GSIXmlElement), GS_XML_INITIAL_ELEMENT_ARRAY_COUNT, gsiXmlUtilElementFree);
	if (newStream->mElementArray == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
			"Out of memory in gsXmlCreateStream with mElementArray=ArrayNew()");
		gsifree(newStream);
		return NULL; // OOM
	}

	newStream->mAttributeArray = ArrayNew(sizeof(GSIXmlAttribute), GS_XML_INITIAL_ATTRIBUTE_ARRAY_COUNT, gsiXmlUtilAttributeFree);
	if (newStream->mAttributeArray == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
			"Out of memory in gsXmlCreateStream with mElementArray=ArrayNew()");
		ArrayFree(newStream->mElementArray);
		gsifree(newStream);
		return NULL; // OOM
	}

	gsXmlMoveToStart(newStream);
	return (GSXmlStreamReader)newStream;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlParseBuffer(GSXmlStreamReader stream, char * data, int len)
{
	GSIXmlStreamReader * reader;
	int readPos = 0;

	GS_ASSERT(data != NULL);
	GS_ASSERT(len > 0);

	reader = (GSIXmlStreamReader*)stream;

	gsXmlResetReader(stream);

	// Parse the root elements (automatically includes sub-elements)
	while(readPos < len)
	{
		if (gsi_is_false(gsiXmlUtilParseElement(reader, data, len, &readPos, -1)))
			return gsi_false;
	}

	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void gsXmlFreeWriter(GSXmlStreamWriter stream)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	gsifree(writer->mBuffer);
	gsifree(writer);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void gsXmlFreeReader(GSXmlStreamReader stream)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	ArrayFree(reader->mAttributeArray);
	ArrayFree(reader->mElementArray);
	gsifree(reader);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void gsXmlResetReader(GSXmlStreamReader stream)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	ArrayClear(reader->mAttributeArray);
	ArrayClear(reader->mElementArray);
	gsXmlMoveToStart(stream);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlCloseWriter(GSXmlStreamWriter stream)
{
	GSIXmlStreamWriter* writer = (GSIXmlStreamWriter*)stream;
	GS_ASSERT(stream != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	if (gsi_is_false(gsiXmlUtilWriteString(writer, GS_XML_SOAP_FOOTER)))
		return gsi_false;
	
	writer->mClosed = gsi_true;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void gsiXmlUtilElementFree(void * elem)
{
	GSI_UNUSED(elem);
	//GSXmlElement * dataPtr = (GSXmlElement*)elem;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void gsiXmlUtilAttributeFree(void * elem)
{
	GSI_UNUSED(elem);
	//GSXmlAttribute * dataPtr = (GSXmlAttribute*)elem;
	//gsifree(dataPtr);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const char * gsXmlWriterGetData (GSXmlStreamWriter stream)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	GS_ASSERT(stream != NULL)
	return writer->mBuffer;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int gsXmlWriterGetDataLength(GSXmlStreamWriter stream)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	GS_ASSERT(stream != NULL);
	return writer->mLen;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilSkipWhiteSpace(GSIXmlStreamReader * stream, const char * buffer, int len, int * pos)
{
	GS_ASSERT(buffer != NULL);
	GS_ASSERT(len > 0);
	//GS_ASSERT(*pos < len);

	// check if the next character is in the whitespace set
	while(*pos < len)
	{
		if (NULL == strchr(GS_XML_WHITESPACE, buffer[*pos]))
			return gsi_true;
		(*pos)++; // move to next character
	}

	GSI_UNUSED(stream);
	return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilParseElement(GSIXmlStreamReader * stream, char * buffer, 
									  int len, int * pos, int parentIndex)
{
	GSIXmlElement newElem;
	int startPos = 0;
	gsi_bool storeElement = gsi_true;

	GS_ASSERT(stream != NULL);
	GS_ASSERT(buffer != NULL);
	GS_ASSERT(len > 0);
	GS_ASSERT(*pos < len);

	gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos);
	if (*pos >= len)
		return gsi_true; // legal EOF

	memset(&newElem, 0, sizeof(newElem));

	// elements must begin with '<'
	if (buffer[*pos] != '<')
		return gsi_false;
	(*pos)++;
	if (*pos >= len)
		return gsi_false;
	GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

	// '<' can be followed with '!', '?', '%', '/' special characters
	if (buffer[*pos] == '!' || buffer[*pos] == '?' ||
		buffer[*pos] == '%' || buffer[*pos] == '/')
	{
		storeElement = gsi_false;
		(*pos)++;
		startPos = (*pos)-2;
	}
	else
		startPos = (*pos)-1; // store the position of '<'

	// should be a name (type) next
	GS_XML_CHECK(gsiXmlUtilParseName(stream, buffer, len, pos, &newElem.mName));
	GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

	if (storeElement)
	{
		GS_ASSERT(newElem.mName.mData != NULL);
		newElem.mIndex = ArrayLength(stream->mElementArray);
		newElem.mParentIndex = parentIndex;
		ArrayAppend(stream->mElementArray, &newElem);
	}

	// read attributes (if any)
	while (*pos < len && isalnum(buffer[*pos]))
	{
		// attribute format is name="<value>", e.g. nickname="player1"
		GSIXmlAttribute newAttr;
		memset(&newAttr, 0, sizeof(newAttr));
		GS_XML_CHECK(gsiXmlUtilParseName(stream, buffer, len, pos, &newAttr.mName));
		GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));
		if (buffer[*pos] != '=')
			return gsi_false;
		(*pos)++; // skip the '='
		GS_XML_CHECK(gsiXmlUtilParseString(stream, buffer, len, pos, &newAttr.mValue));
		GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

		// Store it in the array
		if (storeElement) 
		{
			GS_ASSERT(newElem.mName.mData != NULL);
			GS_ASSERT(newElem.mIndex != -1);
			GS_ASSERT(newAttr.mName.mData != NULL);
			GS_ASSERT(newAttr.mValue.mData != NULL);

			newAttr.mIndex = ArrayLength(stream->mAttributeArray);
			newAttr.mParentIndex = newElem.mIndex;
			ArrayAppend(stream->mAttributeArray, &newAttr);
		}
	}

	GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

	// Check for immediate termination (no value or children)
	//    non-element tags end with the same character they start with
	//    element tags ending with '/>' also have no children
	if ( (!isalnum(buffer[startPos+1]) && buffer[*pos] == buffer[startPos+1]) 
			|| buffer[*pos] == '/')
	{
		(*pos)++;
		GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));
		if (buffer[*pos] != '>')
			return gsi_false; // only legal character here is closing brace
		(*pos)++;
		return gsi_true; // legal termination
	}

	// make sure we've found the end of the start tag
	if (buffer[*pos] != '>')
		return gsi_false;
	(*pos)++;
	GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

	// check for element value
	if (buffer[*pos] != '<')
	{
		GS_XML_CHECK(gsiXmlUtilParseValue(stream, buffer, len, pos, &newElem.mValue));
		// update the array with the value information
		if (storeElement)
			ArrayReplaceAt(stream->mElementArray, &newElem, newElem.mIndex);
		GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));
	}

	// read child elements and close tag
	while (*pos < len)
	{
		int childStartPos = *pos;
		if (buffer[*pos] != '<')
			return gsi_false;
		(*pos)++;
		GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));
		if (buffer[*pos] == '/')
		{
			// this MUST be a close of the current element
			// close tags are in the form: </tagname>
			(*pos)++;
			GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));
			if ((*pos)+newElem.mName.mLen >= len)
				return gsi_false; // EOF before tag close
			if (0 != strncmp((const char*)newElem.mName.mData, &buffer[*pos], (size_t)newElem.mName.mLen))
				return gsi_false; // close tag mismatch
			(*pos) += newElem.mName.mLen;
			GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));
			if (buffer[*pos] != '>')
				return gsi_false;
			(*pos)++;
			return gsi_true;
		}
		else
		{
			if (newElem.mValue.mData != NULL)
				return gsi_false; // elements with value cannot have children

			// move read position to start of child element, then parse
			*pos = childStartPos;
			GS_XML_CHECK(gsiXmlUtilParseElement(stream, buffer, len, pos, newElem.mIndex));
			gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos);
		}
	}
	return gsi_false; // EOF before tag close
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Parse element name
//   Must begin with a letter and contains only alphanumeric | '_' | '-' characters
//   Element names may consist of two "names", <namespace>:<elementname>
static gsi_bool gsiXmlUtilParseName(GSIXmlStreamReader * stream, const char * buffer, 
								   int len, int * pos, GSIXmlString * strOut)
{
	gsi_bool haveNamespace = gsi_false;
	GS_ASSERT(buffer != NULL);
	GS_ASSERT(len > 0);

	GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

	// EOF?
	if (*pos >= len)
		return gsi_false;

	// first character must be alphanumeric but not a digit
	if (!isalnum(buffer[*pos]) || isdigit(buffer[*pos]))
		return gsi_false;

	strOut->mData = (gsi_u8*)&buffer[*pos];
	strOut->mLen = 1;
	(*pos)++;

	while(*pos < len && NULL==strchr(GS_XML_WHITESPACE, buffer[*pos]))
	{
		// only alpha numeric and '_' characters are allowed, plus one namespace separator ':'
		if (buffer[*pos] == ':')
		{
			if (gsi_is_true(haveNamespace))
				return gsi_false; // already have a namespace!
			haveNamespace = gsi_true;
		}
		else if ((buffer[*pos] != '_') && (buffer[*pos] != '-') && (!isalnum(buffer[*pos])))
			return gsi_true; // treat all others as a new token example '='

		strOut->mLen++;
		(*pos)++;
	}

	return gsi_true;	
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilParseString(GSIXmlStreamReader * stream, char * buffer,
									 int len, int * pos, GSIXmlString * strOut)
{
	char startCh = '\0';
	char * strStart = NULL;
	//gsi_bool hasMarkup = gsi_false;


	GS_ASSERT(stream != NULL);
	GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

	// strings may start with either ' or "
	startCh = buffer[*pos];
	if (startCh != '\"' && startCh != '\'')
		return gsi_false;

	(*pos)++;
	strStart = &buffer[*pos];  // remember this for easier processing below
	strOut->mLen = 0;

	if (*pos >= len)
		return gsi_false; // EOF when looking for string terminator
	if (buffer[*pos] == startCh)
	{
		// empty string ?
		strOut->mData = (const gsi_u8*)strStart;
		(*pos)++; // skip the terminating character
		return gsi_true; 
	}

	while(buffer[*pos] != startCh)
	{
		if (*pos >= len)
			return gsi_false; // EOF when looking for string terminator

		//if (buffer[*pos] == '&')
			//hasMarkup = gsi_true;

		(*pos)++;
		strOut->mLen++;
	} 
	(*pos)++; // skip the terminating character

	// decode the string if necessary
	if (gsi_is_false(gsiXmlUtilDecodeString(strStart, &strOut->mLen)))
		return gsi_false;

	// set the data into strOut
	strOut->mData = (const gsi_u8*)strStart;
	return gsi_true;

}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Remove '&' markup from the buffer, converts in place
static gsi_bool gsiXmlUtilDecodeString(char * buffer, int * len)
{
	int readPos = 0;

	while(readPos < *len)
	{
		int charsToRemove = 0;

		// we only want '&'
		if (buffer[readPos] != '&')
		{
			readPos++;
			continue;
		}

		// check single character switches
		if (strncmp(&buffer[readPos], "&amp;", 5)==0)
		{
			buffer[readPos++] = '&';
			charsToRemove = 5-1;
		}
		else if (strncmp(&buffer[readPos], "&quot;", 6)==0)
		{
			buffer[readPos++] = '\"';
			charsToRemove = 6-1;
		}
		else if (strncmp(&buffer[readPos], "&apos;", 6)==0)
		{
			buffer[readPos++] = '\'';
			charsToRemove = 6-1;
		}
		else if (strncmp(&buffer[readPos], "&lt;", 4)==0)
		{
			buffer[readPos++] = '<';
			charsToRemove = 4-1;
		}
		else if (strncmp(&buffer[readPos], "&gt;", 4)==0)
		{
			buffer[readPos++] = '>';
			charsToRemove = 4-1;
		}
		else if (strncmp(&buffer[readPos], "&#x", 3)==0)
		{
			// hex digit
			unsigned int digitValue = 0;
			//unsigned int digitLength = 0;  // 0x00000065 = 1 byte
			char ch = ' ';
			gsi_bool haveWritten = gsi_false;
			int i=0;
			unsigned int mask = 0xFF000000;

			char * digitEnd = strchr(&buffer[readPos+3], ';');
			if (digitEnd == NULL)
				return gsi_false; // missing ';'
			if (digitEnd - &buffer[readPos+3] > 8)
				return gsi_false; // too many digits before end

			// scan digits into memory, do this as a block so that &#x165 = 01 65
			sscanf(&buffer[readPos+3], "%08x", &digitValue);

			// write the digit back as a character array
			for (i=0; i < 4; i++)
			{
				ch = (char)((digitValue & mask) >> ((3-i)*8)); // make 0x00006500 into 0x65
				if (haveWritten || ch != 0x00)
				{
					buffer[readPos++] = ch;
					haveWritten = gsi_true;
				}
				mask = mask >> 8;
			}

			// remove everything between the current read position and the semicolon
			charsToRemove = digitEnd - &buffer[readPos] + 1; // remove the semicolon
		}
		else if (strncmp(&buffer[readPos], "&#", 2)==0)
		{
			// dec digit - like a hex digit, only use atoi instead of sscanf
			unsigned int digitValue = 0;
			//unsigned int digitLength = 0;  // 0x00000065 = 1 byte
			char ch = ' ';
			gsi_bool haveWritten = gsi_false;
			int i=0;
			unsigned int mask = 0xFF000000;

			char * digitEnd = strchr(&buffer[readPos+2], ';');
			if (digitEnd == NULL)
				return gsi_false; // missing ';'

			// scan digits into memory, do this as a block so that &#357 = 0165h = 01 65
			digitValue = (unsigned int)atoi(&buffer[readPos+2]);

			// write the digit back as a character array
			for (i=0; i < 4; i++)
			{
				ch = (char)((digitValue & mask) >> ((3-i)*8)); // make 0x00006500 into 0x65
				if (haveWritten || ch != 0x00)
				{
					buffer[readPos++] = ch;
					haveWritten = gsi_true;
				}
				mask = mask >> 8;
			}

			// remove everything between the current read position and the semicolon
			charsToRemove = digitEnd - &buffer[readPos] + 1; // remove the semicolon

		}
		else
			return gsi_false; // unhandle '&' type

		// remove characters by compressing buffer and adding whitespace at the end
		//      "&amp;&amp;" becomes "&&amp;    " after one iteration
		memmove(&buffer[readPos], &buffer[readPos+charsToRemove], (size_t)(*len-(readPos+charsToRemove)));
		memset(&buffer[*len-charsToRemove], ' ', (size_t)charsToRemove);
		(*len) -= charsToRemove; 
	}

	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Parse an entity value.
//   Todo: resolving escaped strings?
static gsi_bool gsiXmlUtilParseValue(GSIXmlStreamReader * stream, char * buffer,
									 int len, int * pos, GSIXmlString * strOut)
{
	char * strStart = NULL;

	GS_ASSERT(stream != NULL);
	GS_XML_CHECK(gsiXmlUtilSkipWhiteSpace(stream, buffer, len, pos));

	if (buffer[*pos] != '<')
	{
		strStart = &buffer[*pos]; // store this so we can find it later
		strOut->mData = (const gsi_u8*)strStart;
	}

	while(*pos < len)
	{
		if (buffer[*pos] == '<')
		{
			// decode the string if necessary
			if (gsi_is_false(gsiXmlUtilDecodeString(strStart, &strOut->mLen)))
				return gsi_false;
			return gsi_true; // extracted and decoded
		}
		(*pos)++;
		strOut->mLen++;
	} 
	return gsi_false; // EOF before tag end
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteOpenTag(GSXmlStreamWriter stream, const char * namespaceName, 
						   const char * tag)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	if ( gsi_is_false(gsiXmlUtilWriteChar(writer, '<')) ||
		 gsi_is_false(gsiXmlUtilWriteString(writer, namespaceName)) ||
		 gsi_is_false(gsiXmlUtilWriteChar(writer, ':')) ||
		 gsi_is_false(gsiXmlUtilWriteString(writer, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteChar(writer, '>')) 
		 )
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteCloseTag(GSXmlStreamWriter stream,	const char * namespaceName,	
							const char * tag)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	if ( gsi_is_false(gsiXmlUtilWriteChar(writer, '<')) ||
		 gsi_is_false(gsiXmlUtilWriteChar(writer, '/')) ||
 		 gsi_is_false(gsiXmlUtilWriteString(writer, namespaceName)) ||
		 gsi_is_false(gsiXmlUtilWriteChar(writer, ':')) ||
		 gsi_is_false(gsiXmlUtilWriteString(writer, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteChar(writer, '>'))
		 )
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteStringElement(GSXmlStreamWriter stream, const char * namespaceName, 
								 const char * tag, const char * value)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	int i=0;
	int len = 0;

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(value != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	// Check legal ASCII characters
	//  0x9, 0xA, 0xD
	//  [0x20-0xFF]
	len = (int)strlen(value);
	for (i=0; i < len; i++)
	{
		// only check values less than 0x20
		if ((unsigned char)value[i] < 0x20)
		{
			if ((unsigned char)value[i] != 0x09
				&& ((unsigned char)value[i] != 0x0A)
				&& ((unsigned char)value[i] != 0x0D)
				)
			{
				// contains illegal (and unencodable) characters.
				return gsi_false;
			}
		}
	}

	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteXmlSafeString(writer, value)) ||
		 gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		 )
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Converts unicode to ascii strings for writing to XML writer
gsi_bool gsXmlWriteAsciiStringElement(GSXmlStreamWriter stream, const char * namespaceName, 
								 const char * tag, const gsi_char * value)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	int i=0;
	int len = 0;

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(value != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	// Check legal ASCII characters
	//  0x9, 0xA, 0xD
	//  [0x20-0xFF]
	len = (int)_tcslen(value);
	for (i=0; i < len; i++)
	{
		// only check values less than 0x20
		if ((unsigned char)value[i] < 0x20)
		{
			if ((unsigned char)value[i] != 0x09
				&& ((unsigned char)value[i] != 0x0A)
				&& ((unsigned char)value[i] != 0x0D)
				)
			{
				// contains illegal (and unencodable) characters.
				return gsi_false;
			}
		}
	}

#ifdef GSI_UNICODE
	//if unicode, write as Ascii string, otherwise it is already in Ascii format
	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteAsciiString(writer, value)) ||
		 gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		 )
	{
		return gsi_false;
	}
#else
	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteXmlSafeString(writer, value)) ||
		 gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		 )
	{
		return gsi_false;
	}
#endif
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteUnicodeStringElement(GSXmlStreamWriter stream, const char * namespaceName, 
										const char * tag, const unsigned short * value)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	int i = 0;
	int len = 0;

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(value != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	// Check legal UNICODE characters
	//  0x9, 0xA, 0xD
	//  [0x20-0xD7FF]
	//  [xE000-xFFFD]
	//  [x10000-x10FFFF] (UTF-16, not supported)
	len = gsUnicodeStringLen(value);
	for (i=0; i < len; i++)
	{
		// check values less than 0x20
		if (value[i] < 0x0020)
		{
			if ((value[i] != 0x0009) && (value[i] != 0x0A) && (value[i] != 0x0D))
				return gsi_false; // contains illegal (and unencodable) characters.
		}
		else if (value[i] > 0xD7FF && value[i] < 0xE000)
			return gsi_false; // contains illegal (and unencodable) characters.
		else if (value[i] > 0xFFFD)
			return gsi_false; // contains illegal (and unencodable) characters.
	}

	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteUnicodeString(writer, value)) ||
		 gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		 )
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteIntElement(GSXmlStreamWriter stream, const char * namespaceName, 
							  const char * tag,	gsi_u32 value)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	char buf[32];

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	sprintf(buf, "%d", value);

	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteString(writer, buf)) ||
		 gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		 )
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteInt64Element(GSXmlStreamWriter stream, const char * namespaceName, 
								const char * tag,	gsi_i64 value)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	char buf[33];

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	gsiInt64ToString(buf, value);

	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		gsi_is_false(gsiXmlUtilWriteString(writer, buf)) ||
		gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		)
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteFloatElement(GSXmlStreamWriter stream, const char * namespaceName, 
								const char * tag, float value)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	char buf[32];

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	sprintf(buf, "%f", value);

	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteString(writer, buf)) ||
		 gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		 )
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// first character of HEX binary is HIGH byte
gsi_bool gsXmlWriteHexBinaryElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const gsi_u8 * data, int len)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	int pos = 0;
	int temp = 0;
	
	char hex[3];
	hex[2] = '\0';

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(data != NULL);
	GS_ASSERT(len > 0);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	if (gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)))
		return gsi_false;

	while (pos < len)
	{
		temp = data[pos]; // sprintf requires an int parameter for %02x operation
		sprintf(hex, "%02x", temp);
		pos++;
		if (gsi_is_false(gsiXmlUtilWriteChar(writer, hex[0])))
			return gsi_false;
		if (gsi_is_false(gsiXmlUtilWriteChar(writer, hex[1])))
			return gsi_false;
	}
	
	if (gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag)))
		return gsi_false;
	
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteBase64BinaryElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const gsi_u8 * data, int len)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	B64StreamData streamData;
	char b64[5];

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(data != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	if (gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)))
		return gsi_false;

	B64InitEncodeStream(&streamData, (const char*)data, len, GS_XML_BASE64_ENCODING_TYPE);
	while(B64EncodeStream(&streamData, b64))
	{
		b64[4] = '\0';
		if (gsi_is_false(gsiXmlUtilWriteString(writer, b64)))
			return gsi_false;
	}
	
	if (gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag)))
		return gsi_false;
	
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlWriteDateTimeElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, time_t value)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	char timeString[21];
	struct tm *timePtr;

	// convert the time to a string
	timePtr = gsiSecondsToDate(&value);

	sprintf(timeString, "%d-%02d-%02dT%02d:%02d:%02dZ",
		timePtr->tm_year + 1900, timePtr->tm_mon + 1, timePtr->tm_mday,
		timePtr->tm_hour, timePtr->tm_min, timePtr->tm_sec);

	GS_ASSERT(stream != NULL);
	GS_ASSERT(namespaceName != NULL);
	GS_ASSERT(tag != NULL);
	GS_ASSERT(gsi_is_false(writer->mClosed));

	if ( gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)) ||
		 gsi_is_false(gsiXmlUtilWriteString(writer, timeString)) ||
		 gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag))
		 )
	{
		return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Must reverse byte order and strip leading zeroes
gsi_bool gsXmlWriteLargeIntElement(GSXmlStreamWriter stream, const char * namespaceName, const char * tag, const struct gsLargeInt_s * lint)
{
	GSIXmlStreamWriter * writer = (GSIXmlStreamWriter*)stream;
	int readPos = (int)(lint->mLength - 1);
	const l_word *readBuf = (const l_word*)lint->mData;
	unsigned int temp;
	int i;
	char hex[3];
	gsi_bool first = gsi_true;

	if (gsi_is_false(gsXmlWriteOpenTag(stream, namespaceName, tag)))
		return gsi_false;

	// skip leading zeroes
	while(readPos >= 0 && readBuf[readPos] == 0)
		readPos--;

	// dump words
	for (; readPos >= 0; readPos--)
	{
		if(gsi_is_true(first))
		{
			for(i = 0 ; i < GS_LARGEINT_DIGIT_SIZE_BYTES ; i++)
			{
				temp = ((lint->mData[readPos] >> (8 * (GS_LARGEINT_DIGIT_SIZE_BYTES - i - 1))) & 0xFF);
				if(temp != 0)
					break;
			}
			first = gsi_false;
		}
		else
		{
			i = 0;
		}

		// loop through each byte from most to least significant
		for (; i < GS_LARGEINT_DIGIT_SIZE_BYTES; i++)
		{
			temp = ((lint->mData[readPos] >> (8 * (GS_LARGEINT_DIGIT_SIZE_BYTES - i - 1))) & 0xFF);
			sprintf(hex, "%02x", temp);
			if (gsi_is_false(gsiXmlUtilWriteChar(writer, hex[0])))
				return gsi_false;
			if (gsi_is_false(gsiXmlUtilWriteChar(writer, hex[1])))
				return gsi_false;
		}
	}

	if (gsi_is_false(gsXmlWriteCloseTag(stream, namespaceName, tag)))
		return gsi_false;

	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilWriteChar(GSIXmlStreamWriter * stream, char ch)
{
	GS_ASSERT(gsi_is_false(stream->mClosed));

	if (stream->mLen >= stream->mCapacity)
	{
		if (gsi_is_false(gsiXmlUtilGrowBuffer(stream)))
			return gsi_false; // OOM
	}
	stream->mBuffer[stream->mLen] = ch;
	stream->mLen++;

	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilWriteString(GSIXmlStreamWriter * stream, const char * str)
{
	int strLen = 0;

	GS_ASSERT(str != NULL);
	GS_ASSERT(gsi_is_false(stream->mClosed));

	// get URL encoded length
	strLen = (int)strlen(str);
	if (strLen == 0)
		return gsi_true;

	// grow the buffer if necessary
	while ((stream->mCapacity - stream->mLen) <= strLen)
	{
		if (gsi_is_false(gsiXmlUtilGrowBuffer(stream)))
			return gsi_false; // OOM
	}

	strcpy(&stream->mBuffer[stream->mLen], str);
	stream->mLen += strLen;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef GSI_UNICODE
static gsi_bool gsiXmlUtilWriteAsciiString(GSIXmlStreamWriter * stream, const gsi_char * str)
{
	int strLen = 0;

	GS_ASSERT(str != NULL);
	GS_ASSERT(gsi_is_false(stream->mClosed));

	// get URL encoded length
	strLen = (int)_tcslen(str);
	if (strLen == 0)
		return gsi_true;

	// grow the buffer if necessary
	while ((stream->mCapacity - stream->mLen) <= strLen)
	{
		if (gsi_is_false(gsiXmlUtilGrowBuffer(stream)))
			return gsi_false; // OOM
	}

	UCS2ToAsciiString(str, &stream->mBuffer[stream->mLen]);
	stream->mLen += strLen;
	return gsi_true;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilWriteUnicodeString(GSIXmlStreamWriter * stream, const unsigned short * str)
{
	int strLen = 0;
	int pos = 0;
	int utf8Len = 0;
	char utf8String[4] = { '\0' };
	//gsi_bool result = gsi_false;

	GS_ASSERT(str != NULL);
	GS_ASSERT(gsi_is_false(stream->mClosed));

	strLen = gsUnicodeStringLen(str);
	utf8String[3] = '\0';
	
	for (pos = 0; pos < strLen; pos++)
	{
		utf8Len = _UCS2CharToUTF8String(str[pos], utf8String);
		utf8String[utf8Len] = '\0'; // null terminate it
		if (gsi_is_false(gsiXmlUtilWriteXmlSafeString(stream, utf8String)))
			return gsi_false;	
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilWriteXmlSafeString(GSIXmlStreamWriter * stream, const char * str)
{
	int strLen = 0;
	int pos = 0;
	gsi_bool result = gsi_false;

	GS_ASSERT(str != NULL);
	GS_ASSERT(gsi_is_false(stream->mClosed));

	strLen = (int)strlen(str);

	for (pos = 0; pos < strLen; pos++)
	{
		if (str[pos] == '&')
			result = gsiXmlUtilWriteString(stream, "&amp;");
		else if (str[pos] == '\'')
			result = gsiXmlUtilWriteString(stream, "&apos;");
		else if (str[pos] == '"')
			result = gsiXmlUtilWriteString(stream, "&quot;");
		else if (str[pos] == '<')
			result = gsiXmlUtilWriteString(stream, "&lt;");
		else if (str[pos] == '>')
			result = gsiXmlUtilWriteString(stream, "&gt;");
		else if (str[pos] == ' ')
			result = gsiXmlUtilWriteString(stream, "&#x20;");
		else if (str[pos] < 0x20 || ((unsigned char)str[pos]) > 0x7F)
		{
			// write as hex
			char numeric[7];
			sprintf(numeric, "&#x%02x;", (unsigned char)str[pos]);
			numeric[6] = '\0';
			result = gsiXmlUtilWriteString(stream, numeric);
		}
		else
			result = gsiXmlUtilWriteChar(stream, str[pos]);

		if (gsi_is_false(result))
			return gsi_false;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiXmlUtilGrowBuffer(GSIXmlStreamWriter * stream)
{
	int newCapacity = stream->mCapacity + GS_XML_SOAP_BUFFER_INCREMENT_SIZE;
	void* newBuf = NULL;

	newBuf = gsirealloc(stream->mBuffer, (size_t)newCapacity);
	if (newBuf == NULL)
		return gsi_false;
	if (newBuf != stream->mBuffer)
		stream->mBuffer = (char*)newBuf;
	stream->mCapacity = newCapacity;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlMoveToStart(GSXmlStreamReader stream)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	reader->mElemReadIndex = -1; // start BEFORE first element
	reader->mValueReadIndex = -1;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Move to next occurance of "matchtag" at any level
gsi_bool gsXmlMoveToNext(GSXmlStreamReader stream, const char * matchtag)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	int i=0;

	for (i=(reader->mElemReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		GSIXmlElement * elem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &elem->mName)))
		{
			reader->mElemReadIndex = i;
			reader->mValueReadIndex = -1;
			return gsi_true;
		}
	}
	// no matching element found
	return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Move up one level in tree
gsi_bool gsXmlMoveToParent(GSXmlStreamReader stream)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;

	// check for invalid position
	if (reader->mElemReadIndex >= ArrayLength(reader->mElementArray) ||
		reader->mElemReadIndex == -1)
	{
		return gsi_false; // current position invalid
	}
	else
	{
		GSIXmlElement * elem = (GSIXmlElement*)ArrayNth(reader->mElementArray, reader->mElemReadIndex);
		if (elem->mParentIndex == -1)
			return gsi_false; // current elem is at highest level
		if (elem->mParentIndex >= ArrayLength(reader->mElementArray))
			return gsi_false; // parent is invalid!
		reader->mElemReadIndex = elem->mParentIndex;
		reader->mValueReadIndex = -1;
		return gsi_true;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Move to next unit who shares common parent
gsi_bool gsXmlMoveToSibling (GSXmlStreamReader stream, const char * matchtag)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	int i=0;

	int curElemParent = -1;
	GSIXmlElement * searchElem = NULL;
	
	// If the current element is valid use its parent id
	if (reader->mElemReadIndex < ArrayLength(reader->mElementArray))
	{
		GSIXmlElement * curElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, reader->mElemReadIndex);
		curElemParent = curElem->mParentIndex;
	}
	else
		// otherwise search root elements only
		curElemParent = -1;

	for (i=(reader->mElemReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		// if sibling...
		if (searchElem->mParentIndex == curElemParent)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchElem->mName)))
			{
				reader->mElemReadIndex = i;
				reader->mValueReadIndex = -1;
				return gsi_true;
			}
		}
		// bail if we reach a higher brance
		if (searchElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}

	// no matching element found
	return gsi_false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlMoveToChild(GSXmlStreamReader stream, const char * matchtag)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchElem = NULL;
	int i=0;

	for (i=(reader->mElemReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchElem->mName)))
			{
				reader->mElemReadIndex = i;
				reader->mValueReadIndex = -1;
				return gsi_true;
			}
		}
		// check if we've reached a higher branch
		//    -- we know this when we've reached an element whose 
		//       parent is above our level in the tree
		if (searchElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlReadChildAsString(GSXmlStreamReader stream, const char * matchtag, 
								 const char ** valueOut, int * lenOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;

	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				reader->mValueReadIndex = i;
				*valueOut = (const char*)searchValueElem->mValue.mData;
				*lenOut = searchValueElem->mValue.mLen;
				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*
gsi_bool gsXmlReadChildAsUnicodeString(GSXmlStreamReader stream, const char * matchtag, 
										gsi_char ** valueOut, int * lenOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;

	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				reader->mValueReadIndex = i;
				if (searchValueElem->mValue.mData == NULL)
				{
					*valueOut = NULL;
					*lenOut = 0;
					return gsi_true;
				}
				*lenOut = UTF8ToUCS2StringLen((const char *)searchValueElem->mValue.mData, (unsigned short *) *valueOut, searchValueElem->mValue.mLen);
				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}*/


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Same as read child as string, but copies into valueOut and null terminates
gsi_bool gsXmlReadChildAsStringNT(GSXmlStreamReader stream, const char * matchtag, char valueOut[], int maxLen)
{
	const char * strValue = NULL;
	int strLen = 0;

	if (gsi_is_false(gsXmlReadChildAsString(stream, matchtag, &strValue, &strLen)))
	{
		valueOut[0] = '\0';
		return gsi_false;
	}
	else
	{
		strncpy(valueOut, strValue, (size_t)min(maxLen, strLen));
		valueOut[min(maxLen-1, strLen)] = '\0';
		return gsi_true;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Same as readChildAsStringNT, but converts Ascii/UTF-8 to USC2
gsi_bool gsXmlReadChildAsUnicodeStringNT(GSXmlStreamReader stream, const char * matchtag, gsi_char valueOut[], int maxLen)
{
	const char * utf8Value = NULL;
	int unicodeLen = 0;
	int utf8Len = 0;

	if (gsi_is_false(gsXmlReadChildAsString(stream, matchtag, &utf8Value, &utf8Len)))
	{
		valueOut[0] = '\0';
		return gsi_false;
	}
	else
	{
		// Convert into destination buffer
		unicodeLen = UTF8ToUCS2StringLen(utf8Value, (unsigned short *)valueOut, utf8Len);
		valueOut[min(maxLen-1, unicodeLen)] = '\0';
		return gsi_true;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlReadChildAsHexBinary(GSXmlStreamReader stream, const char * matchtag, 
								 gsi_u8 valueOut[], int maxLen, int * lenOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;

	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				// switch endianess, e.g. first character in hexstring is HI byte
				gsi_u32 temp = 0;
				int writepos = 0;
				int readpos = 0;
				int bytesleft = min(maxLen*2, searchValueElem->mValue.mLen);

				// special case: zero length value
				if (searchValueElem->mValue.mLen == 0 || searchValueElem->mValue.mData == NULL)
				{
					valueOut[0] = 0;
					*lenOut = 0;
					return gsi_true;
				}

				// Checking length only?
				if (valueOut == NULL)
				{
					*lenOut = searchValueElem->mValue.mLen / 2;

					// note: read position left at this elemtent so next read can record the data
					return gsi_true;
				}

				// 2 characters of hexbyte = 1 value byte
				while(bytesleft > 1)
				{
					sscanf((char*)(&searchValueElem->mValue.mData[readpos]), "%02x", &temp); // sscanf requires a 4 byte dest
					valueOut[writepos] = (gsi_u8)temp; // then we convert to byte, to ensure correct byte order
					readpos += 2;
					writepos += 1;
					bytesleft -= 2;
				}
				if (bytesleft == 1)
				{
					sscanf((char*)(&searchValueElem->mValue.mData[readpos]), "%01x", &temp); // sscanf requires a 4 byte dest
					valueOut[writepos] = (gsi_u8)temp; // then we convert to byte, to ensure correct byte order
					readpos += 1;
					writepos += 1;
					bytesleft -= 1;
				}
				if (lenOut != NULL)
					*lenOut = writepos;

				reader->mValueReadIndex = i; // mark that this element was read
				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlReadChildAsBase64Binary(GSXmlStreamReader stream, const char * matchtag, gsi_u8 valueOut[], int * lenOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;

	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				if(valueOut)
				{
					reader->mValueReadIndex = i;
					if(searchValueElem->mValue.mData)
						B64Decode((char*)searchValueElem->mValue.mData, (char*)valueOut, searchValueElem->mValue.mLen, lenOut, GS_XML_BASE64_ENCODING_TYPE);
					else
						*lenOut = 0;
				}
				else
				{
					if(searchValueElem->mValue.mData)
						*lenOut = B64DecodeLen((const char*)searchValueElem->mValue.mData, GS_XML_BASE64_ENCODING_TYPE);
					else
						*lenOut = 0;
				}
				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlReadChildAsInt    (GSXmlStreamReader stream, const char * matchtag, 
								 int * valueOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;

	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				reader->mValueReadIndex = i;
				if (searchValueElem->mValue.mData == NULL)
					return gsi_false; // invalid type!
				*valueOut = atoi((const char*)searchValueElem->mValue.mData);
				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlReadChildAsInt64(GSXmlStreamReader stream, const char * matchtag, 
							   gsi_i64 * valueOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;

	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				reader->mValueReadIndex = i;
				if (searchValueElem->mValue.mData == NULL)
					return gsi_false; // invalid type!
				*valueOut = gsiStringToInt64((const char*)searchValueElem->mValue.mData);
				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlReadChildAsDateTimeElement    (GSXmlStreamReader stream, const char * matchtag, 
											 time_t * valueOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;	
	struct tm timePtr;


	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				reader->mValueReadIndex = i;
				if (searchValueElem->mValue.mData == NULL)
					return gsi_false; // invalid type!

				// convert the time to from a string to a time struct
				sscanf((const char*)searchValueElem->mValue.mData, "%i-%02d-%02dT%02d:%02d:%02d",
					&timePtr.tm_year, &timePtr.tm_mon, &timePtr.tm_mday,
					&timePtr.tm_hour, &timePtr.tm_min, &timePtr.tm_sec);

				timePtr.tm_year -= 1900;
				timePtr.tm_mon -= 1;
				timePtr.tm_isdst = -1;
				*valueOut = gsiDateToSeconds(&timePtr);

				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsXmlReadChildAsFloat  (GSXmlStreamReader stream, const char * matchtag, 
								 float * valueOut)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchValueElem = NULL;
	int i=0;

	// Do we have a valid value position already?
	if (reader->mValueReadIndex == -1)
		reader->mValueReadIndex = reader->mElemReadIndex; // start at current element

	for (i=(reader->mValueReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchValueElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchValueElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if (gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchValueElem->mName)))
			{
				reader->mValueReadIndex = i;
				if (searchValueElem->mValue.mData == NULL)
					return gsi_false; // invalid type!
				*valueOut = (float)atof((const char*)searchValueElem->mValue.mData);
				return gsi_true;
			}
		}
		// bail if we've reached a higher branch
		if (searchValueElem->mParentIndex < reader->mElemReadIndex)
			return gsi_false;
	}
	return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Compare only the text following the namespace character
static gsi_bool gsiXmlUtilTagMatches(const char * matchtag, GSIXmlString * xmlstr)
{
	const char * matchNoNamespace = NULL;
	GSIXmlString xmlNoNamespace;
	int xmlNamespacePos=0;

	GS_ASSERT(xmlstr != NULL);

	if (matchtag == NULL)
		return gsi_true;
	if (matchtag[strlen(matchtag)-1] == ':')
		return gsi_false; // illegal to end with ':'

	// find post-namespace positions
	matchNoNamespace = strchr(matchtag, ':');
	if (matchNoNamespace == NULL)
		matchNoNamespace = matchtag;

	while(xmlNamespacePos < xmlstr->mLen && xmlstr->mData[xmlNamespacePos] != ':')
		xmlNamespacePos++;
	if (xmlNamespacePos == xmlstr->mLen)
		xmlNamespacePos=0;
	else
		xmlNamespacePos++; // add one more to skip over the ':'
	xmlNoNamespace.mData = xmlstr->mData + xmlNamespacePos;
	xmlNoNamespace.mLen = xmlstr->mLen - xmlNamespacePos;

	// compare strings
	if (0 == strncmp(matchNoNamespace, (const char*)xmlNoNamespace.mData, (size_t)xmlNoNamespace.mLen))
		return gsi_true;
	else
		return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Reads childs as bin-endian hexbinary, then converts to little-endian large int
gsi_bool gsXmlReadChildAsLargeInt(GSXmlStreamReader stream, const char * matchtag, struct gsLargeInt_s * valueOut)
{
	int len = 0;

	// set to zero
	memset(valueOut, 0, sizeof(struct gsLargeInt_s));

	// parse the hexbinary
	if (gsi_is_false(gsXmlReadChildAsHexBinary(stream, matchtag, (gsi_u8*)valueOut->mData, GS_LARGEINT_BINARY_SIZE/8*2, &len)))
		return gsi_false;

	// save off length
	valueOut->mLength = (l_word)(len/GS_LARGEINT_DIGIT_SIZE_BYTES);
	if (len%GS_LARGEINT_DIGIT_SIZE_BYTES != 0)
		valueOut->mLength++;

	// reverse byte order
	if (gsi_is_false(gsLargeIntReverseBytes(valueOut)))
		return gsi_false;
	else
		return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Resets the child read position to the first child of the current element
//
gsi_bool gsXmlResetChildReadPosition(GSXmlStreamReader stream)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	reader->mValueReadIndex = -1; // no current child position means start at first
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Count the number of children with the tag
// If the tag is NULL, count all the children
// Only counts direct children, not grandchildren or lower
int gsXmlCountChildren(GSXmlStreamReader stream, const char * matchtag)
{
	GSIXmlStreamReader * reader = (GSIXmlStreamReader*)stream;
	GSIXmlElement * searchElem = NULL;
	int i=0;
	int count=0;

	for (i=(reader->mElemReadIndex+1); i < ArrayLength(reader->mElementArray); i++)
	{
		searchElem = (GSIXmlElement*)ArrayNth(reader->mElementArray, i);
		if (searchElem->mParentIndex == reader->mElemReadIndex)
		{
			// check match
			if ((matchtag == NULL) || gsi_is_true(gsiXmlUtilTagMatches(matchtag, &searchElem->mName)))
			{
				count++;
			}
		}
		// check if we've reached a higher branch
		//    -- we know this when we've reached an element whose 
		//       parent is above our level in the tree
		else if (searchElem->mParentIndex < reader->mElemReadIndex)
			break;
	}
	return count;
}



