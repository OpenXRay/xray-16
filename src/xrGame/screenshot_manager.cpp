#include "StdAfx.h"
#include "screenshot_manager.h"
#include "Level.h"
#include "game_cl_mp.h"
#include "xrCore/Compression/ppmd_compressor.h"
#include "screenshots_writer.h"

#ifdef DEBUG
#define CXIMAGE_AS_SHARED_LIBRARY
#endif

#ifdef WINDOWS
#include <ddraw.h>
#endif

#include "ximage.h"
#include "xmemfile.h"

void* cxalloc(size_t size) { return xr_malloc(size); }
void cxfree(void* ptr) { xr_free(ptr); }
void* cxrealloc(void* ptr, size_t size) { return xr_realloc(ptr, size); }
/*
void jpeg_encode_callback(long progress)
{
#ifdef DEBUG
    Msg("* JPEG encoding progress : %d%%", progress);
#endif
    if (progress % 5 == 0)
    {
        if (!SwitchToThread())
            Sleep(10);
    }
}*/

screenshot_manager::screenshot_manager()
{
    m_state = 0;

    m_jpeg_buffer = NULL;
    m_jpeg_buffer_capacity = 0;

    m_buffer_for_compress = NULL;
    m_buffer_for_compress_capacity = 0;

    m_make_start_event = NULL;
    m_make_done_event = NULL;
}
screenshot_manager::~screenshot_manager()
{
    if (is_active())
    {
        Engine.Sheduler.Unregister(this);
        m_state = 0;
    }
    xr_free(m_jpeg_buffer);
    xr_free(m_buffer_for_compress);
    if (m_make_start_event)
    {
#ifndef LINUX // FIXME!!!
        SetEvent(m_make_start_event);
        WaitForSingleObject(m_make_done_event, INFINITE); // thread stoped
        CloseHandle(m_make_done_event);
        CloseHandle(m_make_start_event);
#endif
    }
}

void screenshot_manager::realloc_jpeg_buffer(u32 new_size)
{
    if (m_jpeg_buffer_capacity >= new_size)
        return;
    void* new_buffer = xr_realloc(m_jpeg_buffer, new_size);
    m_jpeg_buffer = static_cast<u8*>(new_buffer);
    m_jpeg_buffer_capacity = new_size;
}

#define RESULT_PIXEL_SIZE 3
#define STRING_SIZE (RESULT_PIXEL_SIZE * RESULT_WIDTH)
// method get the pixel
void screenshot_manager::prepare_image()
{
#pragma pack(push, 1)
    struct rgb24color
    {
        u8 r, g, b;
    };
#pragma pack(pop)
    typedef rgb24color rgb24map[RESULT_HEIGHT][RESULT_WIDTH];
    u32* sizes = reinterpret_cast<u32*>(m_result_writer.pointer());
    u32* width = sizes; // first dword is width
    u32* height = ++sizes; // second dword is height
    u32* rgba = reinterpret_cast<u32*>(++sizes); // then RGBA data
    rgb24map* dest = reinterpret_cast<rgb24map*>(rgba); // WARNING sorce and dest stored in one place ...

    float dx = float(*width) / RESULT_WIDTH;
    float dy = float(*height) / RESULT_HEIGHT;

    // removin alfa byte with resize(first pixel)
    for (int y = 0; y < RESULT_HEIGHT; ++y)
    {
        for (int x = 0; x < RESULT_WIDTH; ++x)
        {
            int rgba_index = (u32(dy * y) * (*width)) + u32(dx * x);
            (*dest)[y][x].r = *reinterpret_cast<u8*>(&rgba[rgba_index]);
            (*dest)[y][x].g = *(reinterpret_cast<u8*>(&rgba[rgba_index]) + 1);
            (*dest)[y][x].b = *(reinterpret_cast<u8*>(&rgba[rgba_index]) + 2);
        }
    }
    *width = RESULT_WIDTH;
    *height = RESULT_HEIGHT;
}

