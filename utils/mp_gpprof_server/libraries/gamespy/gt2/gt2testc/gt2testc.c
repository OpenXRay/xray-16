#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../gt2.h"
#include "../gt2Encode.h"

#define TEST_ENCODE_DECODE

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#define FILL_IN   "fill me in"

typedef struct Message
{
	GT2Bool valid;
	char * data;
	int len;
	GT2Bool reliable;
} Message;

GT2ConnectionCallbacks connectionCallbacks;
GT2Bool server;
GT2Bool quit;
GT2Bool hardcore;
int messageCount;
unsigned long sendTime;
GT2Connection Connection;

Message initialMessage = { GT2True, "123456789012345", -1 };

Message messageList[] =
{
	{ GT2True, NULL, 0, GT2True },       // empty
	{ GT2True, "1", 1, GT2True },        // 1 byte     XXX
	{ GT2True, "1234567", 7, GT2True },  // 7 bytes    XXX
	{ GT2True, "12345678901234567890123456789012345678901234567890", 50, GT2True },  // 50 bytes
	{ GT2True, FILL_IN, 128, GT2True }, 
	{ GT2True, FILL_IN, 256, GT2True }, 
	{ GT2True, FILL_IN, 512, GT2True }, 
	{ GT2True, FILL_IN, 768, GT2True }, 
	{ GT2True, FILL_IN, 1024, GT2True }, 
	{ GT2True, FILL_IN, 1400, GT2True }, 
#if !defined(_NITRO)
	{ GT2True, FILL_IN, 2047, GT2True }, 
	{ GT2True, FILL_IN, (3 * 1024) - 1, GT2True }, 
#if !defined(_REVOLUTION)
	{ GT2True, FILL_IN, (5 * 1024) - 1, GT2True }, 
	{ GT2True, FILL_IN, (7 * 1024) - 1, GT2True }, 
#if !defined(_PS2) && !defined(_PSP)
	{ GT2True, FILL_IN, (8 * 1024) + 1017, GT2True }, 
#if !defined(_MACOSX) && !defined(_PS3) // Maximum send size for PS3 is 9216
	{ GT2True, FILL_IN, (9 * 1024) - 1, GT2True }, 
	{ GT2True, FILL_IN, (32 * 1024) - 1, GT2True }, 
#endif
#endif
#endif
#endif
#if !defined(_NITRO) && !defined(_REVOLUTION)
	{ GT2True, NULL, 0, GT2False },      // empty
#endif
	{ GT2True, "1", 1, GT2False },        // 1 byte     XXX
	{ GT2True, "1234567", 7, GT2False },  // 7 bytes    XXX
	{ GT2True, "12345678901234567890123456789012345678901234567890", 50, GT2False }, // 50 bytes
	{ GT2True, FILL_IN, 128, GT2False }, 
	{ GT2True, FILL_IN, 256, GT2False }, 
	{ GT2True, FILL_IN, 512, GT2False }, 
	{ GT2True, FILL_IN, 768, GT2False }, 
	{ GT2True, FILL_IN, 1024, GT2False }, 
	{ GT2True, FILL_IN, 1400, GT2False }, 
#if !defined(_NITRO)
	{ GT2True, FILL_IN, 2047, GT2False }, 
#if !defined(_REVOLUTION)
	{ GT2True, FILL_IN, (5 * 1024) - 1, GT2False },
	{ GT2True, FILL_IN, (7 * 1024) - 1, GT2False },
#if !defined(_PS2) && !defined(_PSP)  
	{ GT2True, FILL_IN, (9 * 1024), GT2False },
#if !defined (_MACOSX) && !defined(_PS3) // Maximum send size for PS3 is 9216 
	{ GT2True, FILL_IN, (9 * 1024) + 1, GT2False },
#endif
#endif
#endif
#endif

	{ GT2False }
};

Message hardcoreMessage = { GT2True, "12345678901234567890123456789012345678901234567890", 50, GT2True };

static void ConnectedCallback(GT2Connection connection, GT2Result result, GT2Byte * message, int len)
{
	if(result != GT2Success)
	{
		if (!message)
			message = (GT2Byte *)"";
		printf("Connect failed (%d): %s\n", result, message);
		quit = GT2True;
		return;
	}

	printf("Connected\n");
	
	GSI_UNUSED(connection);
	GSI_UNUSED(len);
}

