#include "stdafx.h"
#pragma hdrstop
#include "NET_utils.h"


// ---NET_Packet
// reading
void NET_Packet::read_start()	{	r_pos		= 0; INI_W(move_begin());}

u32 NET_Packet::r_begin			( u16& type	)	// returns time of receiving
{
	r_pos		= 0;
	if(!inistream)
		r_u16		(type);
	else
		inistream->r_u16(type);

	return		timeReceive;
}

void NET_Packet::w_seek	(u32 pos, const void* p, u32 count)	
{
	VERIFY(p && count && (pos+count<=B.count)); 
	CopyMemory(&B.data[pos],p,count);	
//.	INI_ASSERT		(w_seek)
}

void NET_Packet::r_seek	(u32 pos)
{
	INI_ASSERT		(r_seek)
	VERIFY			(pos < B.count);
	r_pos			= pos;
}

u32 NET_Packet::r_tell()	
{ 
	INI_ASSERT		(r_tell)
	return			r_pos; 
}

BOOL NET_Packet::r_eof()
{
	INI_ASSERT		(r_eof)
	return			(r_pos>=B.count);
}

u32 NET_Packet::r_elapsed()
{
	INI_ASSERT		(r_elapsed)
	return			(B.count-r_pos);
}

void NET_Packet::r_advance(u32 size)		
{
	INI_ASSERT		(r_advance)
	r_pos			+= size;	
	VERIFY			(r_pos<=B.count);
}

// reading - utilities
void NET_Packet::r_vec3(Fvector& A)	
{ 
	if(!inistream)
		r(&A,sizeof(Fvector));
	else
		inistream->r_vec3(A);
} // vec3

void NET_Packet::r_vec4(Fvector4& A)	
{ 
	if(!inistream)
		r(&A,sizeof(Fvector4));
	else
		inistream->r_vec4(A);
} // vec4

void NET_Packet::r_float(float& A )		
{ 
	if(!inistream)
		r	(&A,sizeof(float));						
	else
		inistream->r_float(A);
} // float

void NET_Packet::r_u64(u64& A)		
{ 
	if(!inistream)
		r(&A,sizeof(u64));
	else
		inistream->r_u64(A);
} // qword (8b)

void NET_Packet::r_s64(s64& A)		
{ 
	if(!inistream)
		r(&A,sizeof(s64));						
	else
		inistream->r_s64(A);
} // qword (8b)

void NET_Packet::r_u32(u32& A)		
{ 
	if(!inistream)
		r(&A,sizeof(u32));						
	else
		inistream->r_u32(A);
} // dword (4b)

void NET_Packet::r_s32(s32& A)		
{ 
	if(!inistream)
		r(&A,sizeof(s32));
	else
		inistream->r_s32(A);
} // dword (4b)

void NET_Packet::r_u16(u16& A)		
{ 
	if(!inistream)
		r(&A,sizeof(u16));						
	else
		inistream->r_u16(A);
} // word (2b)

void NET_Packet::r_s16(s16& A)		
{ 
	if(!inistream)
		r(&A,sizeof(s16));
	else
		inistream->r_s16(A);
} // word (2b)

void NET_Packet::r_u8(u8&  A)		
{ 
	if(!inistream)
		r(&A,sizeof(u8));
	else
		inistream->r_u8(A);
} // byte (1b)

void NET_Packet::r_s8(s8&  A)		
{ 
	if(!inistream)
		r(&A,sizeof(s8));
	else
		inistream->r_s8(A);
} // byte (1b)

// IReader compatibility
Fvector NET_Packet::r_vec3()		
{
	Fvector		A;
	r_vec3		(A);	
	return		(A);		
}

Fvector4 NET_Packet::r_vec4()
{
	Fvector4	A;
	r_vec4		(A);	
	return		(A);		
}

float NET_Packet::r_float_q8(float min,float max)
{
	float		A;
	r_float_q8	(A,min,max);
	return		A;
}

float NET_Packet::r_float_q16(float min, float max)
{
	float		A;
	r_float_q16	(A,min,max);
	return		A;
}

float NET_Packet::r_float()		
{ 
	float		A; 
	r_float		(A);					
	return		(A);		
} // float

u64 NET_Packet::r_u64()		
{ 
	u64 		A; 
	r_u64		(A);					
	return		(A);		
} // qword (8b)

