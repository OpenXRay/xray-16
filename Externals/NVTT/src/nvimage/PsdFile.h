// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_PSDFILE_H
#define NV_IMAGE_PSDFILE_H

#include <nvcore/Stream.h>

namespace nv
{
	enum PsdColorMode
	{
		PsdColorMode_Bitmap = 0,
		PsdColorMode_GrayScale = 1,
		PsdColorMode_Indexed = 2,
		PsdColorMode_RGB = 3,
		PsdColorMode_CMYK = 4,
		PsdColorMode_MultiChannel = 7,
		PsdColorMode_DuoTone = 8,
		PsdColorMode_LabColor = 9
	};

	/// PSD header.
	struct PsdHeader
	{
		uint32 signature;
		uint16 version;
		uint8 reserved[6];
		uint16 channel_count;
		uint32 height;
		uint32 width;
		uint16 depth;
		uint16 color_mode;
		
		bool isValid() const
		{
			return signature == 0x38425053;	// '8BPS'
		}
		
		bool isSupported() const
		{
			if (version != 1) {
				nvDebug("*** bad version number %u\n", version);
				return false;
			}
			if (channel_count > 4) {
				return false;
			}
			if (depth != 8) {
				return false;
			}
			if (color_mode != PsdColorMode_RGB) {
				return false;
			}
			return true;
		}
	};


	inline Stream & operator<< (Stream & s, PsdHeader & head)
	{
		s << head.signature << head.version;
		for (int i = 0; i < 6; i++) {
			s << head.reserved[i];
		}
		return s << head.channel_count << head.height << head.width << head.depth << head.color_mode;
	}

} // nv namespace

#endif // NV_IMAGE_PSDFILE_H
