//Base32 encoding / decoding
//based on public domain code by Warren Young

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////
// Globals

// Base-32 character set
char* gpcBase32Set = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";


//// LeftShift /////////////////////////////////////////////////////////
// Shift an array of s characters left n bits.

static void 
LeftShift(unsigned char* pc, int s, int n)
{
	int i;
	for (i = s - 1; i > 0; --i) {
		pc[i] <<= n;
		pc[i] |= (pc[i - 1] >> (8 - n));
	}
	pc[0] <<= n;
}


//// RightShift ////////////////////////////////////////////////////////
// Shift an array of s characters right n bits.
static void
RightShift(unsigned char* pc, int s, int n)
{
	int i;
	for (i = 0; i < s - 1; ++i) {
		pc[i] >>= n;
		pc[i] |= (pc[i + 1] << (8 - n));
	}
	pc[i] >>= n;
}


//// Base32Value ///////////////////////////////////////////////////////
// Given a value from the base 32 character set, return its decimal
// value.  Returns -1 if ch isn't a member of the base 32 set.

static int
Base32Value(unsigned char ch)
{
	const char* pc = strchr(gpcBase32Set, ch);
	if (pc != 0) {
		return (int)(pc - gpcBase32Set);
	}
	else {
		return -1;
	}
}

//// MakeBase32Pretty //////////////////////////////////////////////////
// Take a base 32 data block and add dashes, for readability.  Letters
// are already assumed to be in upper case -- pass the string through
// CleanForBase32() if you're not sure.

void
MakeBase32Pretty(char* pcOut, const char* pcIn)
{
	int n = (int)strlen(pcIn);
	int m;
	while (n > 0) {
		if (n % 4 == 0) {
			m = 4;
		}
		else {
			m = n % 4;
		}

		memcpy(pcOut, pcIn, m);
		pcOut += m;
		pcIn += m;

		n -= m;
		if (n > 0) {
			*pcOut++ = '-';
		}
	}

	*pcOut = '\0';
}


//// CleanForBase32 ////////////////////////////////////////////////////
// Take a string and make it fit for decoding as base 32: force all
// characters to uppercase, and remove any dashes.  (The dashes are
// there to make it easier for humans to read.  The poor weak souls...)

int
CleanForBase32(char* newstr, const char *oldstr, int maxoutput)
{
	int numout = 0;
	for (/* */; *oldstr != 0; ++oldstr) {
		char ch;
		if (*oldstr == '-') {
			continue;
		}
		ch = *oldstr;
		if (numout + 1 == maxoutput) //see if we will overflow
			return 0;
		*newstr++ = islower(ch) ? ch - ('a' - 'A') : ch;
		numout++;
	}

	*newstr = '\0';
	return 1;
} 


//// ConvertFromBase32 /////////////////////////////////////////////////
// Given a string in our base-32 format, convert it back into binary
// data.  Returns the total converted bytes if conversion works, else the -value of the character
// that threw us off.

int
ConvertFromBase32(char* pcOut, const char* pcIn, int nInBytes)
{
	const char* pcCursor = pcIn;
	unsigned char acShift[5], acWorkCopy[8];
	int nValue;
	int i;
	int nTotalOut = 0;
	
	while (nInBytes > 0) {
		int nCopyableBytes = (nInBytes > 8 ? 8 : nInBytes);
		int nOutBytes = ((nCopyableBytes * 5)/* + 7*/) / 8;

		memset(acShift, 0, sizeof(acShift));

		for (i = 0; i < nCopyableBytes; ++i) {
			acWorkCopy[i] = *(pcCursor + nCopyableBytes - i - 1);
		}
		pcCursor += nCopyableBytes;

		for (i = 0; i < nCopyableBytes; ++i) {
			// Make room for new bits
			LeftShift(acShift, sizeof(acShift), 5);

			// Put the value onto the end of the register
			nValue = Base32Value(acWorkCopy[i]);
			if (nValue < 0) {
				return -acWorkCopy[i];
			}
			acShift[0] |= nValue;
		}

		nInBytes -= nCopyableBytes;
		
		memcpy(pcOut, acShift, nOutBytes);
		
		pcOut += nOutBytes;
		nTotalOut += nOutBytes;
	}

	return nTotalOut;
}


//// ConvertToBase32 ///////////////////////////////////////////////////
// Given a buffer, convert it to a base-32 string.
//
// Returns the number of bytes it wrote out.
// 
// This is the platform-neutral version, which uses arrays of characters
// to simulate a little-endian environment's integers.  Thus, ac[0] is
// the least significant byte in our 5-byte integer, and ac[4] is the
// most significant byte.


int
ConvertToBase32(char* pcOut, const char* pcIn, int nInBytes)
{
	int nTotalOut = 0;
	unsigned char acShift[5];
	memset(acShift, 0, sizeof(acShift));

	while (nInBytes > 0) {
		int i;
		int nCopyableBytes = (nInBytes > 5 ? 5 : nInBytes);
		int nOutBytes = ((nCopyableBytes * 8) + 4) / 5;

		memcpy(acShift, pcIn, nCopyableBytes);
		pcIn += nCopyableBytes;
		nInBytes -= nCopyableBytes;

		for (i = 0; i < nOutBytes; ++i) {
			pcOut[nTotalOut++] = gpcBase32Set[acShift[0] & 0x1F];
			RightShift(acShift, sizeof(acShift), 5);
		}
	}

	return nTotalOut;
}
