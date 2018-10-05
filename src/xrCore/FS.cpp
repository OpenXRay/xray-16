#include "stdafx.h"
#pragma hdrstop

#include "FS_internal.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#if defined(WINDOWS)
#include <io.h>
#include <direct.h>
#elif defined(LINUX)
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

#pragma warning(pop)

#ifdef M_BORLAND
#define O_SEQUENTIAL 0
#endif // M_BORLAND

#ifdef FS_DEBUG
XRCORE_API u32 g_file_mapped_memory = 0;
u32 g_file_mapped_count = 0;
typedef xr_map<u32, std::pair<u32, shared_str>> FILE_MAPPINGS;
FILE_MAPPINGS g_file_mappings;

void register_file_mapping(void* address, const u32& size, LPCSTR file_name)
{
    FILE_MAPPINGS::const_iterator I = g_file_mappings.find(*(u32*)&address);
    VERIFY(I == g_file_mappings.end());
    g_file_mappings.insert(std::make_pair(*(u32*)&address, std::make_pair(size, shared_str(file_name))));

    // Msg ("++register_file_mapping(%2d): [0x%08x]%s", g_file_mapped_count + 1, *((u32*)&address), file_name);

    g_file_mapped_memory += size;
    ++g_file_mapped_count;
#ifdef USE_MEMORY_MONITOR
    // memory_monitor::monitor_alloc (addres,size,"file mapping");
    string512 temp;
    xr_sprintf(temp, sizeof(temp), "file mapping: %s", file_name);
    memory_monitor::monitor_alloc(address, size, temp);
#endif // USE_MEMORY_MONITOR
}

void unregister_file_mapping(void* address, const u32& size)
{
    FILE_MAPPINGS::iterator I = g_file_mappings.find(*(u32*)&address);
    VERIFY(I != g_file_mappings.end());
    // VERIFY2 ((*I).second.first == size,make_string("file mapping sizes are different: %d ->
    // %d",(*I).second.first,size));
    g_file_mapped_memory -= (*I).second.first;
    --g_file_mapped_count;

    // Msg ("--unregister_file_mapping(%2d): [0x%08x]%s", g_file_mapped_count + 1, *((u32*)&address),
    // (*I).second.second.c_str());

    g_file_mappings.erase(I);

#ifdef USE_MEMORY_MONITOR
    memory_monitor::monitor_free(address);
#endif // USE_MEMORY_MONITOR
}

XRCORE_API void dump_file_mappings()
{
    Msg("* active file mappings (%d):", g_file_mappings.size());

    FILE_MAPPINGS::const_iterator I = g_file_mappings.begin();
    FILE_MAPPINGS::const_iterator E = g_file_mappings.end();
    for (; I != E; ++I)
        Msg("* [0x%08x][%d][%s]", (*I).first, (*I).second.first, (*I).second.second.c_str());
}
#endif // DEBUG
//////////////////////////////////////////////////////////////////////
// Tools
//////////////////////////////////////////////////////////////////////
//---------------------------------------------------
void VerifyPath(LPCSTR path)
{
    string1024 tmp;
    for (int i = 0; path[i]; i++)
    {
        if (path[i] != _DELIMITER || i == 0)
            continue;
        CopyMemory(tmp, path, i);
        tmp[i] = 0;
        _mkdir(tmp);
    }
}

#ifdef _EDITOR
bool file_handle_internal(LPCSTR file_name, u32& size, int& hFile)
{
    hFile = _open(file_name, O_RDONLY | O_BINARY | O_SEQUENTIAL);
    if (hFile <= 0)
    {
        Sleep(1);
        hFile = _open(file_name, O_RDONLY | O_BINARY | O_SEQUENTIAL);
        if (hFile <= 0)
            return (false);
    }

    size = filelength(hFile);
    return (true);
}
#else // EDITOR
static int open_internal(LPCSTR fn, int& handle)
{
#if defined(WINDOWS)
    return (_sopen_s(&handle, fn, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD));
#elif defined(LINUX)
    handle = open(fn, _O_RDONLY);

    return (handle == -1);
#endif
}

bool file_handle_internal(LPCSTR file_name, u32& size, int& file_handle)
{
    if (open_internal(file_name, file_handle))
    {
        Sleep(1);
        if (open_internal(file_name, file_handle))
            return (false);
    }

    size = _filelength(file_handle);
    return (true);
}
#endif // EDITOR

void* FileDownload(LPCSTR file_name, const int& file_handle, u32& file_size)
{
    void* buffer = xr_malloc(file_size);

    int r_bytes = _read(file_handle, buffer, file_size);
    R_ASSERT3(
        // !file_size ||
        // (r_bytes && (file_size >= (u32)r_bytes)),
        file_size == (u32)r_bytes, "can't read from file : ", file_name);

    // file_size = r_bytes;

    R_ASSERT3(!_close(file_handle), "can't close file : ", file_name);

    return (buffer);
}

