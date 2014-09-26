
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(_X360)
char * inet_ntoa(IN_ADDR in_addr)
{
	static char buffer[16];
	sprintf(buffer, "%d.%d.%d.%d", in_addr.S_un.S_un_b.s_b1, in_addr.S_un.S_un_b.s_b2, 
		in_addr.S_un.S_un_b.s_b3, in_addr.S_un.S_un_b.s_b4);
	return buffer;
}

struct hostent * gethostbyname(const char* name)
{
	XNDNS *pxndns;
	static HOSTENT host;
	HOSTENT *rvalue;

	if(XNetDnsLookup(name, NULL, &pxndns) != 0)
		return NULL;

	while (pxndns->iStatus == WSAEINPROGRESS)
	{
		msleep(5);
	}

	if ((pxndns->iStatus == 0) && (pxndns->cina > 0))
	{
		static char * ipPtrs[2];
		static IN_ADDR ip;

		host.h_name = (char*)name;
		host.h_aliases = NULL;
		host.h_addrtype = AF_INET;
		host.h_length = (gsi_u16)sizeof(IN_ADDR);
		host.h_addr_list = (gsi_i8 **)ipPtrs;

		ip = pxndns->aina[0];
		ipPtrs[0] = (char *)&ip;
		ipPtrs[1] = NULL;

		rvalue = &host;
	}
	else
	{
		rvalue = NULL;
	}
	XNetDnsRelease(pxndns);

	return rvalue;
}
#endif