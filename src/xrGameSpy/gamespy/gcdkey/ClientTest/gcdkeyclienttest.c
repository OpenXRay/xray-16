/******
gcdkeyclienttest.c
GameSpy CDKey SDK Client Sample
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

 Please see the GameSpy CDKey SDK documentation for more 
 information

******/

#include "../common/gsCommon.h"
#include "gcdkeyc.h"

#define DEFAULT_SERVER "127.0.0.1"	// Run gcdkeyserver app on local host
#define DEFAULT_PORT 2000
#define DEFAULT_KEY "8014-c119-a892-74d2"

#define BUFSIZE 256

#define readstring(buffer) { fgets(buffer, sizeof(buffer), stdin); buffer[strlen(buffer) - 1] ='\0'; }

int test_main(int argc, char **argv)
{
	char data[256];
	int len;
	char cdkey[64];
	char server[64];
	char port[64];
	char response[RESPONSE_SIZE];
	SOCKET s;
	struct sockaddr_in saddr;

	while(1)
	{
		//get the server IP and a CD Key
		printf("Server to connect to [%s]: ",DEFAULT_SERVER);
		readstring(server);
		if (server[0]==0 || server[0]=='\n')
			strcpy(server,DEFAULT_SERVER);
		printf("Port to connect to [%d]: ",DEFAULT_PORT);
		readstring(port);
		if (port[0]=='\0' || port[0]=='\n')
			sprintf(port,"%d", DEFAULT_PORT);
		printf("Enter your CD Key [%s]: ", DEFAULT_KEY);
		readstring(cdkey);
		if (cdkey[0]==0 || cdkey[0]=='\n')
			strcpy(cdkey,DEFAULT_KEY);

		/* normally we would verify the CD Key (mathematically) at this point,
		if (!ValidateKey(cdkey))
			return;
		*/

		/* create a TCP socket to connect to the server, obviously you would
		connect with whatever protocol your game uses (TCP/UDP/DPlay/etc) */
		SocketStartUp();
		s = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(server);
		saddr.sin_port = htons((unsigned short)atoi(port));
		if (gsiSocketIsError(connect(s,(struct sockaddr *)&saddr,sizeof(saddr))))
		{
			printf("Error connecting to server\n");
			getchar();
			continue;
		}

		/* Once we are connected, there is a "handshake" where the server sends
		a "challenge" in the form "c:challenge", and we send a response in the
		for "r:response". How you do this is again, completely up to you */
		len = recv(s,data,BUFSIZE-1,0);
		if (len <= 0)
		{
			printf("Error connecting to server\n");
			getchar();
			continue;
		}
		data[len] = 0;

		// compute the response based on the challenge (which starts at data[2])
		gcd_compute_response(cdkey, data+2, response, CDResponseMethod_NEWAUTH);

		// send the response back to the server
		printf("Got: %s\nSending: %s\n",data, response);
		sprintf(data,"r:%s",response);
		send(s,data,strlen(data),0);

		// enter our main loop and wait for data
		while (1)
		{
			len = recv(s,data,BUFSIZE-1,0);
			if (len <= 0)
			{
				printf("Server Disconnected\n");
				getchar();
				//goto start; 
				break;
			}
			data[len] = 0;
			
			// Keep a watch for reauthentication requests
			//   In this case, the sample server sends "r:<challenge>"
			//     and the client sample responds with "p:<response>"
			// Your game should use it's own message types and headers.
			if (data[0] == 'r' && data[1] == ':')
			{
				char hint[9];
				memcpy(hint, data+2, 8); // 8 characters are a hint
				hint[8] = '\0';

				printf("Got reauth: %s, %s\n",hint, data+10);

				gcd_compute_response(cdkey, data+10, response, CDResponseMethod_REAUTH);
				sprintf(data, "p:%s%s", hint, response);
				send(s,data,strlen(data),0);
				printf("Sending: %s\n", response);
			}
			else
				printf("Got: %s\n", data);
		}
	}
//	goto start;

	SocketShutDown();
	return 0;

	GSI_UNUSED(argv);
	GSI_UNUSED(argc);
}
