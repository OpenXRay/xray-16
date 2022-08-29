#include "stdafx.h"
#include "Image.hpp"

#if __has_include(<jpeglib.h>)
#   include <jpeglib.h>
#   undef HAVE_STDDEF_H // XXX: warning workaround

#   include <setjmp.h>
#endif

namespace XRay::Media
{
#ifdef JPEGLIB_H
class xr_jpeg_error_mgr final : public jpeg_error_mgr
{
    jmp_buf setjmp_buffer; // for return to caller

public:
    xr_jpeg_error_mgr(jpeg_compress_struct& cinfo)
    {
        cinfo.err = jpeg_std_error(this);
        this->error_exit = on_error_exit;
    }

    auto get_jmp_buffer()
    {
        return setjmp_buffer;
    }

private:
    static void on_error_exit(j_common_ptr cinfo)
    {
        auto& self = *reinterpret_cast<xr_jpeg_error_mgr*>(cinfo->err);

        char buffer[JMSG_LENGTH_MAX];
        self.format_message(cinfo, buffer);
        Msg("! JPEG fail: %s", buffer);

        longjmp(self.setjmp_buffer, 1);
    }
};

class xr_jpeg_destination_mgr final : public jpeg_destination_mgr
{
    static constexpr size_t OUT_BUFFER_SIZE = 4096;

    u8 m_buffer[OUT_BUFFER_SIZE];
    IWriter& m_writer;

public:
    xr_jpeg_destination_mgr(jpeg_compress_struct& cinfo, IWriter& writer) : m_writer(writer)
    {
        next_output_byte = nullptr;
        free_in_buffer = 0;

        init_destination = initialize_destination;
        empty_output_buffer = write_buffer;
        term_destination = finalize_destination;

        cinfo.dest = this;
    }

private:
    static void initialize_destination(j_compress_ptr cinfo)
    {
        auto& self = *reinterpret_cast<xr_jpeg_destination_mgr*>(cinfo->dest);
        self.next_output_byte = self.m_buffer;
        self.free_in_buffer = OUT_BUFFER_SIZE;
    }

    static boolean write_buffer(j_compress_ptr cinfo)
    {
        auto& self = *reinterpret_cast<xr_jpeg_destination_mgr*>(cinfo->dest);

        self.m_writer.w(self.m_buffer, OUT_BUFFER_SIZE);

        self.next_output_byte = self.m_buffer;
        self.free_in_buffer = OUT_BUFFER_SIZE;

        return TRUE;
    }

    static void finalize_destination(j_compress_ptr cinfo)
    {
        auto& self = *reinterpret_cast<xr_jpeg_destination_mgr*>(cinfo->dest);

        const size_t data_left = OUT_BUFFER_SIZE - self.free_in_buffer;

        if (data_left > 0)
        {
            self.m_writer.w(self.m_buffer, data_left);
        }
    }
};
#endif // JPEGLIB_H


bool Image::SaveJPEG(IWriter& writer, int quality, bool invert /*= false*/)
{
    clamp(quality, 0, 100);

    if (format == ImageDataFormat::RGBA8)
    {
        Msg("! %s: Unsupported data format", __FUNCTION__);
        return false;
    }

#ifdef JPEGLIB_H
    jpeg_compress_struct cinfo;
    xr_jpeg_error_mgr jerr(cinfo);

    // Setup error handling
    if (setjmp(jerr.get_jmp_buffer()))
    {
        // If we get here, the JPEG code has signaled an error.
        jpeg_destroy_compress(&cinfo);
        return false;
    }

    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);
    xr_jpeg_destination_mgr cdest(cinfo, writer);
    {
        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);
        {
            JSAMPLE* image_data = static_cast<JSAMPLE*>(data);
            const auto row_stride = width * 3; /* JSAMPLEs per row in image_buffer */

            while (cinfo.next_scanline < cinfo.image_height)
            {
                const auto scanline = invert ? cinfo.image_height - 1 - cinfo.next_scanline : cinfo.next_scanline;
                auto row_pointer = &image_data[scanline * row_stride];
                (void)jpeg_write_scanlines(&cinfo, &row_pointer, 1);
            }
        }
        jpeg_finish_compress(&cinfo);
    }
    jpeg_destroy_compress(&cinfo);
    return true;
#else
    Msg("~ %s: Engine was build without libjpeg.", __FUNCTION__);
    return false;
#endif // JPEGLIB_H
}
} // namespace XRay::Media
