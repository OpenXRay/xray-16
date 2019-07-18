#ifndef FILE_STREAM_READER_H
#define FILE_STREAM_READER_H

#include "stream_reader.h"

class CFileStreamReader : public CStreamReader
{
    using inherited = CStreamReader;

private:
#if defined(WINDOWS)
    HANDLE m_file_handle;
#elif defined(LINUX)
    int m_file_handle;
#endif

public:
    virtual void construct(pcstr file_name, const size_t& window_size);
    virtual void destroy();
};

#endif // FILE_STREAM_READER_H
