#include "sb_serverbrowsing.h"
#include "sb_internal.h"

#ifdef GSI_MANIC_DEBUG
// Make sure the server isn't already in the fifo
void FIFODebugCheckAdd(SBServerFIFO *fifo, SBServer server)
{
	SBServer aServer = fifo->first;
	while(aServer != NULL)
	{
		assert(aServer != server);
		aServer = aServer->next;
	}
}


// Verify the contents of the fifo
void FIFODebugCheck(SBServerFIFO *fifo)
{
	int i=0;
	SBServer aServer;

	assert(fifo != NULL);
	aServer = fifo->first;
	for (i=0; i < fifo->count; i++)
	{
		assert(aServer != NULL);
		aServer = aServer->next;
	}
}
#else
#define FIFODebugCheckAdd(a,b)
#define FIFODebugCheck(a)
#endif

//FIFO Queue management functions
static void FIFOAddRear(SBServerFIFO *fifo, SBServer server)
{
	FIFODebugCheckAdd(fifo, server);

	if (fifo->last != NULL)
		fifo->last->next = server;
	fifo->last = server;
	server->next = NULL;
	if (fifo->first == NULL)
		fifo->first = server;
	fifo->count++;

	FIFODebugCheck(fifo);
}

static void FIFOAddFront(SBServerFIFO *fifo, SBServer server)
{
	FIFODebugCheckAdd(fifo, server);

	server->next = fifo->first;
	fifo->first = server;
	if (fifo->last == NULL)
		fifo->last = server;
	fifo->count++;

	FIFODebugCheck(fifo);
}

static SBServer FIFOGetFirst(SBServerFIFO *fifo)
{
	SBServer hold;
	hold = fifo->first;
	if (hold != NULL)
	{
		fifo->first = hold->next;
		if (fifo->first == NULL)
			fifo->last = NULL;
		fifo->count--;
	}

	FIFODebugCheck(fifo);
	return hold;
}

static SBBool FIFORemove(SBServerFIFO *fifo, SBServer server)
{
	SBServer hold, prev;
	prev = NULL;
	hold = fifo->first;
	while (hold != NULL)
	{
		if (hold == server) //found
		{
			if (prev != NULL) //there is a previous..
				prev->next = hold->next;
			if (fifo->first == hold)
				fifo->first = hold->next;
			if (fifo->last == hold)
				fifo->last = prev;
			fifo->count--;
		//	assert((fifo->count == 0 && fifo->first == NULL && fifo->last == NULL) || fifo->count > 0);
			return SBTrue;
		}
		prev = hold;
		hold = hold->next;
	}

	FIFODebugCheck(fifo);
	return SBFalse;
}

static void FIFOClear(SBServerFIFO *fifo)
{
	fifo->first = fifo->last = NULL;
	fifo->count = 0;

	FIFODebugCheck(fifo);
}

#ifdef SB_ICMP_SUPPORT
static unsigned short IPChecksum(const unsigned short *buf, int len)
{
	unsigned long cksum = 0;
	
	//Calculate the checksum
	while (len > 1)
	{
		cksum += *buf++;
		len -= sizeof(unsigned short);
	}
	
	//If we have one char left
	if (len) {
		cksum += *(unsigned char*)buf;
	}
	
	//Complete the calculations
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	
	//Return the value (inversed)
	return (unsigned short)(~cksum);
}
#endif