static void ReceivedCallback( GT2Connection connection, GT2Byte * message, int len, GT2Bool reliable)
{
	Message * pcurr;

	// Server echo.
	if(server)
	{
		messageCount++;

		if(!hardcore || !(messageCount % 100))
			printf("Got message %d (%d bytes, %s)\n", messageCount, len, reliable ? "reliable" : "unreliable");

		gt2Send(connection, message, len, reliable);
	}
	else
	{
		if(hardcore)
		{
			pcurr = &hardcoreMessage;
			messageCount++;
		}
		else
		{
			pcurr = &messageList[messageCount];
		}
		
		// Check this against the message list.
		if((pcurr->reliable != reliable) || (pcurr->len != len) || (memcmp(pcurr->data, message, (size_t)len) != 0))
		{
			printf("Message check failed (%d, %d)\n",pcurr->len, len);
		}
		else
		{
			if(!hardcore || !(messageCount % 100))
			{
				printf("Message %d validated (%d bytes, %s)", messageCount + 1, len, reliable ? "reliable" : "unreliable");
				if(server)
					printf("\n");
				else
					printf(": %lu ms\n", current_time() - sendTime);
			}
		}
		sendTime = 0;
	}
}
 
static void ClosedCallback(GT2Connection connection, GT2CloseReason reason)
{
	// Print out the reason.
	printf("Connection closed: ");
	if(reason == GT2LocalClose)
		printf("Local Close\n");
	else if(reason == GT2RemoteClose)
		printf("Remote Close\n");
	else if(reason == GT2CommunicationError)
		printf("Communication Error\n");
	else if(reason == GT2SocketError)
		printf("Socket Error\n");
	else if(reason == GT2NotEnoughMemory)
		printf("Not Enough Memory\n");

	quit = GT2True;
	Connection = NULL; 
	
	GSI_UNUSED(connection);
}

static void PingCallback(GT2Connection connection, int latency)
{
	printf("Ping: %dms\n", latency);
	
	GSI_UNUSED(connection);
}

static void SocketErrorCallback(GT2Socket socket)
{
	printf("SOCKET ERROR!!\n");
	
	GSI_UNUSED(socket);
}

static void ConnectAttemptCallback(GT2Socket socket, GT2Connection connection, unsigned int ip, unsigned short port, int latency, GT2Byte * message, int len)
{
	if((len == ((int)strlen(initialMessage.data) + 1)) && (memcmp(message, initialMessage.data, (size_t)len) == 0))
	{
		gt2Accept(connection, &connectionCallbacks);
		printf("Accepted connection from %s\n",gt2AddressToString(ip,port,NULL));
	}
	else
	{
		gt2Reject(connection, (const GT2Byte *)"Invalid intial message.", -1);
		printf("Rejected connection\n");
	}
	
	GSI_UNUSED(socket);
	GSI_UNUSED(latency);
}

static void SendDumpCallback
(
	GT2Socket socket,
	GT2Connection connection,
	unsigned int ip,
	unsigned short port,
	GT2Bool reset,
	const GT2Byte * message,
	int len
)
{
	char buffer[128];
	sprintf(buffer, "SEND %p %p %s %d\n", socket, connection, gt2AddressToString(ip, port, NULL), len);
#ifdef WIN32
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(reset);
	GSI_UNUSED(message);
}

static void ReceiveDumpCallback
(
	GT2Socket socket,
	GT2Connection connection,
	unsigned int ip,
	unsigned short port,
	GT2Bool reset,
	const GT2Byte * message,
	int len
)
{
	char buffer[128];
	if(reset)
		sprintf(buffer, "RECV %p %p %s RESET\n", socket, connection, gt2AddressToString(ip, port, NULL));
	else
		sprintf(buffer, "RECV %p %p %s %d\n", socket, connection, gt2AddressToString(ip, port, NULL), len);
#ifdef WIN32
	OutputDebugString(buffer);
#endif

	GSI_UNUSED(message);
}

#ifdef TEST_ENCODE_DECODE

