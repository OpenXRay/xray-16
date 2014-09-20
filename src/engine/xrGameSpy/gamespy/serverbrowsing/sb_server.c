#include "sb_internal.h"
#include "sb_ascii.h"

//for the unique value list
#if defined(_NITRO)
	#define LIST_NUMKEYBUCKETS 100
	#define LIST_NUMKEYCHAINS 2
#else
	#define LIST_NUMKEYBUCKETS 500
	#define LIST_NUMKEYCHAINS 4
#endif

// for unicode version of key/value pairs
#define UKEY_LENGTH_MAX		255
#define UVALUE_LENGTH_MAX	255

#ifndef EXTERN_REFSTR_HASH
//global, shared unique value list
#if defined(_WIN32) && !defined(_DLL) && !defined (_USRDLL) && !defined(_MANAGED) && defined(GM_2B)
//for gmaster2b
__declspec( thread ) 
#endif
HashTable g_SBRefStrList = NULL;
#endif


/***********
 * REF COUNTING STRING HASHTABLE FUNCTIONS
 **********/
static int StringHash(const char *s, int numbuckets);
static int RefStringHash(const void *elem, int numbuckets)
{
	return StringHash(((SBRefString *)elem)->str, numbuckets);
}



/* keyval
* Compares two gkeyvaluepair 
*/

static int GS_STATIC_CALLBACK RefStringCompare(const void *entry1, const void *entry2)
{
   	return strcasecmp(((SBRefString *)entry1)->str,((SBRefString *)entry2)->str);
}


static void RefStringFree(void *elem)
{
	gsifree((char *)((SBRefString *)elem)->str);
#ifdef GSI_UNICODE
	gsifree((unsigned short *)((SBRefString *)elem)->str_W);
#endif
}


#ifndef EXTERN_REFSTR_HASH

HashTable SBRefStrHash(SBServerList *slist)
{
	if (g_SBRefStrList == NULL)
		g_SBRefStrList = TableNew2(sizeof(SBRefString),LIST_NUMKEYBUCKETS,LIST_NUMKEYCHAINS,RefStringHash, RefStringCompare, RefStringFree);

	GSI_UNUSED(slist);
	return g_SBRefStrList;
}

void SBRefStrHashCleanup(SBServerList *slist)
{
	if (g_SBRefStrList != NULL && TableCount(g_SBRefStrList) == 0)
	{
		TableFree(g_SBRefStrList);
		g_SBRefStrList = NULL;
	}

	GSI_UNUSED(slist);
}

#endif


void SBServerFree(void *elem)
{
	SBServer server = *(SBServer *)elem;
	//free all the keys..
	TableFree(server->keyvals);
	server->keyvals = NULL;
	gsifree(server);
}

void SBServerAddKeyValue(SBServer server, const char *keyname, const char *value)
{
	SBKeyValuePair kv;
	kv.key = SBRefStr(NULL, keyname);
	kv.value = SBRefStr(NULL, value);
	TableEnter(server->keyvals, &kv);

	gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Misc, GSIDebugLevel_Comment,
			"SBServerAddKeyValue added %s\\%s\r\n", keyname, value);
}

void SBServerAddIntKeyValue(SBServer server, const char *keyname, int value)
{
	char stemp[20];
	sprintf(stemp, "%d", value);
	SBServerAddKeyValue(server, keyname, stemp);
}

typedef struct 
{
	SBServerKeyEnumFn EnumFn;
	void *instance;
} SBServerEnumData;

/* ServerEnumKeys 
-----------------
Enumerates the keys/values for a given server by calling KeyEnumFn with each
key/value. The user-defined instance data will be passed to the KeyFn callback */

static void KeyMapF(void *elem, void *clientData)
{
	SBKeyValuePair *kv = (SBKeyValuePair *)elem;
	SBServerEnumData *ped = (SBServerEnumData *)clientData;

#ifndef GSI_UNICODE
	ped->EnumFn((char *)kv->key, (char *)kv->value, ped->instance);
#else
	unsigned short key_W[UKEY_LENGTH_MAX];
	unsigned short value_W[UVALUE_LENGTH_MAX];
	UTF8ToUCS2String(kv->key, key_W);
	UTF8ToUCS2String(kv->value, value_W);
	ped->EnumFn(key_W, value_W, ped->instance);
#endif
}
void SBServerEnumKeys(SBServer server, SBServerKeyEnumFn KeyFn, void *instance)
{
	SBServerEnumData ed;

	ed.EnumFn = KeyFn;
	ed.instance = instance;
	TableMap(server->keyvals, KeyMapF, &ed);
}


