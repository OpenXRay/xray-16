#include <stddef.h>
#include "nninternal.h"
#include "NATify.h"

gsi_bool gotERT1, gotERT2, gotERT3;
gsi_bool gotADDRESS1a, gotADDRESS1b, gotADDRESS2, gotADDRESS3;

// prototypes for compilers that complain
const char * AddressToString(unsigned int ip, unsigned short port, char string[22]);
unsigned int NameToIp(const char *name);


const char * AddressToString(unsigned int ip, unsigned short port, char string[22])
{
	static char strAddressArray[2][22];
	static int nIndex;
	char * strAddress;

	if(string)
		strAddress = string;
	else
	{
		nIndex ^= 1;
		strAddress = strAddressArray[nIndex];
	}

	if(ip)
	{
		IN_ADDR inAddr;

		inAddr.s_addr = ip;

		if(port)
			sprintf(strAddress, "%s:%d", inet_ntoa(inAddr), port);
		else
			sprintf(strAddress, "%s", inet_ntoa(inAddr));
	}
	else if(port)
		sprintf(strAddress, ":%d", port);
	else
		strAddress[0] = '\0';

	return strAddress;
}

static unsigned int GetLocalIP()
{
	int num_local_ips;
	struct hostent *phost;
	struct in_addr *addr;
	unsigned int localip = 0;
	phost = getlocalhost();
	if (phost == NULL)
		return 0;
	for (num_local_ips = 0 ; ; num_local_ips++)
	{
		if (phost->h_addr_list[num_local_ips] == 0)
			break;
		addr = (struct in_addr *)phost->h_addr_list[num_local_ips];
		if (addr->s_addr == htonl(0x7F000001))
			continue;
		localip = addr->s_addr;

		if(IsPrivateIP(addr))
			return localip;
	}
	return localip; //else a specific private address wasn't found - return what we've got
}

static unsigned short GetLocalPort(SOCKET sock)
{
	int ret;
	struct sockaddr_in saddr;
	int saddrlen = sizeof(saddr);
	ret = getsockname(sock,(struct sockaddr *)&saddr, &saddrlen);
	if (gsiSocketIsError(ret))
		return 0;
	return saddr.sin_port;
}

unsigned int NameToIp(const char *name)
{
	unsigned int ret;
	struct hostent *hent;

	ret = inet_addr(name);

	if (ret == INADDR_NONE)
	{
		hent = gethostbyname(name);
		if (!hent)
			return 0;
		ret = *(unsigned int *)hent->h_addr_list[0];
	}
	return ret;
}

int DiscoverReachability(SOCKET sock, unsigned int ip, unsigned short port, int portType)
{
	NatNegPacket p;
	SOCKADDR_IN addr;
	int len = sizeof(SOCKADDR_IN);
	int success = 1;

	gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
		"(%d) Sending ERT Request to %s\n", portType, AddressToString(ip, port, NULL));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);

	memset(&p, 0, sizeof(p));
	p.magic[0]		= NN_MAGIC_0;
	p.magic[1]		= NN_MAGIC_1;
	p.magic[2]		= NN_MAGIC_2;
	p.magic[3]		= NN_MAGIC_3;
	p.magic[4]		= NN_MAGIC_4;
	p.magic[5]		= NN_MAGIC_5;
	p.version		= NN_PROTVER;
	p.packettype	= NN_NATIFY_REQUEST;
	p.cookie		= (int)htonl(NATIFY_COOKIE);
	p.Packet.Init.porttype	= (unsigned char)portType;

	success = sendto(sock, (const char *)&p, sizeof(p), 0, (SOCKADDR *)&addr, sizeof(addr));

	GSI_UNUSED(len);
	return(success);
}

int DiscoverMapping(SOCKET sock, unsigned int ip, unsigned short port, int portType, int id)
{
	NatNegPacket p;
	SOCKADDR_IN addr;
	int len = sizeof(SOCKADDR_IN);
	int success;

	gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
		"Sending ADDRESS CHECK %d to %s\n", id, AddressToString(ip, port, NULL));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);

	memset(&p, 0, sizeof(p));
	p.magic[0]		= NN_MAGIC_0;
	p.magic[1]		= NN_MAGIC_1;
	p.magic[2]		= NN_MAGIC_2;
	p.magic[3]		= NN_MAGIC_3;
	p.magic[4]		= NN_MAGIC_4;
	p.magic[5]		= NN_MAGIC_5;
	p.version		= NN_PROTVER;
	p.packettype	= NN_ADDRESS_CHECK;
	p.cookie		= (int)htonl(id);
	p.Packet.Init.porttype	= (unsigned char)portType;

	success = sendto(sock, (const char *)&p, sizeof(p), 0, (SOCKADDR *)&addr, sizeof(addr));

	GSI_UNUSED(len);
	return(success);
}

