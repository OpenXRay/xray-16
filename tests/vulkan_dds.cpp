#include "src/Layers/xrRenderVK/DdsTexture.h"
#include <gli/texture.hpp>
#include <gli/save_dds.hpp>
#include <cassert>
#include <cstring>
using namespace xray::render::vulkan;
int main()
{
    for (bool cube : {false, true})
    for (auto format : {gli::FORMAT_RGBA8_UNORM_PACK8, gli::FORMAT_RGBA8_SRGB_PACK8,
        gli::FORMAT_BGRA8_UNORM_PACK8, gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16,
        gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16})
    {
        gli::texture original(cube ? gli::TARGET_CUBE : gli::TARGET_2D, format, {8,8,1}, 1, cube ? 6 : 1, 4);
        for (size_t face=0; face<original.faces(); ++face)
            for (size_t level=0; level<4; ++level)
                std::memset(original.data(0,face,level), int(face*16+level), original.size(level));
        std::vector<char> bytes;
        assert(gli::save_dds(original,bytes));
        DdsTexture decoded;
        std::string error;
        assert(decode_dds(bytes.data(),bytes.size(),true,decoded,error));
        assert(decoded.cube==cube && decoded.mip_levels==4 && decoded.layers==original.faces());
        assert(decoded.copies.size()==original.faces()*4);
        for (const auto& copy : decoded.copies)
        {
            const auto level=copy.imageSubresource.mipLevel, face=copy.imageSubresource.baseArrayLayer;
            assert(!std::memcmp(decoded.pixels.data()+copy.bufferOffset, original.data(0,face,level), original.size(level)));
        }
        assert(!decode_dds(bytes.data(),bytes.size()-1,true,decoded,error));
        if (format==gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16 || format==gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16)
        {
            assert(decode_dds(bytes.data(),bytes.size(),false,decoded,error));
            assert(decoded.format==(format==gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16 ?
                VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM));
            assert(decoded.copies.size()==original.faces()*4);
        }
    }
}
