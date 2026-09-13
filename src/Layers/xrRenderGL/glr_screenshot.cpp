#include "stdafx.h"

//#include "xr_effgamma.h"
#include "xrCore/Media/Image.hpp"
#include "xrEngine/xrImage_Resampler.h"

#include <gli/gli.hpp>

namespace xray::render::RENDER_NAMESPACE
{
using namespace XRay::Media;

#define GAMESAVE_SIZE 128

#define SM_FOR_SEND_WIDTH 640
#define SM_FOR_SEND_HEIGHT 480

void CRender::Screenshot(ScreenshotMode mode /*= SM_NORMAL*/, pcstr name /*= nullptr*/)
{
    switch (mode)
    {
    case SM_NORMAL:
    {
        pcstr extension = "jpg";

        string64 time;
        string_path buf;
        xr_sprintf(buf, sizeof(buf), "ss_%s_%s_(%s).%s", Core.UserName, timestamp(time),
            g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu", extension);

        IWriter* fs = FS.w_open("$screenshots$", buf);
        R_ASSERT(fs);

        xr_vector<u8> pixels;
        pixels.resize(Device.dwWidth * Device.dwHeight * 3);

        glReadPixels(0, 0, Device.dwWidth, Device.dwHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        Image img{ Device.dwWidth, Device.dwHeight, pixels.data(), ImageDataFormat::RGB8 };
        if (!img.SaveJPEG(*fs, 100, true))
            Log("! Failed to make a screenshot.");

        FS.w_close(fs);
        break;
    }

    case SM_FOR_GAMESAVE:
    {
        VERIFY(name);

        xr_vector<u32> pixels(Device.dwWidth * Device.dwHeight);
        glReadPixels(0, 0, Device.dwWidth, Device.dwHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // OpenGL returns the bottom row first. DDS textures use the top row first.
        const size_t rowSize = Device.dwWidth;
        for (u32 y = 0; y < Device.dwHeight / 2; ++y)
        {
            auto top = pixels.begin() + y * rowSize;
            auto bottom = pixels.begin() + (Device.dwHeight - y - 1) * rowSize;
            std::swap_ranges(top, top + rowSize, bottom);
        }

        xr_vector<u32> resized(GAMESAVE_SIZE * GAMESAVE_SIZE);
        imf_Process(resized.data(), GAMESAVE_SIZE, GAMESAVE_SIZE, pixels.data(),
            Device.dwWidth, Device.dwHeight, imf_box);

        gli::texture2d texture(gli::FORMAT_RGBA8_UNORM_PACK8,
            gli::texture2d::extent_type(GAMESAVE_SIZE, GAMESAVE_SIZE), 1);
        std::memcpy(texture.data(), resized.data(), resized.size() * sizeof(resized.front()));

        std::vector<char> encoded;
        if (!gli::save_dds(texture, encoded))
        {
            Log("! Failed to encode the game-save screenshot.");
            break;
        }

        IWriter* fs = FS.w_open(name);
        if (!fs)
        {
            Msg("! Failed to open the game-save screenshot file: %s", name);
            break;
        }

        fs->w(encoded.data(), encoded.size());
        FS.w_close(fs);
        break;
    }

    default:
        VERIFY(!"CRender::Screenshot. This screenshot type is not supported for OGL.");
    }
}
} // namespace xray::render::RENDER_NAMESPACE
