#include <stdlib.h>
#include <string.h>
#include "sb_crypt.h"


static unsigned char keyrand(GOACryptState *state, int limit,
							 unsigned char *user_key,
							 unsigned char keysize,
							 unsigned char *rsum,
							 unsigned *keypos)
{
    unsigned int u,             // Value from 0 to limit to return.
        retry_limiter,      // No infinite loops allowed.
        mask;               // Select just enough bits.
	
    if (!limit) return 0;   // Avoid divide by zero error.
    retry_limiter = 0;
    mask = 1;               // Fill mask with enough bits to cover
    while (mask < (unsigned)limit)    // the desired range.
        mask = (mask << 1) + 1;
    do
	{
        *rsum = (unsigned char)(state->cards[*rsum] + user_key[(*keypos)++]);
        if (*keypos >= keysize)
		{
            *keypos = 0;            // Recycle the user key.
            *rsum = (unsigned char)(*rsum + keysize);   // key "aaaa" != key "aaaaaaaa"
		}
        u = mask & *rsum;
        if (++retry_limiter > 11)
            u %= (unsigned int)limit;     // Prevent very rare long loops.
	}
    while (u > (unsigned)limit);
    return (unsigned char)(u);
}


void GOAHashInit(GOACryptState *state)
{
    // This function is used to initialize non-keyed hash
    // computation.
	
    int i, j;
	
    // Initialize the indices and data dependencies.
	
    state->rotor = 1;
    state->ratchet = 3;
    state->avalanche = 5;
    state->last_plain = 7;
    state->last_cipher = 11;
	
    // Start with state->cards all in inverse order.
	
    for (i=0, j=255;i<256;i++,j--)
        state->cards[i] = (unsigned char) j;
}


void GOACryptInit(GOACryptState *state, unsigned char *key, unsigned char keysize)
{
    // Key size may be up to 256 bytes.
    // Pass phrases may be used directly, with longer length
    // compensating for the low entropy expected in such keys.
    // Alternatively, shorter keys hashed from a pass phrase or
    // generated randomly may be used. For random keys, lengths
    // of from 4 to 16 bytes are recommended, depending on how
    // secure you want this to be.
	
    int i;
    unsigned char toswap, swaptemp, rsum;
    unsigned keypos;
	
    // If we have been given no key, assume the default hash setup.
	
    if (keysize < 1)
	{
        GOAHashInit(state);
        return;
	}
	
    // Start with state->cards all in order, one of each.
	
    for (i=0;i<256;i++)
        state->cards[i] = (unsigned char)(i);
	
    // Swap the card at each position with some other card.
	
    toswap = 0;
    keypos = 0;         // Start with first byte of user key.
    rsum = 0;
    for (i=255;i>=0;i--)
	{
        toswap = keyrand(state, i, key, keysize, &rsum, &keypos);
        swaptemp = state->cards[i];
        state->cards[i] = state->cards[toswap];
        state->cards[toswap] = swaptemp;
	}
	
    // Initialize the indices and data dependencies.
    // Indices are set to different values instead of all 0
    // to reduce what is known about the state of the state->cards
    // when the first byte is emitted.
	
    state->rotor = state->cards[1];
    state->ratchet = state->cards[3];
    state->avalanche = state->cards[5];
    state->last_plain = state->cards[7];
    state->last_cipher = state->cards[rsum];
	
    toswap = swaptemp = rsum = 0;
    keypos = 0;
}


unsigned char GOAEncryptByte(GOACryptState *state, unsigned char b)
{
    // Picture a single enigma state->rotor with 256 positions, rewired
    // on the fly by card-shuffling.
	
    // This cipher is a variant of one invented and written
    // by Michael Paul Johnson in November, 1993.
	
    unsigned char swaptemp;
	
    // Shuffle the deck a little more.
	
    state->ratchet = (unsigned char)(state->ratchet + state->cards[state->rotor++]);
    swaptemp = state->cards[state->last_cipher];
    state->cards[state->last_cipher] = state->cards[state->ratchet];
    state->cards[state->ratchet] = state->cards[state->last_plain];
    state->cards[state->last_plain] = state->cards[state->rotor];
    state->cards[state->rotor] = swaptemp;
    state->avalanche = (unsigned char)(state->avalanche + state->cards[swaptemp]);
	
    // Output one byte from the state in such a way as to make it
    // very hard to figure out which one you are looking at.
	/*
    state->last_cipher = b^state->cards[(state->cards[state->ratchet] + state->cards[state->rotor]) & 0xFF] ^
	state->cards[state->cards[(state->cards[state->last_plain] +
	state->cards[state->last_cipher] +
	state->cards[state->avalanche])&0xFF]];
	*/
    state->last_cipher = (unsigned char)(b^state->cards[(state->cards[state->avalanche] + state->cards[state->rotor]) & 0xFF] ^
		state->cards[state->cards[(state->cards[state->last_plain] +
		state->cards[state->last_cipher] +
		state->cards[state->ratchet])&0xFF]]);
    state->last_plain = b;
    return state->last_cipher;
}


void GOAEncrypt(GOACryptState *state, unsigned char *bp, int len)
{
	int i;
	for (i = 0 ; i < len ; i++)
	{
		bp[i] = GOAEncryptByte(state, bp[i]);
	}
}

unsigned char GOADecryptByte(GOACryptState *state, unsigned char b)
{
    unsigned char swaptemp;
	
    // Shuffle the deck a little more.
	
    state->ratchet = (unsigned char)(state->ratchet + state->cards[state->rotor++]);
    swaptemp = state->cards[state->last_cipher];
    state->cards[state->last_cipher] = state->cards[state->ratchet];
    state->cards[state->ratchet] = state->cards[state->last_plain];
    state->cards[state->last_plain] = state->cards[state->rotor];
    state->cards[state->rotor] = swaptemp;
    state->avalanche = (unsigned char)(state->avalanche + state->cards[swaptemp]);
	
    // Output one byte from the state in such a way as to make it
    // very hard to figure out which one you are looking at.
	/*
    state->last_plain = b^state->cards[(state->cards[state->ratchet] + state->cards[state->rotor]) & 0xFF] ^
	state->cards[state->cards[(state->cards[state->last_plain] +
	state->cards[state->last_cipher] +
	state->cards[state->avalanche])&0xFF]];
	*/
	//crt - change this around
	state->last_plain = (unsigned char)(b^state->cards[(state->cards[state->avalanche] + state->cards[state->rotor]) & 0xFF] ^
		state->cards[state->cards[(state->cards[state->last_plain] +
		state->cards[state->last_cipher] +
		state->cards[state->ratchet])&0xFF]]);
    state->last_cipher = b;
    return state->last_plain;
}

void GOADecrypt(GOACryptState *state, unsigned char *bp, int len)
{
	int i;
	for (i = 0 ; i < len ; i++)
	{
		bp[i] = GOADecryptByte(state, bp[i]);
	}
}

void GOAHashFinal(GOACryptState *state, unsigned char *outputhash,      // Destination
				  unsigned char hashlength) // Size of hash.
{
    int i;
	
    for (i=255;i>=0;i--)
        GOAEncryptByte(state, (unsigned char) i);
    for (i=0;i<hashlength;i++)
        outputhash[i] = GOAEncryptByte(state, 0);
}

