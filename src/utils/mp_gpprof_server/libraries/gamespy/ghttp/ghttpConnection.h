 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GHTTPCONNECTION_H_
#define _GHTTPCONNECTION_H_

#include "ghttpMain.h"
#include "ghttpEncryption.h"
#include "ghttpBuffer.h"
#include "ghttpPost.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initial size and increment amount for the send buffer.
/////////////////////////////////////////////////////////
#define SEND_BUFFER_INITIAL_SIZE            (2 * 1024)
#define SEND_BUFFER_INCREMENT_SIZE          (4 * 1024)

// Initial size and increment amount for the recv buffer.
/////////////////////////////////////////////////////////////////
#define RECV_BUFFER_INITIAL_SIZE            (2 * 1024)
#define RECV_BUFFER_INCREMENT_SIZE          (2 * 1024)

// Initial size and increment amount for the get file buffer.
/////////////////////////////////////////////////////////////
#define GET_FILE_BUFFER_INITIAL_SIZE        (2 * 1024)
#define GET_FILE_BUFFER_INCREMENT_SIZE      (2 * 1024)

// Initial size and increment amount for the ssl encoding buffer.
/////////////////////////////////////////////////////////////////
#define ENCODE_BUFFER_INITIAL_SIZE          (2 * 1024)
#define ENCODE_BUFFER_INCREMENT_SIZE        (1 * 1024)

// Initial size and increment amount for the ssl decoding buffer.
/////////////////////////////////////////////////////////////////
#define DECODE_BUFFER_INITIAL_SIZE          (2 * 1024)
#define DECODE_BUFFER_INCREMENT_SIZE        (1 * 1024)

// The size of the buffer for chunk headers (NOT including the NUL).
////////////////////////////////////////////////////////////////////
#define CHUNK_HEADER_SIZE                   10

// The type of request made.
////////////////////////////
typedef enum
{
	GHIGET,     // Buffer the file.
	GHISAVE,    // Save the file to disk.
	GHISTREAM,  // Stream the file.
	GHIHEAD,    // Get just the headers for a request.
	GHIPOST     // Only posting data (all types can post).
} GHIRequestType;

// Chunk-reading states.
////////////////////////
typedef enum
{
	CRHeader,       // Reading a chunk header.
	CRChunk,        // Reading chunk (actual content).
	CRCRLF,         // Reading the CRLF at the end of a chunk.
	CRFooter        // Reading the footer at the end of the file (chunk with size of 0).
} CRState;

// Protocol.
////////////
typedef enum
{
	GHIHttp,
	GHIHttps
} GHIProtocol;

// This is the data for a single http connection.
/////////////////////////////////////////////////
typedef struct GHIConnection
{
	GHTTPBool inUse;              // If true, this connection object is being used.
	GHTTPRequest request;         // This object's request index.
	int uniqueID;                 // Every connection object has a unqiue ID.

	GHIRequestType type;          // The type of request this connection is for.

	GHTTPState state;             // The state of the request.

	char * URL;                   // The URL for the file.
	char * serverAddress;         // The address of the server as contained in the URL.
	unsigned int serverIP;        // The server's IP.
	unsigned short serverPort;    // The server's port.
	char * requestPath;           // The path as contained in the URL.

	GHIProtocol protocol;         // Protocol used for this connection.

	char * sendHeaders;           // Optional headers to pass with the request.

	FILE * saveFile;              // If saving to disk, the file being saved to.

	GHTTPBool blocking;           // Blocking flag.
	
	GHTTPBool persistConnection;  // If TRUE, Connection: close will not be sent in the headers and the connection will be left open

	GHTTPResult result;           // The result of the request.
	ghttpProgressCallback progressCallback;    // Called periodically with progress updates.
	ghttpCompletedCallback completedCallback;  // Called when the file has been received.
	void * callbackParam;         // User-data to be passed to the callbacks.

	SOCKET socket;                // The socket for this connection.
	int socketError;              // If there was a socket error, the last error code is stored here.

	GHIBuffer sendBuffer;         // The buffer for outgoing data. 
	GHIBuffer encodeBuffer;       // The buffer for outgoing data. (will be encrypted; only used with https)
	GHIBuffer recvBuffer;         // The buffer for incoming data. (plain text)
	GHIBuffer decodeBuffer;       // The buffer for incoming data. (encrypted)(only used with https)
	
	GHIBuffer getFileBuffer;      // ghttpGetFile[Ex] uses this buffer (which may be user-supplied).
	GHTTPBool userBufferSupplied; // True if a user buffer was supplied.

	int statusMajorVersion;       // The major-version number from the server's response.
	int statusMinorVersion;       // The minor-version number from the server's response.
	int statusCode;               // The status-code from the server's response.
	int statusStringIndex;        // Index in the recvBuffer where the status string starts.
	
	int headerStringIndex;        // Index in the recvBuffer where the headers begin

	GHTTPBool completed;          // This connection is completed - call the callback and kill it.

	GHTTPByteCount fileBytesReceived; // Number of file bytes received.
	GHTTPByteCount totalSize;         // Total size of the file, -1 if unknown.

	char * redirectURL;           // If this is not NULL, we need to redirect the download to this URL.
	int redirectCount;            // Number of redirections done for this request.

	GHTTPBool chunkedTransfer;    // The body of the response is chunky ("Transfer-Encoding: chunked").
	char chunkHeader[CHUNK_HEADER_SIZE + 1];  // Partial chunk headers are stored in here.
	int chunkHeaderLen;           // The number of bytes in chunkHeader.
	int chunkBytesLeft;           // Number of bytes left in the chunk (only valid for CRChunk).
	CRState chunkReadingState;    // Determines if a chunk header or chunk data is being read.

	GHTTPBool processing;         // If true, being processed.  Used to prevent recursive processing.
	GHTTPBool connectionClosed;   // If true, the connection has been closed (orderly or abortive)

	GHTTPBool throttle;           // If true, throttle this connection.
	gsi_time lastThrottleRecv;  // The last time we received on a throttled connection.

	GHTTPPost post;               // If not NULL, a reference to a post object to upload with the request.
	GHIPostingState postingState; // If posting, the state of the upload.
	
	gsi_time maxRecvTime;         // Max time spent receiving per call to "Think" - Prevents blocking on ultrafast connections
	char * proxyOverrideServer;	  // Allows use of a different proxy than the global proxy
	unsigned short proxyOverridePort;

	struct GHIEncryptor encryptor;

#if !defined(GSI_NO_THREADS)
	GSIResolveHostnameHandle handle; //handle used for asychronous DNS lookups
#endif
	
} GHIConnection;

// Create a new connection object.
// Returns NULL on failure.
//////////////////////////////////
GHIConnection * ghiNewConnection
(
	void
);

// Frees the connection object.
///////////////////////////////
GHTTPBool ghiFreeConnection
(
	GHIConnection * connection
);

// Returns the connection object for a request index.
// Returns NULL if the index is bad.
/////////////////////////////////////////////////////
GHIConnection * ghiRequestToConnection
(
	GHTTPRequest request
);

// Calls the callback on each connection.
/////////////////////////////////////////
void ghiEnumConnections
(
	GHTTPBool (* callback)(GHIConnection *)
);

// Redirects the given connection.
// Resets the connection to get the
// file at connection->redirectURL.
///////////////////////////////////
void ghiRedirectConnection
(
	GHIConnection * connection
);

// Kills all connections and frees up all memory.
/////////////////////////////////////////////////
void ghiCleanupConnections
(
	void
);

#ifdef __cplusplus
}
#endif

#endif