static void QEStartQuery(SBQueryEngine *engine, SBServer server)
{
	unsigned char queryBuffer[256];
	int queryLen;
	gsi_bool querySuccess = gsi_false;
	struct sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	server->updatetime = current_time();

	if (server->state & STATE_PENDINGICMPQUERY) //send an ICMP ping request
	{
#ifdef SB_ICMP_SUPPORT
	#if !defined(SN_SYSTEMS)
		SBICMPHeader *_icmp = (SBICMPHeader *)(queryBuffer);
		//todo: alignment issues on PS2
		_icmp->type = SB_ICMP_ECHO;
		_icmp->code = 0;
		_icmp->un.idseq = server->updatetime; //no need for network byte order since only we read the reply
		_icmp->cksum = 0;
		queryLen = sizeof(SBICMPHeader) + 6;
		memcpy(queryBuffer + sizeof(SBICMPHeader), &server->publicip, 4); //put some data in the echo packet that we can use to verify the reply
		memcpy(queryBuffer + sizeof(SBICMPHeader) + 4, &server->publicport, 2); 
		_icmp->cksum = IPChecksum((unsigned short *)queryBuffer, queryLen);
		if (SBServerGetFlags(server) & ICMP_IP_FLAG) //there is a special ICMP address
		{
			saddr.sin_addr.s_addr = server->icmpip;
		} else
		{
			saddr.sin_addr.s_addr = server->publicip;
		}

		sendto(engine->icmpsock, (char *)queryBuffer, queryLen, 0, (struct sockaddr *)&saddr, sizeof(saddr));
		querySuccess = gsi_true;
	#else
		int result;
		sndev_set_ping_ip_type optval;
		optval.ip_addr = server->icmpip;

		gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
				"Attempting to send ICMP ping to %s\r\n", inet_ntoa(*((struct in_addr*)&server->icmpip)));

		result = sndev_set_options(0, SN_DEV_SET_PING_IP, (void*)&optval, sizeof(optval)); // tell SN to ping this addr
		if (result==SN_PING_FAIL)
		{
			gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
				"ICMP ping attempt failed on SNSystems (unknown error)\r\n");
			querySuccess = gsi_true; // let the SDK process failures as a timed out ping
		}
		else if (result==SN_PING_FULL)
		{
			gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
				"ICMP ping attempt failed on SNSystems (SN_PING_FULL)\r\n");
			querySuccess = gsi_false;
		}
		else
			querySuccess = gsi_true;
	#endif
#endif
	} else //send a UDP query
	{
		if (engine->queryversion == QVERSION_QR2)
		{
			if (server->state & STATE_PENDINGQUERYCHALLENGE)
			{
				// send IP verify request now, but send qr2 query later when IP verify returns
				int pos = 0;
				queryBuffer[pos++] = QR2_MAGIC_1;
				queryBuffer[pos++] = QR2_MAGIC_2;
				queryBuffer[pos++] = 0x09; // ip verify prequery
				//set the request key
				memcpy(&queryBuffer[pos], &server->updatetime, 4);
				pos += 4;
				queryLen = pos;
			}
			else
			{
				gsi_u32 challengeNBO = htonl(server->querychallenge);
				int pos = 0;

				//set the header
				queryBuffer[pos++] = QR2_MAGIC_1;
				queryBuffer[pos++] = QR2_MAGIC_2;
				queryBuffer[pos++] = 0;
				memcpy(&queryBuffer[pos], &server->updatetime, 4); //set the request key
				pos += 4;
				if (challengeNBO != 0)
				{
					memcpy(&queryBuffer[pos], &challengeNBO, 4); // set the challenge
					pos += 4;
				}
				if (server->state & STATE_PENDINGBASICQUERY) 
				{
					int i;
					queryBuffer[pos++] = (unsigned char)engine->numserverkeys;
					for (i = 0 ; i < engine->numserverkeys ; i++)
						queryBuffer[pos++] = engine->serverkeys[i];

					//don't request any player or team keys
					queryBuffer[pos++] = 0x00;
					queryBuffer[pos++] = 0x00;
					queryLen = pos;

				} else  //request all keys for everyone
				{
					queryBuffer[pos++] = 0xFF;
					queryBuffer[pos++] = 0xFF;
					queryBuffer[pos++] = 0xFF;
					
					// Tell the server we support split packets
					queryBuffer[pos++] = 0x1;
					queryLen = pos; // 11
				}
			}
		} else //GOA
		{
			if (server->state & STATE_PENDINGBASICQUERY) //original - do a \basic\info\ query
			{
				memcpy(queryBuffer, BASIC_GOA_QUERY, BASIC_GOA_QUERY_LEN);
				queryLen = BASIC_GOA_QUERY_LEN;
			} else //original - do a \status\ query
			{
				memcpy(queryBuffer, FULL_GOA_QUERY, FULL_GOA_QUERY_LEN);
				queryLen = FULL_GOA_QUERY_LEN;		
			}
		}
		if (server->publicip == engine->mypublicip && (server->flags & PRIVATE_IP_FLAG)) //try querying the private IP
		{
			saddr.sin_addr.s_addr = server->privateip;
			saddr.sin_port = server->privateport;
		} else
		{
			saddr.sin_addr.s_addr = server->publicip;
			saddr.sin_port = server->publicport;
		}
		sendto(engine->querysock, (char *)queryBuffer, queryLen, 0, (struct sockaddr *)&saddr, sizeof(saddr));
		querySuccess = gsi_true;
	}

	//add it to the query list
	if (gsi_is_true(querySuccess))
		FIFOAddRear(&engine->querylist, server);
	else
		server->updatetime = 0;
}


