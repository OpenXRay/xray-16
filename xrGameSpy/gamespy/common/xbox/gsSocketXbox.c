
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(_XBOX)
char * inet_ntoa(IN_ADDR in_addr)
{
	static char buffer[16];
	sprintf(buffer, "%d.%d.%d.%d", in_addr.S_un.S_un_b.s_b1, in_addr.S_un.S_un_b.s_b2, 
		in_addr.S_un.S_un_b.s_b3, in_addr.S_un.S_un_b.s_b4);
	return buffer;
}
#endif