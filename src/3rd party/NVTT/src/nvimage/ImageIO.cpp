// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/Ptr.h>
#include <nvcore/Containers.h>
#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>
//#include <nvcore/Tokenizer.h>	// @@ Disable temporarily
#include <nvcore/TextWriter.h>

#include <nvmath/Color.h>

#include "ImageIO.h"
#include "Image.h"
#include "FloatImage.h"
#include "TgaFile.h"
#include "PsdFile.h"

// Extern
#if defined(HAVE_JPEG)
extern "C" {
#	include <jpeglib.h>
}
#endif

#if defined(HAVE_PNG)
#	include <png.h>
#endif

#if defined(HAVE_TIFF)
#	define _TIFF_DATA_TYPEDEFS_
#	include <tiffio.h>
#endif

#if defined(HAVE_OPENEXR)
#	include <ImfIO.h>
#	include <ImathBox.h>
#	include <ImfChannelList.h>
#	include <ImfInputFile.h>
#	include <ImfOutputFile.h>
#	include <ImfArray.h>
#endif

using namespace nv;

namespace {

	// Array of image load plugins.
//	static HashMap<String, ImageInput_Plugin> s_plugin_load_map;

	// Array of image save plugins.
//	static HashMap<String, ImageOutput_Plugin> s_plugin_save_map;
	
	struct Color555 {
		uint16 b : 5;
		uint16 g : 5;
		uint16 r : 5;
	};
	
} // namespace


Image * nv::ImageIO::load(const char * fileName)
{
	nvDebugCheck(fileName != NULL);

	StdInputStream stream(fileName);
	
	if (stream.isError()) {
		return NULL;
	}
	
	return ImageIO::load(fileName, stream);
}

Image * nv::ImageIO::load(const char * fileName, Stream & s)
{
	nvDebugCheck(fileName != NULL);
	nvDebugCheck(s.isLoading());

	const char * extension = Path::extension(fileName);
	
	if (strCaseCmp(extension, ".tga") == 0) {
		return ImageIO::loadTGA(s);
	}
#if defined(HAVE_JPEG)
	if (strCaseCmp(extension, ".jpg") == 0 || strCaseCmp(extension, ".jpeg") == 0) {
		return loadJPG(s);
	}
#endif
#if defined(HAVE_PNG)
	if (strCaseCmp(extension, ".png") == 0) {
		return loadPNG(s);
	}
#endif
	if (strCaseCmp(extension, ".psd") == 0) {
		return loadPSD(s);
	}
	// @@ use image plugins?
	return NULL;
}

bool nv::ImageIO::save(const char * fileName, Stream & s, Image * img)
{
	nvDebugCheck(fileName != NULL);
	nvDebugCheck(s.isSaving());
	nvDebugCheck(img != NULL);

	const char * extension = Path::extension(fileName);

	if (strCaseCmp(extension, ".tga") == 0) {
		return ImageIO::saveTGA(s, img);
	}

	return false;
}

bool nv::ImageIO::save(const char * fileName, Image * img)
{
	nvDebugCheck(fileName != NULL);
	nvDebugCheck(img != NULL);

	StdOutputStream stream(fileName);
	if (stream.isError())
	{
		return false;
	}

	return ImageIO::save(fileName, stream, img);
}

FloatImage * nv::ImageIO::loadFloat(const char * fileName)
{
	nvDebugCheck(fileName != NULL);

	StdInputStream stream(fileName);
	
	if (stream.isError()) {
		return false;
	}
	
	return loadFloat(fileName, stream);
}

FloatImage * nv::ImageIO::loadFloat(const char * fileName, Stream & s)
{
	nvDebugCheck(fileName != NULL);

	const char * extension = Path::extension(fileName);
	
#if defined(HAVE_TIFF)
	if (strCaseCmp(extension, ".tif") == 0 || strCaseCmp(extension, ".tiff") == 0) {
		return loadFloatTIFF(fileName, s);
	}
#endif
#if defined(HAVE_OPENEXR)
	if (strCaseCmp(extension, ".exr") == 0) {
		return loadFloatEXR(fileName, s);
	}
#endif

/* // @@ Disable temporarily
	if (strCaseCmp(extension, ".pfm") == 0) {
		return loadFloatPFM(fileName, s);
	}
*/

	return NULL;
}


bool nv::ImageIO::saveFloat(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components)
{
	const char * extension = Path::extension(fileName);

#if defined(HAVE_OPENEXR)
	if (strCaseCmp(extension, ".exr") == 0)
	{
		return ImageIO::saveFloatEXR(fileName, fimage, base_component, num_components);
	}
#endif

#if defined(HAVE_TIFF)
	if (strCaseCmp(extension, ".tif") == 0 || strCaseCmp(extension, ".tiff") == 0)
	{
		return ImageIO::saveFloatTIFF(fileName, fimage, base_component, num_components);
	}
#endif

/* // @@ Disable Temporarily
	if (strCaseCmp(extension, ".pfm") == 0)
	{
//		return ImageIO::saveFloatPFM(fileName, fimage, base_component, num_components);
	}
*/

	if (num_components == 3 || num_components == 4)
	{
		AutoPtr<Image> image(fimage->createImage(base_component, num_components));
		nvCheck(image != NULL);

		if (num_components == 4)
		{
			image->setFormat(Image::Format_ARGB);
		}

		return ImageIO::save(fileName, image.ptr());
	}

	return false;
}


