////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Communication Engine
#include "gsUdpEngine.h"


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Internal Structures

// Internal representation of a connection 
// Also includes a ip and port for mapping to a connection
typedef struct  
{
	unsigned int mAddr;
	unsigned short mPort;
	GT2Connection mConnection;
} GSUdpRemotePeer;

// Message handler used filter traffic to a specific SDK or part of application
typedef struct 
{
	unsigned char mInitialMsg[GS_UDP_MSG_HEADER_LEN];
	unsigned char mHeader[GS_UDP_MSG_HEADER_LEN];
	DArray mPendingConnections;
	gsUdpConnClosedCallback mClosed;
	gsUdpConnReceivedDataCallback mReceived;
	gsUdpConnConnectedCallback mConnected;
	gsUdpConnPingCallback mPingReply;
	gsUdpErrorCallback mNetworkError;
	void *mUserData;
} GSUdpMsgHandler;

// The internal representation of UDP Communication Engine
typedef struct 
{	
	GT2Socket mSocket;
	DArray mRemotePeers;
	DArray mMsgHandlers;
	gsi_bool mInitialized;
	// Application callbacks for connection that gets
	// un-handled messages 
	gsUdpConnConnectedCallback mAppConnected;
	gsUdpConnClosedCallback mAppClosed; 
	gsUdpConnPingCallback mAppPingReply;
	gsUdpConnReceivedDataCallback mAppRecvData;
	gsUdpAppConnectAttemptCallback mAppConnAttempt;
	// Error callback ?
	gsUdpErrorCallback mAppNetworkError;
	// Unknown Message 
	gsUdpUnknownMsgCallback mAppUnknownMessage;
	void *mAppUserData;
	
	// This was any easier way to keep track of pending connections
	// It does not rely on an address since it only keeps track of pending 
	// connections an app makes.
	int mAppPendingConnections;
	unsigned int mLocalAddr;
	unsigned short mLocalPort;
}GSUdpEngineObject;


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Acts as the function to return the singleton
// This function is not exposed. It will only be 
// used internally to do any modifications to the 
// GSUdpEngineObject
GSUdpEngineObject * gsUdpEngineGetEngine()
{
	static GSUdpEngineObject aUdpObject;
	return &aUdpObject;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Message Handler DArray functions
void gsUdpMsgHandlerFree(void *theMsgHandler)
{
	GSUdpMsgHandler *aHandler= (GSUdpMsgHandler *)theMsgHandler;
	ArrayFree(aHandler->mPendingConnections);
}

// Used to find a message handler based on the initial message.
int gsUdpMsgHandlerCompare(const void *theFirstHandler, const void *theSecondHandler)
{
	GSUdpMsgHandler *msgHandler1 = (GSUdpMsgHandler *)theFirstHandler, 
                    *msgHandler2 = (GSUdpMsgHandler *)theSecondHandler;
	int initCmp;
	initCmp = memcmp(msgHandler1->mInitialMsg, msgHandler2->mInitialMsg, GS_UDP_MSG_HEADER_LEN);
	return initCmp;

}

// Used to find a message handler based on the header.
int gsUdpMsgHandlerCompare2(const void *theFirstHandler, const void *theSecondHandler)
{
	GSUdpMsgHandler *msgHandler1 = (GSUdpMsgHandler *)theFirstHandler, 
                    *msgHandler2 = (GSUdpMsgHandler *)theSecondHandler;
	int headerCmp;
	headerCmp = memcmp(msgHandler1->mHeader, msgHandler2->mHeader, GS_UDP_MSG_HEADER_LEN);
	return headerCmp;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Remote Peer DArray compare functions 

// Finds a remote peer based on IP and Port
int gsUdpRemotePeerCompare(const void *theFirstPeer, const void *theSecondPeer)
{
	GSUdpRemotePeer *aPeer1 = (GSUdpRemotePeer *)theFirstPeer,
					*aPeer2 = (GSUdpRemotePeer *)theSecondPeer;
	if (aPeer1->mAddr != aPeer2->mAddr)
		return 1;
	if (aPeer1->mPort != aPeer2->mPort)
		return 1;
	
	return 0;
}

// Finds a remote peer based on a GT2Connection
int gsUdpRemotePeerCompare2(const void *theFirstPeer, const void *theSecondPeer)
{
	GSUdpRemotePeer *aPeer1 = (GSUdpRemotePeer *)theFirstPeer,
		*aPeer2 = (GSUdpRemotePeer *)theSecondPeer;
	if (aPeer1->mConnection != aPeer2->mConnection)
		return 1;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Lets the Message Handler and App know about network errors 
void gsUdpSocketError(GT2Socket theSocket)
{
	int i, len;
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	
	gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
		"[Udp Engine] Socket error, passing to app and message handlers\n");
	if (aUdp->mAppNetworkError)
		aUdp->mAppNetworkError(GS_UDP_NETWORK_ERROR, aUdp->mAppUserData);
	len = ArrayLength(aUdp->mMsgHandlers);
	for (i = 0; i < len; i++)
	{
		GSUdpMsgHandler *aMsgHandler = (GSUdpMsgHandler *)ArrayNth(aUdp->mMsgHandlers, i);
		if (aMsgHandler->mNetworkError)
			aMsgHandler->mNetworkError(GS_UDP_NETWORK_ERROR, aMsgHandler->mUserData);
	}
	GSI_UNUSED(theSocket);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Lets the App and Message Handlers know a peer left
void gsUdpClosedRoutingCB(GT2Connection theConnection, GT2CloseReason reason)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GSUdpRemotePeer aRemotePeer;
	int index, len;
	GSUdpCloseReason aReason;
	char anAddr[GS_IP_ADDR_AND_PORT];

	if (reason == GT2CommunicationError || reason == GT2SocketError)
		aReason = GS_UDP_CLOSED_BY_COMM_ERROR;
	else if (reason == GT2NotEnoughMemory)
		aReason = GS_UDP_CLOSED_BY_LOW_MEM;
	else 
		aReason = (GSUdpCloseReason)reason;

	gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
		"[Udp Engine] Connection closed to %s\n", gt2AddressToString(gt2GetRemoteIP(theConnection), 
		gt2GetRemotePort(theConnection), anAddr));
	len = ArrayLength(aUdp->mMsgHandlers);
	for (index = 0; index < len; index++)
	{
		GSUdpMsgHandler *aHandler = (GSUdpMsgHandler *)ArrayNth(aUdp->mMsgHandlers, index);
		if (aHandler->mClosed)
		{			
			gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
				"[Udp Engine] Connection closed: passed to message handler\n");
			aHandler->mClosed(gt2GetRemoteIP(theConnection), gt2GetRemotePort(theConnection), aReason, aHandler->mUserData);
		}	
	}
	
	if (aUdp->mAppClosed)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
			"[Udp Engine] Connection closed: passed to app\n");
		aUdp->mAppClosed(gt2GetRemoteIP(theConnection), gt2GetRemotePort(theConnection), aReason, aUdp->mAppUserData);
	}

	aRemotePeer.mConnection = theConnection;
	index = ArraySearch(aUdp->mRemotePeers, &aRemotePeer, gsUdpRemotePeerCompare2, 0, 0);
	if (index != NOT_FOUND)
	{
		ArrayDeleteAt(aUdp->mRemotePeers, index);
	}
	GSI_UNUSED(anAddr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// When a peer has accepted a GT2Connection the UDP layer needs to let 
// higher level app or message handler know that it accepted the request to
// to message a peer.
void gsUdpConnectedRoutingCB(GT2Connection theConnection, GT2Result theResult, GT2Byte *theMessage,
							 int theMessageLen)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	int aIndex, len;
	GSUdpErrorCode aCode;
	char anAddr[GS_IP_ADDR_AND_PORT];

	switch(theResult)
	{
		case GT2NegotiationError:
			aCode = GS_UDP_REMOTE_ERROR;
			break;
		case GT2Rejected:
			aCode = GS_UDP_REJECTED;
			break;
		case GT2TimedOut:
			aCode = GS_UDP_TIMED_OUT;
			break;
		case GT2Success:
			aCode = GS_UDP_NO_ERROR;
			break;
		default: 
			aCode = GS_UDP_UNKNOWN_ERROR;
			break;
	}
	if (theResult == GT2Rejected)
	{
		int aRemotePeerIdx;
		GSUdpRemotePeer aRemotePeer;
		aRemotePeer.mAddr = gt2GetRemoteIP(theConnection);
		aRemotePeer.mPort = gt2GetRemotePort(theConnection);
		aRemotePeerIdx = ArraySearch(aUdp->mRemotePeers, &aRemotePeer, gsUdpRemotePeerCompare, 0, 0);
		if (aRemotePeerIdx != NOT_FOUND)
		{
			gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
				"[Udp Engine] Connect rejected by %s\n", gt2AddressToString(gt2GetRemoteIP(theConnection), 
				gt2GetRemotePort(theConnection), anAddr));

			ArrayDeleteAt(aUdp->mRemotePeers, aRemotePeerIdx);
		}
	}

	len = ArrayLength(aUdp->mMsgHandlers);
	for (aIndex = 0; aIndex < len; aIndex++)
	{
		int aRemotePeerIdx;
		GSUdpRemotePeer aRemotePeer;
		GSUdpMsgHandler *aTempHandler = (GSUdpMsgHandler *)ArrayNth(aUdp->mMsgHandlers, aIndex);
		
		aRemotePeer.mAddr = gt2GetRemoteIP(theConnection);
		aRemotePeer.mPort = gt2GetRemotePort(theConnection);
		aRemotePeerIdx = ArraySearch(aTempHandler->mPendingConnections, &aRemotePeer, gsUdpRemotePeerCompare, 0, 0);
		if (aRemotePeerIdx != NOT_FOUND)
		{
			if (aTempHandler->mConnected)
			{	
				gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
					"[Udp Engine] Passing connect result to message handler\n");
				aTempHandler->mConnected(gt2GetRemoteIP(theConnection), gt2GetRemotePort(theConnection), 
					aCode, theResult == GT2Rejected ? gsi_true : gsi_false, aTempHandler->mUserData);				
			}
			ArrayDeleteAt(aTempHandler->mPendingConnections, aRemotePeerIdx);
			return;
		}
	}
	
	if (aUdp->mAppPendingConnections > 0)
	{
		if (aUdp->mAppConnected)
		{
			gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
				"[Udp Engine] Passing connect result to app\n");
			aUdp->mAppConnected(gt2GetRemoteIP(theConnection),gt2GetRemotePort(theConnection), 
				aCode, theResult == GT2Rejected ? gsi_true : gsi_false, aUdp->mAppUserData);
		}
		aUdp->mAppPendingConnections--;
	}
	GSI_UNUSED(theMessage);
	GSI_UNUSED(theMessageLen);
	GSI_UNUSED(anAddr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Pings are passed on to higher level app or message handlers
void gsUdpPingRoutingCB(GT2Connection theConnection, int theLatency)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	int index, len;
	char anAddr[GS_IP_ADDR_AND_PORT];

	gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
		"[Udp Engine] Received ping from %s\n", gt2AddressToString(gt2GetRemoteIP(theConnection), 
		gt2GetRemotePort(theConnection), anAddr));
	len = ArrayLength(aUdp->mMsgHandlers);
	for (index = 0; index < len; index++)
	{
		GSUdpMsgHandler *aHandler = (GSUdpMsgHandler *)ArrayNth(aUdp->mMsgHandlers, index);
		if (aHandler->mPingReply)
		{			
			gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
				"[Udp Engine] Passed to message handler\n");
			aHandler->mPingReply(gt2GetRemoteIP(theConnection), gt2GetRemotePort(theConnection), (unsigned int)theLatency, aHandler->mUserData);
			return;
		}	
	}

	if (aUdp->mAppPingReply)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
			"[Udp Engine] Passed to app\n");
		aUdp->mAppPingReply(gt2GetRemoteIP(theConnection), gt2GetRemotePort(theConnection), (unsigned int)theLatency, aUdp->mAppUserData);
	}
	GSI_UNUSED(anAddr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Any data received prompts the UDP layer to first find a higher level 
// message handler to handle the data.  If there was no message handler 
// found, the data is passed to the higher level app.
void gsUdpReceivedRoutingCB(GT2Connection theConnection, GT2Byte *theMessage, int theMessageLen, 
							GT2Bool reliable)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GSUdpMsgHandler aHandler;
	int index;
	char anAddr[GS_IP_ADDR_AND_PORT];	

	gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
		"[Udp Engine] Received data from %s\n", gt2AddressToString(gt2GetRemoteIP(theConnection), 
		gt2GetRemotePort(theConnection), anAddr));
	//If there is a handler, pass it to the handler
	//The header should not be stripped off
	if (theMessageLen >= GS_UDP_MSG_HEADER_LEN)
	{    
		memcpy(aHandler.mHeader, theMessage, GS_UDP_MSG_HEADER_LEN);

		index = ArraySearch(aUdp->mMsgHandlers, &aHandler, gsUdpMsgHandlerCompare2, 0, 0);
		if (index != NOT_FOUND)
		{
			GSUdpMsgHandler *aHandlerFound = (GSUdpMsgHandler *)ArrayNth(aUdp->mMsgHandlers, index);
			if (aHandlerFound->mReceived)
			{
				gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
					"[Udp Engine] Passed to message handler\n");
				aHandlerFound->mReceived(gt2GetRemoteIP(theConnection), gt2GetRemotePort(theConnection), 
					theMessage + GS_UDP_MSG_HEADER_LEN, (unsigned int)(theMessageLen - GS_UDP_MSG_HEADER_LEN), reliable, aHandlerFound->mUserData);
				return;
			}
		}
	}

	if (aUdp->mAppRecvData) 
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
			"[Udp Engine] Passed to app\n");
		aUdp->mAppRecvData(gt2GetRemoteIP(theConnection), gt2GetRemotePort(theConnection), theMessage, (unsigned int)theMessageLen, reliable,
			aUdp->mAppUserData);
	}
	GSI_UNUSED(anAddr);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Messages that are not recognized are passed only to the higher level app