void screenshot_manager::make_jpeg_file()
{
    u32* sizes = reinterpret_cast<u32*>(m_result_writer.pointer());
    u32 width = *sizes;
    u32 height = *(++sizes);
    u8* rgb24data = reinterpret_cast<u8*>(m_result_writer.pointer() + 2 * sizeof(u32));

    CxImage jpg_image;

    jpg_image.CreateFromArray(rgb24data,
        width, // width
        height, // height
        24, width * 3, true);

    jpg_image.SetJpegQuality(30);

    realloc_jpeg_buffer(m_result_writer.size() + screenshots::writer::info_max_size);

    CxMemFile tmp_mem_file(m_jpeg_buffer, m_jpeg_buffer_capacity);
    jpg_image.Encode(&tmp_mem_file, CXIMAGE_FORMAT_JPG);

    m_jpeg_buffer_size = static_cast<u32>(tmp_mem_file.Tell());

#ifdef DEBUG
    Msg("* JPEG encoded to %d bytes", m_jpeg_buffer_size);
#endif
}

void screenshot_manager::sign_jpeg_file()
{
    screenshots::writer tmp_writer(m_jpeg_buffer, m_jpeg_buffer_size, m_jpeg_buffer_capacity);
    game_cl_mp* tmp_cl_game = smart_cast<game_cl_mp*>(&Game());
    tmp_writer.set_player_name(tmp_cl_game->local_player->getName());
    shared_str tmp_cdkey_digest = Level().get_cdkey_digest();
    if (tmp_cdkey_digest.size() == 0)
        tmp_cdkey_digest = "null";
    tmp_writer.set_player_cdkey_digest(tmp_cdkey_digest);
    m_jpeg_buffer_size = tmp_writer.write_info();
}

void screenshot_manager::shedule_Update(u32 dt)
{
    R_ASSERT(m_state & making_screenshot || m_state & drawing_download_states);
    bool is_make_in_progress = is_making_screenshot();
    if (is_make_in_progress && (m_defered_ssframe_counter == 0))
    {
        if (!m_make_done_event)
        {
            prepare_image();
            make_jpeg_file();
            sign_jpeg_file();
            compress_image();
            m_complete_callback(m_buffer_for_compress, m_buffer_for_compress_size, m_jpeg_buffer_size);
            m_state &= ~making_screenshot;
        }
        else
        {
#ifndef LINUX // FIXME!!!
            DWORD thread_result = WaitForSingleObject(m_make_done_event, 0);
            R_ASSERT((thread_result != WAIT_ABANDONED) && (thread_result != WAIT_FAILED));
            if (thread_result == WAIT_OBJECT_0)
            {
                m_complete_callback(m_buffer_for_compress, m_buffer_for_compress_size, m_jpeg_buffer_size);
                m_state &= ~making_screenshot;
            }
#endif
        }
        if (!is_making_screenshot() && !is_drawing_downloads())
        {
            Engine.Sheduler.Unregister(this);
        }
    }
    else if (is_make_in_progress && (--m_defered_ssframe_counter == 0))
    {
        GEnv.Render->ScreenshotAsyncEnd(m_result_writer);
        /*	//---------
        #ifdef DEBUG
                if (!m_result_writer.size())
                {
                    string_path screen_shot_path;
                    FS.update_path(screen_shot_path, "$screenshots$", "tmp_response.dds");
                    IReader* tmp_reader = FS.r_open(screen_shot_path);
                    if (tmp_reader)
                    {
                        m_result_writer.w(tmp_reader->pointer(), tmp_reader->length());
                        FS.r_close(tmp_reader);
                    }
                }
        #endif //#ifdef DEBUG*/
        ULONG_PTR process_affinity_mask, tmp_dword;
#ifndef LINUX // FIXME!!!
        GetProcessAffinityMask(GetCurrentProcess(), &process_affinity_mask, &tmp_dword);
        process_screenshot(btwCount1(static_cast<u32>(process_affinity_mask)) == 1);
#endif
    }
    if (is_drawing_downloads())
    {
        static_cast<game_cl_mp*>(Level().game)->draw_all_active_binder_states();
    }
}

