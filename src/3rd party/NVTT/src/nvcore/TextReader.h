// This code is in the public domain -- castanyo@yahoo.es

#ifndef NVCORE_TEXTREADER_H
#define NVCORE_TEXTREADER_H

#include <nvcore/nvcore.h>
#include <nvcore/Stream.h>
#include <nvcore/Containers.h>

namespace nv
{

/// Text reader.
class NVCORE_CLASS TextReader {
public:
	
	/// Ctor.
	TextReader(Stream * stream) : m_stream(stream), m_text(512) {
		nvCheck(stream != NULL);
		nvCheck(stream->isLoading());
	}
	
	char peek();
	char read();
	
	const char *readToEnd();

	// Returns a temporary string.
	const char * readLine(); 

private:
	Stream * m_stream;
	Array<char> m_text;
};

} // nv namespace

#endif // NVCORE_TEXTREADER_H