void SBQueryEngineInit(SBQueryEngine *engine, int maxupdates, int queryversion, SBBool lanBrowse, SBEngineCallbackFn callback, void *instance)
{
	// 11-03-2004 : Added by Saad Nader
	// fix for LANs and unnecessary availability check
	///////////////////////////////////////////////////
	if(lanBrowse == SBFalse)
	{
		if(__GSIACResult != GSIACAvailable)
		return;
	}
		SocketStartUp();
	engine->queryversion = queryversion;
	engine->maxupdates = maxupdates;
	engine->numserverkeys = 0;
	engine->ListCallback = callback;
	engine->instance = instance;
	engine->mypublicip = 0;
	engine->querysock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#if defined(SB_ICMP_SUPPORT)
	#if defined(SN_SYSTEMS)
	{
		// reset SNSystems internal ICMP ping structures
		sndev_set_ping_reset_type optval;
		optval.timeout_ms = MAX_QUERY_MSEC; // this gets rounded up to 3 sec
		optval.reserved = 0;
		sndev_set_options(0, SN_DEV_SET_PING_RESET, &optval, sizeof(optval));
	}
	#else
	engine->icmpsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	#endif
#endif
	FIFOClear(&engine->pendinglist);
	FIFOClear(&engine->querylist);
}

void SBQueryEngineSetPublicIP(SBQueryEngine *engine, goa_uint32 mypublicip)
{
	engine->mypublicip = mypublicip;
}

void SBEngineHaltUpdates(SBQueryEngine *engine)
{
	FIFOClear(&engine->pendinglist);
	FIFOClear(&engine->querylist);
}


void SBEngineCleanup(SBQueryEngine *engine)
{
	closesocket(engine->querysock);
#ifdef SB_ICMP_SUPPORT
	#if !defined(SN_SYSTEMS)
	closesocket(engine->icmpsock);
	#endif
#endif
	engine->querysock = INVALID_SOCKET;
	FIFOClear(&engine->pendinglist);
	FIFOClear(&engine->querylist);
}


//NOTE: the server must not be in the pending or update list currently!
void SBQueryEngineUpdateServer(SBQueryEngine *engine, SBServer server, int addfront, int querytype, SBBool usequerychallenge)
{
	// Assert state of FIFOs
	FIFODebugCheckAdd(&engine->pendinglist, server);
	FIFODebugCheckAdd(&engine->querylist, server);

	server->state &= (unsigned char)~(STATE_PENDINGBASICQUERY|STATE_PENDINGFULLQUERY|STATE_PENDINGICMPQUERY|STATE_PENDINGQUERYCHALLENGE|STATE_QUERYFAILED); //clear out these flags
	server->splitResponseBitmap = 0;
	server->querychallenge = 0;

#ifndef SB_ICMP_SUPPORT
	if (querytype == QTYPE_ICMP)
		return; // ICMP not supported
#endif

	if (querytype == QTYPE_BASIC)
		server->state |= STATE_PENDINGBASICQUERY;
	else if (querytype == QTYPE_FULL)
		server->state |= STATE_PENDINGFULLQUERY;
	else if (querytype == QTYPE_ICMP)
		server->state |= STATE_PENDINGICMPQUERY;
	else
		return; // hoterror: unsupported querytype!
		
	if (usequerychallenge && (querytype == QTYPE_FULL || querytype == QTYPE_BASIC))
		server->state |= STATE_PENDINGQUERYCHALLENGE;
	
	if (engine->querylist.count < engine->maxupdates) //add it now..
	{
		QEStartQuery(engine, server);
		return;
	}
	//else need to queue it
	
	if (addfront)
		FIFOAddFront(&engine->pendinglist, server);
	else
		FIFOAddRear(&engine->pendinglist, server);
}

