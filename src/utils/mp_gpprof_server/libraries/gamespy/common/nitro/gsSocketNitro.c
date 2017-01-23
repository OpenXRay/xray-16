///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(_NITRO)


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// static variables
static int GSINitroErrno;

// prototypes of static functions
static int CheckRcode(int rcode, int errCode);

#define NITRO_SOCKET_ERROR -1

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int CheckRcode(int rcode, int errCode)
{
	if(rcode >= 0)
		return rcode;
	GSINitroErrno = rcode;
	return errCode;
}

int socket(int pf, int type, int protocol)
{
	int rcode = SOC_Socket(pf, type, protocol);
	return CheckRcode(rcode, INVALID_SOCKET);
}
int closesocket(SOCKET sock)
{
	int rcode = SOC_Close(sock);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
int shutdown(SOCKET sock, int how)
{
	int rcode = SOC_Shutdown(sock, how);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
int bind(SOCKET sock, const SOCKADDR* addr, int len)
{
	SOCKADDR localAddr;
	int rcode;

	// with nitro, don't bind to 0, just start using the port
	if(((const SOCKADDR_IN*)addr)->port == 0)
		return 0;

	memcpy(&localAddr, addr, sizeof(SOCKADDR));
	localAddr.len = (u8)len;

	rcode = SOC_Bind(sock, &localAddr);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}

int connect(SOCKET sock, const SOCKADDR* addr, int len)
{
	SOCKADDR remoteAddr;
	int rcode;

	memcpy(&remoteAddr, addr, sizeof(SOCKADDR));
	remoteAddr.len = (u8)len;

	rcode = SOC_Connect(sock, &remoteAddr);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
int listen(SOCKET sock, int backlog)
{
	int rcode = SOC_Listen(sock, backlog);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
SOCKET accept(SOCKET sock, SOCKADDR* addr, int* len)
{
	int rcode;
	addr->len = (u8)*len;
	rcode = SOC_Accept(sock, addr);
	*len = addr->len;
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}

int recv(SOCKET sock, char* buf, int len, int flags)
{
	int rcode = SOC_Recv(sock, buf, len, flags);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
int recvfrom(SOCKET sock, char* buf, int len, int flags, SOCKADDR* addr, int* fromlen)
{
	int rcode;
	addr->len = (u8)*fromlen;
	rcode = SOC_RecvFrom(sock, buf, len, flags, addr);
	*fromlen = addr->len;
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
SOCKET send(SOCKET sock, const char* buf, int len, int flags)
{
	int rcode = SOC_Send(sock, buf, len, flags);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
SOCKET sendto(SOCKET sock, const char* buf, int len, int flags, const SOCKADDR* addr, int tolen)
{
	SOCKADDR remoteAddr;
	int rcode;

	memcpy(&remoteAddr, addr, sizeof(SOCKADDR));
	remoteAddr.len = (u8)tolen;

	rcode = SOC_SendTo(sock, buf, len, flags, &remoteAddr);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}

int getsockopt(SOCKET sock, int level, int optname, char* optval, int* optlen)
{
	int rcode = SOC_GetSockOpt(sock, level, optname, optval, optlen);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}
SOCKET setsockopt(SOCKET sock, int level, int optname, const char* optval, int optlen)
{
	int rcode = SOC_SetSockOpt(sock, level, optname, optval, optlen);
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}

int getsockname(SOCKET sock, SOCKADDR* addr, int* len)
{
	int rcode;
	addr->len = (u8)*len;
	rcode = SOC_GetSockName(sock, addr);
	*len = addr->len;
	return CheckRcode(rcode, NITRO_SOCKET_ERROR);
}

unsigned long inet_addr(const char* name)
{
	int rcode;
	SOInAddr addr;
	rcode = SOC_InetAtoN(name, &addr);
	if(rcode == FALSE)
		return INADDR_NONE;
	return addr.addr;
}

int GOAGetLastError(SOCKET sock)
{
	GSI_UNUSED(sock);
	return GSINitroErrno;
}



int GSISocketSelect(SOCKET theSocket, int* theReadFlag, int* theWriteFlag, int* theExceptFlag)
{
	SOPollFD pollFD;
	int rcode;
	
	pollFD.fd = theSocket;
	pollFD.events = 0;
	if(theReadFlag != NULL)
		pollFD.events |= SOC_POLLRDNORM;
	if(theWriteFlag != NULL)
		pollFD.events |= SOC_POLLWRNORM;
	pollFD.revents = 0;

	rcode = SOC_Poll(&pollFD, 1, 0);
	if(rcode < 0)
		return NITRO_SOCKET_ERROR;

	if(theReadFlag != NULL)
	{
		if((rcode > 0) && (pollFD.revents & (SOC_POLLRDNORM|SOC_POLLHUP)))
			*theReadFlag = 1;
		else
			*theReadFlag = 0;
	}
	if(theWriteFlag != NULL)
	{
		if((rcode > 0) && (pollFD.revents & SOC_POLLWRNORM))
			*theWriteFlag = 1;
		else
			*theWriteFlag = 0;
	}
	if(theExceptFlag != NULL)
	{
		if((rcode > 0) && (pollFD.revents & SOC_POLLERR))
			*theExceptFlag = 1;
		else
			*theExceptFlag = 0;
	}
	return rcode;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // _NITRO