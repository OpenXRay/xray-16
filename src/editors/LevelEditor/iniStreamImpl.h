#ifndef inistream_impl_h_included
#define inistream_impl_h_included

#pragma pack(push,1)

struct SIniFileStream :public IIniFileStream
{
	CInifile*		ini;
	shared_str		sect;
	string128		tmp_buff;
	u32				counter;
	LPCSTR			gen_name	();

	virtual void	 __stdcall 	move_begin		()		{counter=0;};

	virtual void	 __stdcall 	w_float			( float a);
	virtual void 	 __stdcall 	w_vec3			( const Fvector& a);
	virtual void 	 __stdcall 	w_vec4			( const Fvector4& a);
	virtual void 	 __stdcall 	w_u64			( u64 a);				
	virtual void 	 __stdcall 	w_s64			( s64 a);				
	virtual void 	 __stdcall 	w_u32			( u32 a);				
	virtual void 	 __stdcall 	w_s32			( s32 a);				
	virtual void 	 __stdcall 	w_u16			( u16 a);				
	virtual void 	__stdcall	w_s16			( s16 a);				
	virtual void	__stdcall	w_u8			( u8 a);				
	virtual void	__stdcall	w_s8			( s8 a);
	virtual void	__stdcall	w_stringZ		( LPCSTR S);
	
	virtual void	__stdcall	r_vec3			(Fvector&);			
	virtual void	__stdcall	r_vec4			(Fvector4&);
	virtual void	__stdcall	r_float			(float&);
	virtual void	__stdcall	r_u8			(u8&);					
	virtual void	__stdcall	r_u16			(u16&);					
	virtual void	__stdcall	r_u32			(u32&);					
	virtual void	__stdcall	r_u64			(u64&);					
	virtual void	__stdcall	r_s8			(s8&);					
	virtual void	__stdcall	r_s16			(s16&);					
	virtual void	__stdcall	r_s32			(s32&);					
	virtual void	__stdcall	r_s64			(s64&);					

	virtual void	__stdcall	r_string		(LPSTR dest, u32 dest_size)	;
	virtual void	__stdcall	skip_stringZ	();
};

#pragma pack (pop)

#endif