#include <stdarg.h>
#include <string.h>
#include "gt2Encode.h"
#include "gt2Main.h"



// This handles alignment issues and endianess
void gt2MemCopy16(char *out, char const *in)
{
	#ifdef _GT2_ENDIAN_CONVERT
		*out   = in[1];
		out[1] = *in;
	#else
	// straight copy
		*out   = *in;
		out[1] = in[1];
	#endif
}

// This handles alignment issues and endianess
void gt2MemCopy32(char *out, char const *in)
{
	#ifdef _GT2_ENDIAN_CONVERT
		out[0] = in[3];
		out[1] = in[2];
		out[2] = in[1];
		out[3] = in[0];
	#else
	// straight copy
		*out   = *in;
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
	#endif
}

// This handles alignment issues and endianess
void gt2MemCopy64(char *out, char const *in)
{		
	#ifdef _GT2_ENDIAN_CONVERT
		out[0] = in[7];
		out[1] = in[6];
		out[2] = in[5];
		out[3] = in[4];
		out[4] = in[3];
		out[5] = in[2];
		out[6] = in[1];
		out[7] = in[0];
	#else
		// straight copy
		memcpy(out, in, 8);
	#endif

}
void gt2MemCopy(char *out, char const *in, int size)
{
	if (size == 2)
	{
		gt2MemCopy16(out, in);
	}
	else
	if (size == 4)
	{
		gt2MemCopy32(out, in);
	}
	else
	if (size == 8)
	{
		gt2MemCopy64(out, in);
	}
	else
	{
		// warning... no endianess decode.
		memcpy(out,in,(size_t)size);
	}
}
#if defined(_PS2) || defined(_UNIX) || defined(_PS3) || defined(_WIN64) || defined(_X360)

#define GT_ENCODE_ELEM(TYPE,b,l,args) \
{ \
	TYPE v; \
	if (l < sizeof(TYPE)) \
		return -1; \
	v = (TYPE)va_arg(*args, int); \
	gt2MemCopy(b, (const char *)&v, sizeof(TYPE)); \
	return (int)sizeof(TYPE); \
}

#define GT_DECODE_ELEM(TYPE,b,l,args) \
{ \
	TYPE* v; \
	if (l < sizeof(TYPE)) \
		return -1; \
	v = va_arg(*args, TYPE*); \
	gt2MemCopy((char *)v, b, sizeof(TYPE)); \
	return (int)sizeof(TYPE); \
}

#define GT_ENCODE_ELEM_NC(TYPE,b,l,args) \
{ \
	TYPE v; \
	if (l < sizeof(TYPE)) \
		return -1; \
	v = (TYPE)va_arg(*args, int); \
	memcpy(b, &v, sizeof(TYPE)); \
	return (int)sizeof(TYPE); \
}

#define GT_DECODE_ELEM_NC(TYPE,b,l,args) \
{ \
	TYPE* v; \
	if (l < sizeof(TYPE)) \
		return -1; \
	v = va_arg(*args, TYPE*); \
	memcpy(v, b, sizeof(TYPE)); \
	return (int)sizeof(TYPE); \
}

#else

#define GT_ENCODE_ELEM(TYPE,b,l,args) {if (l < sizeof(TYPE)) return -1; gt2MemCopy(b,(const char *)&va_arg(*args,TYPE),sizeof(TYPE)); return sizeof(TYPE);}
#define GT_DECODE_ELEM(TYPE,b,l,args) {if (l < sizeof(TYPE)) return -1; gt2MemCopy((char *)va_arg(*args,TYPE*),b,sizeof(TYPE)); return sizeof(TYPE);}
// nc = no endian convert
#define GT_ENCODE_ELEM_NC(TYPE,b,l,args) {if (l < sizeof(TYPE)) return -1; memcpy(b,&va_arg(*args,TYPE),sizeof(TYPE)); return sizeof(TYPE);}
#define GT_DECODE_ELEM_NC(TYPE,b,l,args) {if (l < sizeof(TYPE)) return -1; memcpy(va_arg(*args,TYPE*),b,sizeof(TYPE)); return sizeof(TYPE);}

#endif /* _PS || _UNIX */