/// Load TGA image.
Image * nv::ImageIO::loadTGA(Stream & s)
{
	nvCheck(!s.isError());
	nvCheck(s.isLoading());
	
	TgaHeader tga;
	s << tga;
	s.seek(TgaHeader::Size + tga.id_length);

	// Get header info.
	bool rle = false;
	bool pal = false;
	bool rgb = false;
	bool grey = false;

	switch( tga.image_type ) {
		case TGA_TYPE_RLE_INDEXED:
			rle = true;
			// no break is intended!
		case TGA_TYPE_INDEXED:
			if( tga.colormap_type!=1 || tga.colormap_size!=24 || tga.colormap_length>256 ) {
				nvDebug( "*** ImageIO::loadTGA: Error, only 24bit paletted images are supported.\n" );
				return false;
			}
			pal = true;
			break;

		case TGA_TYPE_RLE_RGB:
			rle = true;
			// no break is intended!
		case TGA_TYPE_RGB:
			rgb = true;
			break;

		case TGA_TYPE_RLE_GREY:
			rle = true;
			// no break is intended!
		case TGA_TYPE_GREY:
			grey = true;
			break;

		default:
			nvDebug( "*** ImageIO::loadTGA: Error, unsupported image type.\n" );
			return false;
	}

	const uint pixel_size = (tga.pixel_size/8);
	nvDebugCheck(pixel_size <= 4);
	
	const uint size = tga.width * tga.height * pixel_size;

	
	// Read palette
	uint8 palette[768];
	if( pal ) {
		nvDebugCheck(tga.colormap_length < 256);
		s.serialize(palette, 3 * tga.colormap_length);
	}

	// Decode image.
	uint8 * mem = new uint8[size];
	if( rle ) {
		// Decompress image in src.
		uint8 * dst = mem;
		int num = size;

		while (num > 0) {
			// Get packet header
			uint8 c; 
			s << c;

			uint count = (c & 0x7f) + 1;
			num -= count * pixel_size;

			if (c & 0x80) {
				// RLE pixels.
				uint8 pixel[4];	// uint8 pixel[pixel_size];
				s.serialize( pixel, pixel_size );
				do {
					memcpy(dst, pixel, pixel_size);
					dst += pixel_size;
				} while (--count);
			}
			else {
				// Raw pixels.
				count *= pixel_size;
				//file->Read8(dst, count);
				s.serialize(dst, count);
				dst += count;
			}
		}
	}
	else {
		s.serialize(mem, size);
	}

	// Allocate image.
	AutoPtr<Image> img(new Image());
	img->allocate(tga.width, tga.height);

	int lstep;
	Color32 * dst;
	if( tga.flags & TGA_ORIGIN_UPPER ) {
		lstep = tga.width;
		dst = img->pixels();
	}
	else {
		lstep = - tga.width;
		dst = img->pixels() + (tga.height-1) * tga.width;
	}

	// Write image.
	uint8 * src = mem;
	if( pal ) {
		for( int y = 0; y < tga.height; y++ ) {
			for( int x = 0; x < tga.width; x++ ) {
				uint8 idx = *src++;
				dst[x].setBGRA(palette[3*idx+0], palette[3*idx+1], palette[3*idx+2], 0xFF);
			}
			dst += lstep;
		}
	}
	else if( grey ) {
		img->setFormat(Image::Format_ARGB);
		
		for( int y = 0; y < tga.height; y++ ) {
			for( int x = 0; x < tga.width; x++ ) {
				dst[x].setBGRA(*src, *src, *src, *src);
				src++;
			}
			dst += lstep;
		}
	}
	else {
		
		if( tga.pixel_size == 16 ) {
			for( int y = 0; y < tga.height; y++ ) {
				for( int x = 0; x < tga.width; x++ ) {
					Color555 c = *reinterpret_cast<Color555 *>(src);
					uint8 b = (c.b << 3) | (c.b >> 2);					
					uint8 g = (c.g << 3) | (c.g >> 2);
					uint8 r = (c.r << 3) | (c.r >> 2);
					dst[x].setBGRA(b, g, r, 0xFF);
					src += 2;
				}
				dst += lstep;
			}
		}
		else if( tga.pixel_size == 24 ) {
			for( int y = 0; y < tga.height; y++ ) {
				for( int x = 0; x < tga.width; x++ ) {
					dst[x].setBGRA(src[0], src[1], src[2], 0xFF);
					src += 3;
				}
				dst += lstep;
			}
		}
		else if( tga.pixel_size == 32 ) {
			img->setFormat(Image::Format_ARGB);
			
			for( int y = 0; y < tga.height; y++ ) {
				for( int x = 0; x < tga.width; x++ ) {
					dst[x].setBGRA(src[0], src[1], src[2], src[3]);
					src += 4;
				}
				dst += lstep;
			}
		}
	}

	// free uncompressed data.
	delete [] mem;

	return img.release();
}

