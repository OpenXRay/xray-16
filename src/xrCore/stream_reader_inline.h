#ifndef STREAM_READER_INLINE_H
#define STREAM_READER_INLINE_H
#if defined(LINUX) || defined(FREEBSD)
#include <sys/mman.h>
#endif

IC CStreamReader::CStreamReader() {}
IC CStreamReader::CStreamReader(const CStreamReader& object)
    : m_start_offset(object.m_start_offset), m_file_size(object.m_file_size), m_archive_size(object.m_archive_size),
      m_window_size(object.m_window_size)
{
    // should be never called
}

IC CStreamReader& CStreamReader::operator=(const CStreamReader&)
{
    // should be never called
    return (*this);
}

#if defined(WINDOWS)
IC const HANDLE& CStreamReader::file_mapping_handle() const { return (m_file_mapping_handle); }
#elif defined(LINUX) || defined(FREEBSD)
IC const int& CStreamReader::file_mapping_handle() const { return (m_file_mapping_handle); }
#endif

#if defined(WINDOWS)
IC void CStreamReader::unmap() { UnmapViewOfFile(m_current_map_view_of_file); }
#else
IC void CStreamReader::unmap() { ::munmap(const_cast<u8*>(m_current_map_view_of_file), m_current_window_size); }
#endif
IC void CStreamReader::remap(const u32& new_offset)
{
    unmap();
    map(new_offset);
}

IC u32 CStreamReader::elapsed() const
{
    u32 offset_from_file_start = tell();
    VERIFY(m_file_size >= offset_from_file_start);
    return (m_file_size - offset_from_file_start);
}

IC const u32& CStreamReader::length() const { return (m_file_size); }
IC void CStreamReader::seek(const int& offset) { advance(offset - tell()); }
IC u32 CStreamReader::tell() const
{
    VERIFY(m_current_pointer >= m_start_pointer);
    VERIFY(u32(m_current_pointer - m_start_pointer) <= m_current_window_size);
    return (m_current_offset_from_start + (m_current_pointer - m_start_pointer));
}

IC void CStreamReader::close()
{
    destroy();
    CStreamReader* self = this;
    xr_delete(self);
}

#endif // STREAM_READER_INLINE_H