static void EncodeTest()
{
	const char encodestr[] = {GT_INT,GT_UINT,GT_SHORT,GT_USHORT,GT_CHAR,GT_UCHAR,
		GT_FLOAT,GT_DOUBLE,GT_CSTR,GT_DBSTR,GT_RAW,0};	
	const char encodestr2[] = {GT_CSTR_PTR,GT_DBSTR_PTR,GT_RAW_PTR,0};	
	const char bitstr[] = {GT_BIT,GT_BIT,GT_BIT,GT_BIT,GT_BIT,GT_BIT,GT_BIT,GT_BIT,GT_BIT,GT_BIT,0};
	int i = -30, i2;
	unsigned int uint = 300, uint2;
	short sh = 10000, sh2;
	unsigned short ush = 23500, ush2;
	char ch = 23, ch2;
	unsigned char uch = 129, uch2;
	float f = 10.0f, f2;
	double d = 1.12312312, d2;
	char mystr[] = "this is a test string, this is only a test", mystr2[255], *mystr3;
	short dbstr[] = {1234,5678,9012,1010,3210,1341,0}, dbstr2[255], *dbstr3;
	char rawstr[] = "raw\001\000\001data", rawstr2[255], *rawstr3;
	int rawdatalen = 10, rawdatalen2 = 10;
	char bits[]={1,0,1,0,1,0,1,1,0,1},bits2[20]; 
	char outBuff[1024];
	int retlen, retlen2;
	int temp;

	//retlen = gtEncode(0,"fs",outBuff,100,10.0f,mystr);
	retlen = gtEncode(0,encodestr,outBuff,1024,i,uint,sh,ush,ch,uch,f,d,mystr,dbstr,rawstr,rawdatalen);
	retlen2 = gtDecode(encodestr,outBuff, retlen,&i2,&uint2,&sh2,&ush2,&ch2,&uch2,&f2,&d2,mystr2,dbstr2,
		rawstr2,&rawdatalen2);
	printf("i=%d,i2=%d\nuint=%u,uint2=%u\nsh=%d,sh2=%d\nush=%u,ush2=%u\nch=%d,ch2=%d\nuch=%u,uch2=%u\nf=%f,f2=%f\nd=%f,d2=%f\n",
		i,i2,uint,uint2,sh,sh2,ush,ush2,ch,ch2,uch,uch2,f,f2,d,d2);
	printf("mystr =%s\nmystr2=%s\n",mystr,mystr2);
	for (temp = 0; dbstr[temp] != 0; temp++)
		printf("dbstr[%d]=%d,dbstr2[%d]=%d  ",temp,dbstr[temp],temp,dbstr2[temp]);
	printf("\nrawdatalen=%d,rawdatalen2=%d\n",rawdatalen,rawdatalen2);
	for (temp = 0 ; temp < rawdatalen ; temp++)
		printf("rawstr[%d]=%d,rawstr2[%d]=%d  ",temp,rawstr[temp],temp,rawstr2[temp]);
	printf("\nretlen=%d,retlen2=%d\n",retlen,retlen2);
	retlen = gtEncode(0,bitstr,outBuff,1024,bits[0],bits[1],bits[2],bits[3],bits[4],bits[5],bits[6],
		bits[7],bits[8],bits[9]);
	retlen2 = gtDecode(bitstr,outBuff,retlen,bits2,bits2+1,bits2+2,bits2+3,bits2+4,bits2+5,bits2+6,bits2+7,
		bits2+8,bits2+9);
	for (temp = 0 ; temp < 10 ; temp++)
		printf("bits[%d]=%d,bits2[%d]=%d  ",temp,bits[temp],temp,bits2[temp]);
	printf("\nretlen=%d,retlen2=%d\n",retlen,retlen2);
	retlen = gtEncode(0,encodestr2,outBuff,1024,mystr,dbstr,rawstr,rawdatalen);
	retlen2 = gtDecode(encodestr2,outBuff,retlen,&mystr3,&dbstr3,&rawstr3,&rawdatalen2);
	printf("mystr =%s\nmystr3=%s\n",mystr,mystr3);
	#ifdef ALIGNED_COPY
		for (temp = 0; dbstr[temp] != 0; temp++)
		{
			memcpy(&sh,&dbstr[temp],sizeof(sh));
			memcpy(&sh2,&dbstr3[temp],sizeof(sh2));
			printf("dbstr[%d]=%d,dbstr3[%d]=%d  ",temp,sh, temp,sh2);
		}
	#else
		for (temp = 0; dbstr[temp] != 0; temp++)
			printf("dbstr[%d]=%d,dbstr3[%d]=%d  ",temp,dbstr[temp],temp,dbstr3[temp]);
	#endif
	printf("\nrawdatalen=%d,rawdatalen2=%d\n",rawdatalen,rawdatalen2);
	for (temp = 0 ; temp < rawdatalen ; temp++)
		printf("rawstr[%d]=%d,rawstr3[%d]=%d  ",temp,rawstr[temp],temp,rawstr3[temp]);
	printf("\nretlen=%d,retlen2=%d\n",retlen,retlen2);
	

}
#endif

#ifdef __MWERKS__ // CodeWarrior will warn if function not prototyped
	int test_main(int argc, char **argv);
#endif

