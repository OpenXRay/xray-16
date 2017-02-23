///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsLargeInt.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Many parameters are gsi_u32* instead of gsLargeInt_t*.  
//    This was done to allow easy conversion of databuffer to gsLargeInt_t
//    Raw buffer destinations must have enough space to store the result
static gsi_bool gsiLargeIntPrint(FILE* logFile, const l_word *data, l_word length);
static gsi_bool gsiLargeIntResize(gsLargeInt_t *lint, l_word length);
static gsi_bool gsiLargeIntStripLeadingZeroes(gsLargeInt_t* lint);
static gsi_bool gsiLargeIntSizePower2(const gsLargeInt_t *src1, const gsLargeInt_t *src2, l_word *lenout);
static gsi_i32  gsiLargeIntCompare(const l_word *data1, l_word len1, const l_word *data2, l_word len2);

static gsi_bool gsiLargeIntKMult(const l_word *data1, const l_word *data2, l_word length, l_word *dest, l_word *lenout, l_word maxlen);
static gsi_bool gsiLargeIntMult (const l_word *data1, l_word length1, const l_word *data2, l_word length2, l_word *dest, l_word *lenout, l_word maxlen);
static gsi_bool gsiLargeIntDiv  (const l_word *src1, l_word length1, const gsLargeInt_t *divisor, gsLargeInt_t *dest, gsLargeInt_t *remainder);

// Dest may be data1 or data2 to support in-place arithmetic
static gsi_bool gsiLargeIntAdd  (const l_word *data1, l_word length1, const l_word *data2, l_word length2, l_word *dest, l_word *lenout, l_word maxlen);
static gsi_bool gsiLargeIntSub  (const l_word *amount, l_word length1, const l_word *from, l_word length2, l_word *dest, l_word *lenout);

// Special division, removes divisor directly from src1, leaving remainder
static gsi_bool gsiLargeIntSubDivide(l_word *src1, l_word length, const l_word *divisor, l_word dlen, gsi_u32 highbit, l_word *quotient);

// Montgomery utilities
//gsi_bool gsiLargeIntSquareM(const gsLargeInt_t *src, const gsLargeInt_t *mod, gsi_u32 modPrime, gsi_u32 R, gsLargeInt_t *dest);
//gsi_bool gsiLargeIntMultM(gsLargeInt_t *src1, gsLargeInt_t *src2, const gsLargeInt_t *mod, gsi_u32 modPrime, gsLargeInt_t *dest);
gsi_bool gsiLargeIntMultM(gsLargeInt_t *src1, gsLargeInt_t *src2, const gsLargeInt_t *mod, gsi_u32 modPrime, gsLargeInt_t *dest);
gsi_bool gsiLargeIntInverseMod(const gsLargeInt_t *mod, l_word *modPrimeOut);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
 // execution timing/profiling
#define GS_LINT_TIMING
#ifdef GS_LINT_TIMING

typedef enum 
{
	GSLintTimerMult,  // "regular" multiplication
	GSLintTimerMultM, // montgomery
	GSLintTimerKMult, // karatsuba
	GSLintTimerAdd,
	GSLintTimerSub,   // subtract
	GSLintTimerDiv,
	GSLintTimerSubDivide, // atomic divide
	GSLintTimerSquareMod,
	GSLintTimerPowerMod, // modular exponentiation

	GSLintTimerCount
} GSLintTimerID;

typedef struct GSLintTimer
{
	gsi_time started;
	gsi_time total;
	gsi_u32  entries;
	gsi_u32  running; // already entered?
} GSLintTimer;
static struct GSLintTimer gTimers[GSLintTimerCount];

static void gsiLargeIntTimerEnter(GSLintTimerID id)
{
	if (gTimers[id].running==0)
	{
		gTimers[id].entries++;
		gTimers[id].started = current_time_hires();
		gTimers[id].running = 1;
	}
}
static void gsiLargeIntTimerExit(GSLintTimerID id)
{
	if (gTimers[id].running==1)
	{
		gTimers[id].total += current_time_hires()-gTimers[id].started;
		gTimers[id].running = 0;
	}
}

#define GSLINT_ENTERTIMER(id) gsiLargeIntTimerEnter(id)
#define GSLINT_EXITTIMER(id) gsiLargeIntTimerExit(id)

