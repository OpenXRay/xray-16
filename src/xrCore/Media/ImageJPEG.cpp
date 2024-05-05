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
    explicit xr_jpeg_error_mgr(jpeg_compress_struct& info)
    {
        info.err = jpeg_std_error(this);
        this->error_exit = on_error_exit;
    }

    explicit xr_jpeg_error_mgr(jpeg_decompress_struct& info)
    {
        info.err = jpeg_std_error(this);
        this->error_exit = on_error_exit;
    }

    auto get_jmp_buffer()
    {
        return setjmp_buffer;
    }

private:
    static void on_error_exit(j_common_ptr info)
    {
        auto& self = *reinterpret_cast<xr_jpeg_error_mgr*>(info->err);

        char buffer[JMSG_LENGTH_MAX];
        self.format_message(info, buffer);
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
    xr_jpeg_destination_mgr(jpeg_compress_struct& info, IWriter& writer) : m_writer(writer)
    {
        next_output_byte = nullptr;
        free_in_buffer = 0;

        init_destination = initialize_destination;
        empty_output_buffer = write_buffer;
        term_destination = finalize_destination;

        info.dest = this;
    }

private:
    static void initialize_destination(j_compress_ptr info)
    {
        auto& self = *reinterpret_cast<xr_jpeg_destination_mgr*>(info->dest);
        self.next_output_byte = self.m_buffer;
        self.free_in_buffer = OUT_BUFFER_SIZE;
    }

    static boolean write_buffer(j_compress_ptr info)
    {
        auto& self = *reinterpret_cast<xr_jpeg_destination_mgr*>(info->dest);

        self.m_writer.w(self.m_buffer, OUT_BUFFER_SIZE);

        self.next_output_byte = self.m_buffer;
        self.free_in_buffer = OUT_BUFFER_SIZE;

        return TRUE;
    }

    static void finalize_destination(j_compress_ptr info)
    {
        const auto& self = *reinterpret_cast<xr_jpeg_destination_mgr*>(info->dest);

        const size_t data_left = OUT_BUFFER_SIZE - self.free_in_buffer;

        if (data_left > 0)
        {
            self.m_writer.w(self.m_buffer, data_left);
        }
    }
};
#endif // JPEGLIB_H

bool Image::OpenJPEG(const IReader& reader)
{
    return OpenJPEG(static_cast<u8*>(reader.pointer()), static_cast<unsigned long>(reader.elapsed()));
}

bool Image::OpenJPEG(const u8* dataPtr, u32 dataSize)
{
#ifdef JPEGLIB_H
    jpeg_decompress_struct info;
    xr_jpeg_error_mgr jerr(info);

    // Setup error handling
    if (setjmp(jerr.get_jmp_buffer()))
    {
        // If we get here, the JPEG code has signaled an error.
        jpeg_destroy_decompress(&info);
        return false;
    }

    jpeg_create_decompress(&info);
    {
        jpeg_mem_src(&info, dataPtr, dataSize);
        jpeg_read_header(&info, true);

        width  = info.image_width;
        height = info.image_height;
        channelCount = info.num_components;

        switch (channelCount)
        {
        case 3: format = ImageDataFormat::RGB8; break;
        case 4: format = ImageDataFormat::RGBA8; break;
        }

        info.out_color_space = JCS_RGB;

        jpeg_start_decompress(&info);
        {
            const auto size = width * height * channelCount;
            data = xr_malloc(size);
            ownsData = true;

            JSAMPROW buf[1];

            while (info.output_scanline < info.output_height)
            {
                buf[0] = JSAMPROW(&((u8*)data)[channelCount * info.output_width * info.output_scanline]);
                jpeg_read_scanlines(&info, buf, 1);
            }
        }
        jpeg_finish_decompress(&info);
    }
    jpeg_destroy_decompress(&info);
    return true;
#else
    Msg("~ %s: Engine was built without libjpeg.", __FUNCTION__);
    return false;
#endif
}

bool Image::SaveJPEG(IWriter& writer, int quality, bool invert /*= false*/)
{
    clamp(quality, 0, 100);

#ifdef JPEGLIB_H
#   if !defined(JCS_EXTENSIONS) && !defined(JCS_ALPHA_EXTENSIONS)
    if (format != ImageDataFormat::RGB8)
    {
        Msg("! %s: Unsupported data format", __FUNCTION__);
        return false;
    }
#   endif

    jpeg_compress_struct info;
    xr_jpeg_error_mgr jerr(info);

    // Setup error handling
    if (setjmp(jerr.get_jmp_buffer()))
    {
        // If we get here, the JPEG code has signaled an error.
        jpeg_destroy_compress(&info);
        return false;
    }

    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&info);
    xr_jpeg_destination_mgr cdest(info, writer);
    {
        info.image_width = width;
        info.image_height = height;
        info.input_components = channelCount;

        switch (channelCount)
        {
        default:
        case 3: info.in_color_space = JCS_RGB; break;

#   if defined(JCS_EXTENSIONS) && defined(JCS_ALPHA_EXTENSIONS)
        case 4: info.in_color_space = JCS_EXT_RGBA; break;
#   endif
        }

        jpeg_set_defaults(&info);
        jpeg_set_quality(&info, quality, TRUE);

        jpeg_start_compress(&info, TRUE);
        {
            JSAMPLE* image_data = static_cast<JSAMPLE*>(data);
            const auto row_stride = width * channelCount; /* JSAMPLEs per row in image_buffer */

            while (info.next_scanline < info.image_height)
            {
                const auto scanline = invert ? info.image_height - 1 - info.next_scanline : info.next_scanline;
                auto row_pointer = &image_data[scanline * row_stride];
                (void)jpeg_write_scanlines(&info, &row_pointer, 1);
            }
        }
        jpeg_finish_compress(&info);
    }
    jpeg_destroy_compress(&info);
    return true;
#else
    std::ignore = writer;
    std::ignore = invert;

    Msg("~ %s: Engine was built without libjpeg.", __FUNCTION__);
    return false;
#endif // JPEGLIB_H
}
} // namespace XRay::Media