const char *SBServerGetStringValueA(SBServer server, const char *keyname, const char *def)
{
	SBKeyValuePair kv, *ptr;
	
	// 11-03-2004 : Saad Nader
	// Check if we are getting a valid server 
	//  before doing anything!
	///////////////////////////////////////////
	assert(server);
	if (!server)
		return NULL;
	kv.key = keyname;
	ptr =  (SBKeyValuePair *)TableLookup(server->keyvals, &kv);
	if (ptr == NULL)
		return def;
	return ptr->value;
}
#ifdef GSI_UNICODE
const unsigned short *SBServerGetStringValueW(SBServer server, const unsigned short *keyname, const unsigned short *def)
{
	char* keyname_A = UCS2ToUTF8StringAlloc(keyname);
	const char* value = SBServerGetStringValueA(server, keyname_A, NULL);
	if (value == NULL)
		return def;
	else
	{
		// Since we need the unicode version, we have to dig down to the 
		// reference counted string structure
		SBRefString ref, *val;
		ref.str = value;
		val = (SBRefString *)TableLookup(SBRefStrHash(NULL), &ref);
		if (val == NULL)
			return def; // this shouldn't happen

		return val->str_W;
	}
}
#endif

int SBServerGetIntValueA(SBServer server, const char *key, int idefault)
{
	const char *s, *s2;
	// check assumtions during development
	GS_ASSERT(key != NULL);
	GS_ASSERT(server != NULL);
	if (server == NULL)
		return idefault;
	if (strcmp(key,"ping") == 0) //ooh! they want the ping!
		return SBServerGetPing(server);
	s = SBServerGetStringValueA(server, key, NULL);
	if (s == NULL)
		return idefault;
	s2 = (*s != '-') ? s : s+1;  // check for signed values
	if (!isdigit((unsigned char)*s2)) // empty-string/non-numeric should return idefault
		return idefault;
	else
		return atoi(s);
}
#ifdef GSI_UNICODE
int SBServerGetIntValueW(SBServer server, const unsigned short *key, int idefault)
{
	char keyname_A[255];
	UCS2ToUTF8String(key, keyname_A);
	return SBServerGetIntValueA(server, keyname_A, idefault);
}
#endif

double SBServerGetFloatValueA(SBServer server, const char *key, double fdefault)
{
	const char *s;
	s = SBServerGetStringValueA(server, key, NULL);
	if (s == NULL)
		return fdefault;
	else
		return atof(s);
}
#ifdef GSI_UNICODE
double SBServerGetFloatValueW(SBServer server, const unsigned short *key, double fdefault)
{
	char keyname_A[255];
	UCS2ToUTF8String(key, keyname_A);
	return SBServerGetFloatValueA(server, keyname_A, fdefault);
}
#endif

SBBool SBServerGetBoolValueA(SBServer server, const char *key, SBBool bdefault)
{
	const char *s;
	s = SBServerGetStringValueA(server, key, NULL);
	if (!s || !s[0])
		return bdefault;

	// check the first char for known "false" values
	if('0' == s[0]|| 'F' == s[0] || 'f' == s[0] || 'N' == s[0] || 'n' == s[0])
		return SBFalse;

	// presume that all other non-zero values are "true"
	return SBTrue;
}
#ifdef GSI_UNICODE
SBBool SBServerGetBoolValueW(SBServer server, const unsigned short *key, SBBool bdefault)
{
	char keyname_A[255];
	UCS2ToUTF8String(key, keyname_A);
	return SBServerGetBoolValueA(server, keyname_A, bdefault);
}
#endif

const char *SBServerGetPlayerStringValueA(SBServer server, int playernum, const char *key, const char *sdefault)
{
	char keyname[128];
	sprintf(keyname, "%s_%d", key, playernum);
	return SBServerGetStringValueA(server, keyname, sdefault);
}
#ifdef GSI_UNICODE
const unsigned short *SBServerGetPlayerStringValueW(SBServer server, int playernum, const unsigned short *key, const unsigned short *sdefault)
{
	char keyname_A[UKEY_LENGTH_MAX];
	char default_A[UKEY_LENGTH_MAX];
	const char* value_A = NULL;
	UCS2ToUTF8String(key, keyname_A);
	UCS2ToUTF8String(sdefault, default_A);
	
	value_A = SBServerGetPlayerStringValueA(server, playernum, keyname_A, default_A);
	if (value_A == NULL)
		return sdefault;
	else
	{
		// Since we need the unicode version, we have to dig down to the SBRefString structure
		SBRefString ref, *val;
		ref.str = value_A;
		val = (SBRefString *)TableLookup(SBRefStrHash(NULL), &ref);
		if (val == NULL)
			return sdefault;
		return val->str_W;
	}
}
#endif

