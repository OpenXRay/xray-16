// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/TextWriter.h>

using namespace nv;


/// Constructor
TextWriter::TextWriter(Stream * s) : 
	s(s), 
	str(1024)
{
	nvCheck(s != NULL);
	nvCheck(s->isSaving());
}

void TextWriter::writeString(const char * str)
{
	nvDebugCheck(s != NULL);
	s->serialize(const_cast<char *>(str), (int)strlen(str));
}

void TextWriter::writeString(const char * str, uint len)
{
	nvDebugCheck(s != NULL);
	s->serialize(const_cast<char *>(str), len);
}

void TextWriter::write(const char * format, ...)
{
	va_list arg;
	va_start(arg,format);
	str.format(format, arg);
	writeString(str.str(), str.length());
	va_end(arg);
}

void TextWriter::write(const char * format, va_list arg)
{
	va_list tmp;
	va_copy(tmp, arg);
	str.format(format, arg);
	writeString(str.str(), str.length());
	va_end(tmp);
}