static int dbstrlen(GT_DBSTR_TYPE dbstr)
{
	int len = 0;
#ifdef ALIGNED_COPY
	short achar;
	do
	{
		memcpy(&achar, dbstr, sizeof(achar));
		dbstr++;
		len++;
	} while (achar != 0);
	len--;
#else
	while (*dbstr++)
		len++;
#endif
	return len;
}

static short *dbstrcpy(GT_DBSTR_TYPE dest, GT_DBSTR_TYPE src)
{
	GT_DBSTR_TYPE hold = dest;
	#ifdef ALIGNED_COPY
		int len = dbstrlen(src);
		memcpy(dest, src, (unsigned int)(len + 1) * 2);
	#else
		while ((*dest++ = *src++) != 0) ;
	#endif
	return hold;
}

static int gtiDecodeBits(int bitcount, char *inBuffer, int inLength, va_list *args)
{
	char bucket;
	int i;

	if (inLength < 1)
		return -1;
	bucket = *inBuffer;
	for (i = 0 ; i < bitcount ; i++)
	{
		*va_arg(*args,char*) = (char)((bucket & (1 << i)) ? 1 : 0);
	}
	
	return 1;	
}

static int gtiEncodeBits(int bitcount, char *outBuffer, int outLength, va_list *args)
{
	char bucket = 0;
	int i;
	
	if (outLength < 1)
		return -1;
	for (i = 0 ; i < bitcount ; i++)
	{
		bucket |= (char)((va_arg(*args,int) ? 1 : 0) << i);
		//bucket |= ((va_arg(*args,char) ? 1 : 0) << i);
	}
	*outBuffer = bucket;
	return 1;
}

// length in bytes including NUL, or -1 if error
static int gtiCheckStringLen(char *inBuffer, int inLength)
{
	int len = 0;
	do
	{
		len++;
		if(len > inLength)
			return -1;
	}
	while(inBuffer[len - 1] != '\0');
	return len;
}

// length in bytes (not chars) including NUL, or -1 if error
static int gtiCheckDoubleStringLen(char *inBuffer, int inLength)
{
	int len = 0;
	do
	{
		len += 2;
		if(len > inLength)
			return -1;
	}
	while((inBuffer[len - 2] != '\0') || (inBuffer[len - 1] != '\0'));
	return len;
}

// length in bytes including NULs, or -1 if error
static int gtiCheckStringArrayLen(char *inBuffer, int inLength)
{
	int len = 0;
	int strLen;
	do
	{
		strLen = gtiCheckStringLen(inBuffer + len, inLength - len);
		if(strLen == -1)
			return -1;
		len += strLen;
	}
	while(strLen > 1);
	return len;
}

