///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __GS_CRYPT_H__
#define __GS_CRYPT_H__


#include "gsLargeInt.h"
#include "../md5.h"


#if defined(__cplusplus)
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// RSA
//
//     Based on PKCS #1 v2.1, RSA Laboratories June 14, 2002
//     
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define GS_CRYPT_HASHSIZE          GS_CRYPT_SHA1_HASHSIZE

#define GS_CRYPT_SHA1_HASHSIZE     20
#define GS_CRYPT_MD5_HASHSIZE      16

//#define GS_CRYPT_RSA_ES_OAEP          
#define GS_CRYPT_RSA_ES_PKCS1v1_5 

#ifndef GS_CRYPT_RSA_BINARY_SIZE 
#define GS_CRYPT_RSA_BINARY_SIZE   1024
#endif

#define GS_CRYPT_RSA_BYTE_SIZE     (GS_CRYPT_RSA_BINARY_SIZE/8) //1024/8 = 128

#define GS_CRYPT_RSA_DATABLOCKSIZE (GS_CRYPT_RSA_BYTE_SIZE-GS_CRYPT_HASHSIZE-1)





///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef struct 
{
	gsLargeInt_t modulus;
	gsLargeInt_t exponent;
} gsCryptRSAKey;

typedef struct 
{
	gsi_u8 headerByte; // always 0x00
	gsi_u8 maskedSeed[GS_CRYPT_HASHSIZE]; // not a MD5 hash, but must be same size
	gsi_u8 maskedData[GS_CRYPT_RSA_DATABLOCKSIZE]; // data block xor'd
} gsCryptRSAOAEPPacket;

typedef struct 
{
	gsi_u8 headerByte[2]; // always 0x00 0x02
	gsi_u8 data[GS_CRYPT_RSA_BYTE_SIZE-2]; // data block xor'd
} gsCryptRSAPKCS1Packet;


// The cipherText must be equal to GS_CRYPT_RSA_BYTE_SIZE
// The plainText maximum len is:
//     OAEP:  62-bytes when using 1024-bit encryption (GS_CRYPT_RSA_BYTE_SIZE-2*GS_CRYPT_MD5_HASHSIZE-2)
//     PKCS1: 117-bytes when using 1024-bit encryption (GS_CRYPT_RSA_BYTE_SIZE-11)
gsi_i32 gsCryptRSAEncryptBuffer(const gsCryptRSAKey *publicKey, const unsigned char *plainText, gsi_u32 len, unsigned char cipherText[GS_CRYPT_RSA_BYTE_SIZE]);
gsi_i32 gsCryptRSAVerifySignedHash(const gsCryptRSAKey *publicKey, const unsigned char *hash, gsi_u32 hashLen, const unsigned char *sig, gsi_u32 sigLen);


// These require the private key, which only the server should have. Included here for test purposes
gsi_i32 gsCryptRSADecryptBuffer(const gsCryptRSAKey *privateKey, const unsigned char cipherText[GS_CRYPT_RSA_BYTE_SIZE], unsigned char *plainTextOut, gsi_u32 *lenOut);
//     Note: There is a debate on whether or not to sign-first-then-encrypt, or encrypt-first-then-sign
//           SignFirst: Decryption must take place before the signature is validated.  This is a high overhead for invalid signatures.
//           EncryptFirst: Signature validation takes place before decryption, but the MAC is unencrypted to packet sniffers.  (Exposes it to attack)
//     We use SignFirst since it's unlikely a client will receive a invalid signature DOS attack.
gsi_i32 gsCryptRSASignData(const gsCryptRSAKey *privateKey, const unsigned char *plainText, gsi_u32 plainTextLen, unsigned char *signedDataOut, gsi_u32 *lenOut);
gsi_i32 gsCryptRSASignHash(const gsCryptRSAKey *privateKey, const unsigned char *hash, gsi_u32 hashLen, unsigned char *signedDataOut, gsi_u32 *lenOut);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
}
#endif

#endif //__GS_CRYPT_H__