/// Save TGA image.
bool nv::ImageIO::saveTGA(Stream & s, const Image * img)
{
	nvCheck(!s.isError());
	nvCheck(img != NULL);
	nvCheck(img->pixels() != NULL);
	
	TgaFile tga;
	tga.head.id_length = 0;
	tga.head.colormap_type = 0;
	tga.head.image_type = TGA_TYPE_RGB;

	tga.head.colormap_index = 0;
	tga.head.colormap_length = 0;
	tga.head.colormap_size = 0;

	tga.head.x_origin = 0;
	tga.head.y_origin = 0;
	tga.head.width = img->width();
	tga.head.height = img->height();
	if(img->format() == Image::Format_ARGB) {
		tga.head.pixel_size = 32;
		tga.head.flags = TGA_ORIGIN_UPPER | TGA_HAS_ALPHA;
	}
	else {
		tga.head.pixel_size = 24;
		tga.head.flags = TGA_ORIGIN_UPPER;
	}

	// @@ Serialize directly.
	tga.allocate();

	const uint n = img->width() * img->height();
	if(img->format() == Image::Format_ARGB) {
		for(uint i = 0; i < n; i++) {
			Color32 color = img->pixel(i);
			tga.mem[4 * i + 0] = color.b;
			tga.mem[4 * i + 1] = color.g;
			tga.mem[4 * i + 2] = color.r;
			tga.mem[4 * i + 3] = color.a;
		}
	}
	else {
		for(uint i = 0; i < n; i++) {
			Color32 color = img->pixel(i);
			tga.mem[3 * i + 0] = color.b;
			tga.mem[3 * i + 1] = color.g;
			tga.mem[3 * i + 2] = color.r;
		}
	}

	s << tga;
	
	tga.free();
	
	return true;
}

/// Load PSD image.
Image * nv::ImageIO::loadPSD(Stream & s)
{
	nvCheck(!s.isError());
	nvCheck(s.isLoading());
	
	s.setByteOrder(Stream::BigEndian);
	
	PsdHeader header;
	s << header;
	
	if (!header.isValid())
	{
		printf("invalid header!\n");
		return NULL;
	}
	
	if (!header.isSupported())
	{
		printf("unsupported file!\n");
		return NULL;
	}
	
	int tmp;
	
	// Skip mode data.
	s << tmp;
	s.seek(s.tell() + tmp);

	// Skip image resources.
	s << tmp;
	s.seek(s.tell() + tmp);
	
	// Skip the reserved data.
	s << tmp;
	s.seek(s.tell() + tmp);
	
	// Find out if the data is compressed.
	// Known values:
	//   0: no compression
	//   1: RLE compressed
	uint16 compression;
	s << compression;
	
	if (compression > 1) {
		// Unknown compression type.
		return NULL;
	}
	
	uint channel_num = header.channel_count;
	
	AutoPtr<Image> img(new Image());
	img->allocate(header.width, header.height);
	
	if (channel_num < 4)
	{
		// Clear the image.
		img->fill(Color32(0, 0, 0, 0xFF));
	}
	else
	{
		// Enable alpha.
		img->setFormat(Image::Format_ARGB);
		
		// Ignore remaining channels.
		channel_num = 4;
	}
	
	
	const uint pixel_count = header.height * header.width;
	
	static const uint components[4] = {2, 1, 0, 3};
	
	if (compression)
	{
		s.seek(s.tell() + header.height * header.channel_count * sizeof(uint16));
		
		// Read RLE data.						
		for (uint channel = 0; channel < channel_num; channel++)
		{
			uint8 * ptr = (uint8 *)img->pixels() + components[channel];
			
			uint count = 0;
			while( count < pixel_count )
			{
				if (s.isAtEnd()) return NULL;
				
				uint8 c;
				s << c;
				
				uint len = c;
				if (len < 128)
				{
					// Copy next len+1 bytes literally.
					len++;
					count += len;
					if (count > pixel_count) return NULL;
	
					while (len != 0)
					{
						s << *ptr;
						ptr += 4;
						len--;
					}
				} 
				else if (len > 128)
				{
					// Next -len+1 bytes in the dest are replicated from next source byte.
					// (Interpret len as a negative 8-bit int.)
					len ^= 0xFF;
					len += 2;
					count += len;
					if (s.isAtEnd() || count > pixel_count) return NULL;
					
					uint8 val;
					s << val;
					while( len != 0 ) {
						*ptr = val;
						ptr += 4;
						len--;
					}
				}
				else if( len == 128 ) {
					// No-op.
				}
			}
		}
	}
	else
	{
		// We're at the raw image data. It's each channel in order (Red, Green, Blue, Alpha, ...)
		// where each channel consists of an 8-bit value for each pixel in the image.
		
		// Read the data by channel.
		for (uint channel = 0; channel < channel_num; channel++)
		{
			uint8 * ptr = (uint8 *)img->pixels() + components[channel];
			
			// Read the data.
			uint count = pixel_count;
			while (count != 0)
			{
				s << *ptr;
				ptr += 4;
				count--;
			}
		}
	}

	return img.release();
}

