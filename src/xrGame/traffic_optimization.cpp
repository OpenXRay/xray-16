#include "StdAfx.h"
#include "traffic_optimization.h"

namespace compression
{
bool init_ppmd_trained_stream(ppmd_trained_stream*& dest)
{
    VERIFY(dest == NULL);
    string_path file_name;
    FS.update_path(file_name, "$game_config$", "mp" DELIMITER "ppmd_updates.mdl");
    if (!FS.exist(file_name))
    {
        Log("! Can't open trained ppmd stream with path:", file_name);
        dest = nullptr;
        return false;
    }
    IReader* reader = FS.r_open(file_name);
    R_ASSERT(reader);

    const size_t buffer_size = reader->length();
    u8* buffer = (u8*)xr_malloc(buffer_size);
    reader->r(buffer, buffer_size);
    FS.r_close(reader);

    dest = new compression::ppmd::stream(buffer, buffer_size);
    return true;
}

void deinit_ppmd_trained_stream(ppmd_trained_stream*& src)
{
    VERIFY(src);
    src->rewind();
    u8* buffer = src->buffer();
    xr_free(buffer);
    xr_delete(src);
}

bool init_lzo(u8*& dest_wm, u8*& wm_buffer, lzo_dictionary_buffer& dest_dict)
{
    string_path file_name;
    FS.update_path(file_name, "$game_config$", "mp" DELIMITER "lzo_updates.dic");
    if (!FS.exist(file_name))
    {
        Log("! Can't open lzo dictionary with path:", file_name);
        dest_wm = nullptr;
        wm_buffer = nullptr;
        return false;
    }
    IReader* reader = FS.r_open(file_name);
    R_ASSERT(reader);

    const size_t buffer_size = reader->length();
    u8* buffer = (u8*)xr_malloc(buffer_size);
    reader->r(buffer, buffer_size);
    FS.r_close(reader);

    dest_dict.data = buffer;
    dest_dict.size = buffer_size;

    lzo_initialize();
    wm_buffer = static_cast<u8*>(xr_malloc(lzo_get_workmem_size() + 16));
    // buffer must be alligned to 16 bytes
    dest_wm = (u8*)(size_t(wm_buffer + 16) & ~0xf);
    return true;
}

void deinit_lzo(u8*& src_wm_buffer, lzo_dictionary_buffer& src_dict)
{
    xr_free(src_wm_buffer);
    xr_free(src_dict.data);
}

} // namespace compression