SBServer SBQueryEngineUpdateServerByIP(SBQueryEngine *engine, const char *ip, unsigned short queryport, int addfront, int querytype, SBBool usequerychallenge)
{
	//need to create a new server
	SBServer server;
	goa_uint32 ipaddr;
	ipaddr = inet_addr(ip);
	server = SBAllocServer(NULL, ipaddr, htons(queryport));
	server->flags = UNSOLICITED_UDP_FLAG; //we assume we can talk directly to it
	SBQueryEngineUpdateServer(engine, server, addfront, querytype, usequerychallenge);
	return server;
}


static void ParseSingleQR2Reply(SBQueryEngine *engine, SBServer server, char *data, int len)
{
	int i;
	int dlen;

	// 0x00 == qr2 query response, 0x09 == qr2 challenge response
	if (data[0] != 0x00 && data[0] != 0x09)
		return;

	//we could test the request key here for added security, or skip
	data += 5;
	len -= 5;
	if (server->state & STATE_PENDINGQUERYCHALLENGE)
	{
		server->state &= (unsigned char)~(STATE_PENDINGQUERYCHALLENGE);

		if (len > 0)
		{
			server->querychallenge = (gsi_u32)atoi(data);
			FIFORemove(&engine->querylist, server); // remove it
			QEStartQuery(engine, server); // readd it with a keys query
			engine->ListCallback(engine, qe_challengereceived, server, engine->instance);
			return;
		}
	}
	else if (server->state & STATE_PENDINGBASICQUERY)
	{
		//need to pick out the keys they selected
		for (i = 0 ; i < engine->numserverkeys ; i++)
		{
			dlen = NTSLengthSB(data, len);
			if (dlen < 0)
				break;
			//add the value if its not a Query-From-Master-Only key
			if (!qr2_internal_is_master_only_key(qr2_registered_key_list[engine->serverkeys[i]]))
			{
				SBServerAddKeyValue(server, qr2_registered_key_list[engine->serverkeys[i]], data);
			}
			data += dlen;
			len -= dlen;
		}
		server->state |= STATE_BASICKEYS|STATE_VALIDPING;
	}
	else //need to parse out all the keys
	{
		// Is this a split packet format?
		if (*data && strncmp("splitnum", data, 8)==0)
		{
			SBServerParseQR2FullKeysSplit(server, data, len);
			if (server->splitResponseBitmap != 0xFF)
				return;
			server->state |= STATE_FULLKEYS|STATE_BASICKEYS|STATE_VALIDPING;
		}
		else
		{
			// single packet
			SBServerParseQR2FullKeysSingle(server, data, len);
			server->state |= STATE_FULLKEYS|STATE_BASICKEYS|STATE_VALIDPING;
		}
	}
	server->state &= (unsigned char)~(STATE_PENDINGBASICQUERY|STATE_PENDINGFULLQUERY);
	server->updatetime = current_time() - server->updatetime;
	FIFORemove(&engine->querylist, server);
	engine->ListCallback(engine, qe_updatesuccess, server, engine->instance);
}

static void ParseSingleGOAReply(SBQueryEngine *engine, SBServer server, char *data, int len)
{
	int isfinal;
	//need to check before parse as it will modify the string
	isfinal = (strstr(data,"\\final\\") != NULL);
	SBServerParseKeyVals(server, data);
	if (isfinal)
	{
		if (server->state & STATE_PENDINGBASICQUERY)
			server->state |= STATE_BASICKEYS|STATE_VALIDPING;
		else
			server->state |= STATE_FULLKEYS|STATE_VALIDPING;
		server->state &= (unsigned char)~(STATE_PENDINGBASICQUERY|STATE_PENDINGFULLQUERY);
		server->updatetime = current_time() - server->updatetime;
		FIFORemove(&engine->querylist, server);
		engine->ListCallback(engine, qe_updatesuccess, server, engine->instance);
	}
	
	GSI_UNUSED(len);
}