#if defined(HAVE_PNG)

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	nvDebugCheck(png_ptr != NULL);
	
	Stream * s = (Stream *)png_ptr->io_ptr;
	s->serialize(data, (int)length);
	
	if (s->isError()) {
		png_error(png_ptr, "Read Error");
	}
}


Image * nv::ImageIO::loadPNG(Stream & s)
{
	nvCheck(!s.isError());
	
	// Set up a read buffer and check the library version
	png_structp png_ptr;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
	//	nvDebug( "*** LoadPNG: Error allocating read buffer in file '%s'.\n", name );
		return false;
	}

	// Allocate/initialize a memory block for the image information
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
	//	nvDebug( "*** LoadPNG: Error allocating image information for '%s'.\n", name );
		return false;
	}

	// Set up the error handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	//	nvDebug( "*** LoadPNG: Error reading png file '%s'.\n", name );
		return false;
	}

	// Set up the I/O functions.
	png_set_read_fn(png_ptr, (void*)&s, user_read_data);


	// Retrieve the image header information
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);


	if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8) {
		// Convert indexed images to RGB.
		png_set_expand(png_ptr);
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		// Convert grayscale to RGB.
		png_set_expand(png_ptr);
	}
	else if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		// Expand images with transparency to full alpha channels
		// so the data will be available as RGBA quartets.
		png_set_expand(png_ptr);
	}
	else if (bit_depth < 8) {
		// If we have < 8 scale it up to 8.
		//png_set_expand(png_ptr);
		png_set_packing(png_ptr);
	}

	// Reduce bit depth.
	if (bit_depth == 16) {
		png_set_strip_16(png_ptr);
	}

	// Represent gray as RGB
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}

	// Convert to RGBA filling alpha with 0xFF.
	if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	}

	// @todo Choose gamma according to the platform?
	double screen_gamma = 2.2;
	int intent;
	if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
		png_set_gamma(png_ptr, screen_gamma, 0.45455);
	}
	else {
		double image_gamma;
		if (png_get_gAMA(png_ptr, info_ptr, &image_gamma)) {
			png_set_gamma(png_ptr, screen_gamma, image_gamma);
		}
		else {
			png_set_gamma(png_ptr, screen_gamma, 0.45455);
		}
	}

	// Perform the selected transforms.
	png_read_update_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	AutoPtr<Image> img(new Image());
	img->allocate(width, height);

	// Set internal format flags.
	if(color_type & PNG_COLOR_MASK_COLOR) {
		//img->flags |= PI_IF_HAS_COLOR;
	}
	if(color_type & PNG_COLOR_MASK_ALPHA) {
		//img->flags |= PI_IF_HAS_ALPHA;
		img->setFormat(Image::Format_ARGB);
	}

	// Read the image
	uint8 * pixels = (uint8 *)img->pixels();
	png_bytep * row_data = new png_bytep[sizeof(png_byte) * height];
	for (uint i = 0; i < height; i++) {
		row_data[i] = &(pixels[width * 4 * i]);
	}

	png_read_image(png_ptr, row_data);
	delete [] row_data;

	// Finish things up
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	// RGBA to BGRA.
	uint num = width * height;
	for(uint i = 0; i < num; i++)
	{
		Color32 c = img->pixel(i);
		img->pixel(i) = Color32(c.b, c.g, c.r, c.a);
	}
	
	// Compute alpha channel if needed.
	/*if( img->flags & PI_IU_BUMPMAP || img->flags & PI_IU_ALPHAMAP ) {
		if( img->flags & PI_IF_HAS_COLOR && !(img->flags & PI_IF_HAS_ALPHA)) {
			img->ComputeAlphaFromColor();
		}
	}*/

	return img.release();
}

#endif // defined(HAVE_PNG)

#if defined(HAVE_JPEG)

static void init_source (j_decompress_ptr /*cinfo*/){
}

static boolean fill_input_buffer (j_decompress_ptr cinfo){
	struct jpeg_source_mgr * src = cinfo->src;
	static JOCTET FakeEOI[] = { 0xFF, JPEG_EOI };

	// Generate warning
	nvDebug("jpeglib: Premature end of file\n");

	// Insert a fake EOI marker
	src->next_input_byte = FakeEOI;
	src->bytes_in_buffer = 2;

	return TRUE;
}

