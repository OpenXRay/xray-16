#include "stdafx.h"
#include "lzo_compressor.h"
#include "rt_lzo1x.h"

int lzo_compress_dict(const void *input, u32 inputSize, void *output, u32 &outputSize,
    void *workMem, const void *dict, u32 dictSize)
{
    return lzo1x_999_compress_dict((lzo_bytep)input, inputSize, (lzo_bytep)output, (lzo_uintp)&outputSize,
        workMem, (lzo_bytep)dict, dictSize);
}

int lzo_decompress_dict(const void *input, u32 inputSize, void *output, u32 &outputSize,
    void *workMem, const void *dict, u32 dictSize)
{
    return lzo1x_decompress_dict_safe((lzo_bytep)input, inputSize, (lzo_bytep)output, (lzo_uintp)&outputSize,
        workMem, (lzo_bytep)dict, dictSize);
}

int lzo_initialize()
{ return lzo_init(); }

u32 lzo_get_workmem_size()
{ return LZO1X_999_MEM_COMPRESS; }
