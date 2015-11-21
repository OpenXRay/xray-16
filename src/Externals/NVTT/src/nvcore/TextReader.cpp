// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/TextReader.h>

using namespace nv;

/// Peek next character.
char TextReader::peek()
{
	nvDebugCheck(m_stream != NULL);
	nvDebugCheck(m_stream->isSeekable());
	
	if (m_stream->isAtEnd()) {
		return 0;
	}

	uint pos = m_stream->tell();

	char c;
	m_stream->serialize(&c, 1);
	m_stream->seek(pos);
	return c;
}

/// Read a single char.
char TextReader::read()
{
	nvDebugCheck(m_stream != NULL);
	
	char c;
	m_stream->serialize(&c, 1);

	if( m_stream->isAtEnd() ) {
		return 0;
	}
	
	return c;
}

/// Read from the current location to the end of the stream.
const char * TextReader::readToEnd()
{
	nvDebugCheck(m_stream != NULL);
	const int size = m_stream->size();
	
	m_text.clear();
	
	m_text.reserve(size + 1);
	m_text.resize(size);
	
	m_stream->serialize(m_text.unsecureBuffer(), size);
	m_text.pushBack('\0');
	
	return m_text.buffer();
}

/// Read from the current location to the end of the line.
const char * TextReader::readLine()
{
	m_text.clear();

	if (m_stream->isAtEnd()) {
		return NULL;
	}
	
	while (true) {
		char c = read();
		
		if (c == 0 || c == '\n') {
			break;
		}
		else if (c == '\r') {
			if( peek() == '\n' ) {
				read();
			}
			break;
		}
		
		m_text.pushBack(c);
	}
	
	m_text.pushBack('\0');
	return m_text.buffer();
}