static int gtiDecodeSingle(char elemType, char *inBuffer, int inLength, va_list *args)
{
	switch (elemType)
	{
	case GT_INT:
		GT_DECODE_ELEM(GT_INT_TYPE,inBuffer, inLength, args);
		//break;
	case GT_UINT:
		GT_DECODE_ELEM(GT_UINT_TYPE,inBuffer, inLength, args);
		//break;
	case GT_SHORT:
		GT_DECODE_ELEM(GT_SHORT_TYPE,inBuffer, inLength, args);
		//break;
	case GT_USHORT:
		GT_DECODE_ELEM(GT_USHORT_TYPE,inBuffer, inLength, args);
		//break;
	case GT_CHAR:
		GT_DECODE_ELEM(GT_CHAR_TYPE,inBuffer, inLength, args);
		//break;
	case GT_UCHAR:
		GT_DECODE_ELEM(GT_UCHAR_TYPE,inBuffer, inLength, args);
		//break;
	case GT_FLOAT: 
		{
		#if(0)
			// no endian convert
				GT_FLOAT_TYPE* v; 
				if (inLength < sizeof(GT_FLOAT_TYPE)) 
					return -1; 
				v = va_arg(*args, GT_FLOAT_TYPE*); 
				v[0] = inBuffer[0];
				v[1] = inBuffer[1];
				v[2] = inBuffer[2];
				v[3] = inBuffer[3];
				return (int)sizeof(GT_FLOAT_TYPE); 
		#else
			GT_DECODE_ELEM_NC(GT_FLOAT_TYPE,inBuffer, inLength, args);
		#endif
		}
	case GT_DOUBLE:
		#if(0)
			// no endian convert
		{		GT_DOUBLE_TYPE* v; 
				if (inLength < sizeof(GT_DOUBLE_TYPE)) 
					return -1; 
				v = va_arg(*args, GT_DOUBLE_TYPE*); 
				v[0] = inBuffer[0];
				v[1] = inBuffer[1];
				v[2] = inBuffer[2];
				v[3] = inBuffer[3];
				v[4] = inBuffer[4];
				v[5] = inBuffer[5];
				v[6] = inBuffer[6];
				v[7] = inBuffer[7];
				return (int)sizeof(GT_DOUBLE_TYPE); 
		}
		#else
			GT_DECODE_ELEM_NC(GT_DOUBLE_TYPE,inBuffer, inLength, args);
		#endif
		//break;
	case GT_BIT:
		GT_DECODE_ELEM(GT_BIT_TYPE,inBuffer, inLength, args);
		//break;
	case GT_CSTR:
		{
			int len;
			GT_CSTR_TYPE s = va_arg(*args, GT_CSTR_TYPE);
			assert(s != NULL);
			len = gtiCheckStringLen(inBuffer, inLength);
			if(len == -1)
				return -1;
			memcpy(s, inBuffer, (size_t)len);
			return len;
		}
		//break;
	case GT_CSTR_PTR:
		*va_arg(*args, GT_CSTR_PTR_TYPE) = (GT_CSTR_TYPE)inBuffer;
		return gtiCheckStringLen(inBuffer, inLength);
		//break;
	case GT_DBSTR:
		{	
			int len;
			GT_DBSTR_TYPE s = va_arg(*args, GT_DBSTR_TYPE);
			assert(s != NULL);
			len = gtiCheckDoubleStringLen(inBuffer, inLength);
			if (len == -1)
				return -1;
			memcpy(s, inBuffer, (size_t)len);
			return len;
		}
		//break;
	case GT_DBSTR_PTR:
		*va_arg(*args, GT_DBSTR_PTR_TYPE) = (GT_DBSTR_TYPE)inBuffer;
		return gtiCheckDoubleStringLen(inBuffer, inLength);
		//break;
	case GT_CSTR_ARRAY:
		{
			int len;
			GT_CSTR_ARRAY_TYPE s = va_arg(*args, GT_CSTR_ARRAY_TYPE);
			assert(s != NULL);
			len = gtiCheckStringArrayLen(inBuffer, inLength);
			if(len == -1)
				return -1;
			memcpy(s, inBuffer, (size_t)len);
			return len;
		}
		//break;
	case GT_CSTR_ARRAY_PTR:
		*va_arg(*args, GT_CSTR_ARRAY_PTR_TYPE) = (GT_CSTR_ARRAY_TYPE)inBuffer;
		return gtiCheckStringArrayLen(inBuffer, inLength);
		//break;
	case GT_RAW:
		{
			int *len, holdlen;
			GT_RAW_TYPE data = va_arg(*args, GT_RAW_TYPE);
			len = va_arg(*args, int *);
			if (inLength < sizeof(*len))
				return -1;
			holdlen = *len;
			memcpy(len, inBuffer, sizeof(*len));
			if (*len > holdlen) //there isn't enough room in their dest!
				return -1;
			if (inLength < (int)sizeof(*len) + *len)
				return -1;
			memcpy(data, inBuffer + sizeof(*len), (unsigned int)*len);
			return *len + (int)sizeof(*len);
		}
	case GT_RAW_PTR:
		{
			int *len;
			*va_arg(*args, GT_RAW_PTR_TYPE) = (GT_RAW_TYPE)(inBuffer + sizeof(*len));
			len = va_arg(*args, int *);
			if (inLength < sizeof(*len))
				return -1;
			memcpy(len, inBuffer, sizeof(*len));
			return *len + (int)sizeof(*len);
		}
		//break;
		
	}
	return -1; //bad type!
}

