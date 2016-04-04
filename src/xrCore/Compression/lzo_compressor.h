#pragma once
#include "xrCore/xrCore.h"

XRCORE_API int lzo_compress_dict(const void *input, u32 inputSize, void *output, u32 &outputSize,
    void *workMem, const void *dict, u32 dictSize);

XRCORE_API int lzo_decompress_dict(const void *input, u32 inputSize, void *output, u32 &outputSize,
    void *workMem, const void *dict, u32 dict_len);

XRCORE_API int lzo_initialize();

XRCORE_API u32 lzo_get_workmem_size();
