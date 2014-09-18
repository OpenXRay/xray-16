#ifndef COMPRESSION_PPMD_STREAM_INLINE_H
#define COMPRESSION_PPMD_STREAM_INLINE_H

namespace compression { namespace ppmd  { 

inline stream::stream			(const void *buffer, const u32 &buffer_size)
{
	m_buffer_size	= buffer_size;
	m_buffer		= (u8*)buffer;
	m_pointer		= m_buffer;
}

inline void stream::put_char	(const u8 &object)
{
	VERIFY			(m_pointer >= m_buffer);
	VERIFY			(m_pointer < (m_buffer + m_buffer_size));
	*m_pointer		= object;
	++m_pointer;
}

inline int stream::get_char		()
{
	VERIFY			(m_pointer >= m_buffer);
	if (m_pointer < (m_buffer + m_buffer_size))
		return		(*m_pointer++);
	return			(EOF);
}

inline void stream::rewind		()
{
	m_pointer		= m_buffer;
}

inline u8 *stream::buffer		() const
{
	return			(m_buffer);
}

inline u32 stream::tell			() const
{
	VERIFY			(m_pointer >= m_buffer);
	VERIFY2			(m_pointer <= (m_buffer + m_buffer_size),make_string("0x%08x <= 0x%08x + %d",m_pointer,m_buffer,m_buffer_size));
	return			(u32(m_pointer - m_buffer));
}

} // namespace ppmd
} // namespace compression

#endif // COMPRESSION_PPMD_STREAM_INLINE_H