static SBBool ParseSingleICMPReply(SBQueryEngine *engine, SBServer server, char *data, int len)
{
#ifdef SB_ICMP_SUPPORT
	SBIPHeader *ipheader = (SBIPHeader *)data;
	SBICMPHeader *icmpheader;
	int ipheaderlen;
	goa_uint32 packetpublicip;
	unsigned short packetpublicport;
	//todo: byte alignment on PS2
	ipheaderlen = (gsi_u8)(ipheader->ip_hl_ver & 15);
	ipheaderlen *= 4;
	icmpheader = (SBICMPHeader *)(data + ipheaderlen);
	if (icmpheader->type != SB_ICMP_ECHO_REPLY)
		return SBFalse;
	if (icmpheader->un.idseq != server->updatetime)
		return SBFalse;
	if (len < ipheaderlen + (int)sizeof(SBICMPHeader) + 6)
		return SBFalse; //not enough data
	//check the server IP and port
	memcpy(&packetpublicip, data + ipheaderlen + sizeof(SBICMPHeader), 4);
	memcpy(&packetpublicport, data + ipheaderlen + sizeof(SBICMPHeader) + 4, 2);
	if (packetpublicport != server->publicport || packetpublicip != server->publicip)
		return SBFalse;
	//else its a valid echo
	server->updatetime = current_time() - server->updatetime;
	server->state |= STATE_VALIDPING;
	server->state &= (unsigned char)~(STATE_PENDINGICMPQUERY);	
	FIFORemove(&engine->querylist, server);
	engine->ListCallback(engine, qe_updatesuccess, server, engine->instance);
#else
	GSI_UNUSED(engine);
	GSI_UNUSED(server);
	GSI_UNUSED(data);
	GSI_UNUSED(len);
#endif
	return SBTrue;
	
}

#if defined(SN_SYSTEMS) && defined(SB_ICMP_SUPPORT)
static void ProcessIncomingICMPReplies(SBQueryEngine *engine)
{
	SBServer server;
	int result = 0;
	int found  = 0;
	int i      = 0;
	sndev_stat_ping_times_type optval;
	gsi_i32 optsize = sizeof(optval);

	// Get the ICMP replies from the SNSystems stack
	result = sndev_get_status(0, SN_DEV_STAT_PING_TIMES, (void*)&optval, &optsize);
	if (result != 0)
	{
		gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
			"Failed on sndev_get_status (checking ICMP pings): %d\r\n", result);
		return;
	}
	if (optval.num_entries == 0)
		return; // no outstanding pings (according to sn_systems)

	// match servers to ping responses
	for (server = engine->querylist.first; server != NULL; server = server->next)
	{
		if ((server->state & STATE_PENDINGICMPQUERY) == 0 ||
			(server->flags & ICMP_IP_FLAG) == 0)
			continue; // server not flagged for ICMP

		// find this server
		for (i=0; i<optval.num_entries; i++)
		{
			if (server->icmpip == optval.times[i].ip_addr)
			{
				if (optval.times[i].status == SN_PING_TIMES_CODE_GOTREPLY)
				{
					server->updatetime = optval.times[i].time_ms;
					server->state |= STATE_VALIDPING;
					server->state &= (unsigned char)~(STATE_PENDINGICMPQUERY);
					FIFORemove(&engine->querylist, server);
					engine->ListCallback(engine, qe_updatesuccess, server, engine->instance);
				}
				//else
				//   let query engine timeout queries on its own (for simplicity)

				found++;
				if (found == optval.num_entries)
					return; // found them all
			}
		}
	}
}
#endif // SN_SYSTEMS && SB_ICMP_SUPPORT