static void skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
	struct jpeg_source_mgr * src = cinfo->src;

	if(num_bytes >= (long)src->bytes_in_buffer) {
		fill_input_buffer(cinfo);
		return;
	}

	src->bytes_in_buffer -= num_bytes;
	src->next_input_byte += num_bytes;
}

static void term_source (j_decompress_ptr /*cinfo*/){
	// no work necessary here
}


Image * nv::ImageIO::loadJPG(Stream & s)
{
	nvCheck(!s.isError());
	
	// Read the entire file.
	Array<uint8> byte_array;
	byte_array.resize(s.size());
	s.serialize(byte_array.unsecureBuffer(), s.size());
	
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	cinfo.src = (struct jpeg_source_mgr *) (*cinfo.mem->alloc_small)
			((j_common_ptr) &cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
	cinfo.src->init_source = init_source;
	cinfo.src->fill_input_buffer = fill_input_buffer;
	cinfo.src->skip_input_data = skip_input_data;
	cinfo.src->resync_to_restart = jpeg_resync_to_restart;	// use default method
	cinfo.src->term_source = term_source;
	cinfo.src->bytes_in_buffer = byte_array.size();
	cinfo.src->next_input_byte = byte_array.buffer();

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	/*
	cinfo.do_fancy_upsampling = FALSE;	// fast decompression
	cinfo.dct_method = JDCT_FLOAT;			// Choose floating point DCT method.
	*/

	uint8 * tmp_buffer = new uint8 [cinfo.output_width * cinfo.output_height * cinfo.num_components];
	uint8 * scanline = tmp_buffer;

	while( cinfo.output_scanline < cinfo.output_height ){
		int num_scanlines = jpeg_read_scanlines (&cinfo, &scanline, 1);
		scanline += num_scanlines * cinfo.output_width * cinfo.num_components;
	}

	jpeg_finish_decompress(&cinfo);

	AutoPtr<Image> img(new Image());
	img->allocate(cinfo.output_width, cinfo.output_height);

	Color32 * dst = img->pixels();
	const int size = img->height() * img->width();
	const uint8 * src = tmp_buffer;

	if( cinfo.num_components == 3 ) {
		img->setFormat(Image::Format_RGB);
		for( int i = 0; i < size; i++ ) {
			*dst++ = Color32(src[0], src[1], src[2]);
			src += 3;
		}
	}
	else {
		img->setFormat(Image::Format_ARGB);
		for( int i = 0; i < size; i++ ) {
			*dst++ = Color32(*src, *src, *src, *src);
			src++;
		}
	}

	delete [] tmp_buffer;
	jpeg_destroy_decompress (&cinfo);

	return img.release();
}

#endif // defined(HAVE_JPEG)

#if defined(HAVE_TIFF)

/*
static tsize_t tiffReadWriteProc(thandle_t h, tdata_t ptr, tsize_t size)
{
	Stream * s = (Stream *)h;
	nvDebugCheck(s != NULL);

	s->serialize(ptr, size);

	return size;
}

static toff_t tiffSeekProc(thandle_t h, toff_t offset, int whence)
{
	Stream * s = (Stream *)h;
	nvDebugCheck(s != NULL);
	
	if (!s->isSeekable())
	{
		return (toff_t)-1;
	}

	if (whence == SEEK_SET)
	{
		s->seek(offset);
	}
	else if (whence == SEEK_CUR)
	{
		s->seek(s->tell() + offset);
	}
	else if (whence == SEEK_END)
	{
		s->seek(s->size() + offset);
	}

	return s->tell();
}

static int tiffCloseProc(thandle_t)
{
	return 0;
}

static toff_t tiffSizeProc(thandle_t h)
{
	Stream * s = (Stream *)h;
	nvDebugCheck(s != NULL);
	return s->size();
}

static int tiffMapFileProc(thandle_t, tdata_t*, toff_t*)
{
	// @@ TODO, Implement these functions.
	return -1;
}

static void tiffUnmapFileProc(thandle_t, tdata_t, toff_t)
{
	// @@ TODO, Implement these functions.
}
*/

FloatImage * nv::ImageIO::loadFloatTIFF(const char * fileName, Stream & s)
{
	nvCheck(!s.isError());
	
	TIFF * tif = TIFFOpen(fileName, "r");
	//TIFF * tif = TIFFClientOpen(fileName, "r", &s, tiffReadWriteProc, tiffReadWriteProc, tiffSeekProc, tiffCloseProc, tiffSizeProc, tiffMapFileProc, tiffUnmapFileProc);
	
	if (!tif)
	{
		nvDebug("Can't open '%s' for reading\n", fileName);
		return NULL;
	}
	
	::uint16 spp, bpp, format;
	::uint32 width, height;
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bpp);
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &format);
	
	if (bpp != 8 && bpp != 16 && bpp != 32) {
		nvDebug("Can't load '%s', only 1 sample per pixel supported\n", fileName);
		TIFFClose(tif);
		return NULL;
	}
	
	AutoPtr<FloatImage> fimage(new FloatImage());
	fimage->allocate(spp, width, height);
	
	int linesize = TIFFScanlineSize(tif);
	tdata_t buf = (::uint8 *)nv::mem::malloc(linesize);
	
	for (uint y = 0; y < height; y++) 
	{
		TIFFReadScanline(tif, buf, y, 0);

		for (uint c=0; c<spp; c++ ) 
		{
			float * dst = fimage->scanline(y, c);

			for(uint x = 0; x < width; x++) 
			{
				if (bpp == 8)
				{
					dst[x] = float(((::uint8 *)buf)[x*spp+c]) / float(0xFF);
				}
				else if (bpp == 16)
				{
					dst[x] = float(((::uint16 *)buf)[x*spp+c]) / float(0xFFFF);
				}
				else if (bpp == 32)
				{
					if (format==SAMPLEFORMAT_IEEEFP)
					{
						dst[x] = float(((float *)buf)[x*spp+c]);
					}
					else
					{
						dst[x] = float(((::uint32 *)buf)[x*spp+c] >> 8) / float(0xFFFFFF);
					}

				}

			}
		}
	}

	nv::mem::free(buf);
	
	TIFFClose(tif);
	
	return fimage.release();
}

