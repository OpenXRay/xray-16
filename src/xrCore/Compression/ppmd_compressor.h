#ifndef PPMD_COMPRESSOR_H
#define PPMD_COMPRESSOR_H

#include "xrCore/fastdelegate.h"

namespace compression
{
namespace ppmd
{
class stream;
}
}

XRCORE_API u32 ppmd_compress(
    void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer, const u32& source_buffer_size);
XRCORE_API u32 ppmd_trained_compress(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, compression::ppmd::stream* tmodel);
XRCORE_API u32 ppmd_decompress(
    void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer, const u32& source_buffer_size);
XRCORE_API u32 ppmd_trained_decompress(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, compression::ppmd::stream* tmodel);

typedef fastdelegate::FastDelegate<void()> ppmd_yield_callback_t;
XRCORE_API u32 ppmd_compress_mt(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, ppmd_yield_callback_t ycb);
XRCORE_API u32 ppmd_decompress_mt(void* dest_buffer, const u32& dest_buffer_size, const void* source_buffer,
    const u32& source_buffer_size, ppmd_yield_callback_t ycb);

#endif // PPMD_COMPRESSOR_H
