#include "stdafx.h"
#include "file_stream_reader.h"
#ifdef LINUX
#include <fcntl.h>
#endif

void CFileStreamReader::construct(LPCSTR file_name, const u32& window_size)
{
#if defined(WINDOWS)
    m_file_handle = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    VERIFY(m_file_handle != INVALID_HANDLE_VALUE);
    u32 file_size = (u32)GetFileSize(m_file_handle, NULL);

    HANDLE file_mapping_handle = CreateFileMapping(m_file_handle, 0, PAGE_READONLY, 0, 0, 0);
    VERIFY(file_mapping_handle != INVALID_HANDLE_VALUE);

    inherited::construct(file_mapping_handle, 0, file_size, file_size, window_size);
#elif defined(LINUX)
    pstr conv_fn = xr_strdup(file_name);
    convert_path_separators(conv_fn);
    m_file_handle = ::open(conv_fn, O_RDONLY);
    xr_free(conv_fn);
    VERIFY(m_file_handle != -1);
    struct stat file_info;
    ::fstat(m_file_handle, &file_info);
    u32 file_size = (u32)file_info.st_size;
    inherited::construct(m_file_handle, 0, file_size, file_size, window_size);
#endif
}

void CFileStreamReader::destroy()
{
#if defined(WINDOWS)
    HANDLE file_mapping_handle = this->file_mapping_handle();
    inherited::destroy();
    CloseHandle(file_mapping_handle);
    CloseHandle(m_file_handle);
#elif defined(LINUX)
    int file_mapping_handle = this->file_mapping_handle();
    inherited::destroy();
    ::close(m_file_handle);
    m_file_handle = -1;
#endif
}