bool nv::ImageIO::saveFloatTIFF(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components)
{
	nvCheck(fileName != NULL);
	nvCheck(fimage != NULL);
	nvCheck(base_component + num_components <= fimage->componentNum());
	
	const int iW = fimage->width();
	const int iH = fimage->height();
	const int iC = num_components;

	TIFF * image = TIFFOpen(fileName, "w");

	// Open the TIFF file
	if (image == NULL)
	{
		nvDebug("Could not open '%s' for writing\n", fileName);
		return false;
	}

	TIFFSetField(image, TIFFTAG_IMAGEWIDTH,  iW);
	TIFFSetField(image, TIFFTAG_IMAGELENGTH, iH);
	TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, iC);
	TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 32);
	
	uint32 rowsperstrip = TIFFDefaultStripSize(image, (uint32)-1); 

	TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
	if (num_components == 3)
	{
		// Set this so that it can be visualized with pfstools.
		TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	}
	TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	float * scanline = new float[iW * iC];
	for (int y = 0; y < iH; y++)
	{
		for (int c = 0; c < iC; c++) 
		{
			const float * src = fimage->scanline(y, base_component + c);
			for (int x = 0; x < iW; x++) scanline[x * iC + c] = src[x];
		}
		if (TIFFWriteScanline(image, scanline, y, 0)==-1)
		{
			nvDebug("Error writing scanline %d\n", y);
			return false;
		}
	}
	delete [] scanline;

	// Close the file
	TIFFClose(image);
	return true;
}

#endif

#if defined(HAVE_OPENEXR)

namespace
{
	class ExrStream : public Imf::IStream
	{
	public:
		ExrStream(const char * name, Stream & s) : Imf::IStream(name), m_stream(s)
		{
			nvDebugCheck(s.isLoading());
		}
		
		virtual bool read(char c[], int n)
		{
			m_stream.serialize(c, n);
			
			if (m_stream.isError())
			{
				throw Iex::InputExc("I/O error.");
			}
			
			return m_stream.isAtEnd();
		}
		
		virtual Imf::Int64 tellg()
		{
			return m_stream.tell();
		}
		
		virtual void seekg(Imf::Int64 pos)
		{
			m_stream.seek(pos);
		}
		
		virtual void clear()
		{
			m_stream.clearError();
		}
		
	private:
		Stream & m_stream;
	};

} // namespace

FloatImage * nv::ImageIO::loadFloatEXR(const char * fileName, Stream & s)
{
	nvCheck(s.isLoading());
	nvCheck(!s.isError());

	ExrStream stream(fileName, s);
	Imf::InputFile inputFile(stream);

	Imath::Box2i box = inputFile.header().dataWindow();

	int width = box.max.x - box.min.y + 1;
	int height = box.max.x - box.min.y + 1;

	const Imf::ChannelList & channels = inputFile.header().channels();
	
	// Count channels.
	uint channelCount= 0;
	for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it)
	{
		channelCount++;
	}
	
	// Allocate FloatImage.
	AutoPtr<FloatImage> fimage(new FloatImage());
	fimage->allocate(channelCount, width, height);
	
	// Describe image's layout with a framebuffer.
	Imf::FrameBuffer frameBuffer;
	uint i = 0;
	for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it, ++i)
	{
		frameBuffer.insert(it.name(), Imf::Slice(Imf::FLOAT, (char *)fimage->channel(i), sizeof(float), sizeof(float) * width));
	}
	
	// Read it.
	inputFile.setFrameBuffer (frameBuffer);
	inputFile.readPixels (box.min.y, box.max.y);
	
	return fimage.release();
}