int test_main(int argc, char **argv)
{
	GT2Socket socket = NULL;
	char localAddress[128] = "";
	char remoteAddress[128] = "";
	GT2Bool gotIt = GT2False;
	Message * message;
	int count = 0;
	int msgloopcount = 0;
	GT2Result result;
	int i;

#ifdef TEST_ENCODE_DECODE
	EncodeTest();
#endif
	
	// Set the callbacks.
	memset(&connectionCallbacks, 0, sizeof(GT2ConnectionCallbacks));
	connectionCallbacks.connected = ConnectedCallback;
	connectionCallbacks.received = ReceivedCallback;
	connectionCallbacks.closed = ClosedCallback;
	connectionCallbacks.ping = PingCallback;

	// Fill in long messages.
	for(message = messageList ; message->valid ; message++)
	{
		if(message->data && (strcmp(message->data, FILL_IN) == 0))
		{
			message->data = (char *)malloc((size_t)message->len);
			for(i = 0 ; i < message->len ; i++)
				message->data[i] = (char)('0' + (i % 10));
		}
	}

	// Check the args.
	if(argc > 1)
	{
		argc = 1;
		if(strncmp(argv[argc], "-h", 2) == 0)
		{
			hardcore = GT2True;
			argc++;
		}

		// Check for listen.
		if(strncmp(argv[argc], "-l", 2) == 0)
		{
			gotIt = GT2True;
			server = GT2True;
			argc++;

			if(argv[argc])
				strcpy(localAddress, argv[argc++]);
		}
		// Check for connect.
		else if(strncmp(argv[argc], "-c", 2) == 0)
		{
			gotIt = GT2True;
			server = GT2False;
			argc++;

			if(argv[argc])
				strcpy(remoteAddress, argv[argc++]);
			if(argv[argc])
				strcpy(localAddress, argv[argc++]);
		}
	}

	// Check if we haven't decided yet.
	if(!gotIt)
	{
#if 1
		server = GT2True;
		strcpy(localAddress, ":12345");
#else
		server = GT2False;
		strcpy(localAddress, "");
		strcpy(remoteAddress, "192.168.0.3:12345");
#endif
	}

	do
	{
		// create the socket
		result = gt2CreateSocket(&socket, localAddress, 0, 0, SocketErrorCallback);
		if(result != GT2Success)
		{
			printf("Failed to create socket! (%d)\n", result);
			return 1;
		}

		// set the dump callbacks
		gt2SetSendDump(socket, SendDumpCallback);
		gt2SetReceiveDump(socket, ReceiveDumpCallback);

		// listen if we're the server
		if(server)
		{
			gt2Listen(socket, ConnectAttemptCallback);

			printf("Listening for incoming connections [%s]...\n", localAddress);
			while(gsi_true) ///now we just think forever!
			{
				gt2Think(socket);
				if(hardcore)
					msleep(5);
				else
					msleep(20);
			}
		}

		//the rest is for client only
		quit = GT2False;
		printf("Attemping to connect to %s (max 10 sec)...\n", remoteAddress);
		result = gt2Connect(socket, &Connection, remoteAddress, (GT2Byte *)initialMessage.data, initialMessage.len, 10000, &connectionCallbacks, GT2True);
		if(result != GT2Success)
		{
			printf("gt2Connect failed! (%d)\n", result);
			return 1;
		}

		gt2Ping(Connection);

		if(quit)
			return 1;

		messageCount = 0;
		msgloopcount = 0;
		quit = GT2False;
		while(!quit && msgloopcount < 2)
		{
			unsigned long maxWait;
			if(messageCount == 100000)
			{
				messageCount = 0;
				msgloopcount++;
			}
			if(hardcore)
			{
				message = &hardcoreMessage;
			}
			else
			{
				message = &messageList[messageCount];
				if(!message->valid)
				{
					msgloopcount++;
					continue;
				}
			}

			gt2Send(Connection, (GT2Byte *)message->data, message->len, message->reliable);
			if(hardcore)
			{
				do
				{
					gt2Think(socket);
					msleep(5);
				}
				while(gt2GetOutgoingBufferFreeSpace(Connection) < 100);
			}
			else
			{
				sendTime = current_time();
				maxWait = (unsigned long)((message->reliable) ? 120000 : 20000);
				while (sendTime != 0 && current_time() - sendTime < maxWait && !quit) //wait a max of 20 sec for the reply
				{
					gt2Think(socket);
					msleep(5);
				}
				if (sendTime != 0)
				{
					printf("Sending %d bytes %s failed!!\n",message->len, message->reliable ? "reliable" : "unreliable");
				}
				messageCount++;
			}
			
		}
		if(Connection)
			gt2CloseConnection(Connection);
	}
	while(!server && (++count < 3)); //if client do the sequence 3 times

	gt2CloseSocket(socket);

	return 0;
}