void* FileDownload(LPCSTR file_name, u32* buffer_size)
{
    int file_handle;
    R_ASSERT3(file_handle_internal(file_name, *buffer_size, file_handle), "can't open file : ", file_name);

    return (FileDownload(file_name, file_handle, *buffer_size));
}

typedef char MARK[9];
IC void mk_mark(MARK& M, const char* S) { strncpy_s(M, sizeof(M), S, 8); }
void FileCompress(const char* fn, const char* sign, void* data, u32 size)
{
    MARK M;
    mk_mark(M, sign);

    int H = _open(fn, O_BINARY | O_CREAT | O_WRONLY | O_TRUNC, S_IREAD | S_IWRITE);
    R_ASSERT2(H > 0, fn);
    _write(H, &M, 8);
    _writeLZ(H, data, size);
    _close(H);
}

void* FileDecompress(const char* fn, const char* sign, u32* size)
{
    MARK M, F;
    mk_mark(M, sign);

    int H = _open(fn, O_BINARY | O_RDONLY);
    R_ASSERT2(H > 0, fn);
    _read(H, &F, 8);
    if (strncmp(M, F, 8) != 0)
    {
        F[8] = 0;
        Msg("FATAL: signatures doesn't match, file(%s) / requested(%s)", F, sign);
    }
    R_ASSERT(strncmp(M, F, 8) == 0);

    void* ptr = 0;
    u32 SZ;
    SZ = _readLZ(H, ptr, _filelength(H) - 8);
    _close(H);
    if (size)
        *size = SZ;
    return ptr;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//---------------------------------------------------
// memory
CMemoryWriter::~CMemoryWriter() { xr_free(data); }
void CMemoryWriter::w(const void* ptr, u32 count)
{
    if (position + count > mem_size)
    {
        // reallocate
        if (mem_size == 0)
            mem_size = 128;
        while (mem_size <= (position + count))
            mem_size *= 2;
        if (0 == data)
            data = (BYTE*)xr_malloc(mem_size);
        else
            data = (BYTE*)xr_realloc(data, mem_size);
    }
    CopyMemory(data + position, ptr, count);
    position += count;
    if (position > file_size)
        file_size = position;
}

// static const u32 mb_sz = 0x1000000;
bool CMemoryWriter::save_to(LPCSTR fn)
{
    IWriter* F = FS.w_open(fn);
    if (F)
    {
        F->w(pointer(), size());
        FS.w_close(F);
        return true;
    }
    return false;
}

void IWriter::open_chunk(u32 type)
{
    w_u32(type);
    chunk_pos.push(tell());
    w_u32(0); // the place for 'size'
}
void IWriter::close_chunk()
{
    VERIFY(!chunk_pos.empty());

    int pos = tell();
    seek(chunk_pos.top());
    w_u32(pos - chunk_pos.top() - 4);
    seek(pos);
    chunk_pos.pop();
}
u32 IWriter::chunk_size() // returns size of currently opened chunk, 0 otherwise
{
    if (chunk_pos.empty())
        return 0;
    return tell() - chunk_pos.top() - 4;
}

void IWriter::w_compressed(void* ptr, u32 count)
{
    BYTE* dest = 0;
    unsigned dest_sz = 0;
    _compressLZ(&dest, &dest_sz, ptr, count);

    // if (g_dummy_stuff)
    // g_dummy_stuff (dest,dest_sz,dest);

    if (dest && dest_sz)
        w(dest, dest_sz);
    xr_free(dest);
}

void IWriter::w_chunk(u32 type, void* data, u32 size)
{
    open_chunk(type);
    if (type & CFS_CompressMark)
        w_compressed(data, size);
    else
        w(data, size);
    close_chunk();
}
void IWriter::w_sdir(const Fvector& D)
{
    Fvector C;
    float mag = D.magnitude();
    if (mag > EPS_S)
    {
        C.div(D, mag);
    }
    else
    {
        C.set(0, 0, 1);
        mag = 0;
    }
    w_dir(C);
    w_float(mag);
}
// XXX: reimplement to prevent buffer overflows
void IWriter::w_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    VPrintf(format, args);
    va_end(args);
}

void IWriter::VPrintf(const char* format, va_list args)
{
    char buf[1024];
    std::vsnprintf(buf, sizeof(buf), format, args);
    w(buf, xr_strlen(buf));
}

