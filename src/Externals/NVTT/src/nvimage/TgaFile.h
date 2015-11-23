// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_TGAFILE_H
#define NV_IMAGE_TGAFILE_H

#include <nvcore/Stream.h>

namespace nv
{
	
// TGA types
enum TGAType {
    TGA_TYPE_INDEXED		= 1,
    TGA_TYPE_RGB			= 2,
    TGA_TYPE_GREY			= 3,
    TGA_TYPE_RLE_INDEXED	= 9,
    TGA_TYPE_RLE_RGB		= 10,
    TGA_TYPE_RLE_GREY		= 11
};

#define TGA_INTERLEAVE_MASK	0xc0
#define TGA_INTERLEAVE_NONE	0x00
#define TGA_INTERLEAVE_2WAY	0x40
#define TGA_INTERLEAVE_4WAY	0x80

#define TGA_ORIGIN_MASK		0x30
#define TGA_ORIGIN_LEFT		0x00
#define TGA_ORIGIN_RIGHT	0x10
#define TGA_ORIGIN_LOWER	0x00
#define TGA_ORIGIN_UPPER	0x20

#define TGA_HAS_ALPHA		0x0F


/// Tga Header.
struct TgaHeader {
	uint8	id_length;
	uint8	colormap_type;
	uint8	image_type;
	uint16	colormap_index;
	uint16	colormap_length;
	uint8	colormap_size;
	uint16	x_origin;
	uint16	y_origin;
	uint16	width;
	uint16	height;
	uint8	pixel_size;
	uint8	flags;

	enum { Size = 18 };		//const static int SIZE = 18;
};


/// Tga File.
struct TgaFile {

	TgaFile() {
		mem = NULL;
	}
	~TgaFile() {
		free();
	}

	uint size() const {
		return head.width * head.height * (head.pixel_size / 8);
	}
	void allocate() {
		nvCheck( mem == NULL );
		mem = new uint8[size()];
	}
	void free() {
		delete [] mem;
		mem = NULL;
	}

	TgaHeader head;
	uint8 * mem;
};


inline Stream & operator<< (Stream & s, TgaHeader & head)
{
	s << head.id_length << head.colormap_type << head.image_type;
	s << head.colormap_index << head.colormap_length << head.colormap_size;
	s << head.x_origin << head.y_origin << head.width << head.height;
	s << head.pixel_size << head.flags;
	return s;
}

inline Stream & operator<< (Stream & s, TgaFile & tga)
{
	s << tga.head;

	if( s.isLoading() ) {
		tga.allocate();
	}

	s.serialize( tga.mem, tga.size() );

	return s;
}

} // nv namespace

#endif // NV_IMAGE_TGAFILE_H
