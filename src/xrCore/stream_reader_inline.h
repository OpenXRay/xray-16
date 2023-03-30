#ifndef STREAM_READER_INLINE_H
#define STREAM_READER_INLINE_H
#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
#include <sys/mman.h>
#endif

#if defined(XR_PLATFORM_WINDOWS)
IC const HANDLE& CStreamReader::file_mapping_handle() const { return (m_file_mapping_handle); }
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
IC const int& CStreamReader::file_mapping_handle() const { return (m_file_mapping_handle); }
#else
#   error Select or add implementation for your platform
#endif

#if defined(XR_PLATFORM_WINDOWS)
IC void CStreamReader::unmap() { UnmapViewOfFile(m_current_map_view_of_file); }
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
IC void CStreamReader::unmap() { ::munmap(const_cast<u8*>(m_current_map_view_of_file), m_current_window_size); }
#else
#   error Select or add implementation for your platform
#endif
IC void CStreamReader::remap(const size_t& new_offset)
{
    unmap();
    map(new_offset);
}

IC intptr_t CStreamReader::elapsed() const
{
    const size_t offset_from_file_start = tell();
    VERIFY(m_file_size >= offset_from_file_start);
    return (m_file_size - offset_from_file_start);
}

IC const size_t& CStreamReader::length() const { return m_file_size; }
IC void CStreamReader::seek(const int& offset) { advance(offset - tell()); }
IC size_t CStreamReader::tell() const
{
    VERIFY(m_current_pointer >= m_start_pointer);
    VERIFY(size_t(m_current_pointer - m_start_pointer) <= m_current_window_size);
    return (m_current_offset_from_start + (m_current_pointer - m_start_pointer));
}

IC void CStreamReader::close()
{
    destroy();
    CStreamReader* self = this;
    xr_delete(self);
}

#endif // STREAM_READER_INLINE_H