bool nv::ImageIO::saveFloatEXR(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components)
{
	nvCheck(fileName != NULL);
	nvCheck(fimage != NULL);
	nvCheck(base_component + num_components <= fimage->componentNum());
	nvCheck(num_components > 0 && num_components <= 4);
	
	const int w = fimage->width();
	const int h = fimage->height();
	
	const char * channelNames[] = {"R", "G", "B", "A"};
	
    Imf::Header header (w, h);
	
	for (uint c = 0; c < num_components; c++)
	{
		header.channels().insert(channelNames[c], Imf::Channel(Imf::FLOAT));
	}
	
    Imf::OutputFile file(fileName, header);
    Imf::FrameBuffer frameBuffer;
    
	for (uint c = 0; c < num_components; c++)
	{
		char * channel = (char *) fimage->channel(base_component + c);
		frameBuffer.insert(channelNames[c], Imf::Slice(Imf::FLOAT, channel, sizeof(float), sizeof(float) * w));
	}
	
	file.setFrameBuffer(frameBuffer);
	file.writePixels(h);
	
	return true;
}

#endif // defined(HAVE_OPENEXR)

#if 0 // @@ Disable temporarily.

FloatImage * nv::ImageIO::loadFloatPFM(const char * fileName, Stream & s)
{
	nvCheck(s.isLoading());
	nvCheck(!s.isError());

	Tokenizer parser(&s);

	parser.nextToken();

	bool grayscale;
	if (parser.token() == "PF")
	{
		grayscale = false;
	}
	else if (parser.token() == "Pf")
	{
		grayscale = true;
	}
	else
	{
		// Invalid file.
		return NULL;
	}

	parser.nextLine();
	
	int width = parser.token().toInt(); parser.nextToken();
	int height = parser.token().toInt();

	parser.nextLine();

	float scaleFactor = parser.token().toFloat();

	if (scaleFactor >= 0)
	{
		s.setByteOrder(Stream::BigEndian);
	}
	else
	{
		s.setByteOrder(Stream::LittleEndian);
	}
	scaleFactor = fabsf(scaleFactor);

	// Allocate image.
	AutoPtr<FloatImage> fimage(new FloatImage());

	if (grayscale)
	{
		fimage->allocate(1, width, height);

		float * channel = fimage->channel(0);

		for (int i = 0; i < width * height; i++)
		{
			s << channel[i];
		}
	}
	else
	{
		fimage->allocate(3, width, height);

		float * rchannel = fimage->channel(0);
		float * gchannel = fimage->channel(1);
		float * bchannel = fimage->channel(2);

		for (int i = 0; i < width * height; i++)
		{
			s << rchannel[i] << gchannel[i] << bchannel[i];
		}
	}

	return fimage.release();
}

bool nv::ImageIO::saveFloatPFM(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components)
{
	nvCheck(fileName != NULL);
	nvCheck(fimage != NULL);
	nvCheck(fimage->componentNum() <= base_component + num_components);
	nvCheck(num_components == 1 || num_components == 3);

	StdOutputStream stream(fileName);
	TextWriter writer(&stream);

	if (num_components == 1) writer.write("Pf\n");
	else /*if (num_components == 3)*/ writer.write("PF\n");

	int w = fimage->width();
	int h = fimage->height();
	writer.write("%d %d\n", w, h);
	writer.write("%f\n", -1.0f);	// little endian with 1.0 scale.

	if (num_components == 1)
	{
		float * channel = const_cast<float *>(fimage->channel(0));

		for (int i = 0; i < w * h; i++)
		{
			stream << channel[i];
		}
	}
	else
	{
		float * rchannel = const_cast<float *>(fimage->channel(0));
		float * gchannel = const_cast<float *>(fimage->channel(1));
		float * bchannel = const_cast<float *>(fimage->channel(2));

		for (int i = 0; i < w * h; i++)
		{
			stream << rchannel[i] << gchannel[i] << bchannel[i];
		}
	}

	return true;
}

#endif

#if 0