static int gtiEncodeSingle(char elemType, char *outBuffer, int outLength, va_list *args)
{
	switch (elemType)
	{
	case GT_INT:
		GT_ENCODE_ELEM(GT_INT_TYPE,outBuffer, outLength, args);
		//break;
	case GT_UINT:
		GT_ENCODE_ELEM(GT_UINT_TYPE,outBuffer, outLength, args);
		//break;
	case GT_SHORT:
		GT_ENCODE_ELEM(GT_SHORT_TYPE,outBuffer, outLength, args);
		//break;
	case GT_USHORT:
		GT_ENCODE_ELEM(GT_USHORT_TYPE,outBuffer, outLength, args);
		//break;
	case GT_CHAR:
		GT_ENCODE_ELEM(GT_CHAR_TYPE,outBuffer, outLength, args);
		//break;
	case GT_UCHAR:
		GT_ENCODE_ELEM(GT_UCHAR_TYPE,outBuffer, outLength, args);
		//break;
	case GT_FLOAT: //floats are promoted to double in varargs, need to demote
		{
			double temp;
			float f;
   			double v = va_arg(*args,double);
			memcpy(&temp,&v,sizeof(double));
			f = (float)temp;
			if (outLength < sizeof(float))
				return -1;
			memcpy(outBuffer, &f, sizeof(float));
			return sizeof(float);			
		}
		//break;
	case GT_DOUBLE:
		{
			double v;
			if(outLength < sizeof(double))
				return -1;
			v = va_arg(*args, double);
			memcpy(outBuffer, &v, sizeof(double));
			return sizeof(double);
		}
		//break;
	case GT_BIT:
		GT_ENCODE_ELEM(GT_BIT_TYPE,outBuffer, outLength, args);
		//break;
	case GT_CSTR:
	case GT_CSTR_PTR:
		{
			int len;
			GT_CSTR_TYPE s = va_arg(*args, GT_CSTR_TYPE);
			assert(s != NULL);
			len = (int)strlen(s) + 1;
			if (outLength < len )
				return -1;
			strcpy(outBuffer, s);
			return len;
		}
		//break;
	case GT_DBSTR:
	case GT_DBSTR_PTR:
		{	
			int len;
			GT_DBSTR_TYPE s = va_arg(*args, GT_DBSTR_TYPE);
			assert(s != NULL);
			len = dbstrlen(s) + 1;
			if (outLength < len * 2)
				return -1;
			dbstrcpy((short *)outBuffer, s);
			return len * 2;
		}
		//break;
	case GT_CSTR_ARRAY:
	case GT_CSTR_ARRAY_PTR:
		{
			int len = 0;
			int strLen;
			GT_CSTR_ARRAY_TYPE s = va_arg(*args, GT_CSTR_ARRAY_TYPE);
			assert(s != NULL);
			do
			{
				strLen = (int)strlen(s + len) + 1;
				len += strLen;
				if(outLength < len)
					return -1;
			}
			while(strLen != 1);
			memcpy(outBuffer, s, (size_t)len);
			return len;
		}
		//break;
	case GT_RAW:
	case GT_RAW_PTR:
		{
			int len;
			GT_RAW_TYPE data = va_arg(*args, GT_RAW_TYPE);
			len = va_arg(*args, int);
			if (outLength < len + (int)sizeof(len))
				return -1;
			memcpy(outBuffer, &len, sizeof(len));
			memcpy(outBuffer + sizeof(len), data, (unsigned int)len);
			return len + (int)sizeof(len);
		}
		
	}
	return -1; //bad type!
}

static int gtInternalEncodeV(int usetype, GTMessageType msgType, const char *fmtString, char *outBuffer, int outLength, va_list *args)
{
	int elemSize;
	int totSize = outLength;
	const char *bitCounter;

	//set the message type
	if (usetype)
	{
		elemSize = sizeof(msgType);
		if (outLength < elemSize)
			return -1;

		gt2MemCopy(outBuffer, (const char *)&msgType, elemSize);
		outBuffer += elemSize;
		outLength -= elemSize;
	}
	while (*fmtString)
	{
		if (*fmtString == GT_BIT) //see how many
		{
			for (bitCounter = fmtString; *bitCounter == GT_BIT && bitCounter - fmtString <= 8; bitCounter++)  
				{};
			elemSize = gtiEncodeBits((int)(bitCounter - fmtString), outBuffer, outLength, args);
			fmtString = bitCounter - 1;
		} else
			elemSize = gtiEncodeSingle(*fmtString, outBuffer, outLength, args);
		if (elemSize < 0)
			return -1; //out of space
		outBuffer += elemSize;
		outLength -= elemSize;
		fmtString++;
	}
   	return totSize - outLength;
}

