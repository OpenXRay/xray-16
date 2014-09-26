

#include "../natneg.h"
#include "../../common/gsAvailable.h"

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

int connected = 0;
struct sockaddr_in otheraddr;
SOCKET sock = INVALID_SOCKET;
unsigned int ESTRING_SIZE = 128;
char *aString = NULL;
#define USE_OWN_SOCK
#define DETECT_NAT

#ifdef GSI_COMMON_DEBUG
	static void DebugCallback(GSIDebugCategory theCat, GSIDebugType theType,
	                          GSIDebugLevel theLevel, const char * theTokenStr,
	                          va_list theParamList)
	{
		GSI_UNUSED(theLevel);

		printf("[%s][%s] ", 
				gGSIDebugCatStrings[theCat], 
				gGSIDebugTypeStrings[theType]);

		vprintf(theTokenStr, theParamList);
	}
#endif

static void tryread(SOCKET s)
{
	char buf[256];
	int len;
	struct sockaddr_in saddr;
	int saddrlen = sizeof(saddr);
	while (CanReceiveOnSocket(s))
	{
		len = recvfrom(s, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&saddr, &saddrlen);

		if (len < 0)
		{
			len = GOAGetLastError(s);
			printf("|Got recv error: %d\n", len);
			break;
		}
		buf[len] = 0;
		if (memcmp(buf, NNMagicData, NATNEG_MAGIC_LEN) == 0)
		{
			NNProcessData(buf, len, &saddr);
		} else
			printf("|Got data (%s:%d): %s\n", inet_ntoa(saddr.sin_addr),ntohs(saddr.sin_port), buf);
	}
}

static void pc(NegotiateState state, void *userdata)
{
	if (aString != NULL)
	{
		// Print any status updates to the console
		int charsWritten;

		memset(aString, 0, ESTRING_SIZE);
		charsWritten = sprintf(aString, "Got State Update: ");
		switch(state)
		{
		case ns_initack:
			sprintf(aString + charsWritten,  "ns_initack, Init Packets Acknowledged.\n");
			break;
		case ns_connectping:
			sprintf(aString + charsWritten, "ns_connectping, Starting connection and pinging other machine.\n");
			break;
		default:
			break;
		}
		
		gsDebugFormat(GSIDebugCat_App, GSIDebugType_Misc, GSIDebugLevel_Notice,
			aString);
	}

	GSI_UNUSED(userdata);
}

static void cc(NegotiateResult result, SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *userdata)
{
	struct sockaddr_in saddr;
	int namelen = sizeof(saddr);
	if (gamesocket != INVALID_SOCKET)
	{
		getsockname(gamesocket, (struct sockaddr *)&saddr, &namelen);

		printf("|Local game socket: %d\n", ntohs(saddr.sin_port));
	}
	
	if (result != nr_success)
	{
		if (aString == NULL)
			return;
		memset(aString, 0, ESTRING_SIZE);
		switch(result)
		{
		case nr_deadbeatpartner:
			sprintf(aString,  "Result: nr_deadbeatpartner, The other machine isn't responding.\n");
			break;
		case nr_inittimeout:
			sprintf(aString, "Result: nr_inittimeout, The NAT server could not be contacted.\n");
			break;
		case nr_pingtimeout:
			sprintf(aString, "Result: nr_pingtimeout, The other machine could not be contacted.\n");
			break;
		default:
			break;
		}
		
		gsDebugFormat(GSIDebugCat_App, GSIDebugType_Misc, GSIDebugLevel_Notice,	aString);
		return;
	}

	printf("|Got connected, remoteaddr: %s, remoteport: %d\n", (remoteaddr == NULL) ? "" : inet_ntoa(remoteaddr->sin_addr), (remoteaddr == NULL) ? 0 : ntohs(remoteaddr->sin_port));
	if (result == nr_success)
	{
		connected = 1;
		memcpy(&otheraddr, remoteaddr, sizeof(otheraddr));
		sock = gamesocket;
	}
	
	GSI_UNUSED(userdata);
}

