#include "src/Layers/xrRenderGL/glTextureView.h"
#include <gli/gl.hpp>
#include <cassert>
#include <cstdio>

int main()
{
    const gli::gl gl(gli::gl::PROFILE_ES30);
    for (auto format : {gli::FORMAT_BGRA8_UNORM_PACK8, gli::FORMAT_BGR8_UNORM_PACK8,
        gli::FORMAT_RGBA8_UNORM_PACK8, gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16})
    {
        gli::texture source(gli::TARGET_2D, format, {8, 8, 1}, 1, 1, 4);
        const auto original = gl.translate(source.format(), source.swizzles());
        for (size_t level = 0; level < source.levels(); ++level)
        {
            const auto view = xray::render::texture_mip_view(source, level);
            const auto selected = gl.translate(view.format(), view.swizzles());
            assert(view.data() == source.data(0, 0, level));
            assert(view.levels() == source.levels() - level);
            assert(view.extent().x == (8 >> level));
            for (int channel = 0; channel < 4; ++channel)
                assert(selected.Swizzles[channel] == original.Swizzles[channel]);
        }
    }
    puts("Texture mip views preserve BGRA, BGR, RGBA and BC3 channel mappings");
}