// since message handlers always request to talk to peers
GT2Bool gsUdpUnrecognizedMsgCB(GT2Socket theSocket, unsigned int theIp, unsigned short thePort, GT2Byte * theMessage, 
							   int theMsgLen)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	char anAddr[GS_IP_ADDR_AND_PORT];
	if (aUdp->mAppUnknownMessage)
	{
		gsi_bool aRet;
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
			"[Udp Engine] Unknown message from %s, passing to app\n", gt2AddressToString(theIp, thePort, anAddr));
		aRet = aUdp->mAppUnknownMessage(theIp, thePort, (unsigned char *)theMessage, (unsigned int)theMsgLen, aUdp->mAppUserData);
		return aRet ? GT2True : GT2False;
	}
	
	GSI_UNUSED(theSocket);
	GSI_UNUSED(anAddr);
	return GT2False;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Requests for communication from a peer is handled by first checking if the 
// initial message has a message handler registered for it.  Otherwise 
// the message is passed onto the app.
void gsUdpConnAttemptCB(GT2Socket socket, GT2Connection connection, unsigned int ip, 
						unsigned short port, int latency, GT2Byte * message, int len)
{
	// Get the message handler for the connection 
	int index;
	GSUdpMsgHandler aHandler;
	GSUdpRemotePeer aRemotePeer;
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	char anAddr[GS_IP_ADDR_AND_PORT];
	
	gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
		"[Udp Engine] Connection attempt from %s\n", gt2AddressToString(ip, port, anAddr));
	//If there is a handler, automatically accept a connection if the initial message is
	//the same as the handler's registered initial message
	if (len >= GS_UDP_MSG_HEADER_LEN)
	{    
		memcpy(aHandler.mInitialMsg, message, GS_UDP_MSG_HEADER_LEN);
		
		aRemotePeer.mAddr = ip;
		aRemotePeer.mPort = port;
		aRemotePeer.mConnection = connection;
		
		ArrayAppend(aUdp->mRemotePeers, &aRemotePeer);
		index = ArraySearch(aUdp->mMsgHandlers, &aHandler, gsUdpMsgHandlerCompare, 0, 0);
		if (index != NOT_FOUND)
		{
			GT2ConnectionCallbacks aCallbacks;
			
			aCallbacks.closed = gsUdpClosedRoutingCB;
			aCallbacks.connected = gsUdpConnectedRoutingCB;
			aCallbacks.ping = gsUdpPingRoutingCB;
			aCallbacks.received = gsUdpReceivedRoutingCB;

	       
			// Automatically accept connections for Message Handlers
			gt2Accept(aRemotePeer.mConnection, &aCallbacks);	
			gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
				"[Udp Engine] Connection attempt auto-accepted for message handler\n");
			return;
		}
	}
	// all other messages go to the app
	if (aUdp->mAppConnAttempt) 
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, 
			"[Udp Engine] Connection attempt from %s, asking app to accept/reject\n", gt2AddressToString(ip, port, anAddr));
		aUdp->mAppConnAttempt(ip, port, latency, (unsigned char *)message, (unsigned int)len, aUdp->mAppUserData);
	}
	else 
	{
		// Reject any un-handled connections or unknown connections
		gt2Reject(connection, NULL, 0);
		ArrayRemoveAt(aUdp->mRemotePeers, ArrayLength(aUdp->mRemotePeers) -1);
	}
	GSI_UNUSED(socket);
	GSI_UNUSED(anAddr);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Public functions


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Used to check if the UDP layer needs to be initialized
gsi_bool gsUdpEngineIsInitialized()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	return aUdp->mInitialized;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Initializes the UDP layer
// A specific port can be used if needed.
// An app must register their callbacks here.
// An App must also call this before starting SDKs since they might start 
// the UDP layer without the app having a chance to register its callbacks.
// Any message handler can get the port the UDP layer is using if it did not
// initialize the UDP Layer.
// Use of the UDP Layer requires it to be initialized as is the case with most 
// functions below.
GSUdpErrorCode gsUdpEngineInitialize(unsigned short thePort, int theIncomingBufSize, 
									 int theOutgoingBufSize, gsUdpErrorCallback theAppNetworkError, 
									 gsUdpConnConnectedCallback theAppConnected,
									 gsUdpConnClosedCallback theAppClosed,
									 gsUdpConnPingCallback theAppPing,
									 gsUdpConnReceivedDataCallback theAppReceive,
									 gsUdpUnknownMsgCallback theAppUnownMsg,
									 gsUdpAppConnectAttemptCallback theAppConnectAttempt,
									 void *theAppUserData)
{
	int incomingBufferSize,
		outgoingBufferSize;
	char anAddr[GS_IP_ADDR_AND_PORT];
	GT2Result aGt2Result;
	// Grab the single instance of the UDP Communication Engine
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	
	// Setup our gt2 buffer sizes for reliable messages
	
	incomingBufferSize = theIncomingBufSize != 0 ? theIncomingBufSize : GS_UDP_DEFAULT_IN_BUFFSIZE;
	outgoingBufferSize = theOutgoingBufSize != 0 ? theOutgoingBufSize : GS_UDP_DEFAULT_OUT_BUFFSIZE;

	// Setup our internal socket that will be shared among more than one application
	aUdp->mAppNetworkError = theAppNetworkError;
	aUdp->mAppUnknownMessage = theAppUnownMsg;
	aUdp->mAppConnected = theAppConnected;
	aUdp->mAppClosed = theAppClosed;
	aUdp->mAppPingReply = theAppPing;
	aUdp->mAppRecvData = theAppReceive;
	aUdp->mAppConnAttempt = theAppConnectAttempt;

	// Any port can be used 
	gt2AddressToString(0, thePort, anAddr);
	aGt2Result = gt2CreateSocket(&aUdp->mSocket, anAddr,  outgoingBufferSize, incomingBufferSize, 
		gsUdpSocketError);
	if (aGt2Result != GT2Success)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError, 
			"[Udp Engine] error creating gt2 socket, error code: %d\n", aGt2Result);
		return GS_UDP_NETWORK_ERROR;
	}	
	// We'll need to keep track of connections with an array of GPconnection to address mapping
	aUdp->mRemotePeers = ArrayNew(sizeof(GSUdpRemotePeer), 1, NULL);
	if (aUdp->mRemotePeers == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError, 
			"[Udp Engine] No more memory!!!\n");
		return GS_UDP_NO_MEMORY;
	}

	aUdp->mMsgHandlers = ArrayNew(sizeof(GSUdpMsgHandler), 1, gsUdpMsgHandlerFree);
	if (aUdp->mMsgHandlers == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError, 
			"[Udp Engine] No more memory!!!\n");
		return GS_UDP_NO_MEMORY;
	}
		
	// Used by the manager to receive messages from backend services or other clients
	// that may not be using gt2
	gt2SetUnrecognizedMessageCallback(aUdp->mSocket, gsUdpUnrecognizedMsgCB);
	
	// Automatically start listening for all SDKS and the game if game uses UDP Engine
	gt2Listen(aUdp->mSocket, gsUdpConnAttemptCB);
	aUdp->mLocalAddr = gt2GetLocalIP(aUdp->mSocket);
	aUdp->mLocalPort = gt2GetLocalPort(aUdp->mSocket);
	aUdp->mAppPendingConnections = 0;
	aUdp->mInitialized = gsi_true;
	if (theAppUserData)
	{
		aUdp->mAppUserData = theAppUserData;
	}
	else
		aUdp->mAppUserData = NULL;
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Used obtain the peer's state 
// theIp and thePort cannot be 0 (Zero)
GSUdpErrorCode gsUdpEngineGetPeerState(unsigned int theIp, unsigned short thePort, GSUdpPeerState *thePeerState)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GSUdpRemotePeer aPeer, *aPeerFound;
	int index;
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theIp);
	GS_ASSERT(thePort);
	GS_ASSERT(thePeerState != NULL);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_State, GSIDebugLevel_Debug, 
			"[Udp Engine] Engine not initialized\n");
		*thePeerState = GS_UDP_PEER_CLOSED;
		return GS_UDP_NOT_INITIALIZED;
	}

	aPeer.mAddr = theIp;
	aPeer.mPort = thePort;
	index = ArraySearch(aUdp->mRemotePeers, &aPeer, gsUdpRemotePeerCompare, 0, 0);
	if (index == NOT_FOUND)
	{
		*thePeerState = GS_UDP_PEER_CLOSED;
		return GS_UDP_NO_ERROR;
	}
	
	aPeerFound = (GSUdpRemotePeer *)ArrayNth(aUdp->mRemotePeers, index);

	*thePeerState = (GSUdpPeerState)gt2GetConnectionState(aPeerFound->mConnection);
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// theIp and thePort cannot be 0 (Zero)
// Starts a request to open a communication channel with another peer based on 
// IP and port.  
GSUdpErrorCode gsUdpEngineStartTalkingToPeer(unsigned int theIp, unsigned short thePort,
									  char theInitMsg[GS_UDP_MSG_HEADER_LEN], int timeOut)
{
	char anAddr[GS_IP_ADDR_AND_PORT];
	GSUdpRemotePeer aRemotePeer;	
	GSUdpMsgHandler aHandler;
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GT2ConnectionCallbacks aCallbacks;
	int index;
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theIp);
	GS_ASSERT(thePort);

	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}
	
	if (theIp == 0 || thePort == 0)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Invalid parameter(s), check ip, port");
		return GS_UDP_PARAMETER_ERROR;
	}
	
	aRemotePeer.mAddr = theIp;  // In Network Byte Order for GT2
	aRemotePeer.mPort = thePort;  // In Host Byte Order for GT2

	index = ArraySearch(aUdp->mRemotePeers, &aRemotePeer, gsUdpRemotePeerCompare, 0, 0);
	if (index != NOT_FOUND)
	{
		GSUdpRemotePeer *aPeerFound = (GSUdpRemotePeer *)ArrayNth(aUdp->mRemotePeers, index);
		GT2ConnectionState aState = gt2GetConnectionState(aPeerFound->mConnection);
		if (aState == GT2Connected)
		{
			gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
				"[Udp Engine] Engine is already talking to remote address\n");
			return GS_UDP_ADDRESS_ALREADY_IN_USE;
		}
		else if (aState == GT2Connecting)
		{
			memcpy(aHandler.mInitialMsg, theInitMsg, GS_UDP_MSG_HEADER_LEN);
			
			index = ArraySearch(aUdp->mMsgHandlers, &aHandler, gsUdpMsgHandlerCompare, 0, 0);
			if (index != NOT_FOUND)
			{
				GSUdpMsgHandler *aHandlerFound = (GSUdpMsgHandler *)ArrayNth(aUdp->mMsgHandlers, index);
				ArrayAppend(aHandlerFound->mPendingConnections, aPeerFound);
			}
		}
	}	
	else
	{
		gt2AddressToString(theIp, thePort, anAddr);
		aCallbacks.closed = gsUdpClosedRoutingCB;
		aCallbacks.connected = gsUdpConnectedRoutingCB;
		aCallbacks.ping = gsUdpPingRoutingCB;
		aCallbacks.received = gsUdpReceivedRoutingCB;

		// start the connect without blocking since we want the engine to be as asynchronous as possible
		gt2Connect(aUdp->mSocket, &aRemotePeer.mConnection, anAddr, (unsigned char *)theInitMsg, GS_UDP_MSG_HEADER_LEN, timeOut, &aCallbacks, GT2False);

		ArrayAppend(aUdp->mRemotePeers, &aRemotePeer);
		
		memcpy(aHandler.mInitialMsg, theInitMsg, GS_UDP_MSG_HEADER_LEN);
		
		index = ArraySearch(aUdp->mMsgHandlers, &aHandler, gsUdpMsgHandlerCompare, 0, 0);
		if (index != NOT_FOUND)
		{
			GSUdpRemotePeer *aRemotePeerPtr = (GSUdpRemotePeer *)ArrayNth(aUdp->mRemotePeers, ArrayLength(aUdp->mRemotePeers) - 1);
			GSUdpMsgHandler *aHandlerFound = (GSUdpMsgHandler *)ArrayNth(aUdp->mMsgHandlers, index);
			ArrayAppend(aHandlerFound->mPendingConnections, &aRemotePeerPtr);
		}
		else
		{
			aUdp->mAppPendingConnections++;
		}
	}
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// theIp and thePort cannot be 0 (Zero)
// Accepts a Peer's request for communication 
// Should only be used by App
GSUdpErrorCode gsUdpEngineAcceptPeer(unsigned int theIp, unsigned short thePort)
{	
	GSUdpRemotePeer aRemotePeer;	
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	int index;
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theIp);
	GS_ASSERT(thePort);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}

	if (theIp == 0 || thePort == 0)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Invalid parameter(s), check ip, port\n");
		return GS_UDP_PARAMETER_ERROR;
	}

	aRemotePeer.mAddr = theIp;
	aRemotePeer.mPort = thePort;
	index = ArraySearch(aUdp->mRemotePeers, &aRemotePeer, gsUdpRemotePeerCompare, 0, 0);
	if (index != NOT_FOUND)
	{
		GT2ConnectionCallbacks aCallbacks;
		
		GSUdpRemotePeer *aPeerFound = (GSUdpRemotePeer *)ArrayNth(aUdp->mRemotePeers, index);
		
		aCallbacks.closed = gsUdpClosedRoutingCB;
		aCallbacks.connected = gsUdpConnectedRoutingCB;
		aCallbacks.ping = gsUdpPingRoutingCB;
		aCallbacks.received = gsUdpReceivedRoutingCB;
		
		gt2Accept(aPeerFound->mConnection, &aCallbacks);
	}
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// theIp and thePort cannot be 0 (Zero)
// Rejects a Peer's request for communication 
// Should only be used by App
GSUdpErrorCode gsUdpEngineRejectPeer(unsigned int theIp, unsigned short thePort)
{
	GSUdpRemotePeer aRemotePeer;	
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	int index;
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theIp);
	GS_ASSERT(thePort);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}

	if (theIp == 0 || thePort == 0)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Invalid parameter(s), check ip, port\n");
		return GS_UDP_PARAMETER_ERROR;
	}

	// Find the connection to reject in our array of peers
	aRemotePeer.mAddr = theIp;
	aRemotePeer.mPort = thePort;
	index = ArraySearch(aUdp->mRemotePeers, &aRemotePeer, gsUdpRemotePeerCompare, 0, 0);
	if (index != NOT_FOUND)
	{
		GSUdpRemotePeer *aPeerFound = (GSUdpRemotePeer *)ArrayNth(aUdp->mRemotePeers, index);
		gt2Reject(aPeerFound->mConnection, NULL, 0);
		ArrayDeleteAt(aUdp->mRemotePeers, index);
	}
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Sends a message to a peer using IP and Port
// An empty header constitutes the app sending this message.
// theIp and thePort cannot be 0 (Zero)
// 
// WARNING: Messages should not be greater than the outgoing buffer size minus the header 
// and the 7 byte header for reliable messages (used for internal gt2 operations). Most
// UDP fragmentation occurs if messages are bigger than 1500 bytes.  Also, some routers are 
// known to drop those packets that are larger than 1500 bytes.  The recommended outgoing 
// buffer size is the default (1460).  So take that, and subtract 16 for message handler header
// and reliable message header (if sending data reliably).
// freeSpace = 1460 - 16 - 7
GSUdpErrorCode gsUdpEngineSendMessage(unsigned int theIp, unsigned short thePort, 
									  char theHeader[GS_UDP_MSG_HEADER_LEN], unsigned char *theMsg, 
									  unsigned int theMsgLen, gsi_bool theReliable)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	int aTotalMessageLen, index;
	GSUdpRemotePeer aRemotePeer, *aRemotePeerFound;
	GT2Byte *fullMessage;
	GT2Result aResult;
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theIp);
	GS_ASSERT(thePort);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}

	// Messages being sent with an empty header are treated as app messages
	if (!theHeader[0])
		aTotalMessageLen = (int)theMsgLen;
	else 
		aTotalMessageLen = (int)(GS_UDP_MSG_HEADER_LEN + theMsgLen);

	aRemotePeer.mAddr = theIp;
	aRemotePeer.mPort = thePort;

	index = ArraySearch(aUdp->mRemotePeers, &aRemotePeer, gsUdpRemotePeerCompare, 0, 0);
	if (index == NOT_FOUND)
	{
		char anAddr[GS_IP_ADDR_AND_PORT];
		gt2AddressToString(theIp, thePort, anAddr);
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
			"[Udp Engine] address not found for sending message\n", anAddr);
		return GS_UDP_ADDRESS_ERROR;
	}
	else
	{
		aRemotePeerFound = (GSUdpRemotePeer *)ArrayNth(aUdp->mRemotePeers, index);
	}

	if (aTotalMessageLen > gt2GetOutgoingBufferSize(aRemotePeerFound->mConnection) && theReliable)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
			"[Udp Engine] Message Size too large, dropping message\n");
		return GS_UDP_MSG_TOO_BIG;
	}

	fullMessage = (GT2Byte *)gsimalloc((unsigned long)aTotalMessageLen);
	memcpy(fullMessage, theHeader, GS_UDP_MSG_HEADER_LEN);
	memcpy(fullMessage + GS_UDP_MSG_HEADER_LEN, theMsg, theMsgLen);
	// Send the message 
	// reliable messages will be kept in the outgoing buffers till they are sent
	aResult = gt2Send(aRemotePeerFound->mConnection, fullMessage, aTotalMessageLen, theReliable);
	gsifree(fullMessage);

	if (aResult != GT2Success)
		return GS_UDP_SEND_FAILED;
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// One the UDP Layer is initialized, this function
// should be called every 10-100 ms
// Message handlers already call this which means 
// that the app may not have to call this function 
GSUdpErrorCode gsUdpEngineThink()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GS_ASSERT(aUdp->mInitialized);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}
	gt2Think(aUdp->mSocket);
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Shutdown should only happen if following is met:
// Apps can call this function after they have shutdown message handlers (SDKs), 
//   and verified there are no message handlers remaining.  Calling the function 
//   gsUdpEngineNoMoreMsgHandlers will do this.
// Message handlers cannot call this function without checking if there no more 
// message handlers remaining and if there is no app (gsUdpEngineNoApp).  
GSUdpErrorCode gsUdpEngineShutdown()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GS_ASSERT(aUdp->mInitialized);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}
	gt2CloseSocket(aUdp->mSocket);	
	ArrayFree(aUdp->mMsgHandlers);
	ArrayFree(aUdp->mRemotePeers);
	aUdp->mInitialized = gsi_false;
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Obtains the lower level socket that can be used to perform other operations 
// or pass to other SDKs.
SOCKET gsUdpEngineGetSocket()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();

	GS_ASSERT(aUdp->mInitialized);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_HotError,
			"[Udp Engine] Engine not initialized\n");
		return INVALID_SOCKET;
	}

	return gt2GetSocketSOCKET(aUdp->mSocket);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Gets the local IP the UDP layer is bound to.