int SBServerGetPlayerIntValueA(SBServer server, int playernum, const char *key, int idefault)
{
	char keyname[128];
	sprintf(keyname, "%s_%d", key, playernum);
	return SBServerGetIntValueA(server, keyname, idefault);
}
#ifdef GSI_UNICODE
int SBServerGetPlayerIntValueW(SBServer server, int playernum, const unsigned short *key, int idefault)
{
	char keyname_A[UKEY_LENGTH_MAX];
	UCS2ToUTF8String(key, keyname_A);
	return SBServerGetPlayerIntValueA(server, playernum, keyname_A, idefault);
}
#endif

double SBServerGetPlayerFloatValueA(SBServer server, int playernum, const char *key, double fdefault)
{
	char keyname[128];
	sprintf(keyname, "%s_%d", key, playernum);
	return SBServerGetFloatValueA(server, keyname, fdefault);
}
#ifdef GSI_UNICODE
double SBServerGetPlayerFloatValueW(SBServer server, int playernum, const unsigned short *key, double fdefault)
{
	char keyname_A[UKEY_LENGTH_MAX];
	UCS2ToUTF8String(key, keyname_A);
	return SBServerGetPlayerFloatValueA(server, playernum, keyname_A, fdefault);
}
#endif

const char *SBServerGetTeamStringValueA(SBServer server, int teamnum, const char *key, const char *sdefault)
{
	char keyname[128];
	sprintf(keyname, "%s_t%d", key, teamnum);
	return SBServerGetStringValueA(server, keyname, sdefault);
}
#ifdef GSI_UNICODE
const unsigned short *SBServerGetTeamStringValueW(SBServer server, int teamnum, const unsigned short *key, const unsigned short *sdefault)
{
	char keyname_A[UKEY_LENGTH_MAX];
	char default_A[UKEY_LENGTH_MAX];
	const char* value_A = NULL;
	UCS2ToUTF8String(key, keyname_A);
	UCS2ToUTF8String(sdefault, default_A);
	value_A = SBServerGetTeamStringValueA(server, teamnum, keyname_A, default_A);

	if (value_A == NULL)
		return sdefault;
	else
	{
		// Since we need the unicode version, we have to dig down to the SBRefString structure
		SBRefString ref, *val;
		ref.str = value_A;
		val = (SBRefString *)TableLookup(SBRefStrHash(NULL), &ref);
		if (val == NULL)
			return sdefault;
		return val->str_W;
	}
}
#endif

int SBServerGetTeamIntValueA(SBServer server, int teamnum, const char *key, int idefault)
{
	char keyname[128];
	sprintf(keyname, "%s_t%d", key, teamnum);
	return SBServerGetIntValueA(server, keyname, idefault);
}
#ifdef GSI_UNICODE
int SBServerGetTeamIntValueW(SBServer server, int teamnum, const unsigned short *key, int idefault)
{
	char keyname_A[UKEY_LENGTH_MAX];
	UCS2ToUTF8String(key, keyname_A);
	return SBServerGetTeamIntValueA(server, teamnum, keyname_A, idefault);
}
#endif

double SBServerGetTeamFloatValueA(SBServer server, int teamnum, const char *key, double fdefault)
{
	char keyname[128];
	sprintf(keyname, "%s_t%d", key, teamnum);
	return SBServerGetFloatValueA(server, keyname, fdefault);
}
#ifdef GSI_UNICODE
double SBServerGetTeamFloatValueW(SBServer server, int teamnum, const unsigned short *key, double fdefault)
{
	char keyname_A[UKEY_LENGTH_MAX];
	UCS2ToUTF8String(key, keyname_A);
	return SBServerGetTeamFloatValueA(server, teamnum, keyname_A, fdefault);
}
#endif

SBBool SBServerHasBasicKeys(SBServer server)
{
	return (((server->state & STATE_BASICKEYS) == STATE_BASICKEYS) ? SBTrue : SBFalse);
}

