///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsCrypt.h"
#include "gsLargeInt.h"
#include "gsSHA1.h"

// **Please refer to gsCrypt.h for public interface functions**


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
 
#ifdef GS_CRYPT_RSA_ES_OAEP
	static gsi_i32 gsCryptRSAOAEPEncryptBuffer(const gsCryptRSAKey *publicKey, const unsigned char *plainText, gsi_u32 len, unsigned char buffer[GS_CRYPT_RSA_BYTE_SIZE]);
	static gsi_i32 gsCryptRSAOAEPDecryptBuffer(const gsCryptRSAKey *privateKey, const unsigned char cipherText[GS_CRYPT_RSA_BYTE_SIZE], unsigned char *plainText, gsi_u32 *lenout);

	static void gsiCryptRSAGenerateSeed(unsigned char *buffer, gsi_u32 len);
	static gsi_bool gsiCryptRSAMaskData(unsigned char *data, gsi_u32 len, const unsigned char *maskSeed, gsi_u32 maskLen);
#else
	static gsi_i32 gsCryptRSAPKCS1EncryptBuffer(const gsCryptRSAKey *publicKey, const unsigned char *plainText, gsi_u32 len, unsigned char buffer[GS_CRYPT_RSA_BYTE_SIZE]);
	static gsi_i32 gsCryptRSAPKCS1DecryptBuffer(const gsCryptRSAKey *privateKey, const unsigned char ciperText[GS_CRYPT_RSA_BYTE_SIZE], unsigned char *plainTextOut, gsi_u32 *lenOut);

	static void gsiCryptRSAGeneratePad(unsigned char *buffer, gsi_u32 len);
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef GS_CRYPT_RSA_ES_OAEP
	// generate a random seed for OAEP
	void gsiCryptRSAGenerateSeed(unsigned char *buffer, gsi_u32 len)
	{
		unsigned int i=0;

		Util_RandSeed(current_time());
		for (i=0; i < len; i++)
		{
			//buffer[i] = 0x0c;
			buffer[i] = (unsigned char)(Util_RandInt(0x00, 0xFF)+1);
			GS_ASSERT(buffer[i] != 0x00);
		}
	}

	// PKCS#1 v2.1, B.2.1 MGF1, mask generation function
	//       modification: generates mask and applies in place
	gsi_bool gsiCryptRSAMaskData(unsigned char *data, gsi_u32 dataLen, const unsigned char* seed, gsi_u32 seedLen)
	{
		int i=0;
		int k=0;
		
		// The datablock may be used as the seed
		unsigned char hashValue[GS_CRYPT_HASHSIZE]; // in integer form, NOT HEXSTRING

		// seed should never be larger than the data block size (but it may be less)
		if (seedLen > GS_CRYPT_RSA_DATABLOCKSIZE)
			return gsi_false;

		for (i=0; (gsi_u32)i<dataLen;i+=GS_CRYPT_HASHSIZE)
		{
			// Perform the hash
			#if (GS_CRYPT_HASHSIZE==GS_CRYPT_MD5_HASHSIZE)
			{
				gsi_u32 temp=0;

				// concatenate byte value of i onto seed
				char seedPlusIter[GS_CRYPT_RSA_DATABLOCKSIZE+4]; // seed||i 
				char hashHexStr[GS_CRYPT_RSA_DATABLOCKSIZE*2+1]; // hexstr of hash "A1" rather than 0xA1
				memcpy(&seedPlusIter[0], seed, seedLen);
				seedPlusIter[GS_CRYPT_RSA_DATABLOCKSIZE+0] = 0x00;
				seedPlusIter[GS_CRYPT_RSA_DATABLOCKSIZE+1] = 0x00;
				seedPlusIter[GS_CRYPT_RSA_DATABLOCKSIZE+2] = 0x00;
				seedPlusIter[GS_CRYPT_RSA_DATABLOCKSIZE+3] = (gsi_u8)(i/GS_CRYPT_HASHSIZE);
				MD5Digest(seedPlusIter, seedLen+sizeof(gsi_u32), hashHexStr);

				// convert from hexstr to integer form
				for(k=0; k<seedLen+sizeof(gsi_u32))
				{
					gsi_u32 temp;
					temp = sscanf(&hashHexStr[k*2], "%02X", &temp);
					hashValue[k] = (gsi_u8)temp;
				}
			}
			#elif (GS_CRYPT_HASHSIZE==GS_CRYPT_SHA1_HASHSIZE)
			{
				gsi_u8 counter[4] = { 0x00,0x00,0x00,0x00 };
				SHA1Context sha;
				counter[3] = (gsi_u8)(i/GS_CRYPT_HASHSIZE); // ensure little endian int
				SHA1Reset(&sha);
				SHA1Input(&sha, (const unsigned char*)seed, seedLen);
				SHA1Input(&sha, counter, 4);
				SHA1Result(&sha, hashValue);
			}
			#endif

			// apply the mask to data
			for (k=0; k<GS_CRYPT_HASHSIZE; k++)
			{
				if ((gsi_u32)(i+k) >= dataLen)
					return gsi_true;

				data[i+k] ^= (gsi_u8)hashValue[k];
			}
		}
		return gsi_true;
	}
