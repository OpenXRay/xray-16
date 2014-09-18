#pragma once

template<u32 range>
struct fixed16
{
	s16			_value;

	void		_w			(float a)	{ s32 _v=iFloor(a*32767.f/float(range));	clamp(_v,-32768,32767); _value=_v;	}
	float		_r			()const			{ return float((_value/32767.f)*float(range));	}
};

class  XRLC_LIGHT_API base_color_c
{
public:
	Fvector					rgb;		// - all static lighting
	float					hemi;		// - hemisphere
	float					sun;		// - sun
	float					_tmp_;		// ???
	base_color_c()			{ rgb.set(0,0,0); hemi=0; sun=0; _tmp_=0;	}

	void					mul			(float s)									{	rgb.mul(s);	hemi*=s; sun*=s;				};
	void					add			(float s)									{	rgb.add(s);	hemi+=s; sun+=s;				};
	void					add			(base_color_c& s)							{	rgb.add(s.rgb);	hemi+=s.hemi; sun+=s.sun;	};
	void					scale		(int samples)								{	mul	(1.f/float(samples));					};
	void					max			(base_color_c& s)							{ 	rgb.max(s.rgb); hemi=_max(hemi,s.hemi); sun=_max(sun,s.sun); };
	void					lerp		(base_color_c& A, base_color_c& B, float s)	{ 	rgb.lerp(A.rgb,B.rgb,s); float is=1-s;  hemi=is*A.hemi+s*B.hemi; sun=is*A.sun+s*B.sun; };
};

class XRLC_LIGHT_API base_color
{
public:
	union{
		struct{
			fixed16<16>		r;
			fixed16<16>		g;
			fixed16<16>		b;
			fixed16<16>		h;
			fixed16<16>		s;
			fixed16<16>		t;
		};
		struct{
			float			_x;
			float			_y;
			float			_z;
		};
	};
	void					_set		(float x, float y, float z)	{
		_x					= x;
		_y					= y;
		_z					= z;
	}
	void					_set		(base_color_c& C)	{
		r._w(C.rgb.x);		g._w(C.rgb.y);		b._w(C.rgb.z);
		h._w(C.hemi);		s._w(C.sun);		t._w(C._tmp_);
	}

	void					_set		( Fvector& rgb, float hemi, float sun )	{
		r._w(rgb.x);	g._w(rgb.y);	b._w(rgb.z);
		h._w(hemi);		s._w(sun);		//t._w(_tmp_);
	}
	void					_get		(base_color_c& C)const	{
		C.rgb.x	=r._r();	C.rgb.y	=g._r();	C.rgb.z	=b._r();
		C.hemi	=h._r();	C.sun	=s._r();	C._tmp_	=t._r();
	}
	bool					similar	( const base_color &c, float eps =EPS ) const;

};