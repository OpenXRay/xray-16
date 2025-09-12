/*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4267) //lines: 1334, 1344, 1400, 1410
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS

#include "ghttpPost.h"
#include "ghttpMain.h"
#include "ghttpConnection.h"
#include "ghttpCommon.h"

#include "../common/gsCrypt.h"
#include "../common/gsSSL.h"
#include "../common/gsXML.h"


// The border between parts in a file send.
///////////////////////////////////////////
#define GHI_MULTIPART_BOUNDARY          "Qr4G823s23d---<<><><<<>--7d118e0536"
#define GHI_MULTIPART_BOUNDARY_BASE     "--" GHI_MULTIPART_BOUNDARY
#define GHI_MULTIPART_BOUNDARY_FIRST    GHI_MULTIPART_BOUNDARY_BASE CRLF
#define GHI_MULTIPART_BOUNDARY_NORMAL   CRLF GHI_MULTIPART_BOUNDARY_BASE CRLF
#define GHI_MULTIPART_BOUNDARY_END      CRLF GHI_MULTIPART_BOUNDARY_BASE "--" CRLF

#define GHI_LEGAL_URLENCODED_CHARS      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_@-.*"
#define GHI_DIGITS                      "0123456789ABCDEF"

// DIME header settings
    // first byte is a combination of VERSION + first/last/chunked
#define GHI_DIME_VERSION         (0x1<<3) // 5th bit (from the left)
#define GHI_DIMEFLAG_FIRSTRECORD (1<<2)
#define GHI_DIMEFLAG_LASTRECORD  (1<<1)
#define GHI_DIMEFLAG_CHUNKED     (1<<0)
    // second byte is combination of TYPE_T and reserved (4bits = 0)
#define GHI_DIMETYPE_T_UNCHANGED (0x0 << 4)
#define GHI_DIMETYPE_T_MEDIA     (0x1 << 4)
#define GHI_DIMETYPE_T_URI       (0x2 << 4)
#define GHI_DIMETYPE_T_UNKNOWN   (0x3 << 4)
#define GHI_DIMETYPE_T_EMPTY     (0x4 << 4) // lengths must be set to 0

//#define GHI_DIME_SOAPID "gsi:soap"
#define GHI_DIME_SOAPID "cid:id0"
#define GHI_DIME_SOAPTYPE "http://schemas.xmlsoap.org/soap/envelope/"

typedef struct GSIDimeHeader
{
	gsi_u8 mVersionAndFlags;
	gsi_u8 mTypeT;
	gsi_u16 mOptionsLength;
	gsi_u16 mIdLength;
	gsi_u16 mTypeLength;
	gsi_u32 mDataLength;
	// gsi_u8 mOptions[mOptionsLength];
	// gsi_u8 mId[mIdLength];
	// gsi_u8 mType[mTypeLength];
	// gsi_u8 mData[mDataLength];
} GHIDimeHeader;

// POST TYPES.
//////////////
typedef enum
{
	GHIString,      // A regular string.
	GHIFileDisk,    // A file from disk.
	GHIFileMemory,  // A file from memory.
	GHIXmlData      // XML Soap. (long string)
} GHIPostDataType;

// POST OBJECT.
///////////////
typedef struct GHIPost
{
	DArray data;
	ghttpPostCallback callback;
	void * param;
	GHTTPBool hasFiles;
	GHTTPBool hasSoap;
	GHTTPBool useDIME;
	GHTTPBool autoFree;
} GHIPost;

// POST DATA.
/////////////
typedef struct GHIPostStringData
{
	char * string;
	int len;
	GHTTPBool invalidChars;
	int extendedChars;
} GHIPostStringData;

typedef struct GHIPostFileDiskData
{
	char * filename;
	char * reportFilename;
	char * contentType;
} GHIPostFileDiskData;

typedef struct GHIPostFileMemoryData
{
	const char * buffer;
	int len;
	char * reportFilename;
	char * contentType;
} GHIPostFileMemoryData;

typedef struct GHIPostXmlData
{
	GSXmlStreamWriter xml;
} GHIPostXmlData;

typedef struct GHIPostData
{
	GHIPostDataType type;
	char * name;
	union
	{
		GHIPostStringData string;
		GHIPostFileDiskData fileDisk;
		GHIPostFileMemoryData fileMemory;
		GHIPostXmlData xml;
	} data;
} GHIPostData;

// POST STATE.
//////////////
//typedef struct GHIPostStringState
//{
//} GHIPostStringState;

typedef struct GHIPostFileDiskState
{
	FILE * file;
	long len;
} GHIPostFileDiskState;

//typedef struct GHIPostFileMemoryState
//{
//} GHIPostFileMemoryState;


//typedef struct GHIPostSoapState
//{
//} GHIPostSoapState;

typedef struct GHIPostState
{
	GHIPostData * data;
	int pos;
	union
	{
		//GHIPostStringState string;
		GHIPostFileDiskState fileDisk;
		//GHIPostFileMemoryState fileMemory;
		//GHIPostSoapState soap;
	} state;
} GHIPostState;

// FUNCTIONS.
/////////////
static void ghiPostDataFree
(
	void * elem
)
{
	GHIPostData * data = (GHIPostData *)elem;

	// Free the name.
	/////////////////
	if (data->type != GHIXmlData)
		gsifree(data->name);

	// Free based on type.
	//////////////////////
	if(data->type == GHIString)
	{
		// Free the string string.
		/////////////////////////
		gsifree(data->data.string.string);
	}
	else if(data->type == GHIFileDisk)
	{
		// Free the strings.
		////////////////////
		gsifree(data->data.fileDisk.filename);
		gsifree(data->data.fileDisk.reportFilename);
		gsifree(data->data.fileDisk.contentType);
	}
	else if(data->type == GHIFileMemory)
	{
		// Free the strings.
		////////////////////
		gsifree(data->data.fileMemory.reportFilename);
		gsifree(data->data.fileMemory.contentType);
	}
	else if(data->type == GHIXmlData)
	{
		gsXmlFreeWriter(data->data.xml.xml);
	}
	else
	{
		// The type didn't match any known types.
		/////////////////////////////////////////
		assert(0);
	}
}

GHTTPPost ghiNewPost
(
	void
)
{
	GHIPost * post;

	// Allocate the post object.
	////////////////////////////
	post = (GHIPost *)gsimalloc(sizeof(GHIPost));
	if(!post)
		return NULL;

	// Initialize it.
	/////////////////
	memset(post, 0, sizeof(GHIPost));
	post->autoFree = GHTTPTrue;

	// Create the array of data objects.
	////////////////////////////////////
	post->data = ArrayNew(sizeof(GHIPostData), 0, ghiPostDataFree);
	if(!post->data)
	{
		gsifree(post);
		return NULL;
	}

	return (GHTTPPost)post;
}

void ghiPostSetAutoFree
(
	GHTTPPost _post,
	GHTTPBool autoFree
)
{
	GHIPost * post = (GHIPost *)_post;

	post->autoFree = autoFree;
}

GHTTPBool ghiIsPostAutoFree
(
	GHTTPPost _post
)
{
	GHIPost * post = (GHIPost *)_post;

	return post->autoFree;
}

void ghiFreePost
(
	GHTTPPost _post
)
{
	GHIPost * post = (GHIPost *)_post;

	// Free the array of data objects.
	//////////////////////////////////
	ArrayFree(post->data);

	// Free the post object.
	////////////////////////
	gsifree(post);
}

GHTTPBool ghiPostAddString
(
	GHTTPPost _post,
	const char * name,
	const char * string
)
{
	GHIPost * post = (GHIPost *)_post;
	GHIPostData data;
	int len;
	int rcode;

	// Copy the strings.
	////////////////////
	name = goastrdup(name);
	string = goastrdup(string);
	if(!name || !string)
	{
		gsifree((char *)name);
		gsifree((char *)string);
		return GHTTPFalse;
	}

	// Set the data.
	////////////////
	memset(&data, 0, sizeof(GHIPostData));
	data.type = GHIString;
	data.name = (char *)name;
	data.data.string.string = (char *)string;
	len = (int)strlen(string);
	data.data.string.len = len;
	data.data.string.invalidChars = GHTTPFalse;

	// Are there any invalid characters?
	////////////////////////////////////
	rcode = (int)strspn(string, GHI_LEGAL_URLENCODED_CHARS);
	if(rcode != len)
	{
		int i;
		int count = 0;

		data.data.string.invalidChars = GHTTPTrue;

		// Count the number, not including spaces.
		//////////////////////////////////////////
		for(i = 0 ; string[i] ; i++)
			if(!strchr(GHI_LEGAL_URLENCODED_CHARS, string[i]) && (string[i] != ' '))
				count++;

		data.data.string.extendedChars = count;
	}

	// Add it.
	//////////
	ArrayAppend(post->data, &data);

	return GHTTPTrue;
}

GHTTPBool ghiPostAddFileFromDisk
(
	GHTTPPost _post,
	const char * name,
	const char * filename,
	const char * reportFilename,
	const char * contentType
)
{
	GHIPost * post = (GHIPost *)_post;
	GHIPostData data;

	// Copy the strings.
	////////////////////
	name = goastrdup(name);
	filename = goastrdup(filename);
	reportFilename = goastrdup(reportFilename);
	contentType = goastrdup(contentType);
	if(!name || !filename || !reportFilename || !contentType)
	{
		gsifree((char *)name);
		gsifree((char *)filename);
		gsifree((char *)reportFilename);
		gsifree((char *)contentType);
		return GHTTPFalse;
	}

	// Set the data.
	////////////////
	memset(&data, 0, sizeof(GHIPostData));
	data.type = GHIFileDisk;
	data.name = (char *)name;
	data.data.fileDisk.filename = (char *)filename;
	data.data.fileDisk.reportFilename = (char *)reportFilename;
	data.data.fileDisk.contentType = (char *)contentType;

	// Add it.
	//////////
	ArrayAppend(post->data, &data);

	// We have files.
	/////////////////
	post->hasFiles = GHTTPTrue;

	// if we have both soap and a file we MUST use DIME
	if (post->hasSoap == GHTTPTrue)
		post->useDIME = GHTTPTrue;

	return GHTTPTrue;
}

GHTTPBool ghiPostAddFileFromMemory
(
	GHTTPPost _post,
	const char * name,
	const char * buffer,
	int bufferLen,
	const char * reportFilename,
	const char * contentType
)
{
	GHIPost * post = (GHIPost *)_post;
	GHIPostData data;

	// Copy the strings.
	////////////////////
	name = goastrdup(name);
	reportFilename = goastrdup(reportFilename);
	contentType = goastrdup(contentType);
	if(!name || !reportFilename || !contentType)
	{
		gsifree((char *)name);
		gsifree((char *)reportFilename);
		gsifree((char *)contentType);
		return GHTTPFalse;
	}

	// Set it.
	//////////
	memset(&data, 0, sizeof(GHIPostData));
	data.type = GHIFileMemory;
	data.name = (char *)name;
	data.data.fileMemory.buffer = (char *)buffer;
	data.data.fileMemory.len = bufferLen;
	data.data.fileMemory.reportFilename = (char *)reportFilename;
	data.data.fileMemory.contentType = (char *)contentType;

	// Add it.
	//////////
	ArrayAppend(post->data, &data);

	// We have a file.
	//////////////////
	post->hasFiles = GHTTPTrue;

	// if we have both soap and a file we MUST use DIME
	if (post->hasSoap == GHTTPTrue)
		post->useDIME = GHTTPTrue;

	return GHTTPTrue;
}

GHTTPBool ghiPostAddXml
(
	GHTTPPost _post,
	GSXmlStreamWriter xml
)
{
	GHIPostData data;
	//unsigned int rcode = 0;

	GHIPost * post = (GHIPost *)_post;

	data.type = GHIXmlData;
	data.data.xml.xml = xml;
	ArrayAppend(post->data, &data);
	post->hasSoap = GHTTPTrue;

	// if we have both soap and a file we MUST use DIME
	if (post->hasFiles == GHTTPTrue)
		post->useDIME = GHTTPTrue;

	return GHTTPTrue;
}

void ghiPostSetCallback
(
	GHTTPPost _post,
	ghttpPostCallback callback,
	void * param
)
{
	GHIPost * post = (GHIPost *)_post;

	// Set the callback and param.
	//////////////////////////////
	post->callback = callback;
	post->param = param;
}

const char * ghiPostGetContentType
(
	struct GHIConnection * connection
)
{
	GHIPost * post = connection->post;

	assert(post);
	if(!post)
		return "";

	// Report content-type based on if we are sending files or not.
	///////////////////////////////////////////////////////////////
	if(post->useDIME)
		return ("application/dime");
	else if (post->hasFiles)
	{
		GS_ASSERT(!post->hasSoap);
		return ("multipart/form-data; boundary=" GHI_MULTIPART_BOUNDARY);
	}
	else if (post->hasSoap)
	{
		GS_ASSERT(!post->hasFiles);
		return ("text/xml");
	}
	else
	{
		GS_ASSERT(!post->hasSoap);
		GS_ASSERT(!post->hasFiles);
		return "application/x-www-form-urlencoded";
	}
}

static int ghiPostGetNoFilesContentLength
(
	struct GHIConnection * connection
)
{
	GHIPost * post = connection->post;
	GHIPostData * data;
	int i;
	int num;
	int total = 0;
	int foundSoapAlready = 0;

	num = ArrayLength(post->data);
	if(!num)
		return 0;

	for(i = 0 ; i < num ; i++)
	{
		data = (GHIPostData *)ArrayNth(post->data, i);

		GS_ASSERT(data->type == GHIString || data->type == GHIXmlData);

		if (data->type == GHIString)
		{
			total += (int)strlen(data->name);
			total += data->data.string.len;
			total += (data->data.string.extendedChars * 2);
			total++;  // '='
		}
		else if (data->type == GHIXmlData)
		{
			GS_ASSERT(foundSoapAlready == 0); // only support one soap object per request
			foundSoapAlready = 1;
			total += gsXmlWriterGetDataLength(data->data.xml.xml);
		}
	}

	total += (num - 1);  // '&'

	GSI_UNUSED(foundSoapAlready);
	return total;
}

static int ghiPostGetHasFilesContentLength
(
	struct GHIConnection * connection
)
{
	GHIPost * post = connection->post;
	GHIPostData * data;
	int i;
	int num;
	int total = 0;
	int foundSoapAlready = 0;
	static int boundaryLen;
	static int stringBaseLen;
	static int fileBaseLen;
	static int endLen;
	static int xmlBaseLen;
	
	if(!boundaryLen)
	{
		if (post->useDIME)
		{
			GS_ASSERT(post->hasSoap);
			GS_ASSERT(post->hasFiles);
			boundaryLen = sizeof(GHIDimeHeader);
			stringBaseLen = boundaryLen;
			fileBaseLen = boundaryLen;
			xmlBaseLen = boundaryLen;
			endLen = 0;
		}
		else
		{
			GS_ASSERT(!post->hasSoap);
			boundaryLen = (int)strlen(GHI_MULTIPART_BOUNDARY_BASE);
			stringBaseLen = (boundaryLen + 47);  // + name + string
			fileBaseLen = (boundaryLen + 76);  // + name + filename + content-type + file
			xmlBaseLen = 0; // no boundaries for text/xml type soap
			endLen = (boundaryLen + 4);
		}
	}

	num = ArrayLength(post->data);

	for(i = 0 ; i < num ; i++)
	{
		data = (GHIPostData *)ArrayNth(post->data, i);

		if(data->type == GHIString)
		{
			total += stringBaseLen;
			total += (int)strlen(data->name);
			total += data->data.string.len;
		}
		else if(data->type == GHIFileDisk)
		{
			GHIPostState * state;

			total += fileBaseLen;
			total += (int)strlen(data->name);
			total += (int)strlen(data->data.fileDisk.contentType);
			state = (GHIPostState *)ArrayNth(connection->postingState.states, i);
			assert(state);
			total += (int)state->state.fileDisk.len;

			if (!post->useDIME)
				total += (int)strlen(data->data.fileDisk.reportFilename);

			if (post->useDIME)
			{
				// have to include padding bytes!
				int padBytes = 0;

				padBytes = 4-(int)strlen(data->name)%4;
				if (padBytes != 4)
					total += padBytes;
				padBytes = 4-(int)strlen(data->data.fileDisk.contentType)%4;
				if (padBytes != 4)
					total += padBytes;
				padBytes = 4-(int)state->state.fileDisk.len%4;
				if (padBytes != 4)
					total += padBytes;
			}
		}
		else if(data->type == GHIFileMemory)
		{
			total += fileBaseLen;
			total += (int)strlen(data->name);
			total += (int)strlen(data->data.fileMemory.contentType);
			total += data->data.fileMemory.len;

			if (!post->useDIME)
				total += (int)strlen(data->data.fileMemory.reportFilename);

			if (post->useDIME)
			{
				// have to include padding bytes!
				int padBytes = 0;

				padBytes = 4-(int)strlen(data->name)%4;
				if (padBytes != 4)
					total += padBytes;
				padBytes = 4-(int)strlen(data->data.fileMemory.contentType)%4;
				if (padBytes != 4)
					total += padBytes;
				padBytes = 4-(int)data->data.fileMemory.len%4;
				if (padBytes != 4)
					total += padBytes;
			}
		}
		else if(data->type == GHIXmlData)
		{
			int padBytes = 0;

			GS_ASSERT(foundSoapAlready == 0); // only one soap envelope per request
			GS_ASSERT(post->useDIME); // soap+file = use DIME
			foundSoapAlready = 1;
			total += xmlBaseLen;
			total += gsXmlWriterGetDataLength(data->data.xml.xml);

			// have to include padding bytes!
			padBytes = 4-(int)gsXmlWriterGetDataLength(data->data.xml.xml)%4;
			if (padBytes != 4)
				total += padBytes;
			total += (int)strlen(GHI_DIME_SOAPID);
			padBytes = 4-(int)strlen(GHI_DIME_SOAPID)%4;
			if (padBytes != 4)
				total += padBytes;
			total += (int)strlen(GHI_DIME_SOAPTYPE);
			padBytes = 4-(int)strlen(GHI_DIME_SOAPTYPE)%4;
			if (padBytes != 4)
				total += padBytes;
		}
		else
		{
			assert(0);
			return 0;
		}
	}

	// Add the end.
	///////////////
	total += endLen;

	GSI_UNUSED(foundSoapAlready);
	return total;
}

static int ghiPostGetContentLength
(
	struct GHIConnection * connection
)
{
	GHIPost * post = connection->post;

	assert(post);
	if(!post)
		return 0;

	if(post->hasFiles)
		return ghiPostGetHasFilesContentLength(connection);

	return ghiPostGetNoFilesContentLength(connection);
}

static GHTTPBool ghiPostStateInit
(
	GHIPostState * state
)
{
	GHIPostDataType type;

	// The type.
	////////////
	type = state->data->type;

	// Set the position to sending header.
	//////////////////////////////////////
	state->pos = -1;

	// Init based on type.
	//////////////////////
	if(type == GHIString)
	{
	}
	else if(type == GHIFileDisk)
	{
		// Open the file.
		/////////////////
#ifndef NOFILE
		state->state.fileDisk.file = fopen(state->data->data.fileDisk.filename, "rb");
#endif
		if(!state->state.fileDisk.file)
			return GHTTPFalse;

		// Get the file length.
		///////////////////////
		if(fseek(state->state.fileDisk.file, 0, SEEK_END) != 0)
			return GHTTPFalse;
		state->state.fileDisk.len = ftell(state->state.fileDisk.file);
		if(state->state.fileDisk.len == EOF)
			return GHTTPFalse;
		rewind(state->state.fileDisk.file);
	}
	else if(type == GHIFileMemory)
	{
	}
	else if(type == GHIXmlData)
	{
	}
	else
	{
		// The type didn't match any known types.
		/////////////////////////////////////////
		assert(0);

		return GHTTPFalse;
	}

	return GHTTPTrue;
}

static void ghiPostStateCleanup
(
	GHIPostState * state
)
{
	GHIPostDataType type;

	// The type.
	////////////
	type = state->data->type;

	// Init based on type.
	//////////////////////
	if(type == GHIString)
	{
	}
	else if(type == GHIFileDisk)
	{
		if(state->state.fileDisk.file)
			fclose(state->state.fileDisk.file);
		state->state.fileDisk.file = NULL;
	}
	else if(type == GHIFileMemory)
	{
	}
	else if(type == GHIXmlData)
	{
	}
	else
	{
		// The type didn't match any known types.
		/////////////////////////////////////////
		assert(0);
	}
}

GHTTPBool ghiPostInitState
(
	struct GHIConnection * connection
)
{
	int i;
	int len;
	GHIPostData * data;
	GHIPostState state;
	GHIPostState * pState;

	assert(connection->post);
	if(!connection->post)
		return GHTTPFalse;

	// Create an array for the states.
	//////////////////////////////////
	connection->postingState.index = 0;
	connection->postingState.bytesPosted = 0;
	connection->postingState.totalBytes = 0;
	connection->postingState.completed = GHTTPFalse;
	connection->postingState.callback = connection->post->callback;
	connection->postingState.param = connection->post->param;
	len = ArrayLength(connection->post->data);
	connection->postingState.states = ArrayNew(sizeof(GHIPostState), len, NULL);
	if(!connection->postingState.states)
		return GHTTPFalse;

	// Setup all the states.
	////////////////////////
	for(i = 0 ; i < len ; i++)
	{
		// Get the data object for this index.
		//////////////////////////////////////
		data = (GHIPostData *)ArrayNth(connection->post->data, i);

		// Initialize the state's members.
		//////////////////////////////////
		memset(&state, 0, sizeof(GHIPostState));
		state.data = data;

		// Call the init function.
		//////////////////////////
		if(!ghiPostStateInit(&state))
		{
			// We need to cleanup everything we just initialized.
			/////////////////////////////////////////////////////
			for(i-- ; i >= 0 ; i--)
			{
				pState = (GHIPostState *)ArrayNth(connection->postingState.states, i);
				ghiPostStateCleanup(pState);
			}

			// Free the array.
			//////////////////
			ArrayFree(connection->postingState.states);
			connection->postingState.states = NULL;

			return GHTTPFalse;
		}

		// Add it to the array.
		///////////////////////
		ArrayAppend(connection->postingState.states, &state);
	}

	// If this asserts, there aren't the same number of state objects as data objects.
	// There should be a 1-to-1 mapping between data and states.
	//////////////////////////////////////////////////////////////////////////////////
	assert(ArrayLength(connection->post->data) == ArrayLength(connection->postingState.states));

	// Get the total number of bytes.
	/////////////////////////////////
	connection->postingState.totalBytes = ghiPostGetContentLength(connection);

	// Wait for continue before posting.
	//	  -- Enabled for Soap messages only
	//    -- Disabled for all other content because many web servers do not support it
	//////////////////////////////////////////////////////
	if (connection->post->hasSoap == GHTTPTrue)
		connection->postingState.waitPostContinue = GHTTPTrue;
	else
		connection->postingState.waitPostContinue = GHTTPFalse;

	return GHTTPTrue;
}

void ghiPostCleanupState
(
	struct GHIConnection * connection
)
{
	int i;
	int len;
	GHIPostState * state;

	// Loop through and call the cleanup function.
	//////////////////////////////////////////////
	if(connection->postingState.states)
	{
		len = ArrayLength(connection->postingState.states);
		for(i = 0 ; i < len ; i++)
		{
			state = (GHIPostState *)ArrayNth(connection->postingState.states, i);
			ghiPostStateCleanup(state);
		}

		// Free the array.
		//////////////////
		ArrayFree(connection->postingState.states);
		connection->postingState.states = NULL;
	}

	// Free the post.
	/////////////////
	if(connection->post && connection->post->autoFree)
	{
		ghiFreePost(connection->post);
		connection->post = NULL;
	}
}

static GHIPostingResult ghiPostStringStateDoPosting
(
	GHIPostState * state,
	GHIConnection * connection
)
{
	//GHTTPBool result;
	
	assert(state->pos >= 0);

	// Is this an empty string?
	///////////////////////////
	if(state->data->data.string.len == 0)
		return GHIPostingDone;

	assert(state->pos < state->data->data.string.len);

	// If we're doing a simple post, we need to fix invalid characters.
	//   - only applies to simple posts
	///////////////////////////////////////////////////////////////////
	if(!connection->post->hasFiles && !connection->post->hasSoap && state->data->data.string.invalidChars)
	{
		int i;
		int c;
		const char * string = state->data->data.string.string;
		char hex[4] = "%00";
		GHIBuffer *writeBuffer;

		// When encrypting, we need space for two copies
		if (connection->encryptor.mEngine == GHTTPEncryptionEngine_None)
			writeBuffer = &connection->sendBuffer; 
		else
			writeBuffer = &connection->encodeBuffer;

		// This could probably be done a lot better.
		////////////////////////////////////////////
		for(i = 0 ; (c = string[i]) != 0 ; i++)
		{
			if(strchr(GHI_LEGAL_URLENCODED_CHARS, c))
			{
				// Legal.
				/////////
				//result = ghiAppendCharToBuffer(writeBuffer, c);
				ghiAppendCharToBuffer(writeBuffer, c);
			}
			else if(c == ' ')
			{
				// Space.
				/////////
				//result = ghiAppendCharToBuffer(writeBuffer, '+');
				ghiAppendCharToBuffer(writeBuffer, '+');
			}
			else
			{
				// To hex.
				//////////
				assert((c / 16) < 16);
				hex[1] = GHI_DIGITS[c / 16];
				hex[2] = GHI_DIGITS[c % 16];
				//result = ghiAppendDataToBuffer(writeBuffer, hex, 3);
				ghiAppendDataToBuffer(writeBuffer, hex, 3);
			}
		}
	}
	else
	{
		// copy the string as-is, encrypting if necessary
		GHITrySendResult result = ghiTrySendThenBuffer(connection, 
			state->data->data.string.string, state->data->data.string.len);
		if (result == GHITrySendError)
			return GHIPostingError;
		else
			return GHIPostingDone;
	}

	// Send the URL fixed string
	////////////////////////////
	if (connection->encryptor.mEngine == GHTTPEncryptionEngine_None)
	{
		// The URL fixed string was written to the send buffer, so send it!
		if (!ghiSendBufferedData(connection))
			return GHIPostingError;

		if (connection->sendBuffer.pos == connection->sendBuffer.len)
			ghiResetBuffer(&connection->sendBuffer);
		return GHIPostingDone;
	}
	else
	{
		// SSL data is in the "to be encrypted" buffer, so wait until
		// we have the full MIME form before encrypting (for efficiency)
		return GHIPostingDone;
	}
}

static GHIPostingResult ghiPostXmlStateDoPosting
(
	GHIPostState * state,
	GHIConnection * connection
)
{
	GSXmlStreamWriter xml = state->data->data.xml.xml;
	char pad[3] = { '\0', '\0', '\0' };
	int padlen = 0;
	
	// make sure state is valid
	GS_ASSERT(state->pos >= 0);
	GS_ASSERT(connection->post != NULL);

	// when using a DIME, we have to pad to multiple of 4
	if (connection->post->useDIME)
	{
		padlen = 4-(gsXmlWriterGetDataLength(xml)%4);
		if (padlen == 4)
			padlen = 0;
	}

	if (connection->encryptor.mEngine != GHTTPEncryptionEngine_None &&
		connection->encryptor.mEncryptOnBuffer == GHTTPTrue)
	{
		// Copy to encode buffer before encrypting
		GS_ASSERT(connection->encodeBuffer.len >= 0); // there must be a header for this soap data!
		if (!ghiAppendDataToBuffer(&connection->encodeBuffer, gsXmlWriterGetData(xml), gsXmlWriterGetDataLength(xml)) ||
			!ghiAppendDataToBuffer(&connection->encodeBuffer, pad, padlen) ||
			!ghiEncryptDataToBuffer(&connection->sendBuffer, connection->encodeBuffer.data, connection->encodeBuffer.len)
			)
		{
			return GHIPostingError;
		}

		// Clear out our temporary buffer
		ghiResetBuffer(&connection->encodeBuffer);

		// Send what we can now
		if (GHTTPFalse == ghiSendBufferedData(connection))
			return GHIPostingError;

		// is there more to send?
		if (connection->sendBuffer.pos == connection->sendBuffer.len)
			ghiResetBuffer(&connection->sendBuffer);

		return GHIPostingDone;
	}
	else
	{
		GHITrySendResult result;

		// plain text - send immediately
		result = ghiTrySendThenBuffer(connection, gsXmlWriterGetData(xml), gsXmlWriterGetDataLength(xml));
		if (result == GHITrySendError)
			return GHIPostingError;
		result = ghiTrySendThenBuffer(connection, pad, padlen);
		if (result == GHITrySendError)
			return GHIPostingError;
		return GHIPostingDone;
	}
}

static GHIPostingResult ghiPostFileDiskStateDoPosting
(
	GHIPostState * state,
	GHIConnection * connection
)
{
	char buffer[4096];
	int len;
	GHITrySendResult result;

	assert(state->pos >= 0);
	assert(state->pos < state->state.fileDisk.len);
	assert(state->pos == (int)ftell(state->state.fileDisk.file));

	// Loop while data is being sent.
	/////////////////////////////////
	do
	{
		// Read some data from the file.
		////////////////////////////////
		len = (int)fread(buffer, 1, sizeof(buffer), state->state.fileDisk.file);
		if(len <= 0)
		{
			connection->completed = GHTTPTrue;
			connection->result = GHTTPFileReadFailed;
			return GHIPostingError;
		}

		// Update our position.
		///////////////////////
		state->pos += len;

		// Check for too much.
		//////////////////////
		if(state->pos > state->state.fileDisk.len)
		{
			connection->completed = GHTTPTrue;
			connection->result = GHTTPFileReadFailed;
			return GHIPostingError;
		}

		// Send.
		////////
		result = ghiTrySendThenBuffer(connection, buffer, len);
		if(result == GHITrySendError)
			return GHIPostingError;

		// Check if we've handled everything.
		/////////////////////////////////////
		if(state->pos == state->state.fileDisk.len)
		{
			// when using a DIME, we have to pad to multiple of 4
			if (connection->post->useDIME)
			{
				char pad[3] = { '\0', '\0', '\0' };
				int padlen = 4-state->state.fileDisk.len%4;
				if (padlen != 4 && padlen > 0)
				{
					if (GHITrySendError == ghiTrySendThenBuffer(connection, pad, padlen))
						return GHIPostingError;
				}
			}
			return GHIPostingDone;
		}
	}
	while(result == GHITrySendSent);

	return GHIPostingPosting;
}

static GHIPostingResult ghiPostFileMemoryStateDoPosting
(
	GHIPostState * state,
	GHIConnection * connection
)
{
	int rcode;
	int len;

	assert(state->pos >= 0);

	// Is this an empty file?
	/////////////////////////
	if(state->data->data.fileMemory.len == 0)
		return GHIPostingDone;

	assert(state->pos < state->data->data.fileMemory.len);

	// Send what we can.
	////////////////////
	if (connection->encryptor.mEngine == GHTTPEncryptionEngine_None)
	{
		// Plain text: Send directly from memory
		do
		{
			len = (state->data->data.fileMemory.len - state->pos);
			rcode = ghiDoSend(connection, state->data->data.fileMemory.buffer + state->pos, len);
			if(gsiSocketIsError(rcode))
				return GHIPostingError;

			// Update the pos.
			//////////////////
			state->pos += rcode;

			// Did we send it all?
			//////////////////////
			if(state->data->data.fileMemory.len == state->pos)
			{
				// when using a DIME, we have to pad to multiple of 4
				if (connection->post->useDIME)
				{
					char pad[3] = { '\0', '\0', '\0' };
					int padlen = 4-state->data->data.fileMemory.len%4;
					if (padlen != 4 && padlen > 0)
					{
						if (GHITrySendError == ghiTrySendThenBuffer(connection, pad, padlen))
							return GHIPostingError;
					}
				}
				return GHIPostingDone;
			}
		}
		while(rcode);
		return GHIPostingPosting; // (rcode == 0) ?
	}
	else
	{
		// Encrypted: can't avoid the copy due to encryption+MAC
		GHITrySendResult result;
		do 
		{
			len = (state->data->data.fileMemory.len - state->pos);
			len = min(len, GS_SSL_MAX_CONTENTLENGTH);
			result = ghiTrySendThenBuffer(connection, state->data->data.fileMemory.buffer + state->pos, len);
			if (result == GHITrySendError)
				return GHIPostingError;
			
			// Update the pos.
			//////////////////
			state->pos += len;

			// Did we send it all?
			//////////////////////
			if(state->data->data.fileMemory.len == state->pos)
			{
				// when using a DIME, we have to pad to multiple of 4
				if (connection->post->useDIME)
				{
					char pad[3] = { '\0', '\0', '\0' };
					int padlen = 4-state->data->data.fileMemory.len%4;
					if (padlen != 4 && padlen > 0)
					{
						if (GHITrySendError == ghiTrySendThenBuffer(connection, pad, padlen))
							return GHIPostingError;
					}
				}
				return GHIPostingDone;
			}
		} 
		while(result == GHITrySendSent);
		return GHIPostingPosting;
	}
}

static GHIPostingResult ghiPostStateDoPosting
(
	GHIPostState * state,
	GHIConnection * connection,
	GHTTPBool first,
	GHTTPBool last
)
{
	int len = 0;
	GHITrySendResult result;

	// Check for sending the header.
	////////////////////////////////
	if(state->pos == -1)
	{
		char buffer[2048];
		
		// Bump up the position so we only send the header once.
		////////////////////////////////////////////////////////
		state->pos = 0;

		// Check if this is a simple post.
		//////////////////////////////////
		if(!connection->post->hasFiles && !connection->post->hasSoap)
		{
			// Simple post only supports strings.
			/////////////////////////////////////
			assert(state->data->type == GHIString);

			// Format the header.
			/////////////////////
			if(first)
				sprintf(buffer, "%s=", state->data->name);
			else
				sprintf(buffer, "&%s=", state->data->name);
		}
		else
		{
			// Format the header based on string or file.
			/////////////////////////////////////////////
			if(state->data->type == GHIString)
			{
				sprintf(buffer,
					"%s"
					"Content-Disposition: form-data; "
					"name=\"%s\"" CRLF
					CRLF,
					first?GHI_MULTIPART_BOUNDARY_FIRST:GHI_MULTIPART_BOUNDARY_NORMAL,
					state->data->name);
			}
			else if(state->data->type == GHIXmlData)
			{
				if (connection->post->useDIME)
				{
					// use DIME header
					//    Copy from a temp struct to circumvent alignment issues
					int writePos = 0;
					int padBytes = 0;
					GHIDimeHeader header;

					header.mVersionAndFlags = GHI_DIME_VERSION;
					if (first)
						header.mVersionAndFlags |= GHI_DIMEFLAG_FIRSTRECORD;
					if (last)
						header.mVersionAndFlags |= GHI_DIMEFLAG_LASTRECORD;
					header.mTypeT = GHI_DIMETYPE_T_URI;
					header.mOptionsLength = 0;
					header.mIdLength = htons((short)strlen(GHI_DIME_SOAPID));
					header.mTypeLength = htons((short)strlen(GHI_DIME_SOAPTYPE));
					header.mDataLength = htonl(gsXmlWriterGetDataLength(state->data->data.xml.xml));

					memcpy(&buffer[writePos], &header, sizeof(GHIDimeHeader));
					writePos += sizeof(GHIDimeHeader);

					// id
					strcpy(&buffer[writePos], GHI_DIME_SOAPID);
					writePos += strlen(GHI_DIME_SOAPID);
					padBytes = (int)(4-strlen(GHI_DIME_SOAPID)%4);
					if (padBytes != 4)
					{
						while(padBytes-- > 0)
							buffer[writePos++] = '\0';
					}

					// type
					strcpy(&buffer[writePos], GHI_DIME_SOAPTYPE);
					writePos += strlen(GHI_DIME_SOAPTYPE);
					padBytes = (int)(4-strlen(GHI_DIME_SOAPTYPE)%4);
					if (padBytes != 4)
					{
						while(padBytes-- > 0)
							buffer[writePos++] = '\0';
					}

					len = writePos;
				}
				else
					buffer[0] = '\0';
			}
			else if((state->data->type == GHIFileDisk) || (state->data->type == GHIFileMemory))
			{
				const char * filename;
				const char * contentType;
				int filelen;

				if(state->data->type == GHIFileDisk)
				{
					filelen = state->state.fileDisk.len;
					filename = state->data->data.fileDisk.reportFilename;
					contentType = state->data->data.fileDisk.contentType;
				}
				else
				{
					filelen = state->data->data.fileMemory.len;
					filename = state->data->data.fileMemory.reportFilename;
					contentType = state->data->data.fileMemory.contentType;
				}

				if (connection->post->useDIME)
				{
					// use DIME header
					//    Copy from a temp struct to circumvent alignment issues
					int writePos = 0;
					int padBytes = 0;
					GHIDimeHeader header;

					header.mVersionAndFlags = GHI_DIME_VERSION;
					if (first)
						header.mVersionAndFlags |= GHI_DIMEFLAG_FIRSTRECORD;
					if (last)
						header.mVersionAndFlags |= GHI_DIMEFLAG_LASTRECORD;
					header.mTypeT = GHI_DIMETYPE_T_MEDIA;
					header.mOptionsLength = 0;
					header.mIdLength = htons((short)strlen(state->data->name));
					header.mTypeLength = htons((short)strlen(contentType));
					header.mDataLength = htonl(filelen);

					memcpy(&buffer[writePos], &header, sizeof(GHIDimeHeader));
					writePos += sizeof(GHIDimeHeader);

					// id
					strcpy(&buffer[writePos], state->data->name);
					writePos += strlen(state->data->name);
					padBytes = (int)(4-strlen(state->data->name)%4);
					if (padBytes != 4)
					{
						while(padBytes-- > 0)
							buffer[writePos++] = '\0';
					}

					// type
					strcpy(&buffer[writePos], contentType);
					writePos += strlen(contentType);
					padBytes = (int)(4-strlen(contentType)%4);
					if (padBytes != 4)
					{
						while(padBytes-- > 0)
							buffer[writePos++] = '\0';
					}

					len = writePos;
				}
				else
				{
					// use MIME header
					sprintf(buffer,
						"%s"
						"Content-Disposition: form-data; "
						"name=\"%s\"; "
						"filename=\"%s\"" CRLF
						"Content-Type: %s" CRLF CRLF,
						first?GHI_MULTIPART_BOUNDARY_FIRST:GHI_MULTIPART_BOUNDARY_NORMAL,
						state->data->name,
						filename,
						contentType);
				}
			}
			else
			{
				assert(0);
			}
		}

		// SSL: encrypt and send
		if (connection->encryptor.mEngine != GHTTPEncryptionEngine_None &&
			connection->encryptor.mEncryptOnBuffer == GHTTPTrue)
		{
			if (len == 0)
				len = (int)strlen(buffer);
			if (GHTTPFalse == ghiEncryptDataToBuffer(&connection->sendBuffer, buffer, len))
				return GHIPostingError;
			if (GHTTPFalse == ghiSendBufferedData(connection))
				return GHIPostingError;

			// any data remaining?
			if (connection->sendBuffer.pos < connection->sendBuffer.len)
				return GHIPostingPosting;

			// We sent everything, reset the send buffer to conserve space
			ghiResetBuffer(&connection->sendBuffer);
		}
		// If sending plain text, send right away
		else
		{
			// Try sending. (the one-time header)
			/////////////////////////////////////
			if (len == 0)
				len = (int)strlen(buffer);
			result = ghiTrySendThenBuffer(connection, buffer, len);
			if(result == GHITrySendError)
				return GHIPostingError;

			// If it was buffered, don't try anymore.
			/////////////////////////////////////////
			if(result == GHITrySendBuffered)
				return GHIPostingPosting;

			// We sent everything, reset the send buffer to conserve space
			ghiResetBuffer(&connection->sendBuffer);
		}
	}

	// Post based on type.
	//////////////////////
	if(state->data->type == GHIString)
		return ghiPostStringStateDoPosting(state, connection);

	if(state->data->type == GHIXmlData)
		return ghiPostXmlStateDoPosting(state, connection);

	if(state->data->type == GHIFileDisk)
		return ghiPostFileDiskStateDoPosting(state, connection);

	assert(state->data->type == GHIFileMemory);
	return ghiPostFileMemoryStateDoPosting(state, connection);
}

GHIPostingResult ghiPostDoPosting
(
	struct GHIConnection * connection
)
{
	GHIPostingResult postingResult;
	GHITrySendResult trySendResult;
	GHIPostingState * postingState;
	GHIPostState * postState;
	int len;

	assert(connection);
	assert(connection->post);
	assert(connection->postingState.states);
	assert(ArrayLength(connection->post->data) == ArrayLength(connection->postingState.states));
	assert(connection->postingState.index >= 0);
	assert(connection->postingState.index <= ArrayLength(connection->postingState.states));

	// Cache some stuff.
	////////////////////
	postingState = &connection->postingState;
	len = ArrayLength(postingState->states);

	// Check for buffered data.
	///////////////////////////
	if(connection->sendBuffer.pos < connection->sendBuffer.len)
	{
		// Send the buffered data.
		//////////////////////////
		if(!ghiSendBufferedData(connection))
			return GHIPostingError;

		// Check if we couldn't send it all.
		////////////////////////////////////
		if(connection->sendBuffer.pos < connection->sendBuffer.len)
			return GHIPostingPosting;

		// We sent it all, so reset the buffer.
		///////////////////////////////////////
		ghiResetBuffer(&connection->sendBuffer);

		// If uploading a DIME attachment, wait for HTTP continue.
		//////////////////////////////////////////////////////////
		if (connection->postingState.waitPostContinue)
			return GHIPostingWaitForContinue;

		// Was that all that's left?
		////////////////////////////
		if(connection->postingState.index == len)
			return GHIPostingDone;
	}

	// When posting soap and DIME attachments, we should terminate the
	// header and wait for a response.  This will either be a continue or
	// a server error.
	if (connection->postingState.waitPostContinue)
	{
		if (connection->post->hasFiles || connection->post->hasSoap)
		{
			// terminate the header and wait for a response
		  	GS_ASSERT(connection->encodeBuffer.len == 0);
			trySendResult = ghiTrySendThenBuffer(connection, CRLF, (int)strlen(CRLF));
			if(trySendResult == GHITrySendError)
				return GHIPostingError;
			else if (trySendResult == GHITrySendBuffered)
				return GHIPostingPosting;
			else
			{
				if (connection->postingState.waitPostContinue == GHTTPTrue)
					return GHIPostingWaitForContinue;
				//else
				//	fall through
			}
		}
		else
		{
			// simple posts don't have to wait
			connection->postingState.waitPostContinue = GHTTPFalse;
			// fall through
		}
	}

	// Loop while there's data to upload.
	/////////////////////////////////////
	while(postingState->index < len)
	{
		// Get the current data state.
		//////////////////////////////
		postState = (GHIPostState *)ArrayNth(postingState->states, postingState->index);
		assert(postState);

		// Upload the current data.
		///////////////////////////
		postingResult = ghiPostStateDoPosting(postState, connection, 
			(postingState->index == 0)?GHTTPTrue:GHTTPFalse,
			(postingState->index == (ArrayLength(postingState->states)-1))?GHTTPTrue:GHTTPFalse);

		// Check for error.
		///////////////////
		if(postingResult == GHIPostingError)
		{
			// Make sure we already set the error stuff.
			////////////////////////////////////////////
			assert(connection->completed && connection->result);

			return GHIPostingError;
		}

		// Check for still posting.
		///////////////////////////
		if(postingResult == GHIPostingPosting)
			return GHIPostingPosting;

		// One more done.
		/////////////////
		postingState->index++;
	}

	// Encrypt and send anything left in the encode buffer
	//   -- for example, when posting string data we don't encrypt until we have the entire string (for efficiency only)
	if (connection->encryptor.mEngine != GHTTPEncryptionEngine_None)
	{
		if (connection->encodeBuffer.len > 0)
		{
			GS_ASSERT(connection->encodeBuffer.pos == 0); // if you hit this, it means you forgot the clear the buffer
			if (GHTTPFalse == ghiEncryptDataToBuffer(&connection->sendBuffer, 
						connection->encodeBuffer.data, connection->encodeBuffer.len))
			{
				return GHIPostingError;
			}
			ghiResetBuffer(&connection->encodeBuffer);
		}
	}

	// Send or buffer the end marker.
	/////////////////////////////////
	if(connection->post->hasFiles && !connection->post->useDIME)
	{
		GS_ASSERT(!connection->post->hasSoap);

		// send MIME boundary end
		trySendResult = ghiTrySendThenBuffer(connection, GHI_MULTIPART_BOUNDARY_END, (int)strlen(GHI_MULTIPART_BOUNDARY_END));
		if(trySendResult == GHITrySendError)
			return GHIPostingError;
	}

	// We're not done if there's stuff in the buffer.
	/////////////////////////////////////////////////
	if(connection->sendBuffer.pos < connection->sendBuffer.len)
		return GHIPostingPosting;

	return GHIPostingDone;
}
