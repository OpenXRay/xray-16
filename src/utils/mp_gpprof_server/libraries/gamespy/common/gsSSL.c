///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsSSL.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Don't define the export cipher suites if you can avoid it, they present a security risk
const struct gsSSLCipherSuiteDesc gsSSLCipherSuites[GS_SSL_NUM_CIPHER_SUITES] =
{
	// Since common version of IIS supports these,
	//  we are safe to require the best

	// Algorithm ID (fixed const), KeyLen, CipherLen, IV Len
	{ TLS_RSA_WITH_RC4_128_MD5,            16, 16, 00 },
	//{ TLS_RSA_WITH_3DES_EDE_CBC_SHA,       16, 20, 00 },
	
	// Use of single DES is questionable
	// { TLS_RSA_WITH_DES_CBC_SHA,            00, 00, 00 },

	// Support for export ciphers poses a security risk
	// A hacker can edit the packet stream to use a weak export cipher,
	//   then crack the session and modify the message MACs
	// { TLS_RSA_EXPORT1024_WITH_RC4_56_SHA,  00, 00, 00 },
	// { TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA, 00, 00, 00 },
	// { TLS_RSA_EXPORT_WITH_RC4_40_MD5,      00, 00, 00 },
	// { TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5,  00, 00, 00 },

	// Plain diffie-helmann not supported
	// { TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA,        00, 00, 00 },
	// { TLS_DHE_DSS_WITH_DES_CBC_SHA,             00, 00, 00 },
	// { TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA,  00, 00, 00 }
};


const unsigned char gsSslRsaOid[9] =
{ 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 };
