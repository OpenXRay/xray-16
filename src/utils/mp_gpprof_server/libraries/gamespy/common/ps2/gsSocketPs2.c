///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gscommon.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// INSOCK
#if defined(INSOCK)
#define INSOCK_MAX_UDP_BUFSIZE 8000000  // default max 
#define INSOCK_MAX_TCP_BUFSIZE 32000

extern sceSifMClientData gGSIInsockClientData;
extern u_int             gGSIInsockSocketBuffer[NETBUFSIZE] __attribute__((aligned(64)));

// NOT FULLY IMPLEMENTED
int SetReceiveBufferSize(SOCKET sock, int size)
{return -1; GSI_UNUSED(sock); GSI_UNUSED(size); }

// NOT FULLY IMPLEMENTED
int SetSendBufferSize(SOCKET sock, int size)
{return -1; GSI_UNUSED(sock); GSI_UNUSED(size); }

int GetReceiveBufferSize(SOCKET sock)
{return NETBUFSIZE; GSI_UNUSED(sock); }
	
int GetSendBufferSize(SOCKET sock)
{return NETBUFSIZE; GSI_UNUSED(sock); }

// Poll socket for Send, Recv and Except
int GSISocketSelect(SOCKET theSocket, int* theReadFlag, int* theWriteFlag, int* theExceptFlag)
{
	int result = 0;
	sceInetPollFd_t aPollFdSet;

	// Init the flags to 0
	if ((theReadFlag   != NULL))
		*theReadFlag = 0;
	if ((theWriteFlag  != NULL))
		*theWriteFlag = 0;
	if ((theExceptFlag != NULL))
		*theExceptFlag = 0;

	// Setup the fd set
	aPollFdSet.cid     = theSocket;  // the socket
	aPollFdSet.events  = 0;          // events in
	aPollFdSet.revents = 0;          // events out

	if (theReadFlag   != NULL) aPollFdSet.events |= sceINET_POLLIN;
	if (theWriteFlag  != NULL) aPollFdSet.events |= sceINET_POLLOUT;
	if (theExceptFlag != NULL) aPollFdSet.events |= sceINET_POLLERR;

	// Poll the fds
	//    1 fds, 0 ms timeout
	result = sceInsockPoll(&aPollFdSet, 1, 0);
	if (result > 0)
	{
		// If the Flag is valid, set the return value
		if ((theReadFlag   != NULL))
			*theReadFlag    = (aPollFdSet.revents & sceINET_POLLIN)  ? 1:0;
		if ((theWriteFlag  != NULL))
			*theWriteFlag   = (aPollFdSet.revents & sceINET_POLLOUT) ? 1:0;
		if ((theExceptFlag != NULL))
			*theExceptFlag  = (aPollFdSet.revents & sceINET_POLLERR) ? 1:0;
	}
	return result;	
}

// shutdown needs to have a timeout that can be done 
// right before shutting down
int gsiShutdown(SOCKET s, int how)
{
	// set the shutdown timeout to thirty milliseconds based on most games running
	// thirty frames per second (33ms rounded down to 30)
	sceInsockSetShutdownTimeout(s, 30);
	return sceInsockShutdown(s, how);
}
#endif