void OutputMapping(const AddressMapping * theMap)
{
	if(theMap == NULL)
	{
		gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment, 
			"ERROR: Port mapping not available.");
	}
	else
	{
		gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment, 
			"Discovered map: [%s] -> [%s]\n", AddressToString(theMap->privateIp, theMap->privatePort, NULL),
											  AddressToString(theMap->publicIp,  theMap->publicPort,  NULL));
	}
}

static int Think(SOCKET sock, NAT * nat)
{
	static char data[NNINBUF_LEN];
	int length;
	unsigned char ptype;
	NatNegPacket p;
	struct sockaddr_in saddr;
	int saddrlen = sizeof(struct sockaddr_in);

	// Is natification complete?
	if(gotERT1 && gotERT2 && gotERT3 && 
		gotADDRESS1a && gotADDRESS1b && gotADDRESS2 && gotADDRESS3)
	{
		// Don't need to wait any longer, got all the stuff.
		return(gsi_false);
	}

	// Process incoming data if there is any.
	if(sock != INVALID_SOCKET)
	{
		// Check if there is data.
		while(CanReceiveOnSocket(sock))
		{
			length = recvfrom(sock, data, NNINBUF_LEN, 0, (struct sockaddr *)&saddr, &saddrlen);

			if (gsiSocketIsError(length))
			{
				gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Notice,
					"RECV SOCKET ERROR: %d\n", GOAGetLastError(sock));
				break;
			}

			if (memcmp(data, NNMagicData, NATNEG_MAGIC_LEN) != 0)
				return(gsi_true);

			ptype = *(unsigned char *)(data + offsetof(NatNegPacket, packettype));

			if (length < INITPACKET_SIZE)
				return(gsi_true);
			
			if(ptype == NN_ERTTEST)
			{
				memcpy(&p, data, INITPACKET_SIZE);		

				switch(p.Packet.Init.porttype)
				{
				case NN_PT_NN1:
					// Got the solicited ERT reply.
					gotERT1 = gsi_true;
					gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
						"(1) Got the solicited ERT from: %s\n", AddressToString(saddr.sin_addr.s_addr, ntohs(saddr.sin_port), NULL));
					break;
				case NN_PT_NN2:
					// Got the unsolicited IP ERT reply.
					nat->ipRestricted = gsi_false;
					gotERT2 = gsi_true;
					gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
						"(2) Got the unsolicited (address) ERT from: %s\n", AddressToString(saddr.sin_addr.s_addr, ntohs(saddr.sin_port), NULL));
					break;
				case NN_PT_NN3:
					// Got the unsolicited IP&Port ERT reply.
					nat->portRestricted = gsi_false;
					gotERT3 = gsi_true;
					gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
						"(3) Got the unsolicited (port) ERT from: %s\n", AddressToString(saddr.sin_addr.s_addr, ntohs(saddr.sin_port), NULL));
					break;
				}

			}
			else if(ptype == NN_ADDRESS_REPLY)
			{
				memcpy(&p, data, INITPACKET_SIZE);		

				p.cookie = (int)ntohl(p.cookie);

				switch(p.cookie)
				{
				case packet_map1a:
					gotADDRESS1a = gsi_true; break;
				case packet_map1b:
					gotADDRESS1b = gsi_true; break;
				case packet_map2:
					gotADDRESS2 = gsi_true;  break;
				case packet_map3:
					gotADDRESS3 = gsi_true;  break;
				}

				nat->mappings[p.cookie].privateIp	= GetLocalIP();
				nat->mappings[p.cookie].privatePort	= ntohs(GetLocalPort(sock));
				nat->mappings[p.cookie].publicIp	= p.Packet.Init.localip;
				nat->mappings[p.cookie].publicPort	= ntohs(p.Packet.Init.localport);

				gsDebugFormat(GSIDebugCat_NatNeg, GSIDebugType_Network, GSIDebugLevel_Comment,
					"Got ADDRESS REPLY %d from: %s\n", p.cookie, AddressToString(saddr.sin_addr.s_addr, ntohs(saddr.sin_port), NULL));
				OutputMapping(&nat->mappings[p.cookie]);
			}

			if (sock == INVALID_SOCKET)
				break;
		}
	}

	return(gsi_true);
}

