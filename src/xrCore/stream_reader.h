#ifndef STREAM_READER_H
#define STREAM_READER_H

class XRCORE_API CStreamReader : public IReaderBase<CStreamReader>
{
private:
#if defined(WINDOWS)
    HANDLE m_file_mapping_handle;
#elif defined(LINUX)
    int m_file_mapping_handle;
#endif
    u32 m_start_offset;
    u32 m_file_size;
    u32 m_archive_size;
    u32 m_window_size;

private:
    u32 m_current_offset_from_start;
    u32 m_current_window_size;
    u8* m_current_map_view_of_file;
    u8* m_start_pointer;
    u8* m_current_pointer;

private:
    void map(const u32& new_offset);
    IC void unmap();
    IC void remap(const u32& new_offset);

private:
    // should not be called
    IC CStreamReader(const CStreamReader& object);
    IC CStreamReader& operator=(const CStreamReader&);

public:
    IC CStreamReader();

public:
#if defined(WINDOWS)
    virtual void construct(const HANDLE& file_mapping_handle, const u32& start_offset, const u32& file_size,
        const u32& archive_size, const u32& window_size);
#elif defined(LINUX)
    virtual void construct(int file_mapping_handle, const u32& start_offset, const u32& file_size,
        const u32& archive_size, const u32& window_size);
#endif
    virtual void destroy();

public:
#if defined(WINDOWS)
    IC const HANDLE& file_mapping_handle() const;
#elif defined(LINUX)
    IC const int& file_mapping_handle() const;
#endif
    IC u32 elapsed() const;
    IC const u32& length() const;
    IC void seek(const int& offset);
    IC u32 tell() const;
    IC void close();

public:
    void advance(const int& offset);
    void r(void* buffer, u32 buffer_size);
    CStreamReader* open_chunk(const u32& chunk_id);
    u32 find_chunk(u32 ID, BOOL* bCompressed = 0);
    //. CStreamReader*open_chunk_iterator(const u32 &chunk_id, CStreamReader *previous = 0); // 0 means first

public:
    //. void r_string (char *dest, u32 tgt_sz);
    //. void r_string (xr_string& dest);
    //. void skip_stringZ ();
    //. void r_stringZ (char *dest, u32 tgt_sz);
    void r_stringZ(shared_str& dest);
    //. void r_stringZ (xr_string& dest);
private:
    typedef IReaderBase<CStreamReader> inherited;
};

#include "stream_reader_inline.h"

#endif // STREAM_READER_H