#else
	// generate a random pad for PKCS1
	//    [0x01 - 0xFF]
	void gsiCryptRSAGeneratePad(unsigned char *buffer, gsi_u32 len)
	{
		unsigned int i=0;

		Util_RandSeed(current_time());
		for (i=0; i < len; i++)
		{
			#if defined(GS_CRYPT_NO_RANDOM)
            #pragma message("GS_CRYPT_NO_RANDOM defined, SSL is NOT SECURE!!!!\r\n")
			buffer[i] = 0x0c;
			#else
			buffer[i] = (unsigned char)(Util_RandInt(0x00, 0xFF)+1);
			#endif
			GS_ASSERT(buffer[i] != 0x00);
		}
	}
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// RSA Packet = encrypt(lhash, pad, 0x01, plainText)
//    lhash is constant, since labels are not supported
//    pad is variable length, depending on plainText length
gsi_i32 gsCryptRSAEncryptBuffer(const gsCryptRSAKey *publicKey, const unsigned char *plainText, gsi_u32 len, unsigned char buffer[GS_CRYPT_RSA_BYTE_SIZE])
{
	#ifdef GS_CRYPT_RSA_ES_OAEP
		return gsCryptRSAOAEPEncryptBuffer(publicKey, plainText, len, buffer);
	#else
		return gsCryptRSAPKCS1EncryptBuffer(publicKey, plainText, len, buffer);
	#endif
}

#ifdef GS_CRYPT_RSA_ES_OAEP
	gsi_i32 gsCryptRSAOAEPEncryptBuffer(const gsCryptRSAKey *publicKey, const unsigned char *plainText, gsi_u32 len, unsigned char buffer[GS_CRYPT_RSA_BYTE_SIZE])
	{
		gsLargeInt_t lintRSAPacket;
		gsCryptRSAOAEPPacket* packet = (gsCryptRSAOAEPPacket*)lintRSAPacket.mData; 
		
	#if (GS_CRYPT_HASHSIZE==GS_CRYPT_MD5_HASHSIZE) 
		const gsi_u8 lhash[GS_CRYPT_HASHSIZE] = {0xd4,0x1d,0xd4,0x1d,0x8c,0xd9,0x8f,0x00,0xb2,0x04,0xe9,0x80,0x09,0x98,0xec,0xf8,0x42,0x7e}; // hash of ""
	#else
		const gsi_u8 lhash[GS_CRYPT_HASHSIZE] = {0xda,0x39,0xa3,0xee,0x5e,0x6b,0x4b,0x0d,0x32,0x55,0xbf,0xef,0x95,0x60,0x18,0x90,0xaf,0xd8,0x07,0x09}; // hash of ""
	#endif
		const unsigned int maxPlainTextLen = GS_CRYPT_RSA_BYTE_SIZE-2*GS_CRYPT_HASHSIZE-2;
		const unsigned int padSize = maxPlainTextLen-len;
			
		// The steps below are taken from PKCS#1, section 7.1.1 "Encryption Operation"
		// 1. check length
		if (len > maxPlainTextLen)
			return -1;

		// 2. EME-OAEP encoding (pad & pad format)
		//       a. precalculated above (const lhash)

		//       b. create pad
		//       c. concatenate hash+pad+0x01+plainText
		memcpy(packet->maskedData, lhash, GS_CRYPT_HASHSIZE);
		memset(&packet->maskedData[GS_CRYPT_HASHSIZE], 0, padSize); // pad with zero bytes
		packet->maskedData[GS_CRYPT_HASHSIZE+padSize] = 0x01; // RSA encoding format ID (EME-OAEP)
		memcpy(&packet->maskedData[GS_CRYPT_HASHSIZE+padSize+1], plainText, len);

		//       d. generate random seed (seed isn't masked until h.)
		gsiCryptRSAGenerateSeed(packet->maskedSeed, GS_CRYPT_HASHSIZE);

		//       e. use (still unmasked) seed to generate a mask for the datablock
		//       f. apply it with xor
		gsiCryptRSAMaskData(packet->maskedData, GS_CRYPT_RSA_DATABLOCKSIZE, packet->maskedSeed, GS_CRYPT_HASHSIZE);
		
		//       g. use the masked datablock to generate a mask for the seed
		//       h. apply it with xor
		gsiCryptRSAMaskData(packet->maskedSeed, GS_CRYPT_HASHSIZE, packet->maskedData, GS_CRYPT_RSA_DATABLOCKSIZE);

		//       i. set first byte to 0x00
		packet->headerByte = 0x00;

		// 3. Encryptitize
		/*
		lintRSAPacket.mLength = GS_CRYPT_RSA_BYTE_SIZE/sizeof(gsi_u32);
		gsLargeIntReverseBytes(&lintRSAPacket);
		gsLargeIntPowerMod(&lintRSAPacket, &publicKey->exponent, &publicKey->modulus, &lintRSAPacket);
		gsLargeIntReverseBytes(&lintRSAPacket);
		
		// 4. return cipher text
		memcpy(buffer, lintRSAPacket.mData, GS_CRYPT_RSA_BYTE_SIZE);
		*/
		GS_ASSERT(0); // Section above needs revision due to byte order issues

		return 0;
	}