SBBool SBServerHasFullKeys(SBServer server)
{
	return (((server->state & STATE_FULLKEYS) == STATE_FULLKEYS) ? SBTrue : SBFalse);
}

SBBool SBServerHasValidPing(SBServer server)
{
	return (((server->state & STATE_VALIDPING) == STATE_VALIDPING) ? SBTrue : SBFalse);
}


char *SBServerGetPublicAddress(SBServer server)
{
	return (char *)inet_ntoa(*(struct in_addr *)&server->publicip);
}

unsigned int SBServerGetPublicInetAddress(SBServer server)
{
	return server->publicip;
}


unsigned short SBServerGetPublicQueryPort(SBServer server)
{
	return ntohs(server->publicport);
}

unsigned short SBServerGetPublicQueryPortNBO(SBServer server)
{
	return server->publicport;
}

SBBool SBServerHasPrivateAddress(SBServer server)
{
	return (((server->flags & PRIVATE_IP_FLAG) == PRIVATE_IP_FLAG) ? SBTrue : SBFalse);
}



SBBool SBServerDirectConnect(SBServer server)
{
	return (((server->flags & UNSOLICITED_UDP_FLAG) == UNSOLICITED_UDP_FLAG) ? SBTrue : SBFalse);
}

char *SBServerGetPrivateAddress(SBServer server)
{
	return (char *)inet_ntoa(*(struct in_addr *)&server->privateip);
}

unsigned int SBServerGetPrivateInetAddress(SBServer server)
{
	return server->privateip;
}


unsigned short SBServerGetPrivateQueryPort(SBServer server)
{
	return ntohs(server->privateport);
}

void SBServerSetNext(SBServer server, SBServer next)
{
	server->next = next;
}

SBServer SBServerGetNext(SBServer server)
{
	return server->next;
}

static int CheckValidKey(char *key)
{
	const char *InvalidKeys[]={"queryid","final"};
	int i;
	for (i = 0; i < sizeof(InvalidKeys)/sizeof(InvalidKeys[0]); i++)
	{
		if (strcmp(key,InvalidKeys[i]) == 0)
			return 0;
	}
	return 1;
}

static char *mytok(char *instr, char delim)
{
	char *result;
	static char *thestr;
	
	if (instr)
		thestr = instr;
	result=thestr;
	while (*thestr && *thestr != delim)
	{
		thestr++;
	}
	if (thestr == result)
		result = NULL;
	if (*thestr) //not the null term
		*thestr++ = '\0';
	return result;
}


void SBServerParseKeyVals(SBServer server, char *keyvals)
{
	char *k, *v;
	
	k = mytok(++keyvals,'\\'); //skip over starting backslash
	while (k != NULL)
	{
		v = mytok(NULL,'\\');
		if (v == NULL)
			v = "";
		if (CheckValidKey(k))
		{
			//add the value if its not a Query-From-Master-Only key
			if (!qr2_internal_is_master_only_key(k))
			{
				SBServerAddKeyValue(server, k, v);
			}	
		}
		k = mytok(NULL,'\\');		
	}
}