#else
#define GSLINT_ENTERTIMER(id)
#define GSLINT_EXITTIMER(id)
#endif
 



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsLargeIntSetValue(gsLargeInt_t *lint, l_word value)
{
	lint->mLength = 1;
	lint->mData[0] = value;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Resize by:
//    Padding a GSLINT with leading zeroes.
//    or stripping lead zeroes.
// This function will not strip digits other than zero.
gsi_bool gsiLargeIntResize(gsLargeInt_t *lint, l_word length)
{
	if (length > GS_LARGEINT_MAX_DIGITS)
		return gsi_false;

	// strip leading zeroes until length is reached
	if (lint->mLength >= length)
	{
		while(lint->mLength > length && lint->mData[lint->mLength-1]==0)
			lint->mLength--; // check each digit to make sure it's zero
		if (lint->mLength == length)
			return gsi_true;
		else
			return gsi_false;
	}

	// otherwise, add zeroes until length is reached
	else
	{
		memset(&lint->mData[lint->mLength], 0, (length-lint->mLength)*sizeof(l_word));
		lint->mLength = length;
		return gsi_true;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Makes two GSLINT the same size, the size being a power of 2
//  NOTE: Testing next multiple of two, not power of 2
gsi_bool gsiLargeIntSizePower2(const gsLargeInt_t *src1, const gsLargeInt_t *src2, l_word *lenout)
{
	unsigned int i = 0;

	int len1 = (int)src1->mLength;
	int len2 = (int)src2->mLength;

	// strip leading zeroes
	while(len1>0 && src1->mData[len1-1] == 0)
		len1--;
	while(len2>0 && src2->mData[len2-1] == 0)
		len2--;

	// set to longer length
	*lenout = (l_word)max(len1, len2);
	
	// search for power of two >= length
	//   (this length is in digits, not bits)
	i=1;
	while(i < *lenout)
		i = i<<1;
	*lenout = (l_word)i;

	if (*lenout > GS_LARGEINT_MAX_DIGITS)
		return gsi_false;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Compare two integers
//     -1 = data1 < data2
//      0 = data1 = data2
//      1 = data1 > data2
static gsi_i32 gsiLargeIntCompare(const l_word *data1, l_word len1, const l_word *data2, l_word len2)
{
	// skip leading whitespace, if any
	while(data1[len1-1] == 0 && len1>0)
		len1--;
	while(data2[len2-1] == 0 && len2>0)
		len2--;
	if (len1<len2)
		return -1;
	else if (len1>len2)
		return 1;
	else 
	{
		// same size, compare digits
		while(len1 > 0)
		{
			if (data1[len1-1] < data2[len1-1])
				return -1;
			else if (data1[len1-1] > data2[len1-1])
				return 1;
			len1--;
		}
	}
	return 0; // equal!
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool gsiLargeIntStripLeadingZeroes(gsLargeInt_t* lint)
{
	while(lint->mLength >0 && lint->mData[lint->mLength-1]==0)
		lint->mLength--;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Addition may cause overflow
gsi_bool gsLargeIntAdd(const gsLargeInt_t *src1, const gsLargeInt_t *src2, gsLargeInt_t *dest)
{
	gsi_bool result = gsiLargeIntAdd(src1->mData, src1->mLength, src2->mData, src2->mLength, dest->mData, &dest->mLength, GS_LARGEINT_MAX_DIGITS);
	if (gsi_is_false(result))
		memset(dest, 0, sizeof(gsLargeInt_t)); // overflow
	return result;
}

// len: In value = maxsize
//      Out value = actual size
static gsi_bool gsiLargeIntAdd(const l_word *data1, l_word length1, const l_word *data2, l_word length2, l_word *dest, l_word *lenout, l_word maxlen)
{
	gsi_u32 i=0;
	l_dword carry = 0; // to hold overflow

	gsi_u32 shorterLen = 0;
	gsi_u32 longerLen = 0;
	//const gsi_u32 *shorterSrc = NULL;
	const l_word *longerSrc = NULL;

	GSLINT_ENTERTIMER(GSLintTimerAdd);

	if (maxlen < length1 || maxlen < length2)
		return gsi_false; // dest not large enough, OVERFLOW

	if (length1 < length2)
	{
		shorterLen = length1;
		//shorterSrc = data1;
		longerLen = length2;
		longerSrc = data2;
	}
	else
	{
		shorterLen = length2;
		//shorterSrc = data2;
		longerLen = length1;
		longerSrc = data1;
	}

	// Add digits until the shorterSrc's length is reached
	while(i < shorterLen)
	{
		carry += (l_dword)data1[i] + data2[i];
		dest[i] = (l_word)carry;
		carry = carry >> GS_LARGEINT_DIGIT_SIZE_BITS; //32;
		i++;
	}

	// Continue adding until carry is zero
	while((carry > 0) && (i < longerLen))
	{
		carry += (l_dword)longerSrc[i];
		dest[i] = (l_word)carry;
		carry = carry >> GS_LARGEINT_DIGIT_SIZE_BITS; //32;
		i++;
	}

	// Is there still a carry?
	//    do not perform length check here, so that we can support oversized buffers
	if (carry > 0) // && i < GS_LARGEINT_INT_SIZE)
	{
		if (maxlen <= i)
			return gsi_false; // OVERFLOW, no room for extra digit
		dest[i++] = (l_word)carry;
		carry = 0;
	}

	// Copy the rest of the bytes straight over (careful of memory overlap)
	//    this can't happen if there was a carry (see above carry>0 check)
	if (i < longerLen)
	{
		// check overlap
		if (&dest[i] != &longerSrc[i])
			memcpy(&dest[i], &longerSrc[i], (longerLen-i)*sizeof(l_word));
		i = longerLen;
	}
	*lenout = (l_word)i;

	GSLINT_EXITTIMER(GSLintTimerAdd);

	if (carry)
		return gsi_false; // overflow
	else
		return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Subtraction may cause underflow
//   subtracts src1 FROM src2
//   strips leading zeroes (gsiLargeIntSub doesn't strip for compatability with karatsuba fixed size numbers)
gsi_bool gsLargeIntSub(const gsLargeInt_t *src1, const gsLargeInt_t *src2, gsLargeInt_t *dest)
{
	gsi_bool result = gsiLargeIntSub(src1->mData, src1->mLength, src2->mData, src2->mLength, dest->mData, &dest->mLength);
	if (gsi_is_true(result))
		gsiLargeIntStripLeadingZeroes(dest);
	return result;
}

gsi_bool gsiLargeIntSub(const l_word *src1, l_word length1, const l_word *src2, l_word length2, l_word *dest, l_word *lenout)
{
	l_dword borrow = 0; // to hold overflow
	gsi_u32 shorterLen = min(length1, length2);
	gsi_u32 i=0;

	GSLINT_ENTERTIMER(GSLintTimerSub);

	//printf("--From: ");
	//gsiLargeIntPrint(src2, length2);
	//printf("--Subtracting: ");
	//gsiLargeIntPrint(src1, length1);

	// Subtract digits
	while(i < shorterLen)
	{
		borrow = (l_dword)src2[i] - src1[i] - borrow;
		dest[i] = (l_word)borrow;
		borrow = borrow>>63; // shift to last bit.  This will be 1 if negative, 0 if positive
		i++;
	}
	while(i < length2)
	{
		borrow = (l_dword)src2[i]-borrow;
		dest[i] = (l_word)borrow;
		borrow = borrow>>63;
		i++;
	}

	// check for underflow
	if (borrow != 0)
	{
		GSLINT_EXITTIMER(GSLintTimerSub);
		return gsi_false;
	}
	while(length1 > i) // make sure remaining digits are only leading zeroes
	{
		if (src1[i] != 0)
		{
			GSLINT_EXITTIMER(GSLintTimerSub);
			return gsi_false;
		}
		i++;
	}

	// Don't reduce length from subtraction, instead keep leading zeroes
	// (do this for ease of use with Karatsuba which requires Power2 length)
	*lenout = length2;

	GSLINT_EXITTIMER(GSLintTimerSub);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Multiply using normal method (use KMult when working with LargeInt*LargeInt)
gsi_bool gsLargeIntMult(const gsLargeInt_t *src1, const gsLargeInt_t *src2, gsLargeInt_t *dest)
{
	gsi_bool result = gsiLargeIntMult(src1->mData, src1->mLength, src2->mData, src2->mLength, dest->mData, &dest->mLength, GS_LARGEINT_MAX_DIGITS);
	if (gsi_is_false(result))
		memset(dest, 0, sizeof(gsLargeInt_t)); // overflow
	return result;
}

static gsi_bool gsiLargeIntMult(const l_word *data1, l_word length1, const l_word *data2, l_word length2, l_word *dest, l_word *lenout, l_word maxlen)
{
	unsigned int i=0;
	unsigned int k=0;

	gsLargeInt_t temp;
	memset(&temp, 0, sizeof(temp));
	*lenout = 0;

	GSLINT_ENTERTIMER(GSLintTimerMult);

	for(i=0; i<length2; i++)
	{
		// don't have to multiply by 0
		if(data2[i] != 0)
		{
			// multiply data1 by data2[i]
			for (k=0; k<length1; k++)
			{
				// carry starts out as product
				//   (it is mathematically impossible for carry to overflow
				//    at the first addition [see below])
				l_dword carry = (l_dword)data1[k] * data2[i];
				unsigned int digit = (unsigned int)(i+k);
				if (digit >= maxlen)
				{
					GSLINT_EXITTIMER(GSLintTimerMult);
					return gsi_false; // overflow
				}
				while(carry)
				{
					carry += temp.mData[digit];
					temp.mData[digit] = (l_word)carry;
					carry = carry >> GS_LARGEINT_DIGIT_SIZE_BITS;
					digit++;
					if ((digit > maxlen) ||
						(digit == maxlen && carry>0))
					{
						GSLINT_EXITTIMER(GSLintTimerMult);
						return gsi_false; // overflow
					}
				}
				if (digit > (gsi_i32)temp.mLength)
					temp.mLength = (l_word)digit;
			}
		}
	}
	// copy into destination (calculate length at this time)
	while(temp.mLength>0 && temp.mData[temp.mLength-1] == 0)
		temp.mLength--; // strip leading zeroes
	*lenout = temp.mLength;
	memcpy(dest, temp.mData, (*lenout)*sizeof(l_word));

	GSLINT_EXITTIMER(GSLintTimerMult);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// divide src1 by divisor
gsi_bool gsLargeIntDiv(const gsLargeInt_t *src1, const gsLargeInt_t *divisor, gsLargeInt_t *dest, gsLargeInt_t *remainder)
{
	// call the free-buffer version
	return gsiLargeIntDiv(src1->mData, src1->mLength, divisor, dest, remainder);
}

// length1 can be, at most, 2*GS_LARGEINT_INT_SIZE
static gsi_bool gsiLargeIntDiv(const l_word *src, l_word len, const gsLargeInt_t *div, gsLargeInt_t *dest, gsLargeInt_t *remainder)
{
	gsi_i32 result = 0; // temp, to store compare result
	gsi_i32 divisorHighBit = GS_LARGEINT_DIGIT_SIZE_BITS-1; // pre-calculate this

	// Bytes used from src1
	int readIndex = 0;
	int readLength = 0;

	// setup scratch copies 
	gsLargeInt_t quotient;

	l_word  scopy[GS_LARGEINT_MAX_DIGITS*2];  // we support double length source for division, when dest is null
	l_word  scopyLen = len;

	const l_word* divisorData = div->mData;
	l_word  divisorLen = div->mLength;

	gsi_bool endLoop = gsi_false;
	
	GSLINT_ENTERTIMER(GSLintTimerDiv);

	memset(scopy, 0, sizeof(scopy));

	// we only support oversized sources for calculating a remainder
	//    e.g. dest must be null
	if (scopyLen > GS_LARGEINT_MAX_DIGITS && dest != NULL)
		return gsi_false;

	// strip leading zeroes (from our scratch copies)
	while(scopyLen>0 && src[scopyLen-1]==0)
		scopyLen--;
	while(divisorLen>0 && divisorData[divisorLen-1]==0)
		divisorLen--;

	memcpy(scopy, src, scopyLen*sizeof(l_word));
	memset(&quotient, 0, sizeof(quotient)); 

	// check the unusual cases
	if (scopyLen==0 || divisorLen==0)
	{
		if (dest)
		{
			dest->mData[0] = 0;
			dest->mLength = 0;
		}
		if (remainder)
		{
			remainder->mData[0] = 0;
			remainder->mLength = 0;
		}

		GSLINT_EXITTIMER(GSLintTimerDiv);

		if (divisorLen == 0)
			return gsi_false; // division by zero
		else
			return gsi_true; // zero divided, this is legal
	}
	if (gsiLargeIntCompare(scopy, scopyLen, divisorData, divisorLen)==-1)
	{
		// divisor is larger than source
		if (dest)
		{
			dest->mLength = 0;
			dest->mData[0] = 0;
		}
		remainder->mLength = scopyLen;
		memcpy(remainder->mData, scopy, scopyLen*sizeof(l_word));
		GSLINT_EXITTIMER(GSLintTimerDiv);
		return gsi_true;
	}
	
	// calculate the divisor high bit
	while((divisorData[divisorLen-1]&(1<<(gsi_u32)divisorHighBit))==0 && divisorHighBit>=0)
		divisorHighBit--;
	if (divisorHighBit == -1)
	{
		GSLINT_EXITTIMER(GSLintTimerDiv);
		return gsi_false; // divide by zero
	}
	divisorHighBit += (divisorLen-1)*GS_LARGEINT_DIGIT_SIZE_BITS;
	
	// position "sliding" window for first interation
	// 41529 / [71389]2564
	// WARNING: digits are indexed [2][1][0], first byte to read is index[2]
	readIndex = (int)(scopyLen - divisorLen);
	readLength = (int)divisorLen;

	//if (readIndex < 0)
	//	_asm {int 3}; // overflow readIndex
	
	do
	{
		result = gsiLargeIntCompare(&scopy[readIndex], (l_word)readLength, divisorData, divisorLen);
		if (result == -1)
		{
			// scopy window is smaller, we'll need an extra digit
			if (readIndex > 0)
			{
				readIndex--; 
				readLength++;
			}
			else
			{
				// no more digits!
				endLoop = gsi_true;
			}
		}
		else if (result == 0)
		{
			// not likely! set digits to zero and slide window
			memset(&scopy[readIndex], 0, readLength*sizeof(l_word));
			quotient.mData[readIndex] += 1;
			if (quotient.mLength < (l_word)(readIndex+readLength))
				quotient.mLength = (l_word)(readIndex+readLength);
			readIndex -= readLength;
			readLength = 1;

			if (readIndex < 0)
				endLoop = gsi_true;; // no more digits
		}
		else
		{
			// subtract directly onto our temp copy, so we don't have to worry about carry values
			l_word quotientTemp = 0;
			//if (readLength > 0xffff)
			//	_asm {int 3}
			if (gsi_is_false(gsiLargeIntSubDivide(&scopy[readIndex], (l_word)readLength, divisorData, divisorLen, (gsi_u32)divisorHighBit, &quotientTemp)))
			{
				// overflow
				GSLINT_EXITTIMER(GSLintTimerDiv);
				return gsi_false;
			}
			quotient.mData[readIndex] = (l_word)(quotient.mData[readIndex] + quotientTemp);
			if (quotient.mLength < (l_word)(readIndex+readLength))
				quotient.mLength = (l_word)(readIndex+readLength);
			// remove new leading zeroes
			while(scopy[readIndex+readLength-1] == 0 && readLength>1)
				readLength--;
			while(scopy[readIndex+readLength-1] == 0 && readIndex>1)
				readIndex--;
		}
	}
	while(gsi_is_false(endLoop));

	// no more digits, leftover is remainder
	if (readIndex >= 0)
	{
		memcpy(remainder->mData, &scopy[readIndex], readLength*sizeof(l_word));
		remainder->mLength = (l_word)readLength;
	}
	else
	{
		remainder->mData[0] = 0;
		remainder->mLength = 0;
	}

	// save off quotient, if desired
	if (dest)
	{
		memcpy(dest->mData, quotient.mData, quotient.mLength*sizeof(l_word));
		dest->mLength = quotient.mLength;
	}
	GSLINT_EXITTIMER(GSLintTimerDiv);
	return gsi_true;
}


// atomic divide.  
//    Subtract divisor directly from src.
//    Leave remainder in src.
static gsi_bool gsiLargeIntSubDivide(l_word *src, l_word length, const l_word *divisor, l_word dlen, 
									 gsi_u32 highbit, l_word *quotient)
{
	l_dword aboveBits = 0;
	gsLargeInt_t temp; // stores temporary product before subtraction
	gsLargeInt_t quotientCopy; // copy of quotient, length padded for multiplication

	GSLINT_ENTERTIMER(GSLintTimerSubDivide);
	// assert(src > divisor)
	// assert(src < (MAX_DIGIT_VALUE * divisor))
	//if(dlen==1 && *divisor==0)
	//	_asm {int 3} // division by zero

	// Q: how many times to subtract?
	// A: we estimate by taking the bits in src above the highest bit in divisor
	if (length > dlen)
		aboveBits = (src[length-2]&divisor[dlen-1]) | ((l_dword)src[length-1]<<GS_LARGEINT_DIGIT_SIZE_BITS);
	else
		aboveBits = src[length-1];
	aboveBits /= divisor[dlen-1];

	memset(&quotientCopy, 0, sizeof(quotientCopy));
	quotientCopy.mData[0] = (l_word)(aboveBits);
	quotientCopy.mData[1] = (l_word)(aboveBits>>GS_LARGEINT_DIGIT_SIZE_BITS);

	// We only support quotients up to MAX_INT
	if (quotientCopy.mData[1] != 0)
	{
		quotientCopy.mData[0] = (l_word)(-1);
		quotientCopy.mData[1] = 0;
	}
	quotientCopy.mLength = 1;
		
	// multiply this value by divisor, and that's how much to subtract
	if (gsi_is_false(gsiLargeIntMult(divisor, dlen, quotientCopy.mData, quotientCopy.mLength, temp.mData, &temp.mLength, GS_LARGEINT_MAX_DIGITS)))
	{
		GSLINT_EXITTIMER(GSLintTimerSubDivide);
		return gsi_false; // overflow
	}

	// while subtraction amount is larger than src, reduce it
	while(gsiLargeIntCompare(temp.mData, temp.mLength, src, length)==1)
	{
		// divide by two
		quotientCopy.mData[0] = (l_word)(quotientCopy.mData[0]>>1);
		//if (quotientCopy.mData[0] == 0)
		//	_asm {int 3}
		if (gsi_is_false(gsiLargeIntMult(divisor, dlen, quotientCopy.mData, quotientCopy.mLength, temp.mData, &temp.mLength, GS_LARGEINT_MAX_DIGITS)))
		{
			GSLINT_EXITTIMER(GSLintTimerSubDivide);
			return gsi_false; // overflow
		}
	}
	//if (gsiLargeIntCompare(temp.mData, temp.mLength, src, length)==1)
	//	_asm {int 3} // temp > src, subtraction will cause underflow!
			
	// subtract it
	gsiLargeIntSub(temp.mData, temp.mLength, src, length, src, &length);

	*quotient = quotientCopy.mData[0];
	//if (quotientCopy.mData[1] != 0)
	//	_asm {int 3}
	GSLINT_EXITTIMER(GSLintTimerSubDivide);

	GSI_UNUSED(highbit);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Multiply using Karatsuba
//   Karatsuba requires that the sizes be equal and a power of two
gsi_bool gsLargeIntKMult(const gsLargeInt_t *src1, const gsLargeInt_t *src2, gsLargeInt_t *dest)
{
	l_word len = 0;
	gsi_bool result = gsi_false;

	gsLargeInt_t temp; // to prevent issues if (src1 == src2 == dest)

	// quick check for multiplication by 0
	if (src1->mLength == 0 || src2->mLength == 0)
	{
		dest->mLength = 0;
		return gsi_true;
	}

	// when length is small it's faster to use "normal" multiplication
	if (max(src1->mLength,src2->mLength) < GS_LARGEINT_KARATSUBA_CUTOFF)
		return gsLargeIntMult(src1, src2, dest);

	// Check for size/length restrictions
	result = gsiLargeIntSizePower2(src1, src2, &len);
	if (gsi_is_false(result) || len>(GS_LARGEINT_MAX_DIGITS/2))
	{
		// try regular multiplication
		return gsLargeIntMult(src1, src2, dest); 
	}

	// (don't time above section since it defers to Mult)
	GSLINT_ENTERTIMER(GSLintTimerKMult);

	// clear the temporary dest
	memset(&temp, 0, sizeof(gsLargeInt_t));
	temp.mLength = 0;

	// resize if necessary
	if (src1->mLength != len || src2->mLength != len)
	{
		// size is not correct, make a copy then multiply
		gsLargeInt_t src1Copy;
		gsLargeInt_t src2Copy;
		memcpy(&src1Copy, src1, sizeof(gsLargeInt_t));
		memcpy(&src2Copy, src2, sizeof(gsLargeInt_t));
		gsiLargeIntResize(&src1Copy, len);
		gsiLargeIntResize(&src2Copy, len);

		result = gsiLargeIntKMult(src1Copy.mData, src2Copy.mData, len, temp.mData, &temp.mLength, GS_LARGEINT_MAX_DIGITS);
	}
	else
	{
		// size is correct, perform multiplication
		result = gsiLargeIntKMult(src1->mData, src2->mData, len, temp.mData, &temp.mLength, GS_LARGEINT_MAX_DIGITS);
	}
	if (gsi_is_true(result))
	{
		// strip leading zeroes and copy into dest
		gsiLargeIntStripLeadingZeroes(&temp);
		memcpy(dest, &temp, sizeof(gsLargeInt_t));
	}
	GSLINT_EXITTIMER(GSLintTimerKMult);
	return result;
}


// Utility for Karasuba
static gsi_bool gsiLargeIntKMult(const l_word *data1, const l_word *data2, l_word length,
								 l_word *dest, l_word *lenout, l_word maxlen)
{
	// No timer here, this function is only called from GSLINTKMult
	//GSLINT_ENTERTIMER(GSLintTimerKMult);

	// "normal" multiplication is faster when length is small
	if (length <= GS_LARGEINT_KARATSUBA_CUTOFF)
		return gsiLargeIntMult(data1, length, data2, length, dest, lenout, maxlen);
	else
	{
		gsLargeInt_t temp1, temp2, temp3;
		l_word halfLen = (l_word)(length>>1);

		temp1.mLength = 0;
		temp2.mLength = 0;
		temp3.mLength = 0;

		//printf("Karasuba splitting at %d (1/2 = %d)\r\n", length, halfLen);

		// Karatsuba:  k = 12*34
		//  a = (1*3)
		//  b = (1+2)*(3+4)-a-c
		//  c = (2*4)
		//  k = a*B^N+b*B^(N/2)+c = a*100+b*10+c

		// Enter the recursive portion
		//   TH = top half
		//   BH = bottom half

		// Note that since (a*B^N + c) cannot overlap, we can immediately store both in dest

		// Compute a. (TH of data1 * TH of data2)
		//      Stores in TH of dest, so later *B^N isn't necessary
		//      For the example, this puts 1*3 into the high half 03xx
		gsiLargeIntKMult(&data1[halfLen], &data2[halfLen], halfLen, &dest[length], lenout, (l_word)(maxlen-length));
		//printf("Calculated A (%d) = ", *lenout);
		//gsiLargeIntPrint(&dest[length], *lenout);

		// Compute c. (BH of data1 * BH of data2)
		//      For the example, this puts 2*4 into the low half xx08
		gsiLargeIntKMult(data1, data2, halfLen, dest, lenout, maxlen);
		//printf("Calculated C (%d) = ", *lenout);
		//gsiLargeIntPrint(dest, *lenout);

		// Compute b1. (TH of data1 + BH of data1) 
		gsiLargeIntAdd(&data1[halfLen], halfLen, data1, halfLen, temp1.mData, &temp1.mLength, GS_LARGEINT_MAX_DIGITS);
		//printf("Calculated B1 (%d) = ", temp1.mLength);
		//gsiLargeIntPrint(temp1.mData, temp1.mLength);

		// Compute b2. (TH of data2 + BH of data2)
		gsiLargeIntAdd(&data2[halfLen], halfLen, data2, halfLen, temp2.mData, &temp2.mLength, GS_LARGEINT_MAX_DIGITS);
		//printf("Calculated B2 (%d) = ", temp2.mLength);
		//gsiLargeIntPrint(temp2.mData, temp2.mLength);

		// Compute b3. (b1*b2) (*B^N)
		//      For the example, (1+2)(3+4)*B^N = 21*B^N = 0210
		memset(&temp3, 0, sizeof(gsLargeInt_t));
		
		// May require resizing, but don't go above halfLen
		if (temp1.mLength > halfLen || temp2.mLength > halfLen)
			gsiLargeIntMult(temp1.mData, temp1.mLength, temp2.mData, temp2.mLength, &temp3.mData[halfLen], &temp3.mLength, (l_word)(GS_LARGEINT_MAX_DIGITS-halfLen));
		else
		{
			gsi_bool result = gsiLargeIntSizePower2(&temp1, &temp2, lenout);
			if (gsi_is_false(result))
				return gsi_false; // could not resize
			gsiLargeIntResize(&temp1, *lenout); // pad to new size
			gsiLargeIntResize(&temp2, *lenout); // pad to new size
			gsiLargeIntKMult(temp1.mData, temp2.mData, *lenout, &temp3.mData[halfLen], &temp3.mLength, (l_word)(GS_LARGEINT_MAX_DIGITS-halfLen));
		}
		temp3.mLength = (l_word)(temp3.mLength + halfLen); // fix length for temp3
		//if (temp3.mLength > GS_LARGEINT_INT_SIZE)
		//	_asm {int 3} // this should be at most temp1.mLength+temp2.mLength
		memset(temp3.mData, 0, halfLen*sizeof(l_word));
		//printf("Calculated B3 (%d) = ", temp3.mLength);
		//gsiLargeIntPrint(&temp3.mData[halfLen], temp3.mLength-halfLen);

		// Compute final b. (b3-a-c) (*B^N)
		//      Note: The subtraction is in terms of (*B^N)
		//      For the example, 021x - 03x - 08x = 0100
		gsiLargeIntSub(&dest[length], length, &temp3.mData[halfLen], (l_word)(temp3.mLength-halfLen), &temp3.mData[halfLen], &temp3.mLength);
		temp3.mLength = (l_word)(temp3.mLength + halfLen);
		gsiLargeIntSub( dest        , length, &temp3.mData[halfLen], (l_word)(temp3.mLength-halfLen), &temp3.mData[halfLen], &temp3.mLength);
		temp3.mLength = (l_word)(temp3.mLength + halfLen);
		//printf("Calculated B (%d) = ", temp3.mLength);
		//gsiLargeIntPrint(temp3.mData, temp3.mLength);

		// Add em up
		//      Dest already contains A+C, so Add B
		//      For the example, 0308 + 0100 = 0408 (the correct answer)
		gsiLargeIntAdd(dest, (l_word)(length*2), temp3.mData, temp3.mLength, dest, lenout, maxlen);
	}
	// strip leading zeroes from dest
	while(*lenout > 0 && dest[*lenout-1] == 0)
		*lenout = (l_word)(*lenout-1);

	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsLargeIntSquareMod(const gsLargeInt_t *lint, const gsLargeInt_t *mod, gsLargeInt_t *dest)
{
	int i = 0;
	int k = 0;
	int len = (int)lint->mLength; // signed version
	l_dword carry = 0;
	int oldShiftBit = 0;
	int newShiftBit = 0;
	gsi_bool result = gsi_false;
	unsigned int mask = (unsigned int)1<<(GS_LARGEINT_DIGIT_SIZE_BITS-1);

	l_word squareSums[GS_LARGEINT_MAX_DIGITS*2];   // temp dest for square sums
	l_word otherSums[GS_LARGEINT_MAX_DIGITS*2];    // temp dest for other sums
	l_word squareLen = 0;
	l_word otherLen = 0;

	GSLINT_ENTERTIMER(GSLintTimerSquareMod);

	memset(&squareSums, 0, sizeof(squareSums));
	memset(&otherSums, 0, sizeof(otherSums));

	// Go through each digit, multiplying with each other digit
	// (only do this once per pair, since AB == BA)
	// Ex: ABC * ABC, we want AB,AC,BC only
	for (i=1; i < len; i++)
	{
		for(k=0; k < i; k++)
		{
			carry += (l_dword)lint->mData[i]*lint->mData[k] + otherSums[i+k];
			otherSums[i+k] = (l_word)carry;
			carry  = carry >> GS_LARGEINT_DIGIT_SIZE_BITS;
		}
		if(carry)
		{
			otherSums[i+k] = (l_word)carry;
			carry = carry >> GS_LARGEINT_DIGIT_SIZE_BITS;
		}
	}

	// Multiply by 2 (because each internal pair appears twice)
	for (i=0; i < (2*len); i++)
	{
		newShiftBit = (otherSums[i] & mask)==mask?1:0; // calc next carry 1 or 0
		otherSums[i] = (l_word)((otherSums[i] << 1) + oldShiftBit); // do the shift
		oldShiftBit = newShiftBit;
	}
	// don't worry about left-overy carry because this can't overflow
	// maxlen N-digit*N-digit = 2n-digit

	// Go through each digit, multiplying with itself
	for (i=0; i <len; i++)
	{
		carry = (l_dword)lint->mData[i] * lint->mData[i];
		squareSums[i*2] = (l_word)carry;
		squareSums[i*2+1] = (l_word)(carry >> GS_LARGEINT_DIGIT_SIZE_BITS);
	}
	squareLen = (l_word)(2*len);
	otherLen = (l_word)(2*len); 

	// Add the two together
	result = gsiLargeIntAdd(otherSums, otherLen, squareSums, squareLen, squareSums, &squareLen, GS_LARGEINT_MAX_DIGITS*2);
	result = gsiLargeIntDiv(squareSums, squareLen, mod, NULL, dest);

	GSLINT_EXITTIMER(GSLintTimerSquareMod);
	return result;
}

//#define NEWEXP
#ifdef NEWEXP

//#define printf

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Montgomery exponentiation (see HAC 14.94)
//
// SPECIAL NOTE:
//    A small public exponent will reduce the load on client encryption.
//    (below 65535 is a security risk, so don't go too small)
gsi_bool gsLargeIntPowerMod(const gsLargeInt_t *b, const gsLargeInt_t *p, const gsLargeInt_t *m, gsLargeInt_t *dest)
{
	gsLargeInt_t base;
	gsLargeInt_t power;
	gsLargeInt_t mod;
	gsLargeInt_t one;

	gsi_u32 expHighBit; // highest bit set in exponent;

	int i = 0;        // temp / counter
	int k = 0;        // binary size of our subdigits
	int pow2k = 0;    // 2^k
	int kmask = 0;    // 2^k-1
	int kdigits = 0;  // number of k-sized digits in p
	//int leadingZeroBits = 0; // to make p evenly divisible by k

	l_word modPrime;
	gsLargeInt_t R;     // "R" as used in the montgomery exponentiation algorithm.
	//gsLargeInt_t Rmod;  // R mod n
	//gsLargeInt_t R2mod; // R^2 mod n

	gsLargeInt_t * lut = NULL;

	GSLINT_ENTERTIMER(GSLintTimerPowerMod);

	memcpy(&base, b, sizeof(base));
	memcpy(&power, p, sizeof(power));
	memcpy(&mod, m, sizeof(mod));
	memset(&R, 0, sizeof(R));
	
	gsLargeIntSetValue(&one, 1);

	// Catch the unusual cases
	if (mod.mLength == 0)
	{
		// mod 0 = undefined
		dest->mLength = 0;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}
	else if (mod.mLength==1 && mod.mData[0]==1)
	{
		// mod 1 = 0
		dest->mLength = 0;
		dest->mData[0] = 0;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_true;
	}
	else if (power.mLength == 0)
	{
		// x^0 = 1
		dest->mLength = 1;
		dest->mData[0] = 1;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_true;
	}
	else if ((mod.mData[0]&1) == 0)
	{
		// Montgomery only works with odd modulus!
		// (rsa modulus is prime1*prime2, which must be odd)
		dest->mLength = 0;
		dest->mData[0] = 0;
		//_asm {int 3}
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}
	// If base is larger than mod, we can (must) reduce it
	if (gsiLargeIntCompare(base.mData, base.mLength, mod.mData, mod.mLength)!=-1)
	{
		gsLargeIntDiv(&base, &mod, NULL, &base);
	}
	if (base.mLength == 0)
	{
		// 0^e = 0
		dest->mLength = 0;
		dest->mData[0] = 0;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_true;
	}

	// find the highest bit set in power
	expHighBit=GS_LARGEINT_DIGIT_SIZE_BITS;
	while(((1<<(expHighBit-1))&power.mData[power.mLength-1]) == 0)
		expHighBit--;
	expHighBit += ((power.mLength-1) * GS_LARGEINT_DIGIT_SIZE_BITS); // add in 32 bits for each extra byte

	// The previous algorithm used 1-bit digits
	// This algorithm uses k-bit digits
	// Determine the optimal size for k
	k=8; // this will support up to 4096 bit encryption (and probably higher)
	while ( (k > 1) && 
		(gsi_u32)((k - 1) * (k << ((k - 1) << 1)) / ((1 << k) - k - 1)) >= expHighBit - 1
		)
    {
      --k;
    }
	pow2k = 1 << k;
	kmask = pow2k-1;
	kdigits = (expHighBit+(k-1)) / k;  // ceiling(expHighBit/k)

	// calculate "R" (if mod=5678, R=10000 e.g. One digit higher)
	memset(&R, 0, sizeof(R));
	R.mLength = (l_word)(mod.mLength+1);
	if (R.mLength > GS_LARGEINT_MAX_DIGITS)
		return gsi_false; // you need to increase the large int capacity
	R.mData[R.mLength-1] = 1; // set first bit one byte higher than mod

	// find the multiplicative inverse of mod
	gsiLargeIntInverseMod(&mod, &modPrime);

/*
	// calculate Rmod (R%mod)
	if (gsi_is_false(gsLargeIntDiv(&R, &mod, NULL, &Rmod)))
	{
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}

	// calculate R2mod  (R^2%mod = (Rmod*Rmod)%mod)
	if (gsi_is_false(gsLargeIntSquareMod(&Rmod, &mod, &R2mod)))
	{
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}
*/
	// Allocate space for a table of values that will come up repeatedly
	//    xwiggle is (br mod n)
	//    These are the odd powers of xwiggle, x^3, x^5 and so on
	//    We generate these by repeated multiplications by xwiggle
	//if (k >= 3)
	{
		// no no no[0] = xwiggle^3  (montgomery multiply [2]*[1])
		// no no no[1] = xwiggle^5  (montgomery multiply [2]*[3])
		// no no no[2] = xwiggle^7  (montgomery multiply [2]*[5])

		// allocate space
		// ~1k for typical small RSA public exponents (e.g. 65537)
		// ~16k for 1024-bit RSA exponent
		// ~32k for 2048-bit RSA exponent
		// ~64k for 4096-bit RSA exponent
		int i=0;
		int valuesNeeded = pow2k;//((pow2k/2)-1);
		int spaceneeded = sizeof(gsLargeInt_t) * valuesNeeded;

		lut = (gsLargeInt_t*)gsimalloc(spaceneeded);
		if (lut == NULL)
			return gsi_false; // out of memory
		memset(lut, 0x00, spaceneeded);

		// set first values
		//   [0] = 1
		//   [1] = br mod n (normal multiplication)
		//   [i] = mont([1] * [i-1])
		gsLargeIntSetValue(&lut[0], 1);
		if (gsi_is_false(gsLargeIntMult(&base, &R, &lut[1])) ||
			gsi_is_false(gsLargeIntDiv(&lut[1], &mod, NULL, &lut[1])) )
		{
			gsifree(lut);
			GSLINT_EXITTIMER(GSLintTimerPowerMod);
			return gsi_false;
		}

		// fill in the values
		for (i=2; i < valuesNeeded; i++)
		{
			if (gsi_is_false(gsiLargeIntMultM(&lut[1], &lut[i-1], &mod, modPrime, &lut[i])) )
			{
				gsifree(lut);
				GSLINT_EXITTIMER(GSLintTimerPowerMod);
				return gsi_false;
			}
		}
	}

	// set starting point
	if (gsi_is_false(gsLargeIntMult(&base, &R, dest)) ||      // Normal multiply
		gsi_is_false(gsLargeIntDiv(dest, &mod, NULL, dest)) ) // A mod operation
	{
		gsifree(lut);
		return gsi_false;
	}

	// loop through the k-sized digits
	for (i=0; i < kdigits; i++)
	{
		int bitReadIndex = expHighBit - (i*k); // index of the bit we're reading
		int l_index; // = ((bitReadIndex-1)/GS_LARGEINT_DIGIT_SIZE_BITS); // -1 to use zero based indexes
		int l_firstbit;
		l_dword twodigits;
		l_dword mask;
		l_word digitval;

		l_index = ((bitReadIndex-1)/GS_LARGEINT_DIGIT_SIZE_BITS); // -1 to use zero based indexes

		// for first digit, use leading zeroes when necessary
		if ((bitReadIndex % k) != 0)
			bitReadIndex += k - (bitReadIndex % k); // round up to next k
		if (i != 0)
		{
			if (bitReadIndex - (l_index*GS_LARGEINT_DIGIT_SIZE_BITS)> GS_LARGEINT_DIGIT_SIZE_BITS)
				l_index++;
		}

		if (i==0) 
		{
			// first digit
			l_firstbit = l_index * GS_LARGEINT_DIGIT_SIZE_BITS; // first bit of this digit
			twodigits = p->mData[l_index];
		}
		else if (l_index > 0)
		{
			// middle digits
			l_firstbit = (l_index-1) * GS_LARGEINT_DIGIT_SIZE_BITS; // first bit of this digit
			twodigits = (l_dword)((l_dword)p->mData[l_index] << GS_LARGEINT_DIGIT_SIZE_BITS) | p->mData[l_index-1];
		}
		else if (l_index == 0 && p->mLength > 1)
		{
			// final digit, when there are proceeding digits
			l_firstbit = 0;
			twodigits = (l_dword)(p->mData[l_index+1] << GS_LARGEINT_DIGIT_SIZE_BITS) | p->mData[l_index];
		}
		else
		{
			// final digit, no proceeding digits
			l_firstbit = l_index * GS_LARGEINT_DIGIT_SIZE_BITS; // first bit of this digit
			twodigits = p->mData[l_index];
		}
		mask = (l_dword)kmask << (bitReadIndex-l_firstbit-k);
		digitval = (l_word)((twodigits & mask) >> (bitReadIndex-l_firstbit-k));

		// use digitval to determine how many squaring and multiplication operations we need to perform
		{
			static int twotab[] =
			{0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0,
			 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
			 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
			 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
			 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0,
			 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
			 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};


			static USHORT oddtab[] =
			{0, 1, 1, 3, 1, 5, 3, 7, 1, 9, 5, 11, 3, 13, 7, 15, 1, 17, 9, 19, 5, 21, 11, 23, 3, 25, 13, 27, 7, 29, 15, 31, 1,
			 33, 17, 35, 9, 37, 19, 39, 5, 41, 21, 43, 11, 45, 23, 47, 3, 49, 25, 51, 13, 53, 27, 55, 7, 57, 29, 59, 15,
			 61, 31, 63, 1, 65, 33, 67, 17, 69, 35, 71, 9, 73, 37, 75, 19, 77, 39, 79, 5, 81, 41, 83, 21, 85, 43, 87, 11,
			 89, 45, 91, 23, 93, 47, 95, 3, 97, 49, 99, 25, 101, 51, 103, 13, 105, 53, 107, 27, 109, 55, 111, 7, 113,
			 57, 115, 29, 117, 59, 119, 15, 121, 61, 123, 31, 125, 63, 127, 1, 129, 65, 131, 33, 133, 67, 135, 17,
			 137, 69, 139, 35, 141, 71, 143, 9, 145, 73, 147, 37, 149, 75, 151, 19, 153, 77, 155, 39, 157, 79, 159,
			 5, 161, 81, 163, 41, 165, 83, 167, 21, 169, 85, 171, 43, 173, 87, 175, 11, 177, 89, 179, 45, 181, 91,
			 183, 23, 185, 93, 187, 47, 189, 95, 191, 3, 193, 97, 195, 49, 197, 99, 199, 25, 201, 101, 203, 51, 205,
			 103, 207, 13, 209, 105, 211, 53, 213, 107, 215, 27, 217, 109, 219, 55, 221, 111, 223, 7, 225, 113,
			 227, 57, 229, 115, 231, 29, 233, 117, 235, 59, 237, 119, 239, 15, 241, 121, 243, 61, 245, 123, 247, 31,
			 249, 125, 251, 63, 253, 127, 255};


			//printf("[gsint] Digit %d = %d\r\n", i, digitval);
			if (i==0)
			{
				int counter = 0;

				memcpy(dest, &lut[oddtab[digitval]], sizeof(gsLargeInt_t));
				//printf("[gsint] Set start to %d\r\n", dest->mData[0]);

				for (counter = twotab[digitval]; counter> 0; counter--)
				{
					if (gsi_is_false(gsiLargeIntMultM(dest,dest, &mod, modPrime, dest)))
					{
						gsifree(lut);
						return gsi_false;
					}
					//printf("[gsint] First digit, squared to %d\r\n", dest->mData[0]);
				}
			}
			else if (digitval != 0)
			{
				int counter = 0;
				int lutindex = oddtab[digitval]; // we only precalculate the odd powers
				//int lutindex = (oddtab[digitval]+1)/2; // we only precalculate the odd powers

				for (counter = (int)(k-twotab[digitval]); counter> 0; counter--)
				{
					if (gsi_is_false(gsiLargeIntMultM(dest,dest, &mod, modPrime, dest)))
					{
						gsifree(lut);
						return gsi_false;
					}
					//printf("[gsint]    Squared to %d\r\n", dest->mData[0]);
				}
		
				if (gsi_is_false(gsiLargeIntMultM(dest, &lut[lutindex], &mod, modPrime, dest)))
				{
					gsifree(lut);
					return gsi_false;
				}
				//printf("[gsint]    Mult by [%d](%d) to %d\r\n", lutindex, lut[lutindex].mData[0], dest->mData[0]);
				for (counter = twotab[digitval]; counter> 0; counter--)
				{
					if (gsi_is_false(gsiLargeIntMultM(dest,dest, &mod, modPrime, dest)))
					{
						gsifree(lut);
						return gsi_false;
					}
					//printf("[gsint]    Squared to %d\r\n", dest->mData[0]);
				}
			}
			else
			{
				int counter = 0;
				for (counter = k; counter > 0; counter--)
				{
					if (gsi_is_false(gsiLargeIntMultM(dest,dest, &mod, modPrime, dest)))
					{
						gsifree(lut);
						return gsi_false;
					}
					//printf("[gsint]    Squared to %d\r\n", dest->mData[0]);
				}
			}
		}
	}

	// normalize  (MultM by 1)
	if (gsi_is_false(gsiLargeIntMultM(dest, &one, &mod, modPrime, dest)))
		return gsi_false;

	gsifree(lut);
	return gsi_true;
}

#else


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Montgomery exponentiation (see HAC 14.94)
//
// SPECIAL NOTE:
//    A small public exponent will reduce the load on client encryption.
//    (below 65535 is a security risk, so don't go too small)
gsi_bool gsLargeIntPowerMod(const gsLargeInt_t *b, const gsLargeInt_t *p, const gsLargeInt_t *m, gsLargeInt_t *dest)
{
	int i=0; // temp/counter
	int digitNum=0; // temp/counter
	int digitBit=0;
	
	l_word modPrime;

	gsi_u32 expHighBit; // highest bit set in exponent;

	gsLargeInt_t R; // "R" as used in the montgomery exponentiation algorithm.
	gsLargeInt_t Rmod;   // R%mod
	gsLargeInt_t R2mod;  // R^2%mod
	gsLargeInt_t temp;
	gsLargeInt_t xwiggle; // montgomery mult of (x,R2mod)

	gsLargeInt_t base;
	gsLargeInt_t power;
	gsLargeInt_t mod;

	GSLINT_ENTERTIMER(GSLintTimerPowerMod);

	memset(&R, 0, sizeof(R));
	memset(&Rmod, 0, sizeof(Rmod));
	memset(&R2mod, 0, sizeof(R2mod));
	memset(&temp, 0, sizeof(temp));
	memset(&xwiggle, 0, sizeof(xwiggle));

	memcpy(&base, b, sizeof(base));
	memcpy(&power, p, sizeof(power));
	memcpy(&mod, m, sizeof(mod));

	gsiLargeIntStripLeadingZeroes(&base);
	gsiLargeIntStripLeadingZeroes(&power);
	gsiLargeIntStripLeadingZeroes(&mod);

	// Catch the unusual cases
	if (mod.mLength == 0)
	{
		// mod 0 = undefined
		dest->mLength = 0;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}
	else if (mod.mLength==1 && mod.mData[0]==1)
	{
		// mod 1 = 0
		dest->mLength = 0;
		dest->mData[0] = 0;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_true;
	}
	else if (power.mLength == 0)
	{
		// x^0 = 1
		dest->mLength = 1;
		dest->mData[0] = 1;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_true;
	}
	else if ((mod.mData[0]&1) == 0)
	{
		// Montgomery only works with odd modulus!
		// (rsa modulus is prime1*prime2, which must be odd)
		dest->mLength = 0;
		dest->mData[0] = 0;
		//_asm {int 3}
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}
	// If base is larger than mod, we can (must) reduce it
	if (gsiLargeIntCompare(base.mData, base.mLength, mod.mData, mod.mLength)!=-1)
	{
		gsLargeIntDiv(&base, &mod, NULL, &base);
	}
	if (base.mLength == 0)
	{
		// 0^e = 0
		dest->mLength = 0;
		dest->mData[0] = 0;
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_true;
	}

	// find the highest bit set in power
	expHighBit=GS_LARGEINT_DIGIT_SIZE_BITS;
	while(((1<<(expHighBit-1))&power.mData[power.mLength-1]) == 0)
		expHighBit--;
	expHighBit += ((power.mLength-1) * GS_LARGEINT_DIGIT_SIZE_BITS); // add in 32 bits for each extra byte
	
	// On to the tricky tricky!
	//    1) We can't compute B^P and later apply the mod; B^P is just too big
	//       So we have to make modular reductions along the way
	//    2) Since modular reduction is essentially a division, we would like
	//       to use a mod 2^E so that division is just a bit strip.
	//       ex. (1383 mod 16) = binary(0000010101100111 mod 00010000) = 00000111 = dec 7

	// Precalculate some values that will come up repeatedly

	// calculate "R" (if mod=5678, R=10000 e.g. One digit higher)
	memset(&R, 0, sizeof(R));
	R.mLength = (l_word)(mod.mLength+1);
	if (R.mLength > GS_LARGEINT_MAX_DIGITS)
		return gsi_false; // you need to increase the large int capacity
	R.mData[R.mLength-1] = 1; // set first bit one byte higher than mod

	// find the multiplicative inverse of mod
	gsiLargeIntInverseMod(&mod, &modPrime);

	// calculate Rmod (R%mod)
	if (gsi_is_false(gsLargeIntDiv(&R, &mod, NULL, &Rmod)))
	{
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}

	// calculate R2mod  (R^2%mod = (Rmod*Rmod)%mod)
	if (gsi_is_false(gsLargeIntSquareMod(&Rmod, &mod, &R2mod)))
	{
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}

	// calculate xwiggle
	if (gsi_is_false(gsiLargeIntMultM(&base, &R2mod, &mod, modPrime, &xwiggle)))
	{
		GSLINT_EXITTIMER(GSLintTimerPowerMod);
		return gsi_false;
	}

	// loop through the BITS of power
	//    if the bit is 1, perform a multiplication by xwiggle? (11/2/2006)
	//    TODO:  THIS DOESN'T WORK IF THE HIGHBIT IS EVER ABOVE GS_LARGEINT_DIGIT_SIZE_BITS
	memcpy(dest, &Rmod, sizeof(gsLargeInt_t)); // start dest at Rmod
	for (i=(int)(expHighBit-1); i>=0; i--)
	{
		// mont square the current total
		gsiLargeIntMultM(dest, dest, &mod, modPrime, dest);
		digitNum = (gsi_i32)(i/GS_LARGEINT_DIGIT_SIZE_BITS);    // which digit to extract a bit from?
		digitBit = (gsi_i32)(i % GS_LARGEINT_DIGIT_SIZE_BITS);  // which bit to extract from that digit?
		//if ((power.mData[k] & (1<<i))==((l_word)1<<i))
		
		// HACKED DUE TO COMPILER CRASH
		// THE REPEATED 1<<digitbit caused the optimizer to 'splode
		{
			GS_LARGEINT_DIGIT_TYPE digit = power.mData[digitNum];
			GS_LARGEINT_DIGIT_TYPE mask = (GS_LARGEINT_DIGIT_TYPE)(1<<digitBit); 
			GS_LARGEINT_DIGIT_TYPE masked = digit & mask; //(1<<digitBit);

			// FORCE COMPILER TO NOT OPTIMIZE THIS
			if (mask == masked)
				gsiLargeIntMultM(dest, &xwiggle, &mod, modPrime, dest);
		}
	}

	// Since we're working with Montgomery values (x*R2mod)
	// we have to adjust back to x
	temp.mLength = 1;
	temp.mData[0] = 1;
	gsiLargeIntMultM(dest, &temp, &mod, modPrime, dest);

	GSLINT_EXITTIMER(GSLintTimerPowerMod);
	return gsi_true;
}

#endif


#define NEWMULTM
#ifdef  NEWMULTM

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Montgomery multiplication
//    Computes (src1*src2*r^-1)%mod
gsi_bool gsiLargeIntMultM(gsLargeInt_t *x, gsLargeInt_t *y, const gsLargeInt_t *m, gsi_u32 modPrime, gsLargeInt_t *dest)
{
	l_word tempLen = 0;
	l_word temp[GS_LARGEINT_MAX_DIGITS*2];

	l_word* lasttnptr;
	const l_word* lastnptr;
	l_word* tptr;
	const l_word* nptr;
	l_word* tiptr;
	
	l_dword carry = 0;
	l_word mi = 0;

	l_word logB_r = m->mLength;

	memset(temp, 0, sizeof(temp));

	if (gsi_is_false(gsiLargeIntMult(x->mData, x->mLength, y->mData, y->mLength, temp, &tempLen, GS_LARGEINT_MAX_DIGITS*2)))
		return gsi_false;
	
	lasttnptr = &temp[m->mLength-1];
	lastnptr = &m->mData[m->mLength-1];

	if (tempLen < m->mLength*2)
	{
		memset(&temp[tempLen], 0, (m->mLength*2 - tempLen) * GS_LARGEINT_DIGIT_SIZE_BYTES);
		//memset(&temp[tempLen], 0, sizeof(temp) - tempLen * GS_LARGEINT_DIGIT_SIZE_BYTES); // safer to clear out the whole thing?
		tempLen = (l_word)(m->mLength*2);
	}

	for (tptr = &temp[0]; tptr <= lasttnptr; tptr++)
	{
		carry = 0;
		mi = (l_word)((l_dword)modPrime * (l_dword)*tptr);
		tiptr = tptr;
		for (nptr = &m->mData[0]; nptr <= lastnptr; nptr++, tiptr++)
		{
			carry = (l_dword)mi * (l_dword)*nptr +
				    (l_dword)*tiptr + (l_dword)(l_word)(carry >> GS_LARGEINT_DIGIT_SIZE_BITS);
			*tiptr = (l_word)(carry);
		}

		// apply the carry value
		for (; ((carry >> GS_LARGEINT_DIGIT_SIZE_BITS) > 0) && tiptr <= &temp[tempLen-1]; tiptr++)
		{
			*tiptr = (l_word)(carry = (l_dword)*tiptr + (l_dword)(l_word)(carry >> GS_LARGEINT_DIGIT_SIZE_BITS));
		}

		// If we still have a carry, increase the length of temp
		if (((carry >> GS_LARGEINT_DIGIT_SIZE_BITS) > 0))
		{
			*tiptr = (l_word)(carry >> GS_LARGEINT_DIGIT_SIZE_BITS);
			tempLen++;
		}
	}

	// **WARNING**
	// Bytes from the plain text message may appear within the temporary buffer.
	// These bytes should be cleared to prevent bugs where that data may be exposed.  (buffer overrun?)
	if (gsiLargeIntCompare(&temp[logB_r], tempLen - logB_r, m->mData, m->mLength) != -1)
	{
		if (gsi_is_false(gsiLargeIntSub(m->mData, m->mLength, &temp[logB_r], tempLen - logB_r, dest->mData, &dest->mLength)))
		{
			memset(temp, 0, sizeof(temp));
			memset(dest, 0, sizeof(gsLargeInt_t));
			return gsi_false;
		}
	}
	else
	{
		memset(dest, 0, sizeof(gsLargeInt_t));
		dest->mLength = m->mLength;
		memcpy(dest->mData, &temp[logB_r], (tempLen - logB_r)*GS_LARGEINT_DIGIT_SIZE_BYTES);
		memset(temp, 0, sizeof(temp));
	}

	return gsi_true;
}

#else


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Montgomery multiplication
//    Computes (src1*src2*r^-1)%mod
// Note:
//    This implementation is based on HAC14.36 which has a lot of room for improvement  
//    FLINT algorithm runs approx 30 times faster. 
gsi_bool gsiLargeIntMultM(gsLargeInt_t *x, gsLargeInt_t *y, const gsLargeInt_t *m, gsi_u32 modPrime, gsLargeInt_t *dest)
{
	int i=0;
	l_dword xiy0;
	l_word u = 0;

	gsLargeInt_t A;
	gsLargeInt_t xiy;
	gsLargeInt_t temp; 

	GSLINT_ENTERTIMER(GSLintTimerMultM);

	gsiLargeIntStripLeadingZeroes(x);
	gsiLargeIntStripLeadingZeroes(y);

	// Check inputs
	i=(int)(m->mLength);
	while(i>0 && m->mData[i-1]==0)
		i--;
	if (i==0)
	{
		// modulus is zero, answer undefined
		dest->mData[0] = 0;
		dest->mLength = 0;
		GSLINT_EXITTIMER(GSLintTimerMultM);
		return gsi_false;
	}
	if (x->mLength==0)
	{
		// x == 0
		dest->mLength = 0;
		dest->mData[0] = 0;
		GSLINT_EXITTIMER(GSLintTimerMultM);
		return gsi_true;
	}
	if (y->mLength==0)
	{
		// y == 0
		dest->mLength = 0;
		dest->mData[0] = 0;
		GSLINT_EXITTIMER(GSLintTimerMultM);
		return gsi_true;
	}

	// We pad with zeroes so that we don't have to check for overruns in the loop below
	//   (note: resize will not remove non-zero digits from x or y)
	gsiLargeIntResize(x, m->mLength);
	gsiLargeIntResize(y, m->mLength);

	// Continue with the Multiplication
	memset(&A, 0, sizeof(A));
	memset(&temp, 0, sizeof(temp));
	memset(&xiy, 0, sizeof(xiy));
	
	for (i=0; (gsi_u32)i < m->mLength; i++)
	{
		xiy0 = (l_dword)x->mData[i]*y->mData[0];  // y[0], NOT y[i] !!
		u = (l_word)((xiy0+A.mData[0])*modPrime); // strip bits over the first digit

		// A = (A+x[i]*y + u[i]*m)/b
		//    compute x[i]*y
		memset(temp.mData, 0, y->mLength*sizeof(l_word)); // clear out a portion of temp
		temp.mData[0] = x->mData[i];
		temp.mLength = y->mLength; // xi padded with zeroes
		if (gsi_is_false(gsiLargeIntMult(temp.mData, temp.mLength, y->mData, y->mLength, xiy.mData, &xiy.mLength, GS_LARGEINT_MAX_DIGITS)))
		{
			// overflow
			dest->mLength = 0;
			dest->mData[0] = 0;
			GSLINT_EXITTIMER(GSLintTimerMultM);
			return gsi_false;
		}
		//    compute u[i]*m
		memset(temp.mData, 0, m->mLength*sizeof(l_word)); // clear out a portion of temp
		temp.mData[0] = u;
		temp.mLength = m->mLength;
		//if (gsi_is_false(gsiLargeIntMult(temp.mData, temp.mLength, m->mData, m->mLength, temp.mData, &temp.mLength)))
		if (gsi_is_false(gsLargeIntKMult(&temp, m, &temp)))
		{
			// overflow
			dest->mLength = 0;
			dest->mData[0] = 0;
			GSLINT_EXITTIMER(GSLintTimerMultM);
			return gsi_false;
		}
		//    Add both to A
		if (gsi_is_false(gsiLargeIntAdd(xiy.mData, xiy.mLength, A.mData, A.mLength, A.mData, &A.mLength, GS_LARGEINT_MAX_DIGITS)))
		{
			// overflow
			dest->mLength = 0;
			dest->mData[0] = 0;
			GSLINT_EXITTIMER(GSLintTimerMultM);
			return gsi_false;
		}
		if (gsi_is_false(gsiLargeIntAdd(temp.mData, temp.mLength, A.mData, A.mLength, A.mData, &A.mLength, GS_LARGEINT_MAX_DIGITS)))
		{
			// overflow
			dest->mLength = 0;
			dest->mData[0] = 0;
			GSLINT_EXITTIMER(GSLintTimerMultM);
			return gsi_false;
		}
		//    Divide by b  (e.g. Remove first digit from A)
		if (A.mLength > 1)
		{
			memmove(&A.mData[0], &A.mData[1], (A.mLength-1)*sizeof(l_word));
			A.mData[A.mLength-1] = 0;
			A.mLength--;
		}
		else
		{
			A.mLength = 0;
			A.mData[0] = 0;
		}
	}

	//if (A >= m then subtract another m)
	if (gsiLargeIntCompare(A.mData, A.mLength, m->mData, m->mLength)!=-1)
		gsiLargeIntSub(m->mData, m->mLength, A.mData, A.mLength, dest->mData, &dest->mLength);
	else
		memcpy(dest, &A, sizeof(A));
	GSLINT_EXITTIMER(GSLintTimerMultM);
	return gsi_true;
}

#endif
/*
//    Computes (src*src*r^-1)%mod
static gsi_bool gsiLargeIntSquareM(const gsLargeInt_t *src, const gsLargeInt_t *mod, gsi_u32 modPrime, gsi_u32 R, gsLargeInt_t *dest)
{
	GSI_UNUSED(src);
	GSI_UNUSED(mod);
	GSI_UNUSED(modPrime);
	GSI_UNUSED(R);
	GSI_UNUSED(dest);
	assert(0);
	return gsi_true;
}*/


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Calculate multiplicative inverse of mod, (-mod^-1 mod 2^R)
//    ala. Dusse and Kaliski, extended Euclidean algorithm
gsi_bool gsiLargeIntInverseMod(const gsLargeInt_t *mod, l_word *dest)
{
	l_dword x=2;
	l_dword y=1;
	l_dword check = 0;

	gsi_u32 i;
	for (i = 2; i <= GS_LARGEINT_DIGIT_SIZE_BITS; i++)
	{
		check = (l_dword)mod->mData[0] * (l_dword)y;
		if (x < (check & ((x<<1)-1)))
			y += x;
		x = x << 1;
	}
	*dest = (l_word)(x-y);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsLargeIntPrint(FILE* logFile, const gsLargeInt_t *lint)
{
	return gsiLargeIntPrint(logFile, lint->mData, lint->mLength);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsiLargeIntPrint(FILE* logFile, const l_word *data, l_word length)
{
// this is only specific to NITRO since for other platforms the fprintf will
// resolve to a STDOUT
#if !defined(_NITRO)
	while(length >0)
	{
		fprintf(logFile, "%08X", data[length-1]);
		length--;
	}
	fprintf(logFile, "\r\n");
	return gsi_true;
#else
	GSI_UNUSED(logFile);
	GSI_UNUSED(data);
	GSI_UNUSED(length);
	return gsi_false;			
#endif
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// stream of bytes, big endian.  (first byte = most significant digit)
gsi_bool gsLargeIntSetFromHexString(gsLargeInt_t *lint, const char* hexstream)
{
	l_word* writePos = lint->mData;
	gsi_u32 temp;
	int len = 0;
	int byteIndex = 0;

	GS_ASSERT(hexstream != NULL);
	
	len = (int)strlen(hexstream);
	if (len == 0)
	{
		lint->mLength = 0;
		lint->mData[0] = 0;
		return gsi_true;
	}
	if ((len/2) > (GS_LARGEINT_MAX_DIGITS*GS_LARGEINT_DIGIT_SIZE_BYTES))
		return gsi_false;
	
	// 2 characters per byte, 4 bytes per integer
	lint->mLength = (l_word)((len+(2*GS_LARGEINT_DIGIT_SIZE_BYTES-1))/(2*GS_LARGEINT_DIGIT_SIZE_BYTES));
	lint->mData[lint->mLength-1] = 0; // set last byte to zero for left over characters
	
	while(len > 0)
	{
		if(len >= 2)
			sscanf((char*)(hexstream+len-2), "%02x", &temp); // sscanf requires a 4 byte dest
		else
			sscanf((char*)(hexstream+len-1), "%01x", &temp); // sscanf requires a 4 byte dest
		if(byteIndex == 0)
			*writePos = 0;
		*writePos |= (temp << (byteIndex * 8));
		if(++byteIndex == GS_LARGEINT_DIGIT_SIZE_BYTES)
		{
			writePos++;
			byteIndex = 0;
		}
		len-=min(2,len);
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Reverse bytes in a LINT, which are LittleEndian
//     ex: Packing an RSA message of which the first bytes are 0x00 0x02
//         The first bytes of the packet must become the MSD of the LINT
gsi_bool gsLargeIntReverseBytes(gsLargeInt_t *lint)
{
#if defined(GSI_LITTLE_ENDIAN)
	char *left = (char*)&lint->mData[0];
	char *right = ((char*)&lint->mData[lint->mLength])-1;
	char  temp;
#else
	l_word *left = lint->mData;
	l_word *right = lint->mData + (lint->mLength - 1);
	l_word  temp;
#endif


	if (lint->mLength == 0)
		return gsi_true;

	while(left < right)
	{
		temp = *left;
		(*left++) = (*right);
		(*right--) = temp;
	}
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// hashing is made complicated by differing byte orders 
void gsLargeIntAddToMD5(const gsLargeInt_t * _lint, MD5_CTX * md5)
{
	int byteLength = 0;
	gsi_u8 * dataStart = NULL;

	// Create a non-const copy so we can reverse bytes to add to the MD5 hash
	gsLargeInt_t lint;
	memcpy(&lint, _lint, sizeof(lint));

	// first, calculate the byte length
	byteLength = (int)gsLargeIntGetByteLength(&lint);
	if (byteLength == 0)
		return; // no data

	dataStart = (gsi_u8*)lint.mData;
	if ((byteLength % GS_LARGEINT_DIGIT_SIZE_BYTES) != 0)
		dataStart += GS_LARGEINT_DIGIT_SIZE_BYTES - (byteLength % GS_LARGEINT_DIGIT_SIZE_BYTES);

	// reverse to big-endian (MS) then hash
	gsLargeIntReverseBytes(&lint);
	MD5Update(md5, dataStart, (unsigned int)byteLength);
	gsLargeIntReverseBytes(&lint);
} 


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Length in bytes so leading zeroes can be dropped from hex strings
gsi_u32  gsLargeIntGetByteLength(const gsLargeInt_t *lint)
{
	int intSize = (int)lint->mLength;
	int byteSize = 0;
	int i=0;
	l_word mask = 0xFF;

	// skip leading zeroes
	while(intSize > 0 && lint->mData[intSize-1] == 0)
		intSize --;
	if (intSize == 0)
		return 0; // no data

	byteSize = intSize * (gsi_i32)sizeof(l_word);

	// subtract bytes for each leading 0x00 byte
	mask = 0xFF;
	for (i=1; i < GS_LARGEINT_DIGIT_SIZE_BYTES; i++)
	{
		if (lint->mData[intSize-1] <= mask)
		{
			byteSize -= sizeof(l_word)-i;
			break;
		}
		mask = (l_word)((mask << 8) | 0xFF);
	}

	return (gsi_u32)byteSize;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Creates a large int from a byte buffer
//		Essentially, constructs the array of digits in appropriate byte order
gsi_bool gsLargeIntSetFromMemoryStream(gsLargeInt_t *lint, const gsi_u8* data, gsi_u32 len)
{
	lint->mData[0] = 0;
	memcpy(((char*)lint->mData)+(4-len%4)%4, data, len);

	// Set length to ceiling of len/digit_size
	lint->mLength = (unsigned int)((len+(GS_LARGEINT_DIGIT_SIZE_BYTES-1))/GS_LARGEINT_DIGIT_SIZE_BYTES);

	return gsLargeIntReverseBytes(lint);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsLargeIntWriteToMemoryStream(const gsLargeInt_t *lint, gsi_u8* data)
{
	gsLargeInt_t copy;
	memcpy(&copy, lint, sizeof(gsLargeInt_t));

	gsLargeIntReverseBytes(&copy);

	memcpy(data, copy.mData, copy.mLength * GS_LARGEINT_DIGIT_SIZE_BYTES);
	return gsi_true;
}