void screenshot_manager::make_screenshot(complete_callback_t cb)
{
    if (is_making_screenshot())
    {
#ifdef DEBUG
        Msg("! ERROR: CL: screenshot making in progress...");
#endif
        return;
    }
    if (m_result_writer.size())
        m_result_writer.clear();

    m_complete_callback = cb;
    if (!is_drawing_downloads())
    {
        Engine.Sheduler.Register(this, TRUE);
    }
    m_state |= making_screenshot;
    m_defered_ssframe_counter = defer_framescount;

    GEnv.Render->ScreenshotAsyncBegin();
}

void screenshot_manager::set_draw_downloads(bool draw)
{
    if (draw)
    {
        if (!is_active())
        {
            Engine.Sheduler.Register(this, TRUE);
        }
        m_state |= drawing_download_states;
    }
    else
    {
        if (!is_making_screenshot() && is_drawing_downloads())
        {
            Engine.Sheduler.Unregister(this);
        }
        m_state &= ~drawing_download_states;
    }
}

void screenshot_manager::process_screenshot(bool singlecore)
{
#ifndef LINUX // FIXME!!!
    if (m_make_start_event)
    {
        SetEvent(m_make_start_event);
        return;
    }
    m_make_start_event = CreateEvent(NULL, FALSE, TRUE, NULL);
    m_make_done_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    thread_spawn(&screenshot_manager::screenshot_maker_thread, "screenshot_maker", 0, this);
#endif
}
void __stdcall screenshot_manager::jpeg_compress_cb(long progress)
{
    /*#ifdef DEBUG
        Msg("* JPEG encoding progress : %d%%", progress);
    #endif*/
    if (progress % 5 == 0)
    {
#ifndef LINUX // FIXME!!!
        if (!SwitchToThread())
            Sleep(10);
#endif
    }
}

void screenshot_manager::screenshot_maker_thread(void* arg_ptr)
{
    screenshot_manager* this_ptr = static_cast<screenshot_manager*>(arg_ptr);
#ifndef LINUX // FIXME!!
    DWORD wait_result = WaitForSingleObject(this_ptr->m_make_start_event, INFINITE);
    while ((wait_result != WAIT_ABANDONED) || (wait_result != WAIT_FAILED))
    {
        if (!this_ptr->is_active())
            break;
        this_ptr->timer_begin("preparing image");
        this_ptr->prepare_image();
        this_ptr->timer_end();
        this_ptr->timer_begin("making jpeg");
        this_ptr->make_jpeg_file();
        this_ptr->timer_end();
        this_ptr->timer_begin("signing jpeg data");
        this_ptr->sign_jpeg_file();
        this_ptr->timer_end();
        this_ptr->timer_begin("compressing_image");
        this_ptr->compress_image();
        this_ptr->timer_end();
        SetEvent(this_ptr->m_make_done_event);
        wait_result = WaitForSingleObject(this_ptr->m_make_start_event, INFINITE);
    }
    SetEvent(this_ptr->m_make_done_event);
#endif
}

void screenshot_manager::realloc_compress_buffer(u32 need_size)
{
    if (m_buffer_for_compress && (need_size <= m_buffer_for_compress_capacity))
        return;
#ifdef DEBUG
    Msg("* reallocing compression buffer.");
#endif
    m_buffer_for_compress_capacity = need_size * 2;
    void* new_buffer = xr_realloc(m_buffer_for_compress, m_buffer_for_compress_capacity);
    m_buffer_for_compress = static_cast<u8*>(new_buffer);
}

void screenshot_manager::compress_image()
{
    realloc_compress_buffer(m_jpeg_buffer_size);

    m_buffer_for_compress_size =
        ppmd_compress(m_buffer_for_compress, m_buffer_for_compress_capacity, m_jpeg_buffer, m_jpeg_buffer_size);
}

#ifdef DEBUG

void screenshot_manager::timer_begin(LPCSTR comment)
{
    m_timer_comment = comment;
    m_debug_timer.Start();
}

void screenshot_manager::timer_end() { Msg("* %s : %u ms", m_timer_comment.c_str(), m_debug_timer.GetElapsed_ms()); }
#endif
