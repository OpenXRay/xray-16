///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Large Integer Library
#ifndef __GSLARGEINT_H__
#define __GSLARGEINT_H__

#include "gsCommon.h"
#include "gsXML.h"
#include "../md5.h"


#if defined(__cplusplus)
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// for simplicity, set the binary size to a value > gsCrypt.h binary size
#ifndef GS_LARGEINT_BINARY_SIZE
#define GS_LARGEINT_BINARY_SIZE		 (2048)   // *BIT* size (divide by 8 for byte size)
#endif

	//  !!!!!!WARNING!!!!!!  Encryption is fastest when digit type is the default system type, (ex: gsi_u32 on 32bit processor)
#define GS_LARGEINT_DIGIT_TYPE       gsi_u32
#define GS_LARGEINT_DIGIT_LONG_TYPE  gsi_u64

#define GS_LARGEINT_DIGIT_SIZE_BYTES (sizeof(GS_LARGEINT_DIGIT_TYPE))
#define GS_LARGEINT_DIGIT_SIZE_BITS  (GS_LARGEINT_DIGIT_SIZE_BYTES*8)

// short forms for legibility
#define l_word  GS_LARGEINT_DIGIT_TYPE 
#define l_dword GS_LARGEINT_DIGIT_LONG_TYPE

//#define GS_LARGEINT_BYTE_SIZE       32     // binary size of system data type
//#define GS_LARGEINT_INT_SIZE        (GS_LARGEINT_BINARY_SIZE/GS_LARGEINT_BYTE_SIZE)  // size in values
#define GS_LARGEINT_MAX_DIGITS       (GS_LARGEINT_BINARY_SIZE / GS_LARGEINT_DIGIT_SIZE_BITS)

#define GS_LARGEINT_KARATSUBA_CUTOFF 32

typedef struct gsLargeInt_s
{
	GS_LARGEINT_DIGIT_TYPE mLength;
	GS_LARGEINT_DIGIT_TYPE mData[GS_LARGEINT_MAX_DIGITS];
} gsLargeInt_t;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Commonly used functions
void     gsLargeIntAddToMD5(const gsLargeInt_t * lint, MD5_CTX * md5);
gsi_bool gsLargeIntSetFromHexString(gsLargeInt_t *lint, const char* hexstring);
gsi_bool gsLargeIntPrint (FILE* logFile, const gsLargeInt_t *lint);
gsi_u32  gsLargeIntGetByteLength(const gsLargeInt_t *lint);

         // Modular exponentiation (and helpers)
         //   -- uses Montgomery exponentiation, reduction, multiplication
gsi_bool gsLargeIntPowerMod(const gsLargeInt_t *base, const gsLargeInt_t *power, const gsLargeInt_t *mod, gsLargeInt_t *dest);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool gsLargeIntSquareMod(const gsLargeInt_t *lint, const gsLargeInt_t *mod, gsLargeInt_t *dest);
gsi_bool gsLargeIntAdd   (const gsLargeInt_t *src1, const gsLargeInt_t *src2, gsLargeInt_t *dest);
gsi_bool gsLargeIntSub   (const gsLargeInt_t *src1, const gsLargeInt_t *fromsrc2, gsLargeInt_t *dest);
gsi_bool gsLargeIntMult  (const gsLargeInt_t *src1, const gsLargeInt_t *src2, gsLargeInt_t *dest);
gsi_bool gsLargeIntDiv   (const gsLargeInt_t *src1, const gsLargeInt_t *divisor, gsLargeInt_t *dest, gsLargeInt_t *remainder);

         //Karatsuba requires that the sizes be equal and a power of two
gsi_bool gsLargeIntKMult(const gsLargeInt_t *src1, const gsLargeInt_t *src2, gsLargeInt_t *dest);

         //This is useful when packing a BigEndian message directly into a LittleEndian lint buffer.
gsi_bool gsLargeIntReverseBytes(gsLargeInt_t *lint);
gsi_bool gsLargeIntSetValue(gsLargeInt_t *lint, l_word value);

         //These are necessary to preserve byte order when copying from array-of-int to char*
gsi_bool gsLargeIntSetFromMemoryStream(gsLargeInt_t *lint, const gsi_u8* data, gsi_u32 len);
gsi_bool gsLargeIntWriteToMemoryStream(const gsLargeInt_t *lint, gsi_u8* data);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
}
#endif

#endif // __GSLARGEINT_H__
