#ifndef __C__
#define __C__

// maps unsigned 8 bits/channel to D3DCOLOR
ICF u32	color_argb	(u32 a, u32 r, u32 g, u32 b)	{	return ((a&0xff)<<24)|((r&0xff)<<16)|((g&0xff)<<8)|(b&0xff);	}
ICF u32	color_rgba	(u32 r, u32 g, u32 b, u32 a)	{	return color_argb(a,r,g,b);		}
ICF	u32	color_argb_f(f32 a, f32 r, f32 g, f32 b)	
{
	s32	 _r = clampr(iFloor(r*255.f),0,255);
	s32	 _g = clampr(iFloor(g*255.f),0,255);
	s32	 _b = clampr(iFloor(b*255.f),0,255);
	s32	 _a = clampr(iFloor(a*255.f),0,255);
	return color_argb(_a,_r,_g,_b);
}
ICF u32	color_rgba_f(f32 r, f32 g, f32 b, f32 a)	{	return color_argb_f(a,r,g,b);	}
ICF u32	color_xrgb	(u32 r, u32 g, u32 b)			{	return color_argb(0xff,r,g,b);	}
ICF	u32	color_get_R	(u32 rgba)						{	return (((rgba) >> 16) & 0xff);	}
ICF	u32	color_get_G	(u32 rgba)						{	return (((rgba) >> 8) & 0xff);	}
ICF	u32	color_get_B	(u32 rgba)						{	return ((rgba) & 0xff);			}
ICF	u32 color_get_A (u32 rgba)						{	return ((rgba) >> 24);			}
ICF u32 subst_alpha	(u32 rgba, u32 a)				{	return rgba&~color_rgba(0,0,0,0xff)|color_rgba(0,0,0,a);}
ICF u32 bgr2rgb		(u32 bgr)						{	return color_rgba(color_get_B(bgr),color_get_G(bgr),color_get_R(bgr),0);}
ICF u32 rgb2bgr		(u32 rgb)						{	return bgr2rgb(rgb);}

template <class T>
struct _color {
public:
	typedef _color		Self;
	typedef Self&		SelfRef;
	typedef const Self&	SelfCRef;
public:
	T r,g,b,a;