//---------------------------------------------------
// base stream
IReader* IReader::open_chunk(u32 ID)
{
    BOOL bCompressed;

    u32 dwSize = find_chunk(ID, &bCompressed);
    if (dwSize != 0)
    {
        if (bCompressed)
        {
            BYTE* dest;
            unsigned dest_sz;
            _decompressLZ(&dest, &dest_sz, pointer(), dwSize);
            return new CTempReader(dest, dest_sz, tell() + dwSize);
        }
        else
        {
            return new IReader(pointer(), dwSize, tell() + dwSize);
        }
    }
    else
        return 0;
};
void IReader::close()
{
    IReader* self = this;
    xr_delete(self);
}

#include "FS_impl.h"

#ifdef TESTING_IREADER
IReaderTestPolicy::~IReaderTestPolicy() { xr_delete(m_test); };
#endif // TESTING_IREADER

#ifdef FIND_CHUNK_BENCHMARK_ENABLE
find_chunk_counter g_find_chunk_counter;
#endif // FIND_CHUNK_BENCHMARK_ENABLE

u32 IReader::find_chunk(u32 ID, BOOL* bCompressed) { return inherited::find_chunk(ID, bCompressed); }
IReader* IReader::open_chunk_iterator(u32& ID, IReader* _prev)
{
    if (0 == _prev)
    {
        // first
        rewind();
    }
    else
    {
        // next
        seek(_prev->iterpos);
        _prev->close();
    }

    // open
    if (elapsed() < 8)
        return NULL;
    ID = r_u32();
    u32 _size = r_u32();
    if (ID & CFS_CompressMark)
    {
        // compressed
        u8* dest;
        unsigned dest_sz;
        _decompressLZ(&dest, &dest_sz, pointer(), _size);
        return new CTempReader(dest, dest_sz, tell() + _size);
    }
    else
    {
        // normal
        return new IReader(pointer(), _size, tell() + _size);
    }
}

void IReader::r(void* p, int cnt)
{
    VERIFY(Pos + cnt <= Size);
    CopyMemory(p, pointer(), cnt);
    advance(cnt);
#ifdef DEBUG
    BOOL bShow = FALSE;
    if (dynamic_cast<CFileReader*>(this))
        bShow = TRUE;
    if (dynamic_cast<CVirtualFileReader*>(this))
        bShow = TRUE;
    if (bShow)
    {
        FS.dwOpenCounter++;
    }
#endif
};

IC BOOL is_term(char a) { return (a == 13) || (a == 10); };
IC u32 IReader::advance_term_string()
{
    u32 sz = 0;
    char* src = (char*)data;
    while (!eof())
    {
        Pos++;
        sz++;
        if (!eof() && is_term(src[Pos]))
        {
            while (!eof() && is_term(src[Pos]))
                Pos++;
            break;
        }
    }
    return sz;
}
void IReader::r_string(char* dest, u32 tgt_sz)
{
    char* src = (char*)data + Pos;
    u32 sz = advance_term_string();
    R_ASSERT2(sz < (tgt_sz - 1), "Dest string less than needed.");
#if defined(WINDOWS)
    R_ASSERT(!IsBadReadPtr((void*)src, sz));
#endif

#ifdef _EDITOR
    CopyMemory(dest, src, sz);
#else
    strncpy_s(dest, tgt_sz, src, sz);
#endif
    dest[sz] = 0;
}
void IReader::r_string(xr_string& dest)
{
    char* src = (char*)data + Pos;
    u32 sz = advance_term_string();
    dest.assign(src, sz);
}
void IReader::r_stringZ(char* dest, u32 tgt_sz)
{
    char* src = (char*)data;
    u32 sz = xr_strlen(src);
    R_ASSERT2(sz < tgt_sz, "Dest string less than needed.");
    while ((src[Pos] != 0) && (!eof()))
        *dest++ = src[Pos++];
    *dest = 0;
    Pos++;
}
void IReader::r_stringZ(shared_str& dest)
{
    dest = (char*)(data + Pos);
    Pos += (dest.size() + 1);
}
void IReader::r_stringZ(xr_string& dest)
{
    dest = (char*)(data + Pos);
    Pos += int(dest.size() + 1);
};

void IReader::skip_stringZ()
{
    char* src = (char*)data;
    while ((src[Pos] != 0) && (!eof()))
        Pos++;
    Pos++;
};