#else
	gsi_i32 gsCryptRSAPKCS1EncryptBuffer(const gsCryptRSAKey *publicKey, const unsigned char *plainText, gsi_u32 len, unsigned char buffer[GS_CRYPT_RSA_BYTE_SIZE])
	{
		gsi_u8 buf[GS_CRYPT_RSA_BYTE_SIZE];
		gsCryptRSAPKCS1Packet* packet = (gsCryptRSAPKCS1Packet*)buf;
		gsLargeInt_t lintRSAPacket;

		if (len > (GS_CRYPT_RSA_BYTE_SIZE-11)) // 2 byte header, 8 byte pad minimum, 1 byte separator
			return -1;

		// form the packet
		packet->headerByte[0] = 0x00;
		packet->headerByte[1] = 0x02;

		gsiCryptRSAGeneratePad(packet->data, GS_CRYPT_RSA_BYTE_SIZE-len-3);
		packet->data[GS_CRYPT_RSA_BYTE_SIZE-len-3] = 0x00; // separator
		memcpy(&packet->data[GS_CRYPT_RSA_BYTE_SIZE-len-3+1], plainText, len);

		if (gsi_is_false(gsLargeIntSetFromMemoryStream(&lintRSAPacket, (const gsi_u8*)buf, GS_CRYPT_RSA_BYTE_SIZE)) ||
			gsi_is_false(gsLargeIntPowerMod(&lintRSAPacket, &publicKey->exponent, &publicKey->modulus, &lintRSAPacket)) ||
			gsi_is_false(gsLargeIntWriteToMemoryStream(&lintRSAPacket, buffer)) )
		{
			return -1;
		}
		
		return 0;
	}
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_i32 gsCryptRSADecryptBuffer(const gsCryptRSAKey *privateKey, const unsigned char cipherText[GS_CRYPT_RSA_BYTE_SIZE], unsigned char *plainText, gsi_u32 *lenout)
{
	#ifdef GS_CRYPT_RSA_ES_OAEP
		return gsCryptRSAOAEPDecryptBuffer(privateKey, cipherText, plainText, lenout);
	#else
		return gsCryptRSAPKCS1DecryptBuffer(privateKey, cipherText, plainText, lenout);
	#endif
}