/*
Query Response Format
Packet Type: 1 Byte
Request Key: 4 Bytes (copied from request packet)

Server Keys (if number of keys requested > 0)
If server key list specified:
Server Values: NTS, one per key specified
If server key list not specified:
Server Keys / Server Values: NTS Pairs, terminated with NUL
	
Player Keys (if number of keys requested > 0)
Number of Players: 2 Bytes
If player key list NOT specified
	Player Keys: NTS, one per key, terminated with NUL
Player Values: NTS, one per key specified, per player

Team Keys (if number of keys requested > 0)
Number of Teams: 2 Bytes
If team key list NOT specified
	Team Keys: NTS, one per key, terminated with NUL
Team Values: NTS, one per key specified, per team

*/
void SBServerParseQR2FullKeysSingle(SBServer server, char *data, int len)
{
	int dlen;
	char *k;
	char *v;
	char *keys;
	int nkeys;
	unsigned short nunits;
	int pflag;
	int i,j;
	char tempkey[128];
	//first pull out all the server keys/values
	while (*data)
	{
		dlen = NTSLengthSB(data, len);
		if (dlen < 0)
			return; //not a full NTS
		k = data;
		data += dlen;
		len -= dlen;
		dlen = NTSLengthSB(data, len);
		if (dlen < 0)
			return; //not a full NTS
		v = data;
		data += dlen;
		len -= dlen;
		
		//add the value if its not a Query-From-Master-Only key
		if (!qr2_internal_is_master_only_key(k))
		{
			SBServerAddKeyValue(server, k, v);
		}	
	}
	//skip the NUL
	data++;
	len--;
	//now get out the # of players (or teams) .. we do this whole thing 2X, once for players once for teams
	for (pflag = 0 ; pflag < 2 ; pflag++)
	{
		if (len < 2)
			return;
		memcpy(&nunits, data, 2);
		nunits = ntohs(nunits);
		data += 2;
		len -= 2;
		keys = data;
		nkeys = 0;
		//count up the number of keys..
		while (*data)
		{
			dlen = NTSLengthSB(data, len);
			if (dlen < 0)
				return; //not all there
			if (dlen > 100) //key is too long, may cause buffer overrun
				return;
			nkeys++;
			data += dlen;
			len -= dlen;
		}
		//skip the NUL
		data++;
		len--;
		//now for each player/team
		for (i = 0 ; i < nunits ; i++)
		{
			k = keys;
			//for each key..
			for (j = 0 ; j < nkeys ; j++)
			{
				dlen = NTSLengthSB(data, len);
				if (dlen < 0)
					return; //not all there
				sprintf(tempkey, "%s%d", k, i);
				SBServerAddKeyValue(server, tempkey, data);
				data += dlen;
				len -= dlen;
				k += strlen(k) + 1; //skip to the next key
			}
		}		
	}

}


// FullKeys with split response support
//   Goes something like:
//       [qr2header]["splitnum"][num (byte)][keytype]
//            if keytype is server, read KV's until a NULL
//            otherwise read a keyname, then a number of values until NULL
#define QR2_SPLITPACKET_NUMSTRING "splitnum"
#define QR2_SPLITPACKET_MAX	      7      // -xxxxxxx
#define QR2_SPLITPACKET_FINAL     (1<<7) // x-------
void SBServerParseQR2FullKeysSplit(SBServer server, char *data, int len)
{
	int dlen;
	char *k;
	char *v;
	unsigned int packetNum = 0;
	gsi_bool isFinal = gsi_false;

	// make sure it's valid
	if (*data == '\0')
		return;
	
	// data should have "splitnum" followed by BINARY key
	dlen = NTSLengthSB(data, len);
	if (dlen < 0)
		return;
	k = data;
	data += dlen;
	len -= dlen;
	
	if (strncasecmp(QR2_SPLITPACKET_NUMSTRING,k,strlen(QR2_SPLITPACKET_NUMSTRING))!=0)
		return;
	if (len < 1)
		return;

	packetNum = (unsigned int)((unsigned char)*data);
	data++;
	len--;

	// check final flag
	if ((packetNum & QR2_SPLITPACKET_FINAL) == QR2_SPLITPACKET_FINAL)
	{
		isFinal = gsi_true;
		packetNum ^= QR2_SPLITPACKET_FINAL;
	}

	// sanity check the packet num
	if (packetNum > QR2_SPLITPACKET_MAX)
		return;

	// update the received flags
	if (isFinal == SBTrue)
		// mark all bits higher than the final packet
		server->splitResponseBitmap |= (char)(0xFF<<packetNum); 
	else
		// mark only the bit for the received packet
		server->splitResponseBitmap |= (1<<packetNum);

	// any data in this packet? (could be a final tag)
	if (len < 1)
		return;

	// Parse the key sections (server/player/team)
	while(len > 0)
	{
		int keyType = 0;
		int nindex = 0;

		// Read the key type
		keyType = *data;
		data++;
		len--;

		if (keyType < 0 || keyType > 2)
		{
			gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
					"Split packet parse error, invalid key type! (%d)\r\n", keyType);
			return; // invalid key type!
		}

		// read keys until <null> section terminator
		while(*data)
		{
			// Read key name
			dlen = NTSLengthSB(data, len);
			if (dlen < 0)
				return;
			k = data;
			data += dlen;
			len -= dlen;
				
			if (keyType == 0)
			{
				// read the server key value
				dlen = NTSLengthSB(data, len);
				if (dlen < 0)
					return;
				v = data;
				data += dlen;
				len -= dlen;

				//add the value if its not a Query-From-Master-Only key
				if (!qr2_internal_is_master_only_key(k))
				{
					SBServerAddKeyValue(server, k, v);
				}	
			}
			else
			{
				char tempkey[128];

				// Read first player/team number
				if (len < 1)
					return;

				nindex = *data;
				data++;
				len--;

				// read values until <null>
				while(*data)
				{
					// read the value
					dlen = NTSLengthSB(data, len);
					if (dlen < 0)
						return;
					v = data;
					data += dlen;
					len -= dlen;
					// append team or player index before adding
					sprintf(tempkey, "%s%d", k, nindex);
					SBServerAddKeyValue(server, tempkey, v);
					nindex++; // index increments from start
				}

				// skip the null (key terminator)
				if (len > 0)
				{
					data++;
					len--;
				}
			}
		} 

		// skip the null (section terminator)
		if (len > 0)
		{
			if (*data != '\0')
			{
				gsDebugFormat(GSIDebugCat_SB, GSIDebugType_Network, GSIDebugLevel_WarmError,
					"Split packet parse error, NULL byte expected!\r\n");
				return; // ERROR!
			}
			data++;
			len--;
		}
	}
}

