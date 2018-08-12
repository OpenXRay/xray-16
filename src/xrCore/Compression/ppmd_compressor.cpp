#include "stdafx.h"
#include "ppmd_compressor.h"
#include "PPMd.h"
#include "SDL.h"

#if defined(LINUX)


LONG InterlockedExchange(LONG volatile *dest, LONG val)
{
       LONG ret;
    __asm__ __volatile__( "lock; xchg %0,(%1)" : "=r" (ret) :"r" (dest), "0" (val) : "memory" );
    return ret;
}
#endif

const u32 suballocator_size = 32;
const u32 order_model = 8;
const MR_METHOD restoration_method_cut_off = MRM_RESTART;

typedef compression::ppmd::stream stream;

extern compression::ppmd::stream* trained_model;

void _STDCALL PrintInfo(_PPMD_FILE* DecodedFile, _PPMD_FILE* EncodedFile) {}
static LONG PPMd_Locked = 0;

static inline void PPMd_Lock()
{
    while (::InterlockedExchange(&PPMd_Locked, 1))
        SDL_Delay(0);
}

static inline void PPMd_Unlock() { ::InterlockedExchange(&PPMd_Locked, 0); }
void ppmd_initialize()
{
    if (trained_model)
        trained_model->rewind();

    static bool initialized = false;
    if (initialized)
        return;

    initialized = true;
    if (StartSubAllocator(suballocator_size))
        return;

    exit(-1);
}

u32 ppmd_compress(
    void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer, const u32& source_buffer_size)
{
    PPMd_Lock();
    ppmd_initialize();

    stream source(source_buffer, source_buffer_size);
    stream dest(dest_buffer, dest_buffer_size);
    EncodeFile(&dest, &source, order_model, restoration_method_cut_off);

    PPMd_Unlock();
    return (dest.tell() + 1);
}

u32 ppmd_trained_compress(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, compression::ppmd::stream* tmodel)
{
    PPMd_Lock();

    stream* old_stream = trained_model;
    trained_model = tmodel;

    ppmd_initialize();

    stream source(source_buffer, source_buffer_size);
    stream dest(dest_buffer, dest_buffer_size);
    EncodeFile(&dest, &source, order_model, restoration_method_cut_off);

    trained_model = old_stream;

    PPMd_Unlock();
    return (dest.tell() + 1);
}

u32 ppmd_decompress(
    void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer, const u32& source_buffer_size)
{
    PPMd_Lock();
    ppmd_initialize();

    stream source(source_buffer, source_buffer_size);
    stream dest(dest_buffer, dest_buffer_size);
    DecodeFile(&dest, &source, order_model, restoration_method_cut_off);

    PPMd_Unlock();
    return (dest.tell());
}

u32 ppmd_trained_decompress(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, compression::ppmd::stream* tmodel)
{
    PPMd_Lock();

    stream* old_stream = trained_model;
    trained_model = tmodel;

    ppmd_initialize();

    stream source(source_buffer, source_buffer_size);
    stream dest(dest_buffer, dest_buffer_size);
    DecodeFile(&dest, &source, order_model, restoration_method_cut_off);

    trained_model = old_stream;

    PPMd_Unlock();
    return (dest.tell());
}

static const u32 compress_chunk_size = 100 * 1024; // 100 kb

u32 ppmd_compress_mt(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, ppmd_yield_callback_t ycb)
{
    PPMd_Lock();
    ppmd_initialize();

    u32 rest_src_buff = source_buffer_size;
    u8 const* src_buff_chunk = static_cast<u8 const*>(source_buffer);

    u8* curr_dst_buff = static_cast<u8*>(dest_buffer);
    u32 dst_buff_size = dest_buffer_size;

    u32 result_size = 0;

    while (rest_src_buff)
    {
        u32 to_compress = rest_src_buff > compress_chunk_size ? compress_chunk_size : rest_src_buff;

        stream source(src_buff_chunk, to_compress);
        stream dest(curr_dst_buff, dst_buff_size);
        EncodeFile(&dest, &source, order_model, restoration_method_cut_off);

        u32 dst_encoded = dest.tell();
        curr_dst_buff += dst_encoded;
        R_ASSERT(dest_buffer_size >= dst_encoded);
        dst_buff_size -= dst_encoded;
        result_size += dst_encoded;

        src_buff_chunk += to_compress;
        rest_src_buff -= to_compress;
        if (ycb)
            ycb();
    }

    PPMd_Unlock();
    return result_size;
}

u32 ppmd_decompress_mt(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, ppmd_yield_callback_t ycb)
{
    PPMd_Lock();
    ppmd_initialize();

    u32 rest_src_buff = source_buffer_size;
    u8 const* src_buff_chunk = static_cast<u8 const*>(source_buffer);

    u8* curr_dst_buff = static_cast<u8*>(dest_buffer);
    u32 dst_buff_size = dest_buffer_size;

    u32 result_size = 0;

    while (rest_src_buff)
    {
        stream source(src_buff_chunk, rest_src_buff);
        stream dest(curr_dst_buff, dst_buff_size);
        DecodeFile(&dest, &source, order_model, restoration_method_cut_off);

        u32 src_decoded = source.tell();
        src_buff_chunk += src_decoded;
        R_ASSERT(rest_src_buff >= src_decoded);
        rest_src_buff -= src_decoded;

        u32 unpacked = dest.tell();
        curr_dst_buff += unpacked;
        R_ASSERT(dst_buff_size >= unpacked);
        dst_buff_size -= unpacked;

        result_size += unpacked;

        if (ycb)
            ycb();
    }

    PPMd_Unlock();
    return result_size;
}