s64 NET_Packet::r_s64()		
{ 
	s64 		A; 
	r_s64		(A);					
	return		(A);		
} // qword (8b)

u32 NET_Packet::r_u32()		
{ 
	u32 		A; 
	r_u32		(A);					
	return		(A);		
} // dword (4b)

s32 NET_Packet::r_s32()		
{ 
	s32			A; 
	r_s32		(A);					
	return		(A);		
} // dword (4b)

u16 NET_Packet::r_u16()		
{ 
	u16			A; 
	r_u16		(A);					
	return		(A);		
} // word (2b)

s16 NET_Packet::r_s16()		
{ 
	s16			A; 
	r_s16		(A);					
	return		(A);		
} // word (2b)

u8 NET_Packet::r_u8()		
{ 
	u8			A; 
	r_u8		(A);					
	return		(A);		
} // byte (1b)

s8 NET_Packet::r_s8()		
{ 
	s8			A; 
	r_s8		(A);					
	return		(A);		
}

void NET_Packet::r_float_q16(float& A, float min, float max)
{
	u16			val;
	r_u16		(val);
	A			= (float(val)*(max-min))/65535.f + min;		// floating-point-error possible
	VERIFY		((A >= min-EPS_S) && (A <= max+EPS_S));
}

void NET_Packet::r_float_q8(float& A, float min, float max)
{
	u8			val;
	r_u8		(val);
	A			= (float(val)/255.0001f) *(max-min) + min;	// floating-point-error possible
	VERIFY		((A >= min) && (A <= max));
}

void NET_Packet::r_angle16(float& A)		
{ 
	r_float_q16	(A,0,PI_MUL_2);	
}

void NET_Packet::r_angle8(float& A)		
{ 
	r_float_q8	(A,0,PI_MUL_2);	
}

void NET_Packet::r_dir(Fvector& A)	
{ 
	u16			t; 
	r_u16		(t); 
	pvDecompress(A,t); 
}

void NET_Packet::r_sdir(Fvector& A)
{
	u16				t;	
	float			s;
	r_u16			(t);
	r_float			(s);
	pvDecompress	(A,t);
	A.mul			(s);
}

void NET_Packet::r_stringZ( LPSTR S )
{
 	if(!inistream)
	{
		LPCSTR	data	= LPCSTR(&B.data[r_pos]);
		size_t	len		= xr_strlen(data);
		r				(S,(u32)len+1);
	}else{
		inistream->r_string(S, 4096);//???
	}
}

void NET_Packet::r_stringZ( xr_string& dest )
{
 	if(!inistream)
	{
		dest			= LPCSTR(&B.data[r_pos]);
		r_advance		(u32(dest.size()+1));
	}else{
		string4096		buff;
		inistream->r_string(buff, sizeof(buff));
		dest			= buff;
	}
}

void NET_Packet::r_stringZ(shared_str& dest)
{
 	if(!inistream)
	{
		dest			= LPCSTR(&B.data[r_pos]);
		r_advance		(dest.size()+1);
	}else{
		string4096		buff;
		inistream->r_string(buff, sizeof(buff));
		dest			= buff;
	}
}

void NET_Packet::skip_stringZ()
{
	if (!inistream)
	{
		LPCSTR	data	= LPCSTR(&B.data[r_pos]);
		u32	len		= xr_strlen(data);
		r_advance		(len + 1);
	} else {
		inistream->skip_stringZ	();
	}
}

void NET_Packet::r_matrix(Fmatrix& M)
{
	r_vec3	(M.i);	M._14_	= 0;
	r_vec3	(M.j);	M._24_	= 0;
	r_vec3	(M.k);	M._34_	= 0;
	r_vec3	(M.c);	M._44_	= 1;
}

void NET_Packet::r_clientID(ClientID& C)
{
	u32				tmp;
	r_u32			(tmp);
	C.set			(tmp);
}

void NET_Packet::r_stringZ_s	(LPSTR string, u32 const size)
{
	if ( inistream ) {
		inistream->r_string( string, size );
		return;
	}

	LPCSTR data		= LPCSTR( B.data + r_pos );
	u32 length		= xr_strlen( data );
	R_ASSERT2		( ( length+1 ) <= size, "buffer overrun" );
	r				( string, length+1 );
}