/** Save PNG*/
static bool SavePNG(const PiImage * img, const char * name) {
	nvCheck( img != NULL );
	nvCheck( img->mem != NULL );

	if( piStrCmp(piExtension(name), ".png" ) != 0 ) {
		return false;
	}
	
	if( img->flags & PI_IT_CUBEMAP ) {
		nvDebug("*** Cannot save cubemaps as PNG.");
		return false;
	}
	if( img->flags & PI_IT_DDS ) {
		nvDebug("*** Cannot save DDS surface as PNG.");
		return false;
	}

	nvDebug( "--- Saving '%s'.\n", name );
	
	PiAutoPtr<PiStream> ar( PiFileSystem::CreateFileWriter( name ) );
	if( ar == NULL ) {
		nvDebug( "*** SavePNG: Error, cannot save file '%s'.\n", name );
		return false;
	}

/*
public class PNGEnc {

    public static function encode(img:BitmapData):ByteArray {
        // Create output byte array
        var png:ByteArray = new ByteArray();
        // Write PNG signature
        png.writeUnsignedInt(0x89504e47);
        png.writeUnsignedInt(0x0D0A1A0A);
        // Build IHDR chunk
        var IHDR:ByteArray = new ByteArray();
        IHDR.writeInt(img.width);
        IHDR.writeInt(img.height);
        IHDR.writeUnsignedInt(0x08060000); // 32bit RGBA
        IHDR.writeByte(0);
        writeChunk(png,0x49484452,IHDR);
        // Build IDAT chunk
        var IDAT:ByteArray= new ByteArray();
        for(var i:int=0;i < img.height;i++) {
            // no filter
            IDAT.writeByte(0);
            var p:uint;
            if ( !img.transparent ) {
                for(var j:int=0;j < img.width;j++) {
                    p = img.getPixel(j,i);
                    IDAT.writeUnsignedInt(
                        uint(((p&0xFFFFFF) << 8)|0xFF));
                }
            } else {
                for(var j:int=0;j < img.width;j++) {
                    p = img.getPixel32(j,i);
                    IDAT.writeUnsignedInt(
                        uint(((p&0xFFFFFF) << 8)|
                        (shr(p,24))));
                }
            }
        }
        IDAT.compress();
        writeChunk(png,0x49444154,IDAT);
        // Build IEND chunk
        writeChunk(png,0x49454E44,null);
        // return PNG
        return png;
    }

    private static var crcTable:Array;
    private static var crcTableComputed:Boolean = false;

    private static function writeChunk(png:ByteArray, 
            type:uint, data:ByteArray) {
        if (!crcTableComputed) {
            crcTableComputed = true;
            crcTable = [];
            for (var n:uint = 0; n < 256; n++) {
                var c:uint = n;
                for (var k:uint = 0; k < 8; k++) {
                    if (c & 1) {
                        c = uint(uint(0xedb88320) ^ 
                            uint(c >>> 1));
                    } else {
                        c = uint(c >>> 1);
                    }
                }
                crcTable[n] = c;
            }
        }
        var len:uint = 0;
        if (data != null) {
            len = data.length;
        }
        png.writeUnsignedInt(len);
        var p:uint = png.position;
        png.writeUnsignedInt(type);
        if ( data != null ) {
            png.writeBytes(data);
        }
        var e:uint = png.position;
        png.position = p;
        var c:uint = 0xffffffff;
        for (var i:int = 0; i < (e-p); i++) {
            c = uint(crcTable[
                (c ^ png.readUnsignedByte()) & 
                uint(0xff)] ^ uint(c >>> 8));
        }
        c = uint(c^uint(0xffffffff));
        png.position = e;
        png.writeUnsignedInt(c);
    }
}
*/
}

#endif // 0

#if 0


namespace ImageIO {

	/** Init ImageIO plugins. */
	void InitPlugins() {
	//	AddInputPlugin( "", LoadANY );
		AddInputPlugin( "tga", LoadTGA );
#if HAVE_PNG
		AddInputPlugin( "png", LoadPNG );
#endif
#if HAVE_JPEG
		AddInputPlugin( "jpg", LoadJPG );
#endif
		AddInputPlugin( "dds", LoadDDS );
		
		AddOutputPlugin( "tga", SaveTGA );
	}
	
	/** Reset ImageIO plugins. */
	void ResetPlugins() {
		s_plugin_load_map.Clear();
		s_plugin_save_map.Clear();
	}
	
	/** Add an input plugin. */
	void AddInputPlugin( const char * ext, ImageInput_Plugin plugin ) {
		s_plugin_load_map.Add(ext, plugin);
	}
	
	/** Add an output plugin. */
	void AddOutputPlugin( const char * ext, ImageOutput_Plugin plugin ) {
		s_plugin_save_map.Add(ext, plugin);
	}

	
	bool Load(PiImage * img, const char * name, PiStream & stream) {
			
		// Get name extension.
		const char * extension = piExtension(name);
		
		// Skip the dot.
		if( *extension == '.' ) {
			extension++;
		}
		
		// Lookup plugin in the map.
		ImageInput_Plugin plugin = NULL;
		if( s_plugin_load_map.Get(extension, &plugin) ) {
			return plugin(img, stream);
		}
		
		/*foreach(i, s_plugin_load_map) {
			nvDebug("%s %s %d\n", s_plugin_load_map[i].key.GetStr(), extension, 0 == strcmp(extension, s_plugin_load_map[i].key));
		}
		
		nvDebug("No plugin found for '%s' %d.\n", extension, s_plugin_load_map.Size());*/
		
		return false;
	}

	bool Save(const PiImage * img, const char * name, PiStream & stream) {
				
		// Get name extension.
		const char * extension = piExtension(name);
		
		// Skip the dot.
		if( *extension == '.' ) {
			extension++;
		}
		
		// Lookup plugin in the map.
		ImageOutput_Plugin plugin = NULL;
		if( s_plugin_save_map.Get(extension, &plugin) ) {
			return plugin(img, stream);
		}
		
		return false;
	}
	
} // ImageIO

#endif // 0