int gtEncodeNoTypeV(const char *fmtString, char *outBuffer, int outLength, va_list *args)
{
	return gtInternalEncodeV(0,0,fmtString, outBuffer, outLength, args);
}

int gtEncodeV(GTMessageType msgType, const char *fmtString, char *outBuffer, int outLength, va_list *args)
{
	return gtInternalEncodeV(1,msgType,fmtString, outBuffer, outLength, args);
}

int gtEncode(GTMessageType msgType, const char *fmtString, char *outBuffer, int outLength, ...)
{
	int rcode;
	va_list args;

	//set the values
	va_start(args, outLength);
	rcode = gtEncodeV(msgType, fmtString, outBuffer, outLength, &args);
	va_end(args);

	return rcode;
}

int gtEncodeNoType(const char *fmtString, char *outBuffer, int outLength, ...)
{
	int rcode;
	va_list args;
	
	//set the values
	va_start(args, outLength);
	rcode = gtEncodeNoTypeV(fmtString, outBuffer, outLength, &args);
	va_end(args);

	return rcode;
}

static int gtDecodeInternalV(int usetype, const char *fmtString, char *inBuffer, int inLength, 	va_list *args)
{
	int elemSize;
	int totSize = inLength;
	const char *bitCounter;

	//skip the message type
	if (usetype)
	{
		inBuffer += sizeof(GTMessageType);
		inLength -= sizeof(GTMessageType);
	}

	while (*fmtString)
	{
		if (*fmtString == GT_BIT) //see how many
		{
			for (bitCounter = fmtString; *bitCounter == GT_BIT && bitCounter - fmtString <= 8; bitCounter++)
				{};
			elemSize = gtiDecodeBits((int)(bitCounter - fmtString), inBuffer, inLength, args);
			fmtString = bitCounter - 1;
		} else
			elemSize = gtiDecodeSingle(*fmtString, inBuffer, inLength, args);
		if (elemSize < 0)
			return -1; //out of space
		inBuffer += elemSize;
		inLength -= elemSize;
		fmtString++;
	}
	//NOTE: inLength should be 0 here if we "ate" the whole message
	//If it's not 0, then the encoding and decoding strings probably did not match
	//which would generally indicate a bug
	//PANTS - commented out because we could be decoding the rest with a gtDecodeNoType
//	assert(inLength == 0);
	return totSize - inLength;
}

int gtDecodeV(const char *fmtString, char *inBuffer, int inLength, 	va_list *args)
{
	return gtDecodeInternalV(1,fmtString, inBuffer, inLength, args);	
}

int gtDecodeNoTypeV(const char *fmtString, char *inBuffer, int inLength, 	va_list *args)
{
	return gtDecodeInternalV(0,fmtString, inBuffer, inLength, args);
}

int gtDecode(const char *fmtString, char *inBuffer, int inLength, ...)
{
	int rcode;
	va_list args;
	
	//set the values
	va_start(args, inLength);
	rcode = gtDecodeV(fmtString, inBuffer, inLength, &args);
	va_end(args);

	return rcode;
}

int gtDecodeNoType(const char *fmtString, char *inBuffer, int inLength, ...)
{
	int rcode;
	va_list args;
	
	//set the values
	va_start(args, inLength);
	rcode = gtDecodeNoTypeV(fmtString, inBuffer, inLength, &args);
	va_end(args);

	return rcode;
}

GTMessageType gtEncodedMessageType(char *inBuffer)
{
	GTMessageType type;
	//GS_ASSERT(sizeof(GTMessageType) ==2 )
	gt2MemCopy16((char *)&type, inBuffer);
	return type;
}

// change the message type for an encoded message
void	gtEncodedMessageTypeSet	(char *inBuffer, GTMessageType newtype)
{
	gt2MemCopy16(inBuffer, (char *)&newtype);
}

