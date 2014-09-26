#include "chatCrypt.h"

#define swap_byte(x,y) t = *(x); *(x) = *(y); *(y) = t

void gs_prepare_key(const unsigned char *key_data_ptr, int key_data_len, gs_crypt_key *key)
{
	unsigned char t;
	unsigned char index1;
	unsigned char index2;
	unsigned char* state;
	int counter;
	
	state = &key->state[0];
	for(counter = 0; counter < 256; counter++)
		state[255 - counter] = (unsigned char)counter; //crt - we fill reverse of normal
	key->x = 0;
	key->y = 0;
	index1 = 0;
	index2 = 0;
	for(counter = 0; counter < 256; counter++)
	{
		index2 = (unsigned char)((key_data_ptr[index1] + state[counter] + index2) % 256);
		swap_byte(&state[counter], &state[index2]);
		index1 = (unsigned char)((index1 + 1) % key_data_len);
	}
}

void gs_crypt(unsigned char *buffer_ptr, int buffer_len, gs_crypt_key *key)
{
	unsigned char t;
	unsigned char x;
	unsigned char y;
	unsigned char* state;
	unsigned char xorIndex;
	int counter;
	
	x = key->x;
	y = key->y;
	state = &key->state[0];
	for(counter = 0; counter < buffer_len; counter++)
	{
		x = (unsigned char)((x + 1) % 256);
		y = (unsigned char)((state[x] + y) % 256);
		swap_byte(&state[x], &state[y]);
		xorIndex = (unsigned char)((state[x] + state[y]) % 256);
		buffer_ptr[counter] ^= state[xorIndex];
	}
	key->x = x;
	key->y = y;
}


void gs_xcode_buf(char *buf, int len, char *enckey)
{
	int i;
	char *pos = enckey;

	for (i = 0 ; i < len ; i++)
	{
		buf[i] ^= *pos++;
		if (*pos == 0)
			pos = enckey;
	}

}