// Since decryption requires the privatekey, it is usually done by the server.
// Decryption is also much slower than encryption.
// This is included here as a testing utility but should probably not be used in a game client.
#ifndef GS_CRYPT_RSA_ES_OAEP
	static gsi_i32 gsCryptRSAPKCS1DecryptBuffer(const gsCryptRSAKey *privateKey, const unsigned char cipherText[GS_CRYPT_RSA_BYTE_SIZE], unsigned char *plainText, gsi_u32 *lenout)
	{
		int i=0;
		char* temp;
		gsLargeInt_t lintRSAPacket;
		lintRSAPacket.mLength = GS_CRYPT_RSA_BYTE_SIZE/GS_LARGEINT_DIGIT_SIZE_BYTES;
		memcpy(lintRSAPacket.mData, cipherText, GS_CRYPT_RSA_BYTE_SIZE);

		if (gsi_is_false(gsLargeIntReverseBytes(&lintRSAPacket)) || // reverse from bytebuffer to lint format
			gsi_is_false(gsLargeIntPowerMod(&lintRSAPacket, &privateKey->exponent, &privateKey->modulus, &lintRSAPacket)) ||
			gsi_is_false(gsLargeIntReverseBytes(&lintRSAPacket)) // reverse back into a bytebuffer
			)
		{
			return -1;
		}

		// check post exponentiation length
		if (lintRSAPacket.mLength < (GS_CRYPT_RSA_BYTE_SIZE/GS_LARGEINT_DIGIT_SIZE_BYTES))
			return -1;

		// Check the packet for legality
		//   1. first byte must be 0x00
		//   2. send byte must be 0x02
		//   3. pad must be at least 8 bytes and end with 0x00
		//   4. payload must be at least 1 byte
		temp = (char*)lintRSAPacket.mData;
		if (temp[0] != 0x00)
			return -1;
		if (temp[1] != 0x02)
			return -2;

		// find the start of the data (first 0x00 byte after the 1st)
		temp = (char*)lintRSAPacket.mData;
		for (i=2; i<GS_CRYPT_RSA_BYTE_SIZE; i++)
		{
			if (temp[i] == 0)
				break;
		}
		if (i < (2+8)) // 2 byte header, 8 byte minimum pad
			return -3; // pad too small
		if (i == GS_CRYPT_RSA_BYTE_SIZE)
			return -4; // no payload

		// the rest is the msg
		memcpy(plainText, (temp+i+1), GS_CRYPT_RSA_BYTE_SIZE); // +1 to skip the 0x00
		*lenout = (gsi_u32)(GS_CRYPT_RSA_BYTE_SIZE-(i+1)); // +1 to skip the 0x00

		return 0;
	}
