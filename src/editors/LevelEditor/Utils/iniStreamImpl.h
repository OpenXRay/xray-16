#ifndef inistream_impl_h_included
#define inistream_impl_h_included

#pragma pack(push, 1)

struct SIniFileStream : public IIniFileStream
{
	CInifile *ini;
	shared_str sect;
	string128 tmp_buff;
	u32 counter;
	LPCSTR gen_name();

	virtual void move_begin() { counter = 0; };

	virtual void w_float(float a);
	virtual void w_vec3(const Fvector &a);
	virtual void w_vec4(const Fvector4 &a);
	virtual void w_u64(u64 a);
	virtual void w_s64(s64 a);
	virtual void w_u32(u32 a);
	virtual void w_s32(s32 a);
	virtual void w_u16(u16 a);
	virtual void w_s16(s16 a);
	virtual void w_u8(u8 a);
	virtual void w_s8(s8 a);
	virtual void w_stringZ(LPCSTR S);

	virtual void r_vec3(Fvector &);
	virtual void r_vec4(Fvector4 &);
	virtual void r_float(float &);
	virtual void r_u8(u8 &);
	virtual void r_u16(u16 &);
	virtual void r_u32(u32 &);
	virtual void r_u64(u64 &);
	virtual void r_s8(s8 &);
	virtual void r_s16(s16 &);
	virtual void r_s32(s32 &);
	virtual void r_s64(s64 &);

	virtual void r_string(LPSTR dest, u32 dest_size);
	virtual void r_string(xr_string &name);
	virtual void skip_stringZ();
};

#pragma pack(pop)

#endif