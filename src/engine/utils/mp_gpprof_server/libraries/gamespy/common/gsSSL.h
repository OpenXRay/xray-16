///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __GSSSL_H__
#define __GSSSL_H__

#include "../darray.h"
#include "../md5.h"
#include "gsCrypt.h"
#include "gsSHA1.h"
#include "gsRC4.h"

#if defined(__cplusplus)
extern "C"
{
#endif

	// SSL common types and defines.  Used by HTTP SSL encryption engine

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SSL v3.0
#define GS_SSL_VERSION_MAJOR             (0x03)
#define GS_SSL_VERSION_MINOR             (0x00)


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
	// SSL content types
#define GS_SSL_CONTENT_CHANGECIPHERSPEC  (0x14) // 20
#define GS_SSL_CONTENT_ALERT             (0x15) // 21  Not sure if this is the correct value
#define GS_SSL_CONTENT_HANDSHAKE         (0x16) // 22
#define GS_SSL_CONTENT_APPLICATIONDATA   (0x17) // 23

	// SSL handshake message types
//#define GS_SSL_HANDSHAKE_HELLOREQUEST       (0)
#define GS_SSL_HANDSHAKE_CLIENTHELLO        (1)
#define GS_SSL_HANDSHAKE_SERVERHELLO        (2)
#define GS_SSL_HANDSHAKE_CERTIFICATE        (11) 
//#define GS_SSL_HANDSHAKE_SERVERKEYEXCHANGE  (12) 
//#define GS_SSL_HANDSHAKE_CERTIFICATEREQUEST (13) 
#define GS_SSL_HANDSHAKE_SERVERHELLODONE    (14) 
//#define GS_SSL_HANDSHAKE_CERTIFICATEVERIFY  (15)
#define GS_SSL_HANDSHAKE_CLIENTKEYEXCHANGE  (16) 
#define GS_SSL_HANDSHAKE_FINISHED           (20) 

// the largest payload for a single SSL packet, RFC const
// ----> RFC includes MAC and any padding, actual user data must be less
#define GS_SSL_MAX_CONTENTLENGTH ((0x4000) - (0xFF))

#ifndef HAVE_CIPHER_SUITES
	/* these are the ones used by IE */
	#define TLS_RSA_WITH_RC4_128_MD5                0x04
	#define TLS_RSA_WITH_RCA_128_SHA                0x05
	#define TLS_RSA_WITH_3DES_EDE_CBC_SHA           0x0a
	#define TLS_RSA_WITH_DES_CBC_SHA                0x09
	#define TLS_RSA_EXPORT1024_WITH_RC4_56_SHA      0x64
	#define TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA     0x62
	#define TLS_RSA_EXPORT_WITH_RC4_40_MD5          0x03
	#define TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5      0x06
	#define TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA       0x13
	#define TLS_DHE_DSS_WITH_DES_CBC_SHA            0x12
	#define TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA 0x63
#endif

		// These depend on the SSL cipher suite ranges
#define GS_SSL_MAX_MAC_SECRET_SIZE    (20)
#define GS_SSL_MAX_SYMMETRIC_KEY_SIZE (16)
#define GS_SSL_MAX_IV_SIZE            (16)
#define GS_SSL_NUM_CIPHER_SUITES       (1)  // cipher suite list defined in gsSSL.c
#define GS_SSL_MASTERSECRET_LEN       (48)
#define GS_SSL_PAD_ONE  "666666666666666666666666666666666666666666666666" // 48 bytes
#define GS_SSL_PAD_TWO  "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" // 48 bytes
#define GS_SSL_MD5_PAD_LEN            (48)
#define GS_SSL_SHA1_PAD_LEN           (40) // use only 40 of the 48 bytes
#define GS_SSL_CLIENT_FINISH_VALUE    "CLNT"
#define GS_SSL_SERVER_FINISH_VALUE    "SRVR"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SSL instance/session info
typedef struct gsSSL
{
	int sessionLen;
	unsigned char sessionData[255]; // up to 256 bytes
	unsigned short cipherSuite;

	//DArray certificateArray;
	gsCryptRSAKey serverpub;
	unsigned char sendSeqNBO[8];    // incrementing sequence number (for messages sent)
	unsigned char receiveSeqNBO[8]; // ditto (for messages received)

	// Key buffers
	//   Actual data may be smaller than array size
	unsigned char clientWriteMACSecret[GS_CRYPT_SHA1_HASHSIZE];
	unsigned char clientReadMACSecret [GS_CRYPT_SHA1_HASHSIZE];
	unsigned char clientWriteKey      [GS_SSL_MAX_SYMMETRIC_KEY_SIZE];
	unsigned char clientReadKey       [GS_SSL_MAX_SYMMETRIC_KEY_SIZE];
	unsigned char clientWriteIV       [GS_SSL_MAX_IV_SIZE];
	unsigned char clientReadIV        [GS_SSL_MAX_IV_SIZE];

	// Actual lengths of the above data blocks
	int clientWriteMACLen;
	int clientReadMACLen;
	int clientWriteKeyLen;
	int clientReadKeyLen;
	int clientWriteIVLen;
	int clientReadIVLen;

	RC4Context sendRC4; // initialized ONCE per key exchange
	RC4Context recvRC4; // initialized ONCE per key exchange

	// these are unused once the handshake is complete
	//   todo: dynamically allocate or remove to free space
	MD5_CTX finishHashMD5;
	SHA1Context finishHashSHA1;
	unsigned char serverRandom[32]; // server random for key generation, sent plain text
	unsigned char clientRandom[32]; // client random for key generation, sent plain text
	unsigned char premastersecret[GS_SSL_MASTERSECRET_LEN]; // client random for key generation, sent encrypted with serverpub
	unsigned char mastersecret[GS_SSL_MASTERSECRET_LEN];

} gsSSL;


// SSL messages (like the ClientHello) are wrapped in a "record" struct
typedef struct gsSSLRecordHeaderMsg
{
	unsigned char contentType;  // = GS_SSL_CONTENT_HANDSHAKE;
	unsigned char versionMajor; // = GS_SSL_VERSION_MAJOR;
	unsigned char versionMinor; // = GS_SSL_VERSION_MINOR;
	unsigned char lengthNBO[2]; // length of msg, limited to 2^14
	
	// WARNING: lengthNBO can NOT be an unsigned short
	//          This would create alignment issues from the previous 3 parameters

} gsSSLRecordHeaderMsg;

typedef struct gsSSLClientHelloMsg
{
	gsSSLRecordHeaderMsg header;    // include the header for easier packing
	unsigned char handshakeType; // 0x01
	unsigned char lengthNBO[3];  // 3 byte length, NBO integer! 61 = 0x00 00 3d
	unsigned char versionMajor;  // = GS_SSL_VERSION_MAJOR;
	unsigned char versionMinor;  // = GS_SSL_VERSION_MINOR;
	unsigned char time[4];       // 4 byte random (spec says set to current unix-time)
	unsigned char random[28];    // 28 byte random, total of 32 random bytes
	unsigned char sessionIdLen;  // how many of the bytes that follow are session info? (def:0)
	
	// ALIGNMENT: 44 bytes prior to this, alignment should be OK
	unsigned short cipherSuitesLength; // 2* number of cipher suites
	unsigned short cipherSuites[GS_SSL_NUM_CIPHER_SUITES];
	unsigned char compressionMethodLen; // no standard methods, set to 1
	unsigned char compressionMethodList; // set to 0
} gsSSLClientHelloMsg;

typedef struct gsSSLClientKeyExchangeMsg
{
	gsSSLRecordHeaderMsg header; // included here for easier packing
	unsigned char handshakeType; // 0x10
	unsigned char lengthNBO[3];
	//   The next lengthNBO bytes are the client contribution to the key
} gsSSLClientKeyExchangeMsg;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Information about each cipher suite
typedef struct gsSSLCipherSuiteDesc
{
	int mSuiteID;
	int mKeyLen;
	int mMACLen;
	int mIVLen;
} gsSSLCipherSuiteDesc;

extern const gsSSLCipherSuiteDesc gsSSLCipherSuites[GS_SSL_NUM_CIPHER_SUITES];
extern const unsigned char gsSslRsaOid[9];


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
} // extern "C"
#endif

#endif // __GSSSL_H__
