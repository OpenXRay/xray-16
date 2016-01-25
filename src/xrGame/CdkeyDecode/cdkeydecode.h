

#ifdef __cplusplus
extern "C"
{
#endif

/*
DecodeKeyData
-------------
Obtains the "extra" data encoded as part of the key
Up to 16 bytes of data can be encoded
The "key" can include dashes, they will be stripped automatically before decoding
The data will be returned in "extradata" - this should be allocated already, and large enough to handle all the data
Returns the number of extra bytes decoded, or 0 if there was a decoding error
*/
int DecodeKeyData(const char *key, unsigned char *extradata);

/*
VerifyClientCheck
-----------------
Does a "client-side" validation of the CDKey to make sure it appears to be valid
This can be used to check for typos or other key-entry mistakes
It does not determine whether a key is actually valid (i.e. was generated as part of a valid
key batch) - that will require checking against a valid key list on a backend server.
Pass in the key (with or without dashes) and the "cskey" value that was used when generating
the keys. 
Returns 1 if the key appears to be valid, 0 otherwise
*/
int VerifyClientCheck(const char *key, unsigned short cskey);

#ifdef __cplusplus
}
#endif
