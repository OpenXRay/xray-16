#include "stdafx.h"
#include "stream_reader.h"
#include "xrCore/_std_extensions.h"
#ifdef LINUX
#include <sys/mman.h>
#endif

void CStreamReader::construct(const HANDLE& file_mapping_handle, const u32& start_offset, const u32& file_size,
    const u32& archive_size, const u32& window_size)
{
    m_file_mapping_handle = file_mapping_handle;
    m_start_offset = start_offset;
    m_file_size = file_size;
    m_archive_size = archive_size;
    m_window_size = _max(window_size, FS.dwAllocGranularity);

    map(0);
}

void CStreamReader::destroy() { unmap(); }
void CStreamReader::map(const u32& new_offset)
{
    VERIFY(new_offset <= m_file_size);
    m_current_offset_from_start = new_offset;

    u32 granularity = FS.dwAllocGranularity;
    u32 start_offset = m_start_offset + new_offset;
    u32 pure_start_offset = start_offset;
    start_offset = (start_offset / granularity) * granularity;

    VERIFY(pure_start_offset >= start_offset);
    u32 pure_end_offset = m_window_size + pure_start_offset;
    u32 end_offset = pure_end_offset / granularity;
    if (pure_end_offset % granularity)
        ++end_offset;

    end_offset *= granularity;
    if (end_offset > m_archive_size)
        end_offset = m_archive_size;

    m_current_window_size = end_offset - start_offset;
#if defined(WINDOWS)
    m_current_map_view_of_file =
        (u8*)MapViewOfFile(m_file_mapping_handle, FILE_MAP_READ, 0, start_offset, m_current_window_size);
#elif defined(LINUX)
    m_current_map_view_of_file =
        (u8*)::mmap(NULL, m_current_window_size, PROT_READ, MAP_SHARED, m_file_mapping_handle, start_offset); // TODO проверить не могу до полной сборки под Linux
#endif
    m_current_pointer = m_current_map_view_of_file;

    u32 difference = pure_start_offset - start_offset;
    m_current_window_size -= difference;
    m_current_pointer += difference;
    m_start_pointer = m_current_pointer;
}

void CStreamReader::advance(const int& offset)
{
    VERIFY(m_current_pointer >= m_start_pointer);
    VERIFY(u32(m_current_pointer - m_start_pointer) <= m_current_window_size);
    int offset_inside_window = int(m_current_pointer - m_start_pointer);
    if (offset_inside_window + offset >= (int)m_current_window_size)
    {
        remap(m_current_offset_from_start + offset_inside_window + offset);
        return;
    }

    if (offset_inside_window + offset < 0)
    {
        remap(m_current_offset_from_start + offset_inside_window + offset);
        return;
    }

    m_current_pointer += offset;
}

void CStreamReader::r(void* _buffer, u32 buffer_size)
{
    VERIFY(m_current_pointer >= m_start_pointer);
    VERIFY(u32(m_current_pointer - m_start_pointer) <= m_current_window_size);

    int offset_inside_window = int(m_current_pointer - m_start_pointer);
    if (offset_inside_window + buffer_size < m_current_window_size)
    {
        memcpy(_buffer, m_current_pointer, buffer_size);
        m_current_pointer += buffer_size;
        return;
    }

    u8* buffer = (u8*)_buffer;
    u32 elapsed_in_window = m_current_window_size - (m_current_pointer - m_start_pointer);

    do
    {
        memcpy(buffer, m_current_pointer, elapsed_in_window);
        buffer += elapsed_in_window;
        buffer_size -= elapsed_in_window;
        advance(elapsed_in_window);

        elapsed_in_window = m_current_window_size;
    } while (m_current_window_size < buffer_size);

    memcpy(buffer, m_current_pointer, buffer_size);
    advance(buffer_size);
}

CStreamReader* CStreamReader::open_chunk(const u32& chunk_id)
{
    BOOL compressed;
    u32 size = find_chunk(chunk_id, &compressed);
    if (!size)
        return (0);

    R_ASSERT2(!compressed, "cannot use CStreamReader on compressed chunks");
    CStreamReader* result = new CStreamReader();
    result->construct(file_mapping_handle(), m_start_offset + tell(), size, m_archive_size, m_window_size);
    return (result);
}

#include "FS_impl.h"
u32 CStreamReader::find_chunk(u32 ID, BOOL* bCompressed) { return inherited::find_chunk(ID, bCompressed); }
void CStreamReader::r_stringZ(shared_str& dest)
{
    char* dest_str = NULL;
    u32 current_str_size = 0;
    u8* end_str = NULL;
    do
    {
        u8* end_ptr = m_start_pointer + m_current_window_size;
        end_str = m_current_pointer;
        while (end_str < end_ptr)
        {
            if ((*end_str == 0) && (!dest_str))
            {
                dest = reinterpret_cast<char*>(m_current_pointer);
                m_current_pointer = ++end_str;
                return;
            }
            else if (*end_str == 0)
            {
                ++end_str; // copying with ending zero
                break;
            }
            ++end_str;
        }
        if (!dest_str) // first iteration
            dest_str = static_cast<char*>(_alloca(4096));

        u32 current_chunk_size = static_cast<u32>(end_ptr - m_current_pointer);
        R_ASSERT(current_str_size + current_chunk_size <= 4096);

        CopyMemory(dest_str, m_current_pointer, current_chunk_size);
        current_str_size += current_chunk_size;
        remap(m_current_offset_from_start + current_chunk_size);
        VERIFY(m_current_pointer == m_start_pointer);
    } while (*(end_str - 1) == 0);
    dest = dest_str;
    m_current_pointer = end_str;
}
