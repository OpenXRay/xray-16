/******
gcdkeys.c
GameSpy CDKey SDK Server Code
  
Copyright 1999-2001 GameSpy Industries, Inc

18002 Skypark Circle
Irvine, California 92614
949.798.4200 (Tel)
949.798.4299 (Fax)
devsupport@gamespy.com

******

 Please see the GameSpy CDKey SDK documentation for more 
 information

******/

/********
INCLUDES
********/

#include "gcdkeys.h"
#include "../common/gsCommon.h"
#include "../common/gsAvailable.h"
#include "../common/gsDebug.h"
#include <time.h>

#ifdef GUSE_ASSERTS
	#define gassert(a) assert(a)
#else
	#define gassert(a)
#endif

#define BUFSIZE 512

#ifdef __cplusplus
extern "C" {
#endif

/********
DEFINES
********/
#define VAL_PORT 29910
/* #define VAL_ADDR "key.gamespy.com" */
/*#define VAL_ADDR "204.182.161.103"*/
#define VAL_TIMEOUT 2000
#define VAL_RETRIES 2
#define INBUF_LEN 1024
#define MAX_PRODUCTS 4
#define MAX_KEEP_ALIVE_INTERVAL 20000

#define MAXPENDING_REAUTH 5    // prevent memory growth from spammed reauths.
#define REAUTH_LIFESPAN 5000   // prevent memory growth from unanswered reauths.
#define PROOF_TXT     'p','r','o','o','f'
#define IGNORED_TXT   's','e','e','d'

/********
TYPEDEFS
********/
typedef enum {cs_sentreq, cs_gotok, cs_gotnok, cs_done} gsclientstate_t;

typedef struct gsnode_s
{
	void *object;
	struct gsnode_s *next, *prev;
} gsnode_t;

typedef struct gsclient_s
{
	int localid;
	char hkey[33];
	int sesskey;
	int ip;
	unsigned long sttime;
	int ntries;
	gsclientstate_t state;
	void *instance;
	AuthCallBackFn authfn;
	RefreshAuthCallBackFn refreshauthfn;
	char *errmsg;
	char *reqstr;
	int reqlen;
	gsnode_t reauthq;
} gsclient_t;

typedef struct gsreauth_s
{
	int sesskey;
	char challenge[33];
	struct sockaddr_in fromaddr;
	gsi_time starttime;
} gsreauth_t;

typedef struct gsproduct_s
{
	int pid;
	gsnode_t clientq;
} gsproduct_t;


/********
GLOBALS
********/
char gcd_hostname[64] = "";

/********
PROTOTYPES
********/
static void send_auth_req(gsproduct_t *prod, gsclient_t *client, const char *challenge, const char *response);
static void resend_auth_req(gsclient_t *client);
static void send_keep_alive();
static void send_disconnect_req(gsproduct_t *prod, gsclient_t *client);
static void cdkey_process_buf(char *buf, int len, struct sockaddr *fromaddr);
static void process_oks(char *buf, int isok);
static void process_ison(char *buf, struct sockaddr_in *fromaddr);
static void process_ucount(char *buf, struct sockaddr_in *fromaddr);
static void send_uon(int skey, const char* ignored, const char* proof, struct sockaddr_in *fromaddr);
static void free_client_node(gsnode_t *node);

static int get_sockaddrin(char *host, int port, struct sockaddr_in *saddr, struct hostent **savehent);
static void xcode_buf(char *buf, int len);
static char *value_for_key(const char *s, const char *key);


static void add_to_queue(gsnode_t *t, gsnode_t *que);
static gsnode_t *remove_from_queue(gsnode_t *t, gsnode_t *que);
static int gcd_init_common(int gameid);
static int init_incoming_socket();
static gsproduct_t *find_product(int gameid);

/********
VARS
********/
static SOCKET sock = INVALID_SOCKET;
static unsigned short localport = 0;
static char enc[9]; /* used for xor encoding */
static struct sockaddr_in valaddr;

static int numproducts = 0;
gsproduct_t products[MAX_PRODUCTS];

/****************************************************************************/
/* PUBLIC FUNCTIONS */
/****************************************************************************/


int gcd_init(int gameid)
{
	int ret;
	const char defaulthost[] =  {'k','e','y','.','g','a','m','e','s','p','y','.','c','o','m','\0'}; //key.gamespy.com

	// check if the backend is available
	if(__GSIACResult != GSIACAvailable)
		return -1;

	if (sock == INVALID_SOCKET) //hasn't been initialized yet
	{
		/* set up the UDP socket */
		SocketStartUp();
		ret = init_incoming_socket();
		if (ret < 0)
			return ret;

		if (gcd_hostname[0] == 0)
			strcpy_s(gcd_hostname, sizeof(gcd_hostname), defaulthost);
		get_sockaddrin(gcd_hostname,VAL_PORT,&valaddr,NULL);
	}


	return gcd_init_common(gameid);

}


#ifdef QR2CDKEY_INTEGRATION
extern struct qr2_implementation_s static_qr2_rec;
int gcd_init_qr2(qr2_t qrec, int gameid)
{
	// check if the backend is available
	if(__GSIACResult != GSIACAvailable)
		return -1;

	if (qrec == NULL)
		qrec = &static_qr2_rec;

	localport = (unsigned short)-1; /* we don't process any incoming data ourselves - it gets passed from the QR SDK */

	sock = qrec->hbsock; 
	qrec->cdkeyprocess = cdkey_process_buf;
	/* grab the outgoing address from the QR SDK */
	memset(&valaddr,0,sizeof(struct sockaddr_in));
	valaddr.sin_family = AF_INET;
	valaddr.sin_port = htons((unsigned short)VAL_PORT);
	valaddr.sin_addr.s_addr = qrec->hbaddr.sin_addr.s_addr;
	return gcd_init_common(gameid);
	
}

#endif


void gcd_shutdown(void)
{
	int i;
	/* Make sure everyone is disconnected */
	for (i = 0 ; i < numproducts ; i++)
		gcd_disconnect_all(products[i].pid);
	if(localport != (unsigned short)-1)
	{
		closesocket(sock);
		SocketShutDown();
	}
	sock = INVALID_SOCKET;
	numproducts = 0;
}


void gcd_authenticate_user(int gameid, int localid, unsigned int userip, const char *challenge, 
						   const char *response, AuthCallBackFn authfn, RefreshAuthCallBackFn refreshfn, void *instance)
{
	gsnode_t *node;
	gsclient_t *client;
	char hkey[33];
	char *errmsg = NULL;
	char badcdkey_t[] = {'B','a','d',' ','C','D',' ','K','e','y','\0'}; //Bad CD Key
	char keyinuse_t[] = {'C','D',' ','K','e','y',' ','i','n',' ','u','s','e','\0'}; //CD Key in use
	gsproduct_t *prod = find_product(gameid);

	gassert(prod);
	if (prod == NULL)
		return;

	 /* get the hashed key */
	strncpy(hkey, response, 32);
	hkey[32] = 0;

	/* if response is bogus, lets kill them */
	if (strlen(response) < 72) 
		errmsg = goastrdup(badcdkey_t);

	/* First, scan the current list for the same, or similar client */
	node = &prod->clientq;
	while ((node = node->next) != NULL)
	{
		/* make sure the localid isn't being reused 
	Change this code if you want to allow multiple users with the same CD Key on the
	same server */
		gsclient_t* client = (gsclient_t*)node->object;
		gassert(client->localid != localid); 
		if (strcmp(hkey, client->hkey) == 0) 
		{ /* they appear to be on already!! */
			errmsg = goastrdup(keyinuse_t);
			break;
		}
	} 

	/* Create a new client */
	client = (gsclient_t *)gsimalloc(sizeof(gsclient_t));
	gassert(client);
	client->localid = localid;
	client->ip = (int)userip;
	client->instance = instance;
	client->errmsg = NULL;
	client->reqstr = NULL;
	client->authfn = authfn;
	client->refreshauthfn = refreshfn;
	client->reauthq.next = NULL;
	client->reauthq.object = NULL;
	client->reauthq.prev = NULL;
	strcpy_s(client->hkey, sizeof(client->hkey), hkey);
	node = (gsnode_t *)gsimalloc(sizeof(gsnode_t));
	gassert(node);
	node->object = (void*)client;
	add_to_queue(node, &prod->clientq);

	if (errmsg != NULL) 
	{ /* there was already and error, mark them to die */
		client->state = cs_gotnok;
		client->errmsg = errmsg;
	} else 	/* They aren't on this server, lets check the validation server */
		send_auth_req(prod, client,challenge, response);	
}

void gcd_process_reauth(int gameid, int localid, int skey, const char *response)
{
	// find the pending reauth attempt
	gsnode_t *clientnode;
	gsnode_t *reauthnode;
	gsproduct_t *prod = find_product(gameid);
	
	gassert(prod);
	if (prod == NULL)
		return;

	// find the client for this gameid
	clientnode = &prod->clientq;
	while ((clientnode = clientnode->next) != NULL)
	{
		gsclient_t *client = (gsclient_t*)clientnode->object;
		if (client->localid == localid)
		{
			// find the reauth info for this client/skey
			reauthnode = &client->reauthq;
			while((reauthnode = reauthnode->next) != NULL)
			{
				gsreauth_t *reauth = (gsreauth_t*)reauthnode->object;
				if (reauth->sesskey == skey)
				{
					// send the proof to the keymaster
					send_uon(skey, "", response, &reauth->fromaddr);
					remove_from_queue(reauthnode, &client->reauthq);
					gsifree(reauthnode->object);
					gsifree(reauthnode);
					return;
				}
			}
		}
	}
}

// utility to free memory associated with a client node
static void free_client_node(gsnode_t *node)
{
	if (node)
	{
		gsclient_t* client = (gsclient_t*)node->object;
		if (client)
		{
			if (client->reqstr != NULL)
				gsifree(client->reqstr);
			if (client->errmsg != NULL)
				gsifree(client->errmsg);

			// free auth nodes
			while (client->reauthq.next != NULL)
			{
				gsnode_t* authNode = remove_from_queue(client->reauthq.next, &client->reauthq);
				gsifree(authNode->object);
				gsifree(authNode);
			}
			gsifree(client);
		}
		gsifree(node);
	}
	return;
}

void gcd_disconnect_user(int gameid, int localid)
{
	gsnode_t *node;
	gsproduct_t *prod = find_product(gameid);

	gassert(prod);
	if (prod == NULL)
		return;

	/* First, scan the list for the client*/
	node = &prod->clientq;
	while ((node = node->next) != NULL)
	{
		gsclient_t* client = (gsclient_t*)node->object;
		if (client->localid == localid)
		{
			send_disconnect_req(prod, client);
			remove_from_queue(node, &prod->clientq);
			free_client_node(node);
			return;
		}
	}
	/* No client found -- we should never get here! 
	But we may if you call disconnect_user during an negative authentication 
	(they are already removed) */

}

void gcd_disconnect_all(int gameid)
{
	gsnode_t *node;

	gsproduct_t *prod = find_product(gameid);

	gassert(prod);
	if (prod == NULL)
		return;

	/* Clear the entire list */
	node = &prod->clientq;
	while ((node = node->next) != NULL)
	{
		gsclient_t* client = (gsclient_t*)node->object;
		send_disconnect_req(prod, client);
		remove_from_queue(node, &prod->clientq);
		free_client_node(node);
		node = &prod->clientq;
	}
}

char *gcd_getkeyhash(int gameid, int localid)
{

	gsproduct_t *prod = find_product(gameid);
	gsnode_t *node;

	gassert(prod);
	if (prod == NULL)
		return "";

	node = &prod->clientq;

	/* Scan the list for the client*/
	while ((node = node->next) != NULL)
	{
		gsclient_t* client = (gsclient_t*)node->object;
		if (client->localid == localid)
			return client->hkey;
	}
	return "";	
}

void gcd_think(void)
{
	static char indata[INBUF_LEN]; 
	struct sockaddr_in saddr;
	int saddrlen = sizeof(saddr);
	fd_set set; 
	struct timeval timeout = {0,0};
	int error;
	int i;
	gsnode_t *node, *oldnode;
	char validated_t[] = {'V','a','l','i','d','a','t','e','d','\0'}; //Validated
	char timeout_t[] = {'V','a','l','i','d','a','t','i','o','n',' ','T','i','m','e','o','u','t','\0'}; //Validation Timeout

	gassert (sock != INVALID_SOCKET);
	/* First, check for data on the socket and process commands */

	if (localport != (unsigned short)-1) /* don't check if we are getting data from the QR SDK instead */
	{
		FD_ZERO ( &set );
		FD_SET ( sock, &set );
		while (1)
		{
			error = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
			if (gsiSocketIsError(error) || 0 == error)
				break;
			/* else we have data */
			error = recvfrom(sock, indata, INBUF_LEN - 1, 0, (struct sockaddr *)&saddr, &saddrlen);
			if (gsiSocketIsNotError(error))
			{
				indata[error] = '\0';
				cdkey_process_buf(indata, error, (struct sockaddr *)&saddr);
			}
		}
	}

	send_keep_alive();

	for (i = 0 ; i < numproducts ; i++)
	{		
		/* Next, update the status of any clients and make callbacks */
		node = &products[i].clientq;
		while ((node = node->next) != NULL)
		{
			gsclient_t* client = (gsclient_t*)node->object;
			switch (client->state)
			{
			case cs_sentreq:
				if (current_time() < client->sttime + VAL_TIMEOUT)
					break; /* keep waiting */
				if (client->ntries <= VAL_RETRIES)
				{ /* resend */
					resend_auth_req(client);
					break;
				} /* else, go ahead an auth them, the val server timed out */			
			case cs_gotok:
				 /* if authorized or they timed out with no response, just auth them */
					client->authfn(products[i].pid, client->localid, 1,
						client->state == cs_gotok ? validated_t : timeout_t,
						client->instance);
					client->state = cs_done;
					gsifree(client->reqstr);
					client->reqstr = NULL;
				break;
			case cs_gotnok:
				/* remove them first, in case the user calls disconnect */
				oldnode = node;
				node = node->prev;
				remove_from_queue(oldnode, &products[i].clientq);
				
				client->authfn(products[i].pid, client->localid, 0, 
					client->errmsg == NULL ? "" : client->errmsg,
					client->instance);
				free_client_node(oldnode);
				break;
			case cs_done:
				// check pending reauth timeouts 
				if (client->reauthq.next != NULL)
				{
					// always look at "next" because we may remove nodes
					gsnode_t* authnode = &client->reauthq;
					while(authnode->next != NULL)
					{
						gsreauth_t* authdata = (gsreauth_t*)authnode->next->object;
						gsi_time now = current_time();
						if ((now - authdata->starttime)  > REAUTH_LIFESPAN)
						{
							gsDebugFormat(GSIDebugCat_CDKey, GSIDebugType_Misc, GSIDebugLevel_Notice,
								"Removing timed out reauth request [localid: %d, from: %s\r\n", 
								client->localid, inet_ntoa(authdata->fromaddr.sin_addr));

							// timed out, delete it
							remove_from_queue(authnode->next, &client->reauthq);
							gsifree(authdata);
							gsifree(authnode->next);
							authnode->next = NULL;
						}
						else
							authnode = authnode->next;
					}
				}
				break;
			default:
				break;
			}
		} 
	}
}


/****************************************************************************/
/* UTIL FUNCTIONS */
/****************************************************************************/
static void cdkey_process_buf(char *buf, int len, struct sockaddr *fromaddr)
{
	char tok[32];
	char *pos;
	char uok_t[] = {'u','o','k','\0'}; //uok
	char unok_t[] = {'u','n','o','k','\0'}; //unok
	char ison_t[] = {'i','s','o','n','\0'}; //ison
	char ucount_t[] = {'u','c','o','u','n','t','\0'}; //ucount
	xcode_buf(buf, len);
	
	tok[0] = 0;
	if (buf[0] == '\\')
	{
		pos = strchr(buf+1,'\\');
		if (pos && (pos - buf <= 32)) /* right size token */
		{
			strncpy(tok, buf+1,pos-buf-1);
			tok[pos-buf-1] = 0;
		}
	}
	if (!tok[0])
		return; /* bad command */

	if (!strcmp(tok, uok_t))
	{
		process_oks(buf, 1);
	} 
	else if (!strcmp(tok, unok_t))
	{
		process_oks(buf, 0);
	} 
	else if (!strcmp(tok, ison_t))
	{
		process_ison(buf, (struct sockaddr_in *)fromaddr);
	} 
	else if (!strcmp(tok, ucount_t))
	{
		process_ucount(buf, (struct sockaddr_in *)fromaddr);
	}
	else
	{
		send_keep_alive();
		return; /* bad command */
	}
	send_keep_alive();
}

static int init_incoming_socket()
{
	int ret;
	struct sockaddr_in saddr;
	int saddrlen;
	
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
		return -1;
	get_sockaddrin(NULL,0,&saddr,NULL);
	ret = bind(sock, (struct sockaddr *)&saddr, sizeof(saddr));
	if (gsiSocketIsError(ret))
		return -1;
	
	saddrlen = sizeof(saddr);
	ret = getsockname(sock,(struct sockaddr *)&saddr, &saddrlen);
	if (gsiSocketIsError(ret))
		return -1;
	localport = saddr.sin_port;
	
	return 0;
}

static int gcd_init_common(int gameid)
{
	gsproduct_t *prod;
	gassert(numproducts < MAX_PRODUCTS);
	if (numproducts >= MAX_PRODUCTS)
		return -1; //too many products
	prod = &products[numproducts++];
	prod->pid = gameid;
	prod->clientq.next = NULL;
	prod->clientq.prev = NULL;
	prod->clientq.object = NULL;
	srand((unsigned int)current_time());
	enc[0]='g';enc[1]='a';enc[2]='m';enc[3]='e';
	enc[4]='s';enc[5]='p';enc[6]='y';enc[7]=0;	
	return 0;
}
static gsclient_t *find_client(char *keyhash, int sesskey, int* productid)
{
	gsnode_t *node;
	int i;

	for (i = 0 ; i < numproducts ; i++)
	{
		node = &products[i].clientq;
		while ((node = node->next) != NULL)
		{
			gsclient_t* client = (gsclient_t*)node->object;
			if (strcmp(keyhash, client->hkey) == 0 && (sesskey == -1 || client->sesskey == sesskey)) 
			{
				if (productid != NULL)
					*productid = products[i].pid;
				return client;
			}
		} 
	}
	return NULL;
}

static void process_oks(char *buf, int isok)
{
	int sesskey;
	char keyhash[33];
	gsclient_t *client;
	const char skey_t[] = {'s','k','e','y','\0'}; //skey
	const char cd_t[] = {'c','d','\0'}; //cd
	const char errmsg_t[] = {'e','r','r','m','s','g','\0'}; //errmsg
	
/* Samples
\uok\\cd\fe6667736f0c8ed7ff5cd9c0e74f\skey\2342
\unok\\cd\fe6667736f0c8ed7ff5cd9c0e74f\skey\23423\errmsg\Already playing on xyz server */
	sesskey = atoi(value_for_key(buf,skey_t));
	strncpy(keyhash,value_for_key(buf,cd_t),32);
	keyhash[32] = 0;
	
	client = find_client(keyhash, sesskey, NULL);
	if (!client)
		return;
	if (client->sesskey != sesskey) /* bad session key */
		return;
	if (client->state == cs_done) /* too late to do anything! */
		return;
	if (isok)
		client->state = cs_gotok;
	else
	{
		client->state = cs_gotnok;
		client->errmsg = goastrdup(value_for_key(buf,errmsg_t));
	}
	
}
static void process_ucount(char *buf, struct sockaddr_in *fromaddr)
{
	char outbuf[64];
	char ucountformat[] = {'\\','u','c','o','u','n','t','\\','%','d','\0'}; //\\ucount\\%d
	char pid_t[] = {'p','i','d','\0'}; //pid
	gsnode_t *node;
	int count = 0;
	int len;
	gsproduct_t *prod;
	char *pos = value_for_key(buf, pid_t);
	if (pos[0] == 0 && numproducts > 0) //not present.. use the first product
		prod = &products[0];
	else
		prod = find_product(atoi(pos));
	if (prod != NULL)
	{	
		node = &prod->clientq;
		while ((node = node->next) != NULL)
		{
			count++;
		} 	
	}
	len = sprintf(outbuf, ucountformat, count);
	xcode_buf(outbuf, len);
	sendto(sock, outbuf, len, 0, (struct sockaddr *)fromaddr, sizeof(struct sockaddr_in));
}

static void send_uon(int skey, const char* ignored, const char* proof, struct sockaddr_in *fromaddr)
{
	char outbuf[256];
	int len;
	const char uonformat[] = {'\\','u','o','n','\\','\\','s','k','e','y','\\','%','d','\\',IGNORED_TXT,'\\','%','s','\\',PROOF_TXT,'\\','%','s','\0'}; //\\uon\\\\skey\\%d\\seed\\%s\\proof\\%s
/* \ison\\cd\fe6667736f0c8ed7ff5cd9c0e74f\skey\32423 */
/* \uon\\skey\32423\seed\\proof\ OR \un\skey\32423\proof\fe6667736f0c8ed7ff5cd9c0e74f OR \uoff\\skey\32423 */

	// seed is ignored by server
	len = snprintf(outbuf, 255, uonformat,skey, ignored, proof);
	outbuf[255] = '\0'; // snprintf doesn't null terminate in some cases
	xcode_buf(outbuf, len);
	sendto(sock, outbuf, len, 0, (struct sockaddr *)fromaddr, sizeof(struct sockaddr_in));

	gsDebugFormat(GSIDebugCat_CDKey, GSIDebugType_Network, GSIDebugLevel_Notice,
		"Sent uon response (ison) to %s. (proof: %s)\r\n", 
		inet_ntoa(fromaddr->sin_addr), proof);
}

static void send_uoff(int skey, struct sockaddr_in *fromaddr)
{
	char outbuf[64];
	int len;
	const char uoffformat[] = {'\\','u','o','f','f','\\','\\','s','k','e','y','\\','%','d','\0'}; //\\uoff\\\\skey\\%d
	len = sprintf(outbuf, uoffformat,skey);
	xcode_buf(outbuf, len);
	sendto(sock, outbuf, len, 0, (struct sockaddr *)fromaddr, sizeof(struct sockaddr_in));
}

static int get_queue_size(gsnode_t* node)
{
	int count = 0;
	if (!node)
		return 0;
	if (node->object) // starting from a valid node
		count++;
	while (node->next != NULL)
	{
		count++;
		node = node->next;
	}
	return count;
}

static void process_ison(char *buf, struct sockaddr_in *fromaddr)
{
	int sesskey;
	int productid;
	char* proofchallenge;

	gsclient_t *client;

	const char proofchallenge_t[] = {'p','c','h','\0'}; // proof challenge
	const char skey_t[] = {'s','k','e','y','\0'}; //skey
	const char cd_t[] = {'c','d','\0'}; //cd
	
	sesskey = atoi(value_for_key(buf,skey_t));
	proofchallenge = value_for_key(buf,proofchallenge_t);
	if ( (client = find_client(value_for_key(buf,cd_t), -1, &productid)) != NULL 
		&& (client->state == cs_done)) /* If they are connected, return on */
	{
		// check the queue size to prevent memory growth (from malicious reauth requests)
		int count = get_queue_size(&client->reauthq);
		if (count < MAXPENDING_REAUTH)
		{
			// store the sesskey and fromaddr so we can respond with proof later
			gsnode_t* node = (gsnode_t*)gsimalloc(sizeof(gsnode_t));
			gsreauth_t* reauthdata = (gsreauth_t*)gsimalloc(sizeof(gsreauth_t));
			gassert(node);
			gassert(reauthdata);

			memcpy(reauthdata->challenge, proofchallenge, 32);
			memcpy(&reauthdata->fromaddr, fromaddr, sizeof(struct sockaddr_in));
			reauthdata->sesskey = sesskey;
			reauthdata->starttime = current_time();
			node->object = (void*)reauthdata;
			add_to_queue(node, &client->reauthq);
		
			// send normal ison right away, later we'll followup with proof
			// owatagusiam is ignored by server
			send_uon(sesskey, "owatagusiam", "0", fromaddr);
			
			// notify developer that we need proof of "ison"
			client->refreshauthfn(productid, client->localid, sesskey, proofchallenge, client->instance);
		}
	}
	else
	{
		send_uoff(sesskey, fromaddr);
	}
}

static void send_disconnect_req(gsproduct_t *prod, gsclient_t *client)
{
	char buf[BUFSIZE];
	int len;
	const char discformat[] = {'\\','d','i','s','c','\\','\\','p','i','d','\\','%','d','\\','c','d','\\','%','s','\\','i','p','\\','%','d','\0'}; //\\disc\\\\pid\\%d\\cd\\%s\\ip\\%d
	
/* \disc\\pid\12\cd\fe6667736f0c8ed7ff5cd9c0e74f\ip\2342342 */
	len = sprintf(buf,discformat,
					prod->pid, client->hkey,client->ip);
	xcode_buf(buf, len);
	sendto(sock, buf, len, 0, (struct sockaddr *)&valaddr, sizeof(valaddr));
}

static void send_auth_req(gsproduct_t *prod, gsclient_t *client, const char *challenge, const char *response)
{
	char buf[BUFSIZE];
	int len;
	const char authformat[] = {'\\','a','u','t','h','\\','\\','p','i','d','\\','%','d','\\','c','h','\\','%','s','\\','r','e','s','p','\\','%','s','\\','i','p','\\','%','d','\\','s','k','e','y','\\','%','d','\\','r','e','q','p','r','o','o','f','\\','1','\\','\0'}; //\\auth\\\\pid\\%d\\ch\\%s\\resp\\%s\\ip\\%d\\skey\\%d\\reqproof\\1

	client->state = cs_sentreq;
	client->sesskey = (unsigned int)(rand() ^ current_time()) % 16384;
	client->sttime = current_time();
	client->ntries = 1;
/* \auth\\pid\12\ch\efx3232\resp\fe6667736f0c8ed7ff5cd9c0e74f98fd69e4da39560b82f40a628522ed10f0165c1d44a0\ip\2342342\skey\132432 */
	len = snprintf(buf, BUFSIZE, authformat,
			prod->pid, challenge, response, client->ip, client->sesskey);
	buf[BUFSIZE-1] = '\0'; // sometimes snprintf doesn't null terminate
	xcode_buf(buf, len);
	sendto(sock, buf, len, 0, (struct sockaddr *)&valaddr, sizeof(valaddr));
	/* save a copy for resends */
	client->reqstr = (char *)gsimalloc(len);
	memmove(client->reqstr, buf, len);
	client->reqlen = len;
}

static void resend_auth_req(gsclient_t *client)
{
	client->sttime = current_time();
	client->ntries++;		
	sendto(sock, client->reqstr, client->reqlen, 0, (struct sockaddr *)&valaddr, sizeof(valaddr));
}

static void send_keep_alive()
{
	static gsi_time lastKeepAliveSent = 0;
	static const char *keepAlive = "\\ka\\\0";
	char buf[BUFSIZE];
	if (lastKeepAliveSent == 0)
		lastKeepAliveSent = current_time();
	if (current_time() > lastKeepAliveSent + MAX_KEEP_ALIVE_INTERVAL)
	{	
		strcpy_s(buf, sizeof(buf), keepAlive);
		xcode_buf(buf, strlen(keepAlive));
		sendto(sock, buf, strlen(keepAlive), 0, (struct sockaddr *)&valaddr, sizeof(struct sockaddr_in));
		lastKeepAliveSent = current_time();
	}
}
/* value_for_key: this returns a value for a certain key in s, where s is a string
containing key\value pairs. If the key does not exist, it returns  ""
Note: the value is stored in a common buffer. If you want to keep it, make a copy! */
static char *value_for_key(const char *s, const char *key)
{
	static int valueindex;
	char *pos,*pos2;
	char slash_t[] = {'\\','\0'}; 
	char keyspec[256];
	static char value[2][256];

	valueindex ^= 1;
	strcpy_s(keyspec, sizeof(keyspec), slash_t);
	strcat(keyspec,key);
	strcat(keyspec,slash_t);
	pos = strstr(s,keyspec);
	if (!pos)
		return "";
	pos += strlen(keyspec);
	pos2 = value[valueindex];
	while (*pos && *pos != '\\' && (pos2 - value[valueindex] < 200))
		*pos2++ = *pos++;
	*pos2 = '\0';
	return value[valueindex];
}

/* simple xor encoding */
static void xcode_buf(char *buf, int len)
{
	int i;
	char *pos = enc;

	for (i = 0 ; i < len ; i++)
	{
		buf[i] ^= *pos++;
		if (*pos == 0)
			pos = enc;
	}
}

/* Return a sockaddrin for the given host (numeric or DNS) and port)
Returns the hostent in savehent if it is not NULL */
static int get_sockaddrin(char *host, int port, struct sockaddr_in *saddr, struct hostent **savehent)
{
	struct hostent *hent;
	char broadcast_t[] = {'2','5','5','.','2','5','5','.','2','5','5','.','2','5','5','\0'}; //255.255.255.255

	memset(saddr,0,sizeof(struct sockaddr_in));
	saddr->sin_family = AF_INET;
	saddr->sin_port = htons((unsigned short)port);
	if (host == NULL)
		saddr->sin_addr.s_addr = INADDR_ANY;
	else
		saddr->sin_addr.s_addr = inet_addr(host);
	
	if (saddr->sin_addr.s_addr == INADDR_NONE && strcmp(host,broadcast_t) != 0)
	{
		hent = gethostbyname(host);
		if (!hent)
			return 0;
		saddr->sin_addr.s_addr = *(u_long *)hent->h_addr_list[0];
	}
	if (savehent != NULL)
		*savehent = hent;
	return 1;
} 

static gsproduct_t *find_product(int gameid)
{
	int i;
	for (i = 0 ; i < numproducts ; i++)
		if (products[i].pid == gameid)
			return &products[i];
	return NULL;
}


/***********
Linked List Code
***********/


/*******
add_to_queue
*******/
static void add_to_queue(gsnode_t *t, gsnode_t *que)
{
        while(que->next)
                que=que->next;
        que->next = t;
        t->prev = que;
        t->next = NULL;
}

/*******
remove_from_queue

if NULL is given as first parameter, top list item is popped off

item that is removed is returned, or NULL if not found
*******/
static gsnode_t *remove_from_queue(gsnode_t *t, gsnode_t *que)
{
        
        if(!t) t = que->next;
        if(!t) return(NULL);
        t->prev->next = t->next;
        if(t->next)
                t->next->prev = t->prev;

        return(t);
}

#ifdef __cplusplus
}
#endif
