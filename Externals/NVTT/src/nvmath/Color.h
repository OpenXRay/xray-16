// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_COLOR_H
#define NV_MATH_COLOR_H

#include <nvcore/Debug.h>
#include <nvmath/Vector.h>

namespace nv
{

/// 64 bit color stored as BGRA.
class NVMATH_CLASS Color64 
{
public:
	Color64() { }
	Color64(const Color64 & c) : u(c.u) { }
	Color64(uint16 R, uint16 G, uint16 B, uint16 A) { setRGBA(R, G, B, A); }
	explicit Color64(uint64 U) : u(U) { }

	void setRGBA(uint16 R, uint16 G, uint16 B, uint16 A)
	{
		r = R;
		g = G;
		b = B;
		a = A;
	}

	operator uint64 () const {
		return u;
	}

	union {
		struct {
#if NV_LITTLE_ENDIAN
			uint16 r, a, b, g;
#else
			uint16 a: 16;
			uint16 r: 16;
			uint16 g: 16;
			uint16 b: 16;
#endif
		};
		uint64 u;
	};
};

/// 32 bit color stored as BGRA.
class NVMATH_CLASS Color32
{
public:
	Color32() { }
	Color32(const Color32 & c) : u(c.u) { }
	Color32(uint8 R, uint8 G, uint8 B) { setRGBA(R, G, B, 0xFF); }
	Color32(uint8 R, uint8 G, uint8 B, uint8 A) { setRGBA( R, G, B, A); }
	//Color32(uint8 c[4]) { setRGBA(c[0], c[1], c[2], c[3]); }
	//Color32(float R, float G, float B) { setRGBA(uint(R*255), uint(G*255), uint(B*255), 0xFF); }
	//Color32(float R, float G, float B, float A) { setRGBA(uint(R*255), uint(G*255), uint(B*255), uint(A*255)); }
	explicit Color32(uint32 U) : u(U) { }

	void setRGBA(uint8 R, uint8 G, uint8 B, uint8 A)
	{
		r = R;
		g = G;
		b = B;
		a = A;
	}

	void setBGRA(uint8 B, uint8 G, uint8 R, uint8 A = 0xFF)
	{
		r = R;
		g = G;
		b = B;
		a = A;
	}
	
	operator uint32 () const {
		return u;
	}
	
	union {
		struct {
#if NV_LITTLE_ENDIAN
			uint8 b, g, r, a;
#else
			uint8 a: 8;
			uint8 r: 8;
			uint8 g: 8;
			uint8 b: 8;
#endif
		};
		uint32 u;
	};
};


/// 16 bit 565 BGR color.
class NVMATH_CLASS Color16
{
public:
	Color16() { }
	Color16(const Color16 & c) : u(c.u) { }
	explicit Color16(uint16 U) : u(U) { }
	
	union {
		struct {
#if NV_LITTLE_ENDIAN
			uint16 b : 5;
			uint16 g : 6;
			uint16 r : 5;
#else
			uint16 r : 5;
			uint16 g : 6;
			uint16 b : 5;
#endif
		};
		uint16 u;
	};
};


/// Clamp color components.
inline Vector3 colorClamp(Vector3::Arg c)
{
	return Vector3(clamp(c.x(), 0.0f, 1.0f), clamp(c.y(), 0.0f, 1.0f), clamp(c.z(), 0.0f, 1.0f));
}

/// Clamp without allowing the hue to change.
inline Vector3 colorNormalize(Vector3::Arg c)
{
	float scale = 1.0f;
	if (c.x() > scale) scale = c.x();
	if (c.y() > scale) scale = c.y();
	if (c.z() > scale) scale = c.z();
	return c / scale;
}

/// Convert Color32 to Color16.
inline Color16 toColor16(Color32 c)
{
	Color16 color;
	//         rrrrrggggggbbbbb
	// rrrrr000gggggg00bbbbb000
//	color.u = (c.u >> 3) & 0x1F;
//	color.u |= (c.u >> 5) & 0x7E0;
//	color.u |= (c.u >> 8) & 0xF800;
	
	color.r = c.r >> 3;
	color.g = c.g >> 2;
	color.b = c.b >> 3;
	return color; 
}


/// Promote 16 bit color to 32 bit using regular bit expansion.
inline Color32 toColor32(Color16 c)
{
	Color32 color;
//	c.u = ((col0.u << 3) & 0xf8) | ((col0.u << 5) & 0xfc00) | ((col0.u << 8) & 0xf80000);
//	c.u |= (c.u >> 5) & 0x070007;
//	c.u |= (c.u >> 6) & 0x000300;
	
	color.b = (c.b << 3) | (c.b >> 2);
	color.g = (c.g << 2) | (c.g >> 4);
	color.r = (c.r << 3) | (c.r >> 2);
	color.a = 0xFF;
	
	return color;
}

inline Vector4 toVector4(Color32 c)
{
	const float scale = 1.0f / 255.0f;
	return Vector4(c.r * scale, c.g * scale, c.b * scale, c.a * scale);
}

} // nv namespace

#endif // NV_MATH_COLOR_H