	ICF	SelfRef	set		(u32 dw)
	{
		const T f = T(1.0) / T(255.0);
		a = f * T((dw >> 24)& 0xff);
		r = f * T((dw >> 16)& 0xff);
		g = f * T((dw >>  8)& 0xff);
		b = f * T((dw >>  0)& 0xff);
		return *this;
	};
	IC	SelfRef	set		(T _r, T _g, T _b, T _a)
	{	
		r = _r; g = _g; b = _b; a = _a;		
		return *this;
	};
	IC	SelfRef	set		(SelfCRef dw)
	{	
		r=dw.r; g=dw.g; b=dw.b; a = dw.a;	
		return *this;
	};
	ICF	u32		get( )	const	{ return color_rgba_f(r,g,b,a); }
	IC	u32		get_windows	( ) const
	{
		BYTE _a, _r, _g, _b;
		_a = (BYTE)(a*255.f);
		_r = (BYTE)(r*255.f);
		_g = (BYTE)(g*255.f);
		_b = (BYTE)(b*255.f);
		return ((u32)(_a<<24) | (_b<<16) | (_g<<8) | (_r));
	};
	IC	SelfRef	set_windows(u32 dw)
	{
		const T f = 1.0f / 255.0f;
		a = f * (T) (BYTE) (dw >> 24);
		b = f * (T) (BYTE) (dw >> 16);
		g = f * (T) (BYTE) (dw >>  8);
		r = f * (T) (BYTE) (dw >>  0);
		return *this;
	};
	IC	SelfRef	adjust_contrast(T f)				// >1 - contrast will be increased
	{
		r = 0.5f + f * (r - 0.5f);
		g = 0.5f + f * (g - 0.5f);
		b = 0.5f + f * (b - 0.5f);
		return *this;
	};
	IC	SelfRef	adjust_contrast(SelfCRef in, T f)	// >1 - contrast will be increased
	{
		r = 0.5f + f * (in.r - 0.5f);
		g = 0.5f + f * (in.g - 0.5f);
		b = 0.5f + f * (in.b - 0.5f);
		return *this;
	};
	IC	SelfRef	adjust_saturation(T s)
	{
		// Approximate values for each component's contribution to luminance.
		// Based upon the NTSC standard described in ITU-R Recommendation BT.709.
		T grey = r * 0.2125f + g * 0.7154f + b * 0.0721f;

		r = grey + s * (r - grey);
		g = grey + s * (g - grey);
		b = grey + s * (b - grey);
		return *this;
	};
	IC	SelfRef	adjust_saturation(SelfCRef in, T s)
	{
		// Approximate values for each component's contribution to luminance.
		// Based upon the NTSC standard described in ITU-R Recommendation BT.709.
		T grey = in.r * 0.2125f + in.g * 0.7154f + in.b * 0.0721f;

		r = grey + s * (in.r - grey);
		g = grey + s * (in.g - grey);
		b = grey + s * (in.b - grey);
		return *this;
	};
	IC	SelfRef	modulate(_color &in)
	{
		r*=in.r;
		g*=in.g;
		b*=in.b;
		a*=in.a;
		return *this;
	};
	IC	SelfRef	modulate(SelfCRef in1, SelfCRef in2)
	{
		r=in1.r*in2.r;
		g=in1.g*in2.g;
		b=in1.b*in2.b;
		a=in1.a*in2.a;
		return *this;
	};
	IC	SelfRef	negative(SelfCRef in)
	{
		r=1.0f-in.r;
		g=1.0f-in.g;
		b=1.0f-in.b;
		a=1.0f-in.a;
		return *this;
	};
	IC	SelfRef	negative(void)
	{
		r=1.0f-r;
		g=1.0f-g;
		b=1.0f-b;
		a=1.0f-a;
		return *this;
	};
	IC	SelfRef	sub_rgb(T s)
	{
		r-=s;
		g-=s;
		b-=s;
//		a=1.0f-a;
		return *this;
	};
	IC	SelfRef	add_rgb(T s)
	{
		r+=s;
		g+=s;
		b+=s;
		return *this;
	};
	IC	SelfRef	add_rgba(T s)
	{
		r+=s;
		g+=s;
		b+=s;
		a+=s;
		return *this;
	};
	IC	SelfRef	mul_rgba(T s) 
	{
		r*=s;
		g*=s;
		b*=s;
		a*=s;
		return *this;
	};
	IC	SelfRef	mul_rgb(T s) 
	{
		r*=s;
		g*=s;
		b*=s;
		return *this;
	};
	IC	SelfRef	mul_rgba(SelfCRef c, T s) 
	{
		r=c.r*s;
		g=c.g*s;
		b=c.b*s;
		a=c.a*s;
		return *this;
	};
	IC	SelfRef	mul_rgb(SelfCRef c, T s) 
	{
		r=c.r*s;
		g=c.g*s;
		b=c.b*s;
		return *this;
	};
	
	// SQ magnitude
	IC	T	magnitude_sqr_rgb(void)		const	{
		return r*r + g*g + b*b;
	}
	// magnitude
	IC	T	magnitude_rgb	(void) 		const	{
		return _sqrt(magnitude_sqr_rgb());
	}
	IC	T	intensity		(void) 		const	{
		return (r + g + b)/3.f;
	}

	// Normalize
	IC	SelfRef	normalize_rgb(void)				{
		VERIFY(magnitude_sqr_rgb()>EPS_S);
		return mul_rgb(1.f/magnitude_rgb());
	}
	IC	SelfRef	normalize_rgb(SelfCRef c) 
	{
		VERIFY(c.magnitude_sqr_rgb()>EPS_S);
		return mul_rgb(c,1.f/c.magnitude_rgb());
	}
	IC	SelfRef	lerp(SelfCRef c1, SelfCRef c2, T t)
	{
		T invt = 1.f-t;
		r = c1.r*invt + c2.r*t;
		g = c1.g*invt + c2.g*t;
		b = c1.b*invt + c2.b*t;
		a = c1.a*invt + c2.a*t;
		return *this;
	}
	IC	SelfRef	lerp(SelfCRef c1, SelfCRef c2, SelfCRef c3, T t)
	{
		if (t>.5f){
			return lerp(c2,c3,t*2.f-1.f);
		}else{
			return lerp(c1,c2,t*2.f);
		}
	}
	IC  BOOL	similar_rgba(SelfCRef v, T E=EPS_L) 	const	{ return _abs(r-v.r)<E && _abs(g-v.g)<E && _abs(b-v.b)<E && _abs(a-v.a)<E;};
	IC  BOOL	similar_rgb	(SelfCRef v, T E=EPS_L) 	const	{ return _abs(r-v.r)<E && _abs(g-v.g)<E && _abs(b-v.b)<E;};
};


typedef _color<float>	Fcolor;
typedef _color<double>	Dcolor;

template <class T>
BOOL	_valid			(const _color<T>& c)	{ return _valid(c.r) && _valid(c.g) && _valid(c.b) && _valid(c.a); }

#endif