unsigned int gsUdpEngineGetLocalAddr()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();

	GS_ASSERT(aUdp->mInitialized);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_HotError,
			"[Udp Engine] Engine not initialized\n");
		return (unsigned int)-1;
	}
	return aUdp->mLocalAddr;
	
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Gets the local port the UDP layer is bound to.
unsigned short gsUdpEngineGetLocalPort()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();

	GS_ASSERT(aUdp->mInitialized);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_HotError,
			"[Udp Engine] Engine not initialized\n");
		return 0;
	}
	return aUdp->mLocalPort;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Message handlers are added using this function
// The initial message and header of a message handler cannot be empty
// However, they can be the same.
// User data can be useful for keeping track of Message Handler 
GSUdpErrorCode gsUdpEngineAddMsgHandler(char theInitMsg[GS_UDP_MSG_HEADER_LEN], char theHeader[GS_UDP_MSG_HEADER_LEN], 
									    gsUdpErrorCallback theMsgHandlerError, 
									    gsUdpConnConnectedCallback theMsgHandlerConnected, 
									    gsUdpConnClosedCallback theMsgHandlerClosed, 
									    gsUdpConnPingCallback theMsgHandlerPing,
										gsUdpConnReceivedDataCallback theMsgHandlerRecv,
										void *theUserData)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GSUdpMsgHandler aMsgHandler;
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theInitMsg || theInitMsg[0]);
	GS_ASSERT(theHeader || theHeader[0]);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}
	
	// setup a message handler that the UDP engine will use to pass connection attempts to
	
	//check for valid input
	if (!theInitMsg[0])
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Invalid init message\n");
		return GS_UDP_PARAMETER_ERROR;
	}

	if (!theHeader[0])
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Invalid header\n");
		return GS_UDP_PARAMETER_ERROR;
	}

	// This check is not necessary.  Some SDKs may not use all callbacks
	/*if (!theMsgHandlerError || !theMsgHandlerConnected || !theMsgHandlerClosed 
		|| !theMsgHandlerPing || !theMsgHandlerRecv)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Invalid callback(s)");
		return GS_UDP_PARAMETER_ERROR;
	}
	*/
	aMsgHandler.mClosed = theMsgHandlerClosed;
	aMsgHandler.mConnected = theMsgHandlerConnected;
	aMsgHandler.mPingReply = theMsgHandlerPing;
	aMsgHandler.mReceived = theMsgHandlerRecv;
	
	aMsgHandler.mNetworkError = theMsgHandlerError;

	memcpy(aMsgHandler.mInitialMsg, theInitMsg, GS_UDP_MSG_HEADER_LEN);
	memcpy(aMsgHandler.mHeader, theHeader, GS_UDP_MSG_HEADER_LEN);
		
	aMsgHandler.mPendingConnections = ArrayNew(sizeof(GSUdpRemotePeer *), 1, NULL);
	if (aMsgHandler.mPendingConnections == NULL)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError, 
			"[Udp Engine] No more memory!!!\n");
		return GS_UDP_NO_MEMORY;
	}
	aMsgHandler.mUserData = theUserData;
	ArrayAppend(aUdp->mMsgHandlers, &aMsgHandler);
	
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// When a message handler is done or shutting down, the message handler should remove
// itself from the UDP Layer
// The header cannot be empty
GSUdpErrorCode gsUdpEngineRemoveMsgHandler(char theHeader[GS_UDP_MSG_HEADER_LEN])
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GSUdpMsgHandler aHandler;
	int index;
	
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theHeader);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return GS_UDP_NETWORK_ERROR;
	}

	if (!theHeader || !theHeader[0])
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] invalid or empty header\n");
		return GS_UDP_PARAMETER_ERROR;
	}
	
	memcpy(aHandler.mHeader, theHeader, GS_UDP_MSG_HEADER_LEN);

	index = ArraySearch(aUdp->mMsgHandlers, &aHandler, gsUdpMsgHandlerCompare2, 0, 0);
	if (index != NOT_FOUND)
	{
		ArrayDeleteAt(aUdp->mMsgHandlers, index);
	}
	return GS_UDP_NO_ERROR;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// Checks to see if there any remaining message handlers.
