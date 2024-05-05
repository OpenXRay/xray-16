#include "stdafx.h"
//#include "xr_effgamma.h"
#include "xrCore/Media/Image.hpp"
#include "xrEngine/xrImage_Resampler.h"

using namespace XRay::Media;

#define GAMESAVE_SIZE 128

#define SM_FOR_SEND_WIDTH 640
#define SM_FOR_SEND_HEIGHT 480

// XXX: Provide full implementation
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
        // XXX: Implement
        break;

    default:
        VERIFY(!"CRender::Screenshot. This screenshot type is not supported for OGL.");
    }
}

void CRender::ScreenshotAsyncEnd(CMemoryWriter &memory_writer)
{
    // TODO: OGL: Implement screenshot feature.
    VERIFY(!"CRender::ScreenshotAsyncEnd not implemented.");
}
