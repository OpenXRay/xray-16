/*
 * cgifcgi.c --
 *
 *	CGI to FastCGI bridge
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */
#ifndef lint
static const char rcsid[] = "$Id: cgi-fcgi.c,v 1.15 2001/09/01 01:14:28 robs Exp $";
#endif /* not lint */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fcgi_config.h"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef _WIN32
#include <stdlib.h>
#include <io.h>
#else
extern char **environ;
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "fcgimisc.h"
#include "fcgiapp.h"
#include "fastcgi.h"
#include "fcgios.h"


static int wsReadPending = 0;
static int fcgiReadPending = 0;
static int fcgiWritePending = 0;

static void ScheduleIo(void);


/*
 * Simple buffer (not ring buffer) type, used by all event handlers.
 */
#define BUFFLEN 8192
typedef struct {
    char *next;
    char *stop;
    char buff[BUFFLEN];
} Buffer;

/*
 *----------------------------------------------------------------------
 *
 * GetPtr --
 *
 *      Returns a count of the number of characters available
 *      in the buffer (at most n) and advances past these
 *      characters.  Stores a pointer to the first of these
 *      characters in *ptr.
 *
 *----------------------------------------------------------------------
 */

static int GetPtr(char **ptr, int n, Buffer *pBuf)
{
    int result;
    *ptr = pBuf->next;
    result = min(n, pBuf->stop - pBuf->next);
    pBuf->next += result;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeHeader --
 *
 *      Constructs an FCGI_Header struct.
 *
 *----------------------------------------------------------------------
 */
static FCGI_Header MakeHeader(
        int type,
        int requestId,
        int contentLength,
        int paddingLength)
{
    FCGI_Header header;
    ASSERT(contentLength >= 0 && contentLength <= FCGI_MAX_LENGTH);
    ASSERT(paddingLength >= 0 && paddingLength <= 0xff);
    header.version = FCGI_VERSION_1;
    header.type             = (unsigned char) type;
    header.requestIdB1      = (unsigned char) ((requestId     >> 8) & 0xff);
    header.requestIdB0      = (unsigned char) ((requestId         ) & 0xff);
    header.contentLengthB1  = (unsigned char) ((contentLength >> 8) & 0xff);
    header.contentLengthB0  = (unsigned char) ((contentLength     ) & 0xff);
    header.paddingLength    = (unsigned char) paddingLength;
    header.reserved         =  0;
    return header;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeBeginRequestBody --
 *
 *      Constructs an FCGI_BeginRequestBody record.
 *
 *----------------------------------------------------------------------
 */
static FCGI_BeginRequestBody MakeBeginRequestBody(
        int role,
        int keepConnection)
{
    FCGI_BeginRequestBody body;
    ASSERT((role >> 16) == 0);
    body.roleB1 = (unsigned char) ((role >>  8) & 0xff);
    body.roleB0 = (unsigned char) (role         & 0xff);
    body.flags  = (unsigned char) ((keepConnection) ? FCGI_KEEP_CONN : 0);
    memset(body.reserved, 0, sizeof(body.reserved));
    return body;
}


static int bytesToRead;    /* number of bytes to read from Web Server */
static int appServerSock = -1;  /* Socket connected to FastCGI application,
                                 * used by AppServerReadHandler and
                                 * AppServerWriteHandler. */
static Buffer fromAS;      /* Bytes read from the FCGI application server. */
static FCGI_Header header; /* Header of the current record.  Is global
                            * since read may return a partial header. */
static int headerLen = 0;  /* Number of valid bytes contained in header.
                            * If headerLen < sizeof(header),
                            * AppServerReadHandler is reading a record header;
                            * otherwise it is reading bytes of record content
                            * or padding. */
static int contentLen;     /* If headerLen == sizeof(header), contentLen
                            * is the number of content bytes still to be
                            * read. */
static int paddingLen;     /* If headerLen == sizeof(header), paddingLen
                            * is the number of padding bytes still
                            * to be read. */
static int requestId;      /* RequestId of the current request.
                            * Set by main. */
static FCGI_EndRequestBody erBody;
static int readingEndRequestBody = FALSE;
                           /* If readingEndRequestBody, erBody contains
                            * partial content: contentLen more bytes need
                            * to be read. */
static int exitStatus = 0;
static int exitStatusSet = FALSE;

static int stdinFds[3];


/*
 *----------------------------------------------------------------------
 *
 * FCGIexit --
 *
 *      FCGIexit provides a single point of exit.  It's main use is for
 *      application debug when porting to other operating systems.
 *
 *----------------------------------------------------------------------
 */
static void FCGIexit(int exitCode)
{
    if(appServerSock != -1) {
        OS_Close(appServerSock);
	appServerSock = -1;
    }
    OS_LibShutdown();
    exit(exitCode);
}

#undef exit
#define exit FCGIexit


/*
 *----------------------------------------------------------------------
 *
 * AppServerReadHandler --
 *
 *      Reads data from the FCGI application server and (blocking)
 *      writes all of it to the Web server.  Exits the program upon
 *      reading EOF from the FCGI application server.  Called only when
 *      there's data ready to read from the application server.
 *
 *----------------------------------------------------------------------
 */

static void AppServerReadHandler(ClientData dc, int bytesRead)
{
    int count, outFD;
    char *ptr;

    /* Touch unused parameters to avoid warnings */
    dc = NULL;

    assert(fcgiReadPending == TRUE);
    fcgiReadPending = FALSE;
    count = bytesRead;

    if(count <= 0) {
        if(count < 0) {
            exit(OS_Errno);
        }
        if(headerLen > 0 || paddingLen > 0) {
            exit(FCGX_PROTOCOL_ERROR);
        }
	if(appServerSock != -1) {
	    OS_Close(appServerSock);
	    appServerSock = -1;
	}
        /*
         * XXX: Shouldn't be here if exitStatusSet.
         */
        exit((exitStatusSet) ? exitStatus : FCGX_PROTOCOL_ERROR);
    }
    fromAS.stop = fromAS.next + count;
    while(fromAS.next != fromAS.stop) {
        /*
         * fromAS is not empty.  What to do with the contents?
         */
        if(headerLen < sizeof(header)) {
            /*
             * First priority is to complete the header.
             */
            count = GetPtr(&ptr, sizeof(header) - headerLen, &fromAS);
            assert(count > 0);
            memcpy(&header + headerLen, ptr, count);
            headerLen += count;
            if(headerLen < sizeof(header)) {
                break;
            }
            if(header.version != FCGI_VERSION_1) {
                exit(FCGX_UNSUPPORTED_VERSION);
	    }
            if((header.requestIdB1 << 8) + header.requestIdB0 != requestId) {
                exit(FCGX_PROTOCOL_ERROR);
	    }
            contentLen = (header.contentLengthB1 << 8)
                         + header.contentLengthB0;
            paddingLen =  header.paddingLength;
	} else {
            /*
             * Header is complete (possibly from previous call).  What now?
             */
            switch(header.type) {
	        case FCGI_STDOUT:
                case FCGI_STDERR:
                    /*
                     * Write the buffered content to stdout or stderr.
                     * Blocking writes are OK here; can't prevent a slow
                     * client from tying up the app server without buffering
                     * output in temporary files.
                     */
                    count = GetPtr(&ptr, contentLen, &fromAS);
                    contentLen -= count;
                    if(count > 0) {
                        outFD = (header.type == FCGI_STDOUT) ?
                                    STDOUT_FILENO : STDERR_FILENO;
                        if(OS_Write(outFD, ptr, count) < 0) {
                            exit(OS_Errno);
                        }
	            }
                    break;
                case FCGI_END_REQUEST:
                    if(!readingEndRequestBody) {
                        if(contentLen != sizeof(erBody)) {
                            exit(FCGX_PROTOCOL_ERROR);
		        }
                        readingEndRequestBody = TRUE;
		    }
                    count = GetPtr(&ptr, contentLen, &fromAS);
                    if(count > 0) {
                        memcpy(&erBody + sizeof(erBody) - contentLen,
                                ptr, count);
                        contentLen -= count;
		    }
                    if(contentLen == 0) {
                        if(erBody.protocolStatus != FCGI_REQUEST_COMPLETE) {
                            /*
                             * XXX: What to do with FCGI_OVERLOADED?
                             */
                            exit(FCGX_PROTOCOL_ERROR);
			}
                        exitStatus = (erBody.appStatusB3 << 24)
                                   + (erBody.appStatusB2 << 16)
                                   + (erBody.appStatusB1 <<  8)
                                   + (erBody.appStatusB0      );
                        exitStatusSet = TRUE;
                        readingEndRequestBody = FALSE;
	            }
                    break;
                case FCGI_GET_VALUES_RESULT:
                    /* coming soon */
                case FCGI_UNKNOWN_TYPE:
                    /* coming soon */
                default:
                    exit(FCGX_PROTOCOL_ERROR);
	    }
            if(contentLen == 0) {
                if(paddingLen > 0) {
                    paddingLen -= GetPtr(&ptr, paddingLen, &fromAS);
		}
                /*
                 * If we've processed all the data and skipped all the
                 * padding, discard the header and look for the next one.
                 */
                if(paddingLen == 0) {
                    headerLen = 0;
	        }
	    }
        } /* headerLen >= sizeof(header) */
    } /*while*/
    ScheduleIo();
}

static Buffer fromWS;   /* Buffer for data read from Web server
                         * and written to FastCGI application. Used
                         * by WebServerReadHandler and
                         * AppServerWriteHandler. */
static int webServerReadHandlerEOF;
                        /* TRUE iff WebServerReadHandler has read EOF from
                         * the Web server. Used in main to prevent
                         * rescheduling WebServerReadHandler. */

static void WriteStdinEof(void)
{
    static int stdin_eof_sent = 0;

    if (stdin_eof_sent)
    	return;

    *((FCGI_Header *)fromWS.stop) = MakeHeader(FCGI_STDIN, requestId, 0, 0);
    fromWS.stop += sizeof(FCGI_Header);
    stdin_eof_sent = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * WebServerReadHandler --
 *
 *      Non-blocking reads data from the Web server into the fromWS
 *      buffer.  Called only when fromWS is empty, no EOF has been
 *      received from the Web server, and there's data available to read.
 *
 *----------------------------------------------------------------------
 */

static void WebServerReadHandler(ClientData dc, int bytesRead)
{
    /* Touch unused parameters to avoid warnings */
    dc = NULL;

    assert(fromWS.next == fromWS.stop);
    assert(fromWS.next == &fromWS.buff[0]);
    assert(wsReadPending == TRUE);
    wsReadPending = FALSE;

    if(bytesRead < 0) {
        exit(OS_Errno);
    }
    *((FCGI_Header *) &fromWS.buff[0])
            = MakeHeader(FCGI_STDIN, requestId, bytesRead, 0);
    bytesToRead -= bytesRead;
    fromWS.stop = &fromWS.buff[sizeof(FCGI_Header) + bytesRead];
    webServerReadHandlerEOF = (bytesRead == 0);

    if (bytesToRead <= 0)
	WriteStdinEof();

    ScheduleIo();
}

/*
 *----------------------------------------------------------------------
 *
 * AppServerWriteHandler --
 *
 *      Non-blocking writes data from the fromWS buffer to the FCGI
 *      application server.  Called only when fromWS is non-empty
 *      and the socket is ready to accept some data.
 *
 *----------------------------------------------------------------------
 */

static void AppServerWriteHandler(ClientData dc, int bytesWritten)
{
    int length = fromWS.stop - fromWS.next;

    /* Touch unused parameters to avoid warnings */
    dc = NULL;

    assert(length > 0);
    assert(fcgiWritePending == TRUE);

    fcgiWritePending = FALSE;
    if(bytesWritten < 0) {
        exit(OS_Errno);
    }
    if((int)bytesWritten < length) {
        fromWS.next += bytesWritten;
    } else {
        fromWS.stop = fromWS.next = &fromWS.buff[0];
    }

    ScheduleIo();
}


/*
 * ScheduleIo --
 *
 *      This functions is responsible for scheduling all I/O to move
 *      data between a web server and a FastCGI application.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      This routine will signal the ioEvent upon completion.
 *
 */
static void ScheduleIo(void)
{
    int length;

    /*
     * Move data between standard in and the FastCGI connection.
     */
    if(!fcgiWritePending && appServerSock != -1 &&
       ((length = fromWS.stop - fromWS.next) != 0)) {
	if(OS_AsyncWrite(appServerSock, 0, fromWS.next, length,
			 AppServerWriteHandler,
			 (ClientData)appServerSock) == -1) {
	    FCGIexit(OS_Errno);
	} else {
	    fcgiWritePending = TRUE;
	}
    }

    /*
     * Schedule a read from the FastCGI application if there's not
     * one pending and there's room in the buffer.
     */
    if(!fcgiReadPending && appServerSock != -1) {
	fromAS.next = &fromAS.buff[0];

	if(OS_AsyncRead(appServerSock, 0, fromAS.next, BUFFLEN,
			AppServerReadHandler,
			(ClientData)appServerSock) == -1) {
	    FCGIexit(OS_Errno);
	} else {
	    fcgiReadPending = TRUE;
	}
    }

    /*
     * Schedule a read from standard in if necessary.
     */
    if((bytesToRead > 0) && !webServerReadHandlerEOF && !wsReadPending &&
       !fcgiWritePending &&
       fromWS.next == &fromWS.buff[0]) {
	if(OS_AsyncReadStdin(fromWS.next + sizeof(FCGI_Header),
			     BUFFLEN - sizeof(FCGI_Header),
			     WebServerReadHandler, STDIN_FILENO)== -1) {
	    FCGIexit(OS_Errno);
	} else {
	    wsReadPending = TRUE;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * FCGI_Start --
 *
 *      Starts nServers copies of FCGI application appPath, all
 *      listening to a Unix Domain socket at bindPath.
 *
 *----------------------------------------------------------------------
 */

static void FCGI_Start(char *bindPath, char *appPath, int nServers)
{
    int listenFd, i;

    /* @@@ Should be able to pick up the backlog as an arg */
    if((listenFd = OS_CreateLocalIpcFd(bindPath, 5)) == -1) {
        exit(OS_Errno);
    }

    if(access(appPath, X_OK) == -1) {
	fprintf(stderr, "%s is not executable\n", appPath);
	exit(1);
    }

    /*
     * Create the server processes
     */
    for(i = 0; i < nServers; i++) {
        if(OS_SpawnChild(appPath, listenFd) == -1) {
            exit(OS_Errno);
	}
    }
    OS_Close(listenFd);
}

/*
 *----------------------------------------------------------------------
 *
 * FCGIUtil_BuildNameValueHeader --
 *
 *      Builds a name-value pair header from the name length
 *      and the value length.  Stores the header into *headerBuffPtr,
 *      and stores the length of the header into *headerLenPtr.
 *
 * Side effects:
 *      Stores header's length (at most 8) into *headerLenPtr,
 *      and stores the header itself into
 *      headerBuffPtr[0 .. *headerLenPtr - 1].
 *
 *----------------------------------------------------------------------
 */
static void FCGIUtil_BuildNameValueHeader(
        int nameLen,
        int valueLen,
        unsigned char *headerBuffPtr,
        int *headerLenPtr) {
    unsigned char *startHeaderBuffPtr = headerBuffPtr;

    ASSERT(nameLen >= 0);
    if (nameLen < 0x80) {
        *headerBuffPtr++ = (unsigned char) nameLen;
    } else {
        *headerBuffPtr++ = (unsigned char) ((nameLen >> 24) | 0x80);
        *headerBuffPtr++ = (unsigned char) (nameLen >> 16);
        *headerBuffPtr++ = (unsigned char) (nameLen >> 8);
        *headerBuffPtr++ = (unsigned char) nameLen;
    }
    ASSERT(valueLen >= 0);
    if (valueLen < 0x80) {
        *headerBuffPtr++ = (unsigned char) valueLen;
    } else {
        *headerBuffPtr++ = (unsigned char) ((valueLen >> 24) | 0x80);
        *headerBuffPtr++ = (unsigned char) (valueLen >> 16);
        *headerBuffPtr++ = (unsigned char) (valueLen >> 8);
        *headerBuffPtr++ = (unsigned char) valueLen;
    }
    *headerLenPtr = headerBuffPtr - startHeaderBuffPtr;
}


#define MAXARGS	16
static int ParseArgs(int argc, char *argv[],
        int *doBindPtr, int *doStartPtr,
        char *connectPathPtr, char *appPathPtr, int *nServersPtr) {
    int	    i,
	    x,
	    err = 0,
	    ac;
    char    *tp1,
	    *tp2,
	    *av[MAXARGS];
    FILE    *fp;
    char    line[BUFSIZ];

    *doBindPtr = TRUE;
    *doStartPtr = TRUE;
    *connectPathPtr = '\0';
    *appPathPtr = '\0';
    *nServersPtr = 0;

    for(i = 0; i < MAXARGS; i++)
        av[i] = NULL;
    for(i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(!strcmp(argv[i], "-f")) {
		if(++i == argc) {
		    fprintf(stderr,
                            "Missing command file name after -f\n");
		    return 1;
		}
		if((fp = fopen(argv[i], "r")) == NULL) {
		    fprintf(stderr, "Cannot open command file %s\n", argv[i]);
		    return 1;
		}
		ac = 1;
		while(fgets(line, BUFSIZ, fp)) {
		    if(line[0] == '#') {
			continue;
		    }
		    if((tp1 = (char *) strrchr(line,'\n')) != NULL) {
			*tp1-- = 0;
			while(*tp1 == ' ' || *tp1 =='\t') {
			    *tp1-- = 0;
		        }
		    } else {
			fprintf(stderr, "Line to long\n");
			return 1;
		    }
		    tp1 = line;
		    while(tp1) {
			if((tp2 = strchr(tp1, ' ')) != NULL) {
			    *tp2++ =  0;
		        }
    			if(ac >= MAXARGS) {
			    fprintf(stderr,
                                    "To many arguments, "
                                    "%d is max from a file\n", MAXARGS);
				exit(-1);
			}
			if((av[ac] = (char *)malloc(strlen(tp1)+1)) == NULL) {
			    fprintf(stderr, "Cannot allocate %d bytes\n",
				    strlen(tp1)+1);
			    exit(-1);
			}
			strcpy(av[ac++], tp1);
			tp1 = tp2;
		    }
		}
		err = ParseArgs(ac, av, doBindPtr, doStartPtr,
                        connectPathPtr, appPathPtr, nServersPtr);
		for(x = 1; x < ac; x++) {
		    ASSERT(av[x] != NULL);
		    free(av[x]);
	        }
		return err;
#ifdef _WIN32
	    } else if (!strcmp(argv[i], "-jitcgi")) {
	        DebugBreak();
	    } else if (!strcmp(argv[i], "-dbgfcgi")) {
	        putenv("DEBUG_FCGI=TRUE");
#endif
	    } else if(!strcmp(argv[i], "-start")) {
		*doBindPtr = FALSE;
	    } else if(!strcmp(argv[i], "-bind")) {
		*doStartPtr = FALSE;
	    } else if(!strcmp(argv[i], "-connect")) {
                if(++i == argc) {
	            fprintf(stderr,
                            "Missing connection name after -connect\n");
                    err++;
                } else {
                    strcpy(connectPathPtr, argv[i]);
                }
	    } else {
		fprintf(stderr, "Unknown option %s\n", argv[i]);
		err++;
	    }
	} else if(*appPathPtr == '\0') {
            strcpy(appPathPtr, argv[i]);
        } else if(isdigit((int)argv[i][0]) && *nServersPtr == 0) {
            *nServersPtr = atoi(argv[i]);
            if(*nServersPtr <= 0) {
                fprintf(stderr, "Number of servers must be greater than 0\n");
                err++;
            }
        } else {
            fprintf(stderr, "Unknown argument %s\n", argv[i]);
            err++;
        }
    }
    if(*doStartPtr && *appPathPtr == 0) {
        fprintf(stderr, "Missing application pathname\n");
        err++;
    }
    if(*connectPathPtr == 0) {
	fprintf(stderr, "Missing -connect <connName>\n");
	err++;
    } else if(strchr(connectPathPtr, ':')) {
/*
 * XXX: Test to see if we can use IP connect locally...
        This hack lets me test the ability to create a local process listening
	to a TCP/IP port for connections and subsequently connect to the app
	like we do for Unix domain and named pipes.

        if(*doStartPtr && *doBindPtr) {
	    fprintf(stderr,
                    "<connName> of form hostName:portNumber "
                    "requires -start or -bind\n");
	    err++;
        }
 */
    }
    if(*nServersPtr == 0) {
        *nServersPtr = 1;
    }
    return err;
}

int main(int argc, char **argv)
{
    char **envp = environ;
    int count;
    FCGX_Stream *paramsStream;
    int numFDs;
    unsigned char headerBuff[8];
    int headerLen, valueLen;
    char *equalPtr;
    FCGI_BeginRequestRecord beginRecord;
    int	doBind, doStart, nServers;
    char appPath[MAXPATHLEN], bindPath[MAXPATHLEN];

    if(ParseArgs(argc, argv, &doBind, &doStart,
		   (char *) &bindPath, (char *) &appPath, &nServers)) {
	fprintf(stderr,
"Usage:\n"
"    cgi-fcgi -f <cmdPath> , or\n"
"    cgi-fcgi -connect <connName> <appPath> [<nServers>] , or\n"
"    cgi-fcgi -start -connect <connName> <appPath> [<nServers>] , or\n"
"    cgi-fcgi -bind -connect <connName> ,\n"
"where <connName> is either the pathname of a UNIX domain socket\n"
"or (if -bind is given) a hostName:portNumber specification\n"
"or (if -start is given) a :portNumber specification (uses local host).\n");
	exit(1);
    }

    if(OS_LibInit(stdinFds)) {
        fprintf(stderr, "Error initializing OS library: %d\n", OS_Errno);
	exit(0);
    }

    equalPtr = getenv("CONTENT_LENGTH");
    if(equalPtr != NULL) {
        bytesToRead = atoi(equalPtr);
    } else {
        bytesToRead = 0;
    }

    if(doBind) {
        appServerSock = OS_FcgiConnect(bindPath);
    }
    if(doStart && (!doBind || appServerSock < 0)) {
        FCGI_Start(bindPath, appPath, nServers);
        if(!doBind) {
            exit(0);
        } else {
            appServerSock = OS_FcgiConnect(bindPath);
	}
    }
    if(appServerSock < 0) {
        fprintf(stderr, "Could not connect to %s\n", bindPath);
        exit(OS_Errno);
    }
    /*
     * Set an arbitrary non-null FCGI RequestId
     */
    requestId = 1;
    /*
     * XXX: Send FCGI_GET_VALUES
     */

    /*
     * XXX: Receive FCGI_GET_VALUES_RESULT
     */

    /*
     * Send FCGI_BEGIN_REQUEST (XXX: hack, separate write)
     */
    beginRecord.header = MakeHeader(FCGI_BEGIN_REQUEST, requestId,
            sizeof(beginRecord.body), 0);
    beginRecord.body = MakeBeginRequestBody(FCGI_RESPONDER, FALSE);
    count = OS_Write(appServerSock, (char *)&beginRecord, sizeof(beginRecord));
    if(count != sizeof(beginRecord)) {
        exit(OS_Errno);
    }
    /*
     * Send environment to the FCGI application server
     */
    paramsStream = FCGX_CreateWriter(appServerSock, requestId, 8192, FCGI_PARAMS);
    for( ; *envp != NULL; envp++) {
        equalPtr = strchr(*envp, '=');
        if(equalPtr  == NULL) {
            exit(1000);
        }
        valueLen = strlen(equalPtr + 1);
        FCGIUtil_BuildNameValueHeader(
                equalPtr - *envp,
                valueLen,
                &headerBuff[0],
                &headerLen);
        if(FCGX_PutStr((char *) &headerBuff[0], headerLen, paramsStream) < 0
                || FCGX_PutStr(*envp, equalPtr - *envp, paramsStream) < 0
                || FCGX_PutStr(equalPtr + 1, valueLen, paramsStream) < 0) {
            exit(FCGX_GetError(paramsStream));
        }
    }
    FCGX_FClose(paramsStream);
    FCGX_FreeStream(&paramsStream);
    /*
     * Perform the event loop until AppServerReadHander sees FCGI_END_REQUEST
     */
    fromWS.stop = fromWS.next = &fromWS.buff[0];
    webServerReadHandlerEOF = FALSE;
    /*
     * XXX: might want to use numFDs in the os library.
     */
    numFDs = max(appServerSock, STDIN_FILENO) + 1;
    OS_SetFlags(appServerSock, O_NONBLOCK);

    if (bytesToRead <= 0)
	WriteStdinEof();

    ScheduleIo();

    while(!exitStatusSet) {
        /*
	 * NULL = wait forever (or at least until there's something
	 *        to do.
	 */
        OS_DoIo(NULL);
    }
    if(exitStatusSet) {
        FCGIexit(exitStatus);
    } else {
        FCGIexit(999);
    }

    return 0;
}
