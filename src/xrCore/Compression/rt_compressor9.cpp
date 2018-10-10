#include "stdafx.h"
#pragma hdrstop

#include "lzo/lzo1x.h"

//==============================================================================

#define HEAP_ALLOC(var, size) lzo_align_t __LZO_MMODEL var[((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)]

__declspec(thread) HEAP_ALLOC(rtc9_wrkmem, LZO1X_999_MEM_COMPRESS);

static u8* _LZO_Dictionary = NULL;
static u32 _LZO_DictionarySize = 0;

//------------------------------------------------------------------------------

void rtc9_initialize()
{
    static bool initialized = false;

    if (initialized)
        return;

    VERIFY(lzo_init() == LZO_E_OK);

    string_path file_name;

    FS.update_path(file_name, "$game_config$", "mp" DELIMITER "lzo-dict.bin");

    if (FS.exist(file_name))
    {
        IReader* reader = FS.r_open(file_name);

        R_ASSERT(reader);

        _LZO_DictionarySize = reader->length();
        _LZO_Dictionary = (u8*)xr_malloc(_LZO_DictionarySize);

        reader->r(_LZO_Dictionary, _LZO_DictionarySize);
        FS.r_close(reader);

        Msg("using LZO-dictionary \"%s\"", file_name);
    }
    else
    {
        Msg("\"%s\" not found", file_name);
    }

    initialized = true;
}

//------------------------------------------------------------------------------

void rtc9_uninitialize()
{
    if (_LZO_Dictionary)
    {
        xr_free(_LZO_Dictionary);

        _LZO_Dictionary = NULL;
        _LZO_DictionarySize = 0;
    }
}

//------------------------------------------------------------------------------

u32 rtc9_csize(u32 in)
{
    VERIFY(in);
    return in + in / 64 + 16 + 3;
}

//------------------------------------------------------------------------------

u32 rtc9_compress(void* dst, u32 dst_len, const void* src, u32 src_len)
{
    u32 out_size = dst_len;
    int r = LZO_E_ERROR;

    rtc9_initialize();

    if (_LZO_Dictionary)
    {
        r = lzo1x_999_compress_dict((const lzo_byte*)src, (lzo_uint)src_len, (lzo_byte*)dst, (lzo_uintp)&out_size,
            rtc9_wrkmem, _LZO_Dictionary, _LZO_DictionarySize);
    }
    else
    {
        r = lzo1x_999_compress(
            (const lzo_byte*)src, (lzo_uint)src_len, (lzo_byte*)dst, (lzo_uintp)&out_size, rtc9_wrkmem);
    }

    VERIFY(r == LZO_E_OK);

    return out_size;
}

//------------------------------------------------------------------------------

u32 rtc9_decompress(void* dst, u32 dst_len, const void* src, u32 src_len)
{
    u32 out_size = dst_len;
    int r = LZO_E_ERROR;

    rtc9_initialize();

    if (_LZO_Dictionary)
    {
        r = lzo1x_decompress_dict_safe((const lzo_byte*)src, (lzo_uint)src_len, (lzo_byte*)dst, (lzo_uintp)&out_size,
            NULL, _LZO_Dictionary, _LZO_DictionarySize);
    }
    else
    {
        r = lzo1x_decompress((const lzo_byte*)src, (lzo_uint)src_len, (lzo_byte*)dst, (lzo_uintp)&out_size, NULL);
    }

    VERIFY(r == LZO_E_OK);

    return out_size;
}
