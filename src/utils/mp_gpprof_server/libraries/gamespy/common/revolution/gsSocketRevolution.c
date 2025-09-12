///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(_REVOLUTION)

// include the revolution socket header
#include "../gsPlatform.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// static variables
static int GSIRevolutionErrno;

// prototypes of static functions
static int CheckRcode(int rcode, int errCode);

#define REVOlUTION_SOCKET_ERROR -1

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// parses rcode into a generic -1 error if an error has occured
static int CheckRcode(int rcode, int errCode)
{
	if(rcode >= 0)
		return rcode;
	GSIRevolutionErrno = rcode;
	return errCode;
}

int socket(int pf, int type, int protocol)
{
	int rcode = SOSocket(pf, type, 0);
	GSI_UNUSED(protocol);
	return CheckRcode(rcode, INVALID_SOCKET);
}
int closesocket(SOCKET sock)
{
	int rcode = SOClose(sock);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
int shutdown(SOCKET sock, int how)
{
	int rcode = SOShutdown(sock, how);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
int bind(SOCKET sock, const SOCKADDR* addr, int len)
{
	SOCKADDR localAddr;
	int rcode;

	// with Revolution, don't bind to 0, just start using the port
	if(((const SOCKADDR_IN*)addr)->port == 0)
		return 0;

	memcpy(&localAddr, addr, sizeof(SOCKADDR));
	localAddr.len = (u8)len;

	rcode = SOBind(sock, &localAddr);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}

int connect(SOCKET sock, const SOCKADDR* addr, int len)
{
	SOCKADDR remoteAddr;
	int rcode;

	memcpy(&remoteAddr, addr, sizeof(SOCKADDR));
	remoteAddr.len = (u8)len;

	rcode = SOConnect(sock, &remoteAddr);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
int listen(SOCKET sock, int backlog)
{
	int rcode = SOListen(sock, backlog);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
SOCKET accept(SOCKET sock, SOCKADDR* addr, int* len)
{
	int rcode;
	addr->len = (u8)*len;
	rcode = SOAccept(sock, addr);
	*len = addr->len;
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}

int recv(SOCKET sock, char* buf, int len, int flags)
{
	int rcode = SORecv(sock, buf, len, flags);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
int recvfrom(SOCKET sock, char* buf, int len, int flags, SOCKADDR* addr, int* fromlen)
{
	int rcode;
	addr->len = (u8)*fromlen;
	rcode = SORecvFrom(sock, buf, len, flags, addr);
	*fromlen = addr->len;
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
SOCKET send(SOCKET sock, const char* buf, int len, int flags)
{
	int rcode = SOSend(sock, buf, len, flags);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
SOCKET sendto(SOCKET sock, const char* buf, int len, int flags, const SOCKADDR* addr, int tolen)
{
	SOCKADDR remoteAddr;
	int rcode;

	memcpy(&remoteAddr, addr, sizeof(SOCKADDR));
	remoteAddr.len = (u8)tolen;

	rcode = SOSendTo(sock, buf, len, flags, &remoteAddr);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}

int getsockopt(SOCKET sock, int level, int optname, char* optval, int* optlen)
{
	int rcode = SOGetSockOpt(sock, level, optname, optval, optlen);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}
SOCKET setsockopt(SOCKET sock, int level, int optname, const char* optval, int optlen)
{
	int rcode = SOSetSockOpt(sock, level, optname, optval, optlen);
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}

int getsockname(SOCKET sock, SOCKADDR* addr, int* len)
{
	int rcode;
	addr->len = (u8)*len;
	rcode = SOGetSockName(sock, addr);
	*len = addr->len;
	return CheckRcode(rcode, REVOlUTION_SOCKET_ERROR);
}

unsigned long inet_addr(const char* name)
{
	int rcode;
	SOInAddr addr;
	rcode = SOInetAtoN(name, &addr);
	if(rcode == FALSE)
		return INADDR_NONE;
	return addr.addr;
}

int GOAGetLastError(SOCKET sock)
{
	GSI_UNUSED(sock);
	return GSIRevolutionErrno;
}

int GSISocketSelect(SOCKET theSocket, int* theReadFlag, int* theWriteFlag, int* theExceptFlag)
{
	SOPollFD pollFD;
	int rcode;
	
	pollFD.fd = theSocket;
	pollFD.events = 0;
	if(theReadFlag != NULL)
		pollFD.events |= SO_POLLRDNORM;
	if(theWriteFlag != NULL)
		pollFD.events |= SO_POLLWRNORM;
	pollFD.revents = 0;

	rcode = SOPoll(&pollFD, 1, 0);
	if(rcode < 0)
		return REVOlUTION_SOCKET_ERROR;

	if(theReadFlag != NULL)
	{
		if((rcode > 0) && (pollFD.revents & (SO_POLLRDNORM|SO_POLLHUP)))
			*theReadFlag = 1;
		else
			*theReadFlag = 0;
	}
	if(theWriteFlag != NULL)
	{
		if((rcode > 0) && (pollFD.revents & SO_POLLWRNORM))
			*theWriteFlag = 1;
		else
			*theWriteFlag = 0;
	}
	if(theExceptFlag != NULL)
	{
		if((rcode > 0) && (pollFD.revents & SO_POLLERR))
			*theExceptFlag = 1;
		else
			*theExceptFlag = 0;
	}
	return rcode;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // _REVOLUTION