/***********
 * UTILITY FUNCTIONS
 **********/
#define MULTIPLIER -1664117991
static int StringHash(const char *s, int numbuckets)
{
	goa_uint32 hashcode = 0;
	while (*s != 0)
	{
		hashcode = (goa_uint32)((int)hashcode * MULTIPLIER + tolower(*s));
		s++;
	}
    return (int)(hashcode % numbuckets);

}

static void KeyValFree(void *elem)
{
	SBKeyValuePair *kv = (SBKeyValuePair *)elem;
	SBReleaseStr(NULL, kv->key);
	SBReleaseStr(NULL, kv->value);
}

static int KeyValHashKey(const void *elem, int numbuckets)
{
	return StringHash(((SBKeyValuePair *)elem)->key, numbuckets);
}


static int GS_STATIC_CALLBACK KeyValCompareKey(const void *entry1, const void *entry2)
{
	GS_ASSERT(entry1)
	GS_ASSERT(entry2)
	if	(
			(((SBKeyValuePair *)entry1)->key == NULL)  ||
			(((SBKeyValuePair *)entry2)->key == NULL)  
		)
		return 1;// treat NULL as not the same

	return strcasecmp(((SBKeyValuePair *)entry1)->key, ((SBKeyValuePair *)entry2)->key);
}


int SBServerGetPing(SBServer server)
{
	return (int) server->updatetime;
}

#define NUM_BUCKETS 8
#define NUM_CHAINS 4

//todo: benchmark sorted darray vs. hashtable - memory + speed
SBServer SBAllocServer(SBServerList *slist, goa_uint32 publicip, unsigned short publicport)
{
	SBServer server;
	server = (SBServer)gsimalloc(sizeof(struct _SBServer));
	if (server == NULL)
		return NULL;
	server->keyvals = TableNew2(sizeof(SBKeyValuePair),NUM_BUCKETS,NUM_CHAINS, KeyValHashKey, KeyValCompareKey, KeyValFree);
	if (server->keyvals == NULL)
	{
		gsifree(server);
		return NULL;
	}
	server->state = 0;
	server->flags = 0;
	server->next = NULL;
	server->updatetime = 0;
	server->icmpip = 0;
	server->publicip = publicip;
	server->publicport = publicport;
	server->privateip = 0;
	server->privateport = 0;

	GSI_UNUSED(slist);
	return server;	
}



void SBServerSetFlags(SBServer server, unsigned char flags)
{
	server->flags = flags;
}
void SBServerSetPublicAddr(SBServer server, goa_uint32 ip, unsigned short port)
{
	server->publicip = ip;
	server->publicport = port;

}
void SBServerSetPrivateAddr(SBServer server, goa_uint32 ip, unsigned short port)
{
	server->privateip = ip;
	server->privateport = port;

}
void SBServerSetICMPIP(SBServer server, goa_uint32 icmpip)
{
	server->icmpip = icmpip;

}
void SBServerSetState(SBServer server, unsigned char state)
{
	server->state = state;

}
unsigned char SBServerGetState(SBServer server)
{
	return server->state;

}
unsigned char SBServerGetFlags(SBServer server)
{
	return server->flags;

}

int SBIsNullServer(SBServer server)
{
	return (server == SBNullServer) ? 1 : 0;
}

SBServer SBNullServer = NULL;