//---------------------------------------------------
// temp stream
CTempReader::~CTempReader() { xr_free(data); };
//---------------------------------------------------
// pack stream
CPackReader::~CPackReader()
{
#ifdef FS_DEBUG
    unregister_file_mapping(base_address, Size);
#endif // DEBUG
#if defined(WINDOWS)
    UnmapViewOfFile(base_address);
#elif defined(LINUX)
    ::munmap(base_address, Size);
#endif
};
//---------------------------------------------------
// file stream
CFileReader::CFileReader(const char* name)
{
    data = (char*)FileDownload(name, (u32*)&Size);
    Pos = 0;
};
CFileReader::~CFileReader() { xr_free(data); };
//---------------------------------------------------
// compressed stream
CCompressedReader::CCompressedReader(const char* name, const char* sign)
{
    data = (char*)FileDecompress(name, sign, (u32*)&Size);
    Pos = 0;
}
CCompressedReader::~CCompressedReader() { xr_free(data); };
CVirtualFileRW::CVirtualFileRW(const char* cFileName)
{
#if defined(WINDOWS)
    // Open the file
    hSrcFile = CreateFile(cFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    R_ASSERT3(hSrcFile != INVALID_HANDLE_VALUE, cFileName, xrDebug::ErrorToString(GetLastError()));
    Size = (int)GetFileSize(hSrcFile, NULL);
    R_ASSERT3(Size, cFileName, xrDebug::ErrorToString(GetLastError()));

    hSrcMap = CreateFileMapping(hSrcFile, 0, PAGE_READWRITE, 0, 0, 0);
    R_ASSERT3(hSrcMap != INVALID_HANDLE_VALUE, cFileName, xrDebug::ErrorToString(GetLastError()));

    data = (char*)MapViewOfFile(hSrcMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    R_ASSERT3(data, cFileName, xrDebug::ErrorToString(GetLastError()));
#elif defined(LINUX)
    hSrcFile = ::open(cFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //за такое использование указателя нужно убивать, но пока пусть будет
    R_ASSERT3(hSrcFile != -1, cFileName, xrDebug::ErrorToString(GetLastError()));
    struct stat file_info;
    ::fstat(hSrcFile, &file_info);
    Size = (int)file_info.st_size;
    R_ASSERT3(Size, cFileName, xrDebug::ErrorToString(GetLastError()));
    data = (char*)::mmap(NULL, Size, PROT_READ | PROT_WRITE, MAP_SHARED, hSrcFile, 0);
#endif
#ifdef FS_DEBUG
    register_file_mapping(data, Size, cFileName);
#endif // DEBUG
}

CVirtualFileRW::~CVirtualFileRW()
{
#ifdef FS_DEBUG
    unregister_file_mapping(data, Size);
#endif // DEBUG
#if defined(WINDOWS)
    UnmapViewOfFile((void*)data);
    CloseHandle(hSrcMap);
    CloseHandle(hSrcFile);
#elif defined(LINUX)
    ::munmap((void*)data, Size);
    ::close(hSrcFile);
    hSrcFile = -1;
#endif
}

CVirtualFileReader::CVirtualFileReader(const char* cFileName)
{
#if defined(WINDOWS)
    // Open the file
    hSrcFile = CreateFile(cFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    R_ASSERT3(hSrcFile != INVALID_HANDLE_VALUE, cFileName, xrDebug::ErrorToString(GetLastError()));
    Size = (int)GetFileSize(hSrcFile, NULL);
    R_ASSERT3(Size, cFileName, xrDebug::ErrorToString(GetLastError()));

    hSrcMap = CreateFileMapping(hSrcFile, 0, PAGE_READONLY, 0, 0, 0);
    R_ASSERT3(hSrcMap != INVALID_HANDLE_VALUE, cFileName, xrDebug::ErrorToString(GetLastError()));

    data = (char*)MapViewOfFile(hSrcMap, FILE_MAP_READ, 0, 0, 0);
#elif defined(LINUX)
    hSrcFile = ::open(cFileName, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //за такое использование указателя нужно убивать, но пока пусть будет
    R_ASSERT3(hSrcFile != -1, cFileName, xrDebug::ErrorToString(GetLastError()));
    struct stat file_info;
    ::fstat(hSrcFile, &file_info);
    Size = (int)file_info.st_size;
    R_ASSERT3(Size, cFileName, xrDebug::ErrorToString(GetLastError()));
    data = (char*)::mmap(NULL, Size, PROT_READ, MAP_SHARED, hSrcFile, 0);
#endif
    R_ASSERT3(data, cFileName, xrDebug::ErrorToString(GetLastError()));

#ifdef FS_DEBUG
    register_file_mapping(data, Size, cFileName);
#endif // DEBUG
}

CVirtualFileReader::~CVirtualFileReader()
{
#ifdef FS_DEBUG
    unregister_file_mapping(data, Size);
#endif // DEBUG
#if defined(WINDOWS)
    UnmapViewOfFile((void*)data);
    CloseHandle(hSrcMap);
    CloseHandle(hSrcFile);
#elif defined(LINUX)
    ::munmap((void*)data, Size);
    ::close(hSrcFile);
    hSrcFile = -1;
#endif
}