static void ProcessIncomingReplies(SBQueryEngine *engine, SBBool icmpSocket)
{
	int i;
	char indata[MAX_RECVFROM_SIZE]; 
	struct sockaddr_in saddr;
	int saddrlen = sizeof(saddr);
	SBServer server;
	SOCKET recvSock = 0;

	if (icmpSocket)
	{
		#ifdef SB_ICMP_SUPPORT
			#if defined(SN_SYSTEMS) 
				ProcessIncomingICMPReplies(engine);
				return;
			#else
				recvSock = engine->icmpsock;
			#endif
		#endif
	}
	else
	{
		recvSock = engine->querysock;
	}
	
	// Process all information in the socket buffer 
	while(CanReceiveOnSocket(recvSock))
	{
		i = (int)recvfrom(recvSock, indata, sizeof(indata) - 1, 0, (struct sockaddr *)&saddr, &saddrlen);

		if (gsiSocketIsError(i))
			break;
		indata[i] = 0;
		//find the server in our query list
		for (server = engine->querylist.first ; server != NULL ; server = server->next)
		{
			if ((icmpSocket && (server->flags & ICMP_IP_FLAG) && server->icmpip == saddr.sin_addr.s_addr) || //if it's an ICMP query and it matches the ICMP address			
				(server->publicip == saddr.sin_addr.s_addr && (server->publicport == saddr.sin_port || icmpSocket)) || //if it matches public - port doesnt need to match for ICMP
				(server->publicip == engine->mypublicip && (server->flags & PRIVATE_IP_FLAG) && server->privateip == saddr.sin_addr.s_addr && server->privateport == saddr.sin_port)) //or has a private, and matches
			{
				if (icmpSocket)
				{
					if (ParseSingleICMPReply(engine, server, indata, i))
						break; //only break if it matches exactly, since we may have multiple outstanding pings to the same ICMPIP for different servers!
				} else
				{
					if (engine->queryversion == QVERSION_QR2)
						ParseSingleQR2Reply(engine, server, indata, i);
					else
						ParseSingleGOAReply(engine, server, indata, i);
					break;
				}
			}
		}		
	}


}

static void TimeoutOldQueries(SBQueryEngine *engine)
{
	gsi_time ctime = current_time();
	while (engine->querylist.first != NULL)
	{
		if (ctime > engine->querylist.first->updatetime + MAX_QUERY_MSEC)
		{
			engine->querylist.first->flags |= STATE_QUERYFAILED;
			engine->querylist.first->updatetime = MAX_QUERY_MSEC;
			engine->querylist.first->flags  &= (unsigned char)~(STATE_PENDINGBASICQUERY|STATE_PENDINGFULLQUERY|STATE_PENDINGICMPQUERY);
			engine->ListCallback(engine, qe_updatefailed, engine->querylist.first, engine->instance);			
			FIFOGetFirst(&engine->querylist);
		} else
			break; //since servers are added in FIFO order, nothing later can have already expired
	}
}

static void QueueNextQueries(SBQueryEngine *engine)
{
	while (engine->querylist.count < engine->maxupdates && engine->pendinglist.count > 0)
	{
		SBServer server = FIFOGetFirst(&engine->pendinglist);
		QEStartQuery(engine, server);
	}
}

void SBQueryEngineThink(SBQueryEngine *engine)
{
	if (engine->querylist.count == 0) //not querying anything - we can go away
		return;
	ProcessIncomingReplies(engine, SBFalse);
#ifdef SB_ICMP_SUPPORT
	ProcessIncomingReplies(engine, SBTrue);
#endif
	TimeoutOldQueries(engine);
	if (engine->pendinglist.count > 0)
		QueueNextQueries(engine);
	if (engine->querylist.count == 0) //we are now idle..
		engine->ListCallback(engine, qe_engineidle, NULL, engine->instance);
}

void SBQueryEngineAddQueryKey(SBQueryEngine *engine, unsigned char keyid)
{
	if (engine->numserverkeys < MAX_QUERY_KEYS)
		engine->serverkeys[engine->numserverkeys++] = keyid;
}


//remove a server from our update FIFOs
void SBQueryEngineRemoveServerFromFIFOs(SBQueryEngine *engine, SBServer server)
{
	SBBool ret;

	// remove the server from the current query list
	ret = FIFORemove(&engine->querylist, server);
	if(ret)
		return; // -- Caution: assumes that server will not be in pendinglist
	FIFORemove(&engine->pendinglist, server);
}