#else
	static gsi_i32 gsCryptRSAOAEPDecryptBuffer(const gsCryptRSAKey *privateKey, const unsigned char cipherText[GS_CRYPT_RSA_BYTE_SIZE], unsigned char *plainText, gsi_u32 *lenout)
	{
		int i=0;
		gsLargeInt_t lintRSAPacket;
		gsCryptRSAOAEPPacket* packet = (gsCryptRSAOAEPPacket*)lintRSAPacket.mData;
		
	#if (GS_CRYPT_HASHSIZE==GS_CRYPT_MD5_HASHSIZE) 
		const gsi_u8 lhash[GS_CRYPT_HASHSIZE] = {0xd4,0x1d,0xd4,0x1d,0x8c,0xd9,0x8f,0x00,0xb2,0x04,0xe9,0x80,0x09,0x98,0xec,0xf8,0x42,0x7e}; // hash of ""
	#else
		const gsi_u8 lhash[GS_CRYPT_HASHSIZE] = {0xda,0x39,0xa3,0xee,0x5e,0x6b,0x4b,0x0d,0x32,0x55,0xbf,0xef,0x95,0x60,0x18,0x90,0xaf,0xd8,0x07,0x09}; // hash of ""
	#endif

		lintRSAPacket.mLength = GS_CRYPT_RSA_BYTE_SIZE/4;
		memcpy(lintRSAPacket.mData, cipherText, GS_CRYPT_RSA_BYTE_SIZE);

		gsLargeIntReverseBytes(&lintRSAPacket); // reverse from bytebuffer to lint format
		gsLargeIntPowerMod(&lintRSAPacket, &privateKey->exponent, &privateKey->modulus, &lintRSAPacket);
		gsLargeIntReverseBytes(&lintRSAPacket); // reverse back into a bytebuffer

		// check post exponentiation length
		if (lintRSAPacket.mLength < (GS_CRYPT_RSA_BYTE_SIZE/4))
			return -1;

		// Check the packet for legality
		// 1. "un-mask" the maskedSeed, using the maskedData
		gsiCryptRSAMaskData(packet->maskedSeed, GS_CRYPT_HASHSIZE, packet->maskedData, GS_CRYPT_RSA_DATABLOCKSIZE);
		// 2. "un-mask" the maskedData, using the previously unmasked maskedSeed
		gsiCryptRSAMaskData(packet->maskedData, GS_CRYPT_RSA_DATABLOCKSIZE, packet->maskedSeed, GS_CRYPT_HASHSIZE);
		// 3. datablock = [lhash][0x00...][0x01][M] 
		if (0 != memcmp(packet->maskedData, lhash, GS_CRYPT_HASHSIZE))
			return -2; // label has doesn't match (mismatched hash algorithms?)
		i = 33;
		while(i<GS_CRYPT_RSA_BYTE_SIZE && packet->maskedData[i] == 0x00)
			i++; // may be zero bytes pad
		if (i==GS_CRYPT_RSA_BYTE_SIZE || packet->maskedData[i] != 0x01)
			return -3; // must be a 0x01 following the pad
		i++;
		if (i == GS_CRYPT_RSA_BYTE_SIZE)
			return -4; // founnd the separator, but no message!

		memcpy(plainText, &packet->maskedData[i], (size_t)(GS_CRYPT_RSA_DATABLOCKSIZE-i));
		*lenout = (gsi_u32)(GS_CRYPT_RSA_DATABLOCKSIZE-i); // final length = blocksize - pad
		return 0;
	}
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_i32 gsCryptRSASignData(const gsCryptRSAKey *privateKey, const unsigned char *plainText, gsi_u32 plainTextLen, unsigned char *signedDataOut, gsi_u32 *lenOut)
{
	const unsigned char * hash = NULL;

	GSI_UNUSED(privateKey);
	GSI_UNUSED(plainText);
	GSI_UNUSED(plainTextLen);
	GSI_UNUSED(signedDataOut);
	GSI_UNUSED(lenOut);

	// 1) hash data
	// hash = MD5(plainText);
	//GS_ASSERT(0); // not implemented yet


	// 2) Sign
	return gsCryptRSASignHash(privateKey, hash, GS_CRYPT_MD5_HASHSIZE, signedDataOut, lenOut);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// The more usual use of RSA signatures
//     Constructs a PKCS1 signature form of type  { [0x00][0x01][0xFF..FF][0x00][hash oid][hash] } = RSA key length
gsi_i32 gsCryptRSASignHash(const gsCryptRSAKey *privateKey, const unsigned char *hash, gsi_u32 hashLen, unsigned char *signedDataOut, gsi_u32 *lenOut)
{
	// Encode to PKCS1 signature form
	gsi_u32 aKeyByteLength = privateKey->modulus.mLength * GS_LARGEINT_DIGIT_SIZE_BYTES; // key length in bytes
	gsi_u32 aReservedLength = 3;
	gsi_u32 anOidLen;

	gsLargeInt_t dataToSign;
	char * writeBuf = (char*)dataToSign.mData;

	// Microsoft PKCS #1 headers for various hash algorithms.
	gsi_u8 md5Header[18] =  {0x30,0x20,0x30,0x0C,0x06,0x08,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x02,0x05,0x05,0x00,0x04,0x10};
	gsi_u8 sha1Header[15] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x0E,0x03,0x02,0x1A,0x05,0x00,0x04,0x14};

	if (hashLen == GS_CRYPT_MD5_HASHSIZE)
		anOidLen = sizeof(md5Header);
	else if (hashLen == GS_CRYPT_SHA1_HASHSIZE)
		anOidLen = sizeof(sha1Header);
	else
		return -1; // hash algorithm could not be identified from hashLen

	// Make sure the key is large enough to sign this hash 
	GS_ASSERT(hashLen + anOidLen + aReservedLength <= aKeyByteLength); 
	if (hashLen + anOidLen + aReservedLength > aKeyByteLength)
		return -2; // key is too small or hash is too large

	// fill in header bytes
	writeBuf[0] = 0x00;
	writeBuf[1] = 0x01;

	// pad with 0xFF 
	memset(&writeBuf[2], 0xFF, aKeyByteLength - hashLen - anOidLen - aReservedLength);

	// set a 0x00 at the end of the 0xFF pad
	writeBuf[aKeyByteLength - hashLen - anOidLen - 1] = 0x00;

	// copy in the oid
	if (hashLen == GS_CRYPT_MD5_HASHSIZE)
		memcpy(&writeBuf[aKeyByteLength-hashLen-anOidLen], md5Header, sizeof(md5Header));
	else if (hashLen == GS_CRYPT_SHA1_HASHSIZE)
		memcpy(&writeBuf[aKeyByteLength-hashLen-anOidLen], sha1Header, sizeof(sha1Header));
	else
		return -1; // should probably assert here

	// copy in the hash
	memcpy(&writeBuf[aKeyByteLength-hashLen], hash, hashLen);
	
	// fix byte order for large int
	dataToSign.mLength = privateKey->modulus.mLength;
	gsLargeIntReverseBytes(&dataToSign); 

	// sign (a.k.a. encrypt)
	gsLargeIntPowerMod(&dataToSign, &privateKey->exponent, &privateKey->modulus, &dataToSign);
	
	// length of output data is always the length of the private key's modulus
	GS_ASSERT(dataToSign.mLength == privateKey->modulus.mLength);
	gsLargeIntReverseBytes(&dataToSign); // switch back to rawbuffer byte order
	memcpy(signedDataOut, dataToSign.mData, aKeyByteLength);
	*lenOut = aKeyByteLength;
	
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// "data" is the text that was signed, encrypted or plaintext
// "sig" is the signature value attached to the msg (BIG-endian)
//      Signature format:
//           [ 0x00 0x01 0xFF ... 0x00 HashHeader Hash(Data) ]
gsi_i32 gsCryptRSAVerifySignedHash(const gsCryptRSAKey *publicKey, const unsigned char *hash, gsi_u32 hashLen, const unsigned char *sig, gsi_u32 sigLen)
{
	gsLargeInt_t lintRSASignature;
	gsi_u8* packet = (gsi_u8*)lintRSASignature.mData;
	int i=0;

	// Microsoft PKCS #1 headers for various hash algorithms.
	gsi_u8 md5Header[18] =  {0x30,0x20,0x30,0x0C,0x06,0x08,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x02,0x05,0x05,0x00,0x04,0x10};
	gsi_u8 sha1Header[15] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x0E,0x03,0x02,0x1A,0x05,0x00,0x04,0x14};

	// check parameters for common errors
	if (hash==NULL || sig==NULL)
		return -1;
	if (sigLen != publicKey->modulus.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES)
		return -1;
	if (hashLen != GS_CRYPT_MD5_HASHSIZE && hashLen != GS_CRYPT_SHA1_HASHSIZE)
		return -1; // invalid hashsize

	// "decrypt" the signature
	lintRSASignature.mLength = (l_word)(sigLen / GS_LARGEINT_DIGIT_SIZE_BYTES);
	memcpy(lintRSASignature.mData, sig, sigLen);
	gsLargeIntReverseBytes(&lintRSASignature);
	gsLargeIntPowerMod(&lintRSASignature, &publicKey->exponent, &publicKey->modulus, &lintRSASignature);
	gsLargeIntReverseBytes(&lintRSASignature);

	// Check format, first by 0x00, second byte 0x01
	if (packet[0] != 0x00 || packet[1] != 0x01)
		return -2;

	// Loop through the 0xFF's
	for (i=2; i<GS_CRYPT_RSA_BYTE_SIZE; i++)
	{
		if (packet[i] == 0x00)
			break;
		if (packet[i] != 0xFF)
			return -3;
	}
	i++; // skip the 0x00 seperator byte

	// Next should be the hash header (but we don't know which one!)
	if ((i+sizeof(md5Header)+GS_CRYPT_MD5_HASHSIZE) == (lintRSASignature.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES))
	{
		// MD5 Hash
		//   1. verify header
		if (0 != memcmp(md5Header, &packet[i], sizeof(md5Header)))
			return -4;
		i += sizeof(md5Header);
		//   2. compare hashes
		if (hashLen != GS_CRYPT_MD5_HASHSIZE)
			return -5;
		if (0 != memcmp(&packet[i], hash, GS_CRYPT_MD5_HASHSIZE))
			return -5;
	}
	else if ((i+sizeof(sha1Header)+GS_CRYPT_SHA1_HASHSIZE) == (lintRSASignature.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES))
	{
		// SHA1 Hash
		//   1. verify header
		if (0 != memcmp(sha1Header, &packet[i], sizeof(sha1Header)))
			return -4;
		i += sizeof(sha1Header);
		//   2. compare hashes
		if (hashLen != GS_CRYPT_SHA1_HASHSIZE)
			return -5;
		if (0 != memcmp(&packet[i], hash, GS_CRYPT_SHA1_HASHSIZE))
			return -5;
	}
	else
		return -4; // unsupported hash
	
	// Signature valid!
	return 0;
}