// returns gsi_false if there are none.
gsi_bool gsUdpEngineNoMoreMsgHandlers()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	
	GS_ASSERT(aUdp->mInitialized);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return gsi_true;
	}
	
	return ArrayLength(aUdp->mMsgHandlers) == 0 ? gsi_true : gsi_false;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// returns gsi_false if there is no app using the UDP Layer
gsi_bool gsUdpEngineNoApp()
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();

	GS_ASSERT(aUdp->mInitialized);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return gsi_true;
	}

	if (aUdp->mAppClosed || aUdp->mAppConnAttempt || aUdp->mAppConnected || aUdp->mAppNetworkError 
		|| aUdp->mAppPingReply || aUdp->mAppRecvData || aUdp->mAppUnknownMessage)
	{
		return gsi_false;
	}

	return gsi_true;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Layer must be initialized
// theIp and thePort cannot be 0 (Zero)
// Based on an IP and port, the function will return the amount of free space 
// of a peer's buffer.
int gsUdpEngineGetPeerOutBufferFreeSpace(unsigned int theIp, unsigned short thePort)
{
	GSUdpEngineObject *aUdp = gsUdpEngineGetEngine();
	GSUdpRemotePeer aRemotePeer, *aRemotePeerFound;
	int index;
	GS_ASSERT(aUdp->mInitialized);
	GS_ASSERT(theIp);
	GS_ASSERT(thePort);
	if (!aUdp->mInitialized)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
			"[Udp Engine] Engine not initialized\n");
		return 0;
	}

	aRemotePeer.mAddr = theIp;
	aRemotePeer.mPort = thePort;
	index = ArraySearch(aUdp->mRemotePeers, &aRemotePeer, gsUdpRemotePeerCompare, 0, 0);
	if (index != NOT_FOUND)
	{
		aRemotePeerFound = (GSUdpRemotePeer *)ArrayNth(aUdp->mRemotePeers, index);
		return gt2GetOutgoingBufferFreeSpace(aRemotePeerFound->mConnection);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Creates a string address given an IP and port
// IP and port can be 0.  
void gsUdpEngineAddrToString(unsigned int theIp, unsigned short thePort, char addrstring[GS_IP_ADDR_AND_PORT])
{
	gt2AddressToString(theIp, thePort, addrstring);
}
