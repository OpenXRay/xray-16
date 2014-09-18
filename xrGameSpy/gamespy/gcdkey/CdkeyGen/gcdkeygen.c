/******
gcdkeygen.c
GameSpy CDKey SDK CD Key Generation / Validation Example
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

  This source is simply an example of how you might generate
  CD Keys and how you would validate them on the client side.

  You can use this algorithm, or a derivation thereof, or your own,
  but always make sure that:
  1. Your valid keys are a SMALL subset of the possible keys
  2. The distribution of valid keys within the full set is even, but
     not regular.

 Please see the GameSpy CDKey SDK documentation for more 
 information

******/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <windows.h>

static void Util_RandSeed(unsigned long seed);
static int Util_RandInt(int low, int high);


#define MINKEYSEED 0x0000100000
#define MAXKEYSEED 0xFFFFFFFFFF
/* 
The CD Key is in the form:
RSSS-SSSS-SSSR-CCCC
Where 
R = random chars
S = bytes of the seed (two S's per byte)
C = bytes of the check (two C's per byte)

The seed of the keys is a 40 bit number. If you generate 1 million
keys, then the chances of guessing a valid key (even knowing the algorithm)
is about 1 in 1.09 million.
You can easily take this number into the trillions by using the full 64 bits
(of course it makes your CD Key longer, unless you use base 26 numbers)

In addition to the seed, we use a 16 bit check value. The chances of guessing
a mathematically correct key, without knowing the algorithm, is 1 in 65 thousand.
You could make that 1 in 4 billion by using a 32 bit check value. However, even
if a key is mathematically correct, it is not necessarily valid.

It is important that the key generation algorithm actually pick out random
valid keys out of the entire range of keys. If it picks according to a non-random
algorithm (for example, every 1000000'th seed) then you can figure out all
the valid CD Keys from 1 valid one.

The generate keys function is designed to generate all the keys you will
ever need in 1 shot, if you use it twice you may end up with key collisions
(although the probability is VERY low). You can change this by using different
min/max key seeds each time you run it.
*/
void DoGenerateKeys()
{
	char resp[128] = "";
	char key[128];
	char hexstr[20];
	int keyct;
	unsigned int interval;
	__int64 offset; 
	__int64 base = MINKEYSEED;
	__int64 seed;
	short check;
	FILE *f;

	Util_RandSeed(time(NULL) ^ GetTickCount());
	printf("How many keys would you like? ");
	gets(resp);

	keyct = atoi(resp);

	offset = (MAXKEYSEED - MINKEYSEED) / keyct;
	if (offset > 0xFFFFFFFF) //too big for a uint.. too few keys really
		interval = 0xFFFFFFFF;
	else
		interval = (unsigned int)offset;


	f = fopen("keys.txt","w");
	if (!f)
		return;
	while (keyct != 0)
	{
		seed = base + Util_RandInt(0,interval); //pick a number between this base and the next
		base += offset; //move up the base
		//note: a 1 way xform on seed at this point would increase security
		//print a 40 bit hex number to the string
		sprintf(hexstr,"%.10I64x",seed);

		check = ((short)(seed % 65535)) ^ 0x9249; //these are "secret" check calculation values

		sprintf(&key[11],"%.8x",check); //add the check as the last 4 chars (but it prints 8 chars)
		key[0] = '0' + (char)(Util_RandInt(0,9)); //first character is random
		strncpy(&key[1],hexstr,3); //add first 3 chars of seed
		key[4] = '-';
		strncpy(&key[5],&hexstr[3],4); //next 4 chars of seed
		key[9] = '-';
		strncpy(&key[10],&hexstr[7],3); //last 3 chars of seed
		key[13] = '0' + (char)(Util_RandInt(0,9)); //this character is random
		key[14] = '-';

		fprintf(f,"%s\n",key); //print it to the key file
		keyct--;
	}
	fclose(f);
	printf("Keys output to keys.txt\n");
}

int ValidateKey(char *key)
{
	__int64 seed;
	char hexstr[20] = "0x";
	int check;
	short realcheck;

	//extract the seed value
	strncpy(hexstr + 2,key + 1,3);
	strncpy(hexstr + 5, key + 5, 4);
	strncpy(hexstr + 9, key + 10, 3);
	hexstr[12] = 0;
	sscanf(hexstr,"%I64x",&seed);

	//extract the check value
	strncpy(hexstr + 2, key + 15, 4);
	hexstr[6] = 0;
	sscanf(hexstr, "%x",&check);

	//calc the real check value
	realcheck = ((short)(seed % 65535)) ^ 0x9249;
	return ((short)check == realcheck);
}

void DoValidateKeys()
{
	char resp[128] = "";

	do
	{	
		printf("Enter a CD Key to validate, or [ENTER] to quit: ");
		gets(resp);
		if (!resp[0])
			return;
		if (ValidateKey(resp))
			printf("Key Validated\n");
		else
			printf("Invalid Key\n");

	} while (1);


}

int main(int argc, char **argv)
{
	char resp[10];
	//display a menu
	printf("What would you like to do?\n\t1. Generate CD Keys\n\t2. Validate a CD Key\n:");
	gets(resp);
	if (resp[0] == '1')
		DoGenerateKeys();
	else
		DoValidateKeys();
}

/***********
Random Number Generation Code
***********/
#define RANa            16807         /* multiplier */
#define LONGRAND_MAX    2147483647L   /* 2**31 - 1 */

static long randomnum = 1;

/**************
** FUNCTIONS **
**************/
static long nextlongrand(long seed)
{
	unsigned long lo, hi;
	
	lo = RANa * (long)(seed & 0xFFFF);
	hi = RANa * (long)((unsigned long)seed >> 16);
	lo += (hi & 0x7FFF) << 16;
	if (lo > LONGRAND_MAX)
	{
		lo &= LONGRAND_MAX;
		++lo;
	}
	lo += hi >> 15;
	if (lo > LONGRAND_MAX)
	{
		lo &= LONGRAND_MAX;
		++lo;
	}
	return (long)lo;
}

static long longrand(void)                     /* return next random long */
{
	randomnum = nextlongrand(randomnum);
	return randomnum;
}

static void Util_RandSeed(unsigned long seed)      /* to seed it */
{
	randomnum = seed ? (seed & LONGRAND_MAX) : 1;  /* nonzero seed */
}

static int Util_RandInt(int low, int high)
{
	int  range = high-low;
	int  num = longrand() % range;

	return(num + low);
}
