#define KEY_LENGTH 8 //64 bit
#define CHECK_LENGTH 2 //16 bit
#define MAX_EXTRA_DATA_LENGTH 16 //16 bytes of data
#define MAX_ENCODED_KEY (((MAX_EXTRA_DATA_LENGTH + KEY_LENGTH + CHECK_LENGTH) * 8 + 4) / 5)
#include "base32.h"
#include <string.h>

int DecodeKeyData(const char *key, unsigned char *extradata)
{
	char cleankey[MAX_ENCODED_KEY + 1];
	unsigned char keyandcheck[MAX_EXTRA_DATA_LENGTH + KEY_LENGTH + CHECK_LENGTH];
	int keybytes;
	int extradatalen;
	int i;

	if (!CleanForBase32(cleankey, key, MAX_ENCODED_KEY + 1))
		return 0;
	keybytes = ConvertFromBase32((char*)keyandcheck, cleankey, (int)strlen(cleankey));
	if (keybytes <= 0)
		return 0; //decoded incorrectly
	extradatalen = keybytes - KEY_LENGTH - CHECK_LENGTH;
	if (extradatalen <= 0)
		return 0;
	for (i = 0 ; i < extradatalen ; i++)
		extradata[i] = keyandcheck[i] ^ keyandcheck[extradatalen + (i % KEY_LENGTH)];
	return extradatalen;	
}



static unsigned short CreateCheck(unsigned char *key, int keylen, unsigned short cskey)
{
	int i;
	unsigned int check = 0;

	for (i = 0 ; i < keylen ; i++)
	{
		check = check * 0x9CCF9319 + key[i];
	}
	return (((unsigned short)(check % 65521)) ^ cskey);
}



int VerifyClientCheck(const char *key, unsigned short cskey)
{
	char cleankey[MAX_ENCODED_KEY + 1] = "";
	unsigned char keyandcheck[MAX_EXTRA_DATA_LENGTH + KEY_LENGTH + CHECK_LENGTH] = "";
	int keybytes = 0;
	int extradatalen = 0;
	unsigned short correctcheck = 0;

	if (!CleanForBase32(cleankey, key, MAX_ENCODED_KEY + 1))
		return 0;
	keybytes = ConvertFromBase32((char*)keyandcheck, cleankey, (int)strlen(cleankey));
	if (keybytes <= 0)
		return 0; //decoded incorrectly
	extradatalen = keybytes - KEY_LENGTH - CHECK_LENGTH;

	correctcheck = CreateCheck(keyandcheck, KEY_LENGTH + extradatalen, cskey);
	return (correctcheck == *(unsigned short *)(keyandcheck + extradatalen + KEY_LENGTH)) ? 1 : 0;
}