int NatifyThink(SOCKET sock, NAT * nat)
{
	return(Think(sock, nat));
}

gsi_bool DetermineNatType(NAT * nat)
{
	// Initialize.
	nat->natType = unknown;
	nat->promiscuity = promiscuity_not_applicable;
	nat->qr2Compatible = gsi_true;

	// Some of the address mappings are crucial in determining the NAT type.
	// If we don't have them, then we should not proceed.
	if(nat->mappings[packet_map1a].publicIp == 0 ||
	   nat->mappings[packet_map2].publicIp == 0 ||
	   nat->mappings[packet_map3].publicIp == 0)
	   return(gsi_false);

	// Is there a NAT?
	if(!nat->portRestricted && 
       !nat->ipRestricted &&
	   (nat->mappings[packet_map1a].publicIp == nat->mappings[packet_map1a].privateIp))
	{
		nat->natType = no_nat;
	}
	else if(nat->mappings[packet_map1a].publicIp == nat->mappings[packet_map1a].privateIp)
	{
		nat->natType = firewall_only;
	}
	else
	{
		// What type of NAT is it?
		if(!nat->ipRestricted && 
		   !nat->portRestricted &&
		   (abs(nat->mappings[packet_map3].publicPort - nat->mappings[packet_map2].publicPort) >= 1))
		{
			nat->natType = symmetric;
			nat->promiscuity = promiscuous;
		}
		else if(nat->ipRestricted && 
			    !nat->portRestricted && 
			    (abs(nat->mappings[packet_map3].publicPort - nat->mappings[packet_map2].publicPort) >= 1))
		{
			nat->natType = symmetric;
			nat->promiscuity = port_promiscuous;
		}
		else if(!nat->ipRestricted && 
			    nat->portRestricted && 
			    (abs(nat->mappings[packet_map3].publicPort - nat->mappings[packet_map2].publicPort) >= 1))
		{
			nat->natType = symmetric;
			nat->promiscuity = ip_promiscuous;
		}
		else if(nat->ipRestricted && 
			    nat->portRestricted && 
				(abs(nat->mappings[packet_map3].publicPort - nat->mappings[packet_map2].publicPort) >= 1))
		{
			nat->natType = symmetric;
			nat->promiscuity = not_promiscuous;
		}
		else if(nat->portRestricted)
			nat->natType = port_restricted_cone;
		else if(nat->ipRestricted && !nat->portRestricted)
			nat->natType = restricted_cone;
		else if(!nat->ipRestricted && !nat->portRestricted)
			nat->natType = full_cone;
		else
			nat->natType = unknown;
	}

	// What is the port mapping behavior?
	if(nat->mappings[packet_map1a].publicPort == nat->mappings[packet_map1a].privatePort &&
	   nat->mappings[packet_map2].publicPort == nat->mappings[packet_map2].privatePort &&
	   nat->mappings[packet_map3].publicPort == nat->mappings[packet_map3].privatePort)
		// Using private port as the public port.
		nat->mappingScheme = private_as_public;
	else if(nat->mappings[packet_map1a].publicPort == nat->mappings[packet_map2].publicPort &&
			nat->mappings[packet_map2].publicPort == nat->mappings[packet_map3].publicPort)
		// Using the same public port for all requests from the same private port.
		nat->mappingScheme = consistent_port;
	else if(nat->mappings[packet_map1a].publicPort == nat->mappings[packet_map1a].privatePort && 
			nat->mappings[packet_map3].publicPort - nat->mappings[packet_map2].publicPort == 1)
		// Using private port as the public port for the first mapping.
		// Using an incremental (+1) port mapping scheme there after.
		nat->mappingScheme = mixed;
	else if(nat->mappings[packet_map3].publicPort - nat->mappings[packet_map2].publicPort == 1)
		// Using an incremental (+1) port mapping scheme.
		nat->mappingScheme = incremental;
	else
		// Unrecognized port mapping scheme.
		nat->mappingScheme = unrecognized;

	if(nat->mappings[packet_map1b].publicPort != 0 && nat->mappings[packet_map1a].publicPort != nat->mappings[packet_map1b].publicPort)
	{
		// NOTE: The NAT maps different ports for the same destination.
		// Game servers may not be properly reported to the GameSpy backend.
		nat->qr2Compatible = gsi_false;
	}

   return(gsi_true);
}
