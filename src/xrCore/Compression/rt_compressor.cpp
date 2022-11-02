#include "stdafx.h"
#pragma hdrstop

#include "lzo/lzo1x.h"

#define HEAP_ALLOC(var, size) lzo_align_t __LZO_MMODEL var[((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)]

__declspec(thread) HEAP_ALLOC(rtc_wrkmem, LZO1X_1_MEM_COMPRESS);

void rtc_initialize() { VERIFY(lzo_init() == LZO_E_OK); }
u32 rtc_csize(u32 in)
{
    VERIFY(in);
    return in + in / 64 + 16 + 3;
}

size_t rtc_compress(void* dst, size_t dst_len, const void* src, size_t src_len)
{
    lzo_uint out_size = dst_len;
    [[maybe_unused]] int r = lzo1x_1_compress((const lzo_byte*)src, (lzo_uint)src_len, (lzo_byte*)dst, (lzo_uintp)&out_size, rtc_wrkmem);
    VERIFY(r == LZO_E_OK);
    return out_size;
}
size_t rtc_decompress(void* dst, size_t dst_len, const void* src, size_t src_len)
{
    lzo_uint out_size = dst_len;
    [[maybe_unused]] int r = lzo1x_decompress((const lzo_byte*)src, (lzo_uint)src_len, (lzo_byte*)dst, (lzo_uintp)&out_size, rtc_wrkmem);
    VERIFY(r == LZO_E_OK);
    return out_size;
}
