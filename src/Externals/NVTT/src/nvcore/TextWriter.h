// This code is in the public domain -- castanyo@yahoo.es

#ifndef NVCORE_TEXTWRITER_H
#define NVCORE_TEXTWRITER_H

#include <nvcore/nvcore.h>
#include <nvcore/Stream.h>
#include <nvcore/StrLib.h>

namespace nv
{

	/// Text writer.
	class NVCORE_CLASS TextWriter
	{
	public:
	
		TextWriter(Stream * s);
	
		void writeString(const char * str);
		void writeString(const char * str, uint len);
		void write(const char * format, ...) __attribute__((format (printf, 2, 3)));
		void write(const char * format, va_list arg);
	
	private:
	
		Stream * s;
		
		// Temporary string.
		StringBuilder str;
	
	};


	inline TextWriter & operator<<( TextWriter & tw, int i)
	{
		tw.write("%d", i);
		return tw;
	}

	inline TextWriter & operator<<( TextWriter & tw, uint i)
	{
		tw.write("%u", i);
		return tw;
	}

	inline TextWriter & operator<<( TextWriter & tw, float f)
	{
		tw.write("%f", f);
		return tw;
	}

	inline TextWriter & operator<<( TextWriter & tw, const char * str)
	{
		tw.writeString(str);
		return tw;
	}

} // nv namespace





#endif // NVCORE_TEXTWRITER_H
