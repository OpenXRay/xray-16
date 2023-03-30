#ifndef FILE_STREAM_READER_H
#define FILE_STREAM_READER_H

#include "stream_reader.h"

class CFileStreamReader : public CStreamReader
{
    using inherited = CStreamReader;

private:
#if defined(XR_PLATFORM_WINDOWS)
    HANDLE m_file_handle;
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
    int m_file_handle;
#else
#   error Select or add implementation for your platform
#endif

public:
    virtual void construct(pcstr file_name, const size_t& window_size);
    void destroy() override;
};

#endif // FILE_STREAM_READER_H