static void nr(gsi_bool success, NAT nat)
{
	int charsWritten;

	if(success == gsi_false)
	{
		gsDebugFormat(GSIDebugCat_App, GSIDebugType_Misc, GSIDebugLevel_Notice,	
			"Failed to fully detect the NAT.  Please try the detection again.\n");
		return;
	}

	if(aString == NULL)
		return;

	memset(aString, 0, ESTRING_SIZE);
	charsWritten = sprintf(aString, "The detected NAT is a ");
	switch(nat.natType)
	{
	case no_nat:
		charsWritten = sprintf(aString, "No NAT detected.");
		break;
	case firewall_only:
		charsWritten = sprintf(aString, "No NAT detected, but firewall may be present.");
		break;
	case full_cone:
		charsWritten += sprintf(aString + charsWritten, "full cone ");
		break;
	case restricted_cone:
		charsWritten += sprintf(aString + charsWritten, "restricted cone ");
		break;
	case port_restricted_cone:
		charsWritten += sprintf(aString + charsWritten, "port restricted cone ");
		break;
	case symmetric:
		charsWritten += sprintf(aString + charsWritten, "symmetric ");
		break;
	case unknown:
	default:
		charsWritten = sprintf(aString, "Unknown NAT type detected ");
		break;
	}

	if(nat.natType != no_nat && nat.natType != firewall_only)
		switch(nat.mappingScheme)
		{
		case private_as_public:
			charsWritten += sprintf(aString + charsWritten, "and is using the private port as the public port.");
			break;
		case consistent_port:
			charsWritten += sprintf(aString + charsWritten, "and is using the same public port for all requests from the same private port.");
			break;
		case incremental:
			charsWritten += sprintf(aString + charsWritten, "and is using an incremental port mapping scheme.");
			break;
		case mixed:
			charsWritten += sprintf(aString + charsWritten, "and is using a mixed port mapping scheme.");
			break;
		case unrecognized:
		default:
			charsWritten += sprintf(aString + charsWritten, "and is using an unrecognized port mapping scheme.");
			break;
		}

	charsWritten += sprintf(aString + charsWritten, "\n");
	gsDebugFormat(GSIDebugCat_App, GSIDebugType_Misc, GSIDebugLevel_Notice,	aString);
}

#ifdef __MWERKS__ // CodeWarrior will warn if not prototyped
	int test_main(int argc, char **argp);
#endif
int test_main(int argc, char **argp)
{
	unsigned long lastsendtime = 0;
	GSIACResult result;
	gsi_time startTime;
	NegotiateError error;
	
#ifdef GSI_COMMON_DEBUG
	// Define GSI_COMMON_DEBUG if you want to view the SDK debug output
	// Set the SDK debug log file, or set your own handler using gsSetDebugCallback
	//gsSetDebugFile(stdout); // output to console
	gsSetDebugCallback(DebugCallback);

	// Set debug levels
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Verbose);
#endif
	
#ifdef GSI_MEM_MANAGED	// use gsi mem managed
	{
		#define MEMPOOL_SIZE (8* 1024*1024)
		PRE_ALIGN(16) static char _mempool[MEMPOOL_SIZE]	POST_ALIGN(16);
		gsMemMgrCreate	(gsMemMgrContext_Default, "default",
			_mempool, MEMPOOL_SIZE);
	}
#endif

	aString = (char *)gsimalloc(ESTRING_SIZE);
	// check that the game's backend is available
	GSIStartAvailableCheck("gmtest");
	while((result = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(result != GSIACAvailable)
	{
		printf("The backend is not available\n");
		return 1;
	}

#ifdef DETECT_NAT
	error = NNStartNatDetection(nr);
#endif

#ifdef USE_OWN_SOCK
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	error = NNBeginNegotiationWithSocket(sock, 666, (argc == 1) ? 0 : 1, pc, cc, NULL);
#else
	error = NNBeginNegotiation(666, (argc == 1) ? 0 : 1, pc, cc, NULL);
#endif

	if(error != ne_noerror)
	{
		int charsWritten;
		memset(aString, 0, ESTRING_SIZE);
		charsWritten = sprintf(aString, "Error beginning negotiation: ");
		switch(error)
		{
			case ne_allocerror:
				sprintf(aString + charsWritten, "memory allocation failed.\n");
				break;
			case ne_dnserror:
				sprintf(aString + charsWritten, "DNS lookup failed.\n");
				break;
			case ne_socketerror:
				sprintf(aString + charsWritten, "socket failed to be created.\n");
				break;
			default:
				break;
		}
		
		gsDebugFormat(GSIDebugCat_App, GSIDebugType_Misc, GSIDebugLevel_Notice,	aString);
		gsifree(aString);	
		return 1;
	}
	
	startTime = current_time();
	while ((current_time() - startTime) < 60000)
	{
		NNThink();
		if (connected)
		{
			if (current_time() - lastsendtime > 2000)
			{
				int ret = sendto(sock, "woohoo!!", 8, 0, (struct sockaddr *)&otheraddr, sizeof(struct sockaddr_in));
				int error = GOAGetLastError(sock);
				printf("|Sending (%d:%d), remoteaddr: %s, remoteport: %d\n", ret, error, inet_ntoa(otheraddr.sin_addr), ntohs(otheraddr.sin_port));
				lastsendtime = current_time();
			}			
		}
		if (sock != INVALID_SOCKET)
			tryread(sock);
		msleep(10);
	}
	if (sock != INVALID_SOCKET)
		closesocket(sock);

	sock = INVALID_SOCKET;
	SocketShutDown();
	NNFreeNegotiateList();
	gsifree(aString);

	GSI_UNUSED(argp);
	return 0;
}
