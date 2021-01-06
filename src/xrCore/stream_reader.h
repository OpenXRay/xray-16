#ifndef STREAM_READER_H
#define STREAM_READER_H

class XRCORE_API CStreamReader : public IReaderBase<CStreamReader>, Noncopyable
{
private:
#if defined(XR_PLATFORM_WINDOWS)
    HANDLE m_file_mapping_handle;
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    int m_file_mapping_handle;
#endif
    size_t m_start_offset;
    size_t m_file_size;
    size_t m_archive_size;
    size_t m_window_size;

private:
    size_t m_current_offset_from_start;
    size_t m_current_window_size;
    u8* m_current_map_view_of_file;
    u8* m_start_pointer;
    u8* m_current_pointer;

private:
    void map(const size_t& new_offset);
    IC void unmap();
    IC void remap(const size_t& new_offset);

public:
    IC CStreamReader() = default;

#if defined(XR_PLATFORM_WINDOWS)
    virtual void construct(const HANDLE& file_mapping_handle, const size_t& start_offset, const size_t& file_size,
        const size_t& archive_size, const size_t& window_size);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    virtual void construct(int file_mapping_handle, const size_t& start_offset, const size_t& file_size,
        const size_t& archive_size, const size_t& window_size);
#endif
    virtual void destroy();

public:
#if defined(XR_PLATFORM_WINDOWS)
    IC const HANDLE& file_mapping_handle() const;
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    IC const int& file_mapping_handle() const;
#endif
    IC intptr_t elapsed() const;
    IC const size_t& length() const;
    IC void seek(const int& offset);
    IC size_t tell() const;
    IC void close();

public:
    void advance(const int& offset);
    void r(void* buffer, size_t buffer_size) override;
    CStreamReader* open_chunk(const size_t& chunk_id);
    u32 find_chunk(u32 ID, bool* bCompressed = nullptr);
    //. CStreamReader*open_chunk_iterator(const u32 &chunk_id, CStreamReader *previous = 0); // 0 means first

public:
    //. void r_string (char *dest, u32 tgt_sz);
    //. void r_string (xr_string& dest);
    //. void skip_stringZ ();
    //. void r_stringZ (char *dest, u32 tgt_sz);
    void r_stringZ(shared_str& dest);
    //. void r_stringZ (xr_string& dest);

private:
    using inherited = IReaderBase<CStreamReader>;
};

#include "stream_reader_inline.h"

#endif // STREAM_READER_H
