// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <gli/gli.hpp>

#if defined(XR_PLATFORM_ANDROID)
#include "xrEngine/x_ray.h"
#endif

namespace xray::render::RENDER_NAMESPACE
{
void fix_texture_name(pstr fn)
{
    pstr _ext = strext(fn);
    if (_ext &&
        (0 == xr_stricmp(_ext, ".tga") ||
            0 == xr_stricmp(_ext, ".dds") ||
            0 == xr_stricmp(_ext, ".bmp") ||
            0 == xr_stricmp(_ext, ".ogm")))
        *_ext = 0;
}

int get_texture_load_lod(LPCSTR fn)
{
    CInifile::Sect& sect = pSettings->r_section("reduce_lod_texture_list");

    for (const auto& item : sect.Data)
    {
        if (strstr(fn, item.first.c_str()))
        {
            if (psTextureLOD < 1)
                return 0;
            if (psTextureLOD < 3)
                return 1;
            return 2;
        }
    }

    if (psTextureLOD < 2)
        return 0;
    if (psTextureLOD < 4)
        return 1;
    return 2;
}

u32 calc_texture_size(int lod, u32 mip_cnt, size_t orig_size)
{
    if (1 == mip_cnt)
        return orig_size;

    int _lod = lod;
    float res = float(orig_size);

    while (_lod > 0)
    {
        --_lod;
        res -= res / 1.333f;
    }
    return iFloor(res);
}

#if defined(XR_PLATFORM_ANDROID)
void clear_texture_gl_errors()
{
    while (glGetError() != GL_NO_ERROR)
    {
    }
}

bool check_texture_gl_error(cpcstr operation, cpcstr filename)
{
    bool valid = true;
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
    {
        Msg("! OpenGL ES: 0x%x during %s for texture '%s'", error, operation, filename);
        valid = false;
    }
    return valid;
}

enum class AndroidBcFormat
{
    Unsupported,
    Bc1,
    Bc2,
    Bc3
};

struct AndroidRgba8
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

AndroidBcFormat android_bc_format(gli::format format)
{
    switch (format)
    {
    case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8:
    case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8:
    case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8:
    case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8:
        return AndroidBcFormat::Bc1;
    case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16:
    case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16:
        return AndroidBcFormat::Bc2;
    case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
    case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16:
        return AndroidBcFormat::Bc3;
    default:
        return AndroidBcFormat::Unsupported;
    }
}

u16 read_bc_u16(const u8* data)
{
    return static_cast<u16>(data[0]) | (static_cast<u16>(data[1]) << 8);
}

u32 read_bc_u32(const u8* data)
{
    return static_cast<u32>(data[0]) |
        (static_cast<u32>(data[1]) << 8) |
        (static_cast<u32>(data[2]) << 16) |
        (static_cast<u32>(data[3]) << 24);
}

AndroidRgba8 decode_565(u16 color)
{
    const u8 r5 = static_cast<u8>((color >> 11) & 31);
    const u8 g6 = static_cast<u8>((color >> 5) & 63);
    const u8 b5 = static_cast<u8>(color & 31);
    return {
        static_cast<u8>((r5 << 3) | (r5 >> 2)),
        static_cast<u8>((g6 << 2) | (g6 >> 4)),
        static_cast<u8>((b5 << 3) | (b5 >> 2)),
        255
    };
}

AndroidRgba8 interpolate_bc_color(const AndroidRgba8& a, const AndroidRgba8& b,
    u32 aWeight, u32 bWeight, u32 divisor)
{
    return {
        static_cast<u8>((a.r * aWeight + b.r * bWeight) / divisor),
        static_cast<u8>((a.g * aWeight + b.g * bWeight) / divisor),
        static_cast<u8>((a.b * aWeight + b.b * bWeight) / divisor),
        255
    };
}

void decode_bc_block(const u8* block, AndroidBcFormat format, AndroidRgba8 (&pixels)[16])
{
    const bool hasExplicitAlpha = format != AndroidBcFormat::Bc1;
    const u8* colorBlock = hasExplicitAlpha ? block + 8 : block;
    const u16 color0 = read_bc_u16(colorBlock);
    const u16 color1 = read_bc_u16(colorBlock + 2);
    AndroidRgba8 colors[4] = { decode_565(color0), decode_565(color1), {}, {} };

    if (hasExplicitAlpha || color0 > color1)
    {
        colors[2] = interpolate_bc_color(colors[0], colors[1], 2, 1, 3);
        colors[3] = interpolate_bc_color(colors[0], colors[1], 1, 2, 3);
    }
    else
    {
        colors[2] = interpolate_bc_color(colors[0], colors[1], 1, 1, 2);
        colors[3] = { 0, 0, 0, 0 };
    }

    const u32 colorIndices = read_bc_u32(colorBlock + 4);
    for (u32 pixel = 0; pixel < 16; ++pixel)
        pixels[pixel] = colors[(colorIndices >> (pixel * 2)) & 3];

    if (format == AndroidBcFormat::Bc2)
    {
        for (u32 row = 0; row < 4; ++row)
        {
            const u16 alphaRow = read_bc_u16(block + row * 2);
            for (u32 column = 0; column < 4; ++column)
            {
                const u8 alpha4 = static_cast<u8>((alphaRow >> (column * 4)) & 15);
                pixels[row * 4 + column].a = static_cast<u8>((alpha4 << 4) | alpha4);
            }
        }
    }
    else if (format == AndroidBcFormat::Bc3)
    {
        u8 alpha[8] = { block[0], block[1] };
        if (alpha[0] > alpha[1])
        {
            for (u32 index = 2; index < 8; ++index)
                alpha[index] = static_cast<u8>(((8 - index) * alpha[0] + (index - 1) * alpha[1]) / 7);
        }
        else
        {
            for (u32 index = 2; index < 6; ++index)
                alpha[index] = static_cast<u8>(((6 - index) * alpha[0] + (index - 1) * alpha[1]) / 5);
            alpha[6] = 0;
            alpha[7] = 255;
        }

        u64 alphaIndices = 0;
        for (u32 byte = 0; byte < 6; ++byte)
            alphaIndices |= static_cast<u64>(block[2 + byte]) << (byte * 8);
        for (u32 pixel = 0; pixel < 16; ++pixel)
            pixels[pixel].a = alpha[(alphaIndices >> (pixel * 3)) & 7];
    }
}

bool decode_bc_texture(gli::texture& texture, u32 downscale, AndroidBcFormat format)
{
    if (format == AndroidBcFormat::Unsupported)
        return false;

    constexpr gli::format decodedFormat = gli::FORMAT_RGBA8_UNORM_PACK8;
    const auto sourceExtent = texture.extent();
    const gli::texture::extent_type decodedExtent(
        std::max(1, sourceExtent.x >> downscale),
        std::max(1, sourceExtent.y >> downscale),
        sourceExtent.z);
    gli::texture decoded(texture.target(), decodedFormat, decodedExtent,
        texture.layers(), texture.faces(), texture.levels(), texture.swizzles());
    const size_t blockSize = format == AndroidBcFormat::Bc1 ? 8 : 16;
    const u32 sampleStep = 1u << downscale;
    const u32 sampleOffset = downscale ? sampleStep >> 1 : 0;

    for (size_t layer = 0; layer < texture.layers(); ++layer)
    {
        for (size_t face = 0; face < texture.faces(); ++face)
        {
            for (size_t level = 0; level < texture.levels(); ++level)
            {
                const auto srcExtent = texture.extent(level);
                const auto dstExtent = decoded.extent(level);
                const size_t blocksX = std::max(1, (srcExtent.x + 3) / 4);
                const size_t blocksY = std::max(1, (srcExtent.y + 3) / 4);
                const u8* source = static_cast<const u8*>(texture.data(layer, face, level));
                auto* destination = static_cast<AndroidRgba8*>(decoded.data(layer, face, level));

                if (downscale == 0)
                {
                    for (int z = 0; z < srcExtent.z; ++z)
                    {
                        for (size_t blockY = 0; blockY < blocksY; ++blockY)
                        {
                            for (size_t blockX = 0; blockX < blocksX; ++blockX)
                            {
                                const size_t blockIndex =
                                    (static_cast<size_t>(z) * blocksY + blockY) * blocksX + blockX;
                                AndroidRgba8 pixels[16];
                                decode_bc_block(source + blockIndex * blockSize, format, pixels);
                                for (size_t localY = 0; localY < 4; ++localY)
                                {
                                    const size_t y = blockY * 4 + localY;
                                    if (y >= static_cast<size_t>(srcExtent.y))
                                        break;
                                    for (size_t localX = 0; localX < 4; ++localX)
                                    {
                                        const size_t x = blockX * 4 + localX;
                                        if (x >= static_cast<size_t>(srcExtent.x))
                                            break;
                                        destination[(static_cast<size_t>(z) * dstExtent.y + y) * dstExtent.x + x] =
                                            pixels[localY * 4 + localX];
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (int z = 0; z < dstExtent.z; ++z)
                    {
                        for (int y = 0; y < dstExtent.y; ++y)
                        {
                            const u32 sourceY = std::min<u32>(srcExtent.y - 1, y * sampleStep + sampleOffset);
                            size_t cachedBlock = static_cast<size_t>(-1);
                            AndroidRgba8 pixels[16];
                            for (int x = 0; x < dstExtent.x; ++x)
                            {
                                const u32 sourceX = std::min<u32>(srcExtent.x - 1, x * sampleStep + sampleOffset);
                                const size_t blockIndex =
                                    (static_cast<size_t>(z) * blocksY + sourceY / 4) * blocksX + sourceX / 4;
                                if (blockIndex != cachedBlock)
                                {
                                    decode_bc_block(source + blockIndex * blockSize, format, pixels);
                                    cachedBlock = blockIndex;
                                }
                                destination[(static_cast<size_t>(z) * dstExtent.y + y) * dstExtent.x + x] =
                                    pixels[(sourceY & 3) * 4 + (sourceX & 3)];
                            }
                        }
                    }
                }
            }
        }
    }

    texture = std::move(decoded);
    return true;
}

bool decode_compressed_texture(gli::texture& texture, u32 downscale)
{
    if (!gli::is_compressed(texture.format()))
        return true;

    if (!gli::has_decoder(texture.format()))
        return false;

    // GLI's generic converter fetches each texel separately and therefore
    // expands the same BC block up to sixteen times. On ARM this made a
    // 1024x1024 DXT texture take roughly one second. Decode each block once;
    // this also covers BC1/BC2/BC3 used by the original game assets.
    const AndroidBcFormat bcFormat = android_bc_format(texture.format());
    if (bcFormat != AndroidBcFormat::Unsupported)
        return decode_bc_texture(texture, downscale, bcFormat);

    constexpr gli::format decoded_format = gli::FORMAT_RGBA8_UNORM_PACK8;

    // Terrain DDS files shipped with CoP have no mip chain.  On GLES devices
    // without BC/S3TC support, decoding such a 2048x2048 texture verbatim
    // retains 16 MiB instead of the original 4 MiB.  Decode a sampled mobile
    // copy directly, so we neither allocate nor iterate over the full RGBA
    // image.  Atlases are deliberately excluded by the caller because their
    // pixel-addressed layout must not change.
    if (downscale && texture.target() == gli::TARGET_2D && texture.levels() == 1)
    {
        const gli::texture2d source(texture);
        const gli::texture2d::extent_type sourceExtent = source.extent();
        const gli::texture2d::extent_type decodedExtent(
            std::max(1, sourceExtent.x >> downscale),
            std::max(1, sourceExtent.y >> downscale));
        gli::texture2d decoded(decoded_format, decodedExtent, 1, texture.swizzles());
        gli::fsampler2D sourceSampler(source, gli::WRAP_CLAMP_TO_EDGE);
        gli::fsampler2D decodedSampler(decoded, gli::WRAP_CLAMP_TO_EDGE);
        const int sampleOffset = 1 << (downscale - 1);

        for (int y = 0; y < decodedExtent.y; ++y)
        {
            for (int x = 0; x < decodedExtent.x; ++x)
            {
                const gli::texture2d::extent_type sourceCoord(
                    std::min(sourceExtent.x - 1, (x << downscale) + sampleOffset),
                    std::min(sourceExtent.y - 1, (y << downscale) + sampleOffset));
                decodedSampler.texel_write({ x, y }, 0, sourceSampler.texel_fetch(sourceCoord, 0));
            }
        }

        texture = decoded;
        return true;
    }

    switch (texture.target())
    {
    case gli::TARGET_1D:
        texture = gli::convert(gli::texture1d(texture), decoded_format);
        return true;
    case gli::TARGET_1D_ARRAY:
        texture = gli::convert(gli::texture1d_array(texture), decoded_format);
        return true;
    case gli::TARGET_2D:
        texture = gli::convert(gli::texture2d(texture), decoded_format);
        return true;
    case gli::TARGET_2D_ARRAY:
        texture = gli::convert(gli::texture2d_array(texture), decoded_format);
        return true;
    case gli::TARGET_3D:
        texture = gli::convert(gli::texture3d(texture), decoded_format);
        return true;
    case gli::TARGET_CUBE:
        texture = gli::convert(gli::texture_cube(texture), decoded_format);
        return true;
    case gli::TARGET_CUBE_ARRAY:
        texture = gli::convert(gli::texture_cube_array(texture), decoded_format);
        return true;
    default:
        return false;
    }
}

bool probe_compressed_texture_format(GLenum internalFormat)
{
    GLsizei blockSize = 0;
    switch (internalFormat)
    {
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        blockSize = 8;
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        blockSize = 16;
        break;
    default:
        return false;
    }

    GLint previousTexture = 0;
    GLuint probeTexture = 0;
    GLubyte block[16]{};
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    clear_texture_gl_errors();
    glGenTextures(1, &probeTexture);
    glBindTexture(GL_TEXTURE_2D, probeTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, 4, 4);
    bool supported = glGetError() == GL_NO_ERROR;
    if (supported)
    {
        glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4,
            internalFormat, blockSize, block);
        supported = glGetError() == GL_NO_ERROR;
    }
    while (glGetError() != GL_NO_ERROR)
    {
    }
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    if (probeTexture)
        glDeleteTextures(1, &probeTexture);
    return supported;
}

bool supports_compressed_texture_format(GLenum internalFormat)
{
    // GLES drivers are allowed to advertise S3TC support through extension
    // strings without listing every accepted enum in
    // GL_COMPRESSED_TEXTURE_FORMATS. Adreno does exactly that, which made the
    // old check decode every CoP DXT texture to RGBA on the CPU and inflate
    // memory fourfold near the end of level loading.
    const bool fullS3tc = GLAD_GL_EXT_texture_compression_s3tc != 0;
    const bool dxt1 = fullS3tc || GLAD_GL_EXT_texture_compression_dxt1 != 0;
    const bool dxt3 = fullS3tc || GLAD_GL_ANGLE_texture_compression_dxt3 != 0;
    const bool dxt5 = fullS3tc || GLAD_GL_ANGLE_texture_compression_dxt5 != 0;

    bool extensionSupport = false;
    switch (internalFormat)
    {
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        if (dxt1)
            extensionSupport = true;
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        if (dxt3)
            extensionSupport = true;
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        if (dxt5)
            extensionSupport = true;
        break;
    default:
        break;
    }

    GLint count = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count);
    xr_vector<GLint> formats;
    if (count > 0)
    {
        formats.resize(static_cast<size_t>(count));
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats.data());
    }
    const bool listedSupport = std::find(formats.begin(), formats.end(),
        static_cast<GLint>(internalFormat)) != formats.end();

    struct CachedProbe
    {
        GLenum format;
        bool supported;
    };
    static xr_vector<CachedProbe> probes;
    const auto cached = std::find_if(probes.begin(), probes.end(),
        [internalFormat](const CachedProbe& value) { return value.format == internalFormat; });
    if (cached != probes.end())
        return cached->supported;

    // Some Android GL loaders fail to expose vendor extension flags even when
    // the driver accepts the format.  A tiny real allocation is definitive
    // and prevents unnecessary software decoding on those drivers.
    const bool probedSupport = extensionSupport || listedSupport ||
        probe_compressed_texture_format(internalFormat);
    probes.push_back({ internalFormat, probedSupport });
    Msg("[texture-trace] compressed-format format=0x%x ext=%d listed=%d probe=%d supported=%d "
        "flags(s3tc=%d,dxt1=%d,dxt3=%d,dxt5=%d) listed-count=%d",
        internalFormat, extensionSupport ? 1 : 0, listedSupport ? 1 : 0,
        (!extensionSupport && !listedSupport) ? 1 : 0, probedSupport ? 1 : 0,
        fullS3tc ? 1 : 0, dxt1 ? 1 : 0, dxt3 ? 1 : 0, dxt5 ? 1 : 0, count);
    return probedSupport;
}
#endif

GLuint CRender::texture_load(LPCSTR fRName, u32& ret_msize, GLenum& ret_desc,
    GLint* ret_width, GLint* ret_height)
{
    ret_msize = 0;
    R_ASSERT1_CURE(fRName && fRName[0], { return 0; });

    GLuint pTexture = 0;
    string_path fn;
    {
        // make file name
        string_path fname;
        xr_strcpy(fname, fRName);
        fix_texture_name(fname);

        // Call to FS.exist WRITES to fn !

        if (!FS.exist(fn, "$game_textures$", fname, ".dds") && strstr(fname, "_bump"))
        {
            Msg("! Fallback to default bump map: %s", fname);
            if (strstr(fname, "_bump#"))
                R_ASSERT1_CURE(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump#", ".dds"), return 0);
            else
                R_ASSERT1_CURE(FS.exist(fn, "$game_textures$", "ed\\ed_dummy_bump", ".dds"), return 0);
        }
        else
        {
            bool exist = false;

            for (cpcstr folder : { "$level$", "$game_saves$", "$game_textures$" })
            {
                exist = FS.exist(fn, folder, fname, ".dds");
                if (exist)
                    break;
            }

            if (!exist)
            {
                Msg("! Can't find texture '%s'", fname);
                if (!FS.exist(fn, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"))
                    return 0;
            }
        }
    }

    // Load and get header
    IReader* S = FS.r_open(fn);
    R_ASSERT2_CURE(S, fn, { return 0; });
    size_t img_size = S->length();
#if defined(XR_PLATFORM_ANDROID)
    CTimer androidTextureTimer;
    androidTextureTimer.Start();
#endif
#ifdef DEBUG
    Msg("* Loaded: %s[%d]b", fn, img_size);
#endif // DEBUG
#if defined(XR_PLATFORM_ANDROID)
    string256 androidTextureContext;
    xr_sprintf(androidTextureContext, sizeof(androidTextureContext),
        "texture-dds parse align=%u bytes=%zu '%.150s'",
        static_cast<unsigned>(reinterpret_cast<uintptr_t>(S->pointer()) & 3u), img_size, fn);
    android_set_load_context(androidTextureContext);
#endif
    gli::texture texture = gli::load((char*)S->pointer(), img_size);
    R_ASSERT2(!texture.empty(), fn);
#if defined(XR_PLATFORM_ANDROID)
    xr_sprintf(androidTextureContext, sizeof(androidTextureContext),
        "texture-dds parsed bytes=%zu '%.150s'", img_size, fn);
    android_set_load_context(androidTextureContext);
#endif

    xr_strlwr(fn);
    const u32 sourceMipCount = static_cast<u32>(texture.levels());
    const auto sourceExtent = texture.extent();
    const bool cubeTexture = is_target_cube(texture.target());
    const int configuredLod = cubeTexture ? 0 : get_texture_load_lod(fn);
    int appliedLod = configuredLod;
    bool softwareDecode = false;
    u32 decodeDownscale = 0;
#if defined(XR_PLATFORM_ANDROID)
    GLenum compressedInternal = 0;
#endif

#if defined(XR_PLATFORM_ANDROID)
    // GLES 3.0 does not require desktop S3TC/BC texture formats.  CoP ships
    // many DDS files in DXT form.  Preserve hardware-supported compressed
    // formats (Adreno normally exposes S3TC) to avoid multiplying level
    // texture memory by four; decode only formats the current device cannot
    // upload.  The previous decode-all path could exhaust a 32-bit process
    // near the end of a large CoP level load.
    if (gli::is_compressed(texture.format()))
    {
        const gli::gl compressedGL(gli::gl::PROFILE_ES30);
        const auto compressedFormat = compressedGL.translate(texture.format(), texture.swizzles());
        compressedInternal = compressedFormat.Internal;
        softwareDecode = !supports_compressed_texture_format(compressedInternal);
        const auto sourceMaxExtent = std::max(sourceExtent.x, sourceExtent.y);
        if (softwareDecode && !cubeTexture && sourceMipCount > 1 &&
            sourceMaxExtent >= 512)
        {
            // RGBA fallback is roughly four times larger than BC1/BC3.  Keep
            // at least one lower mip on mobile to control both decode time and
            // the 32-bit process working set, while still honoring a stronger
            // user-selected texture LOD.
            appliedLod = std::max(appliedLod, 1);
            // A software-decoded BC texture occupies about four times its
            // compressed size. On a 32-bit Android process, 1K/2K textures
            // first touched by the initial gameplay frame can push the
            // working set past the address-space limit. Skipping two source
            // mips keeps UVs and atlas layout intact while cutting both the
            // decode work and RGBA allocation by another factor of four.
            if (sourceMaxExtent >= 1024)
                appliedLod = std::max(appliedLod, 2);
        }
        if (softwareDecode && !cubeTexture && sourceMipCount == 1 &&
            sourceMaxExtent >= 1024 &&
            (strstr(fn, "terrain\\") != nullptr || strstr(fn, "wpn\\wpn_crosshair") != nullptr))
        {
            decodeDownscale = 1;
        }
    }
#endif

    if (!cubeTexture && sourceMipCount > 1)
    {
        appliedLod = std::clamp(appliedLod, 0, static_cast<int>(sourceMipCount - 1));
        if (appliedLod > 0)
        {
            texture = gli::texture(texture, texture.target(), texture.format(),
                texture.base_layer(), texture.max_layer(),
                texture.base_face(), texture.max_face(),
                texture.base_level() + static_cast<size_t>(appliedLod), texture.max_level(),
            texture.swizzles());
        }
    }
    else
    {
        // A one-level texture cannot honor a requested mip skip.  Keep the
        // applied value truthful in diagnostics; terrain downscaling is
        // tracked independently below.
        appliedLod = 0;
    }

#if defined(XR_PLATFORM_ANDROID)
    if (appliedLod || decodeDownscale)
    {
        Msg("[texture-trace] lod-policy name='%s' source=%dx%d levels=%u configured=%d applied=%d "
            "decode-scale=%u software=%d result=%dx%d levels=%zu",
            fRName, sourceExtent.x, sourceExtent.y, sourceMipCount, configuredLod, appliedLod,
            decodeDownscale, softwareDecode ? 1 : 0,
            texture.extent().x >> decodeDownscale, texture.extent().y >> decodeDownscale,
            texture.levels());
    }

    if (gli::is_compressed(texture.format()) && softwareDecode)
    {
        Msg("[texture-trace] decode-begin name='%s' source=%zu bytes format=0x%x",
            fRName, img_size, compressedInternal);
        CTimer decodeTimer;
        decodeTimer.Start();
        if (!decode_compressed_texture(texture, decodeDownscale))
        {
            Msg("! Android GLES: no decoder for compressed texture '%s'", fn);
            FS.r_close(S);
            return 0;
        }
        Msg("[texture-trace] decode-end name='%s' elapsed=%llu ms",
            fRName, static_cast<unsigned long long>(decodeTimer.GetElapsed_ms()));
        // DeferredUpload reports bounded progress.  Logging every decoded DDS
        // made a large CoP level produce thousands of lines and also caused
        // the launcher diagnostics view to stutter badly.
    }
#endif

    const u32 mip_cnt = static_cast<u32>(texture.levels());

#if defined(XR_PLATFORM_ANDROID)
    gli::gl GL(gli::gl::PROFILE_ES30);
#else
    gli::gl GL(gli::gl::PROFILE_GL33);
#endif

    gli::gl::format const format = GL.translate(texture.format(), texture.swizzles());
    GLenum target = GL.translate(texture.target());

#if defined(XR_PLATFORM_ANDROID)
    // Attribute an error to the operation that actually caused it.  Release
    // builds make CHK_GL a pass-through, so stale driver errors otherwise get
    // reported later as a bogus glTexStorage2D failure.
    clear_texture_gl_errors();
#endif
    glGenTextures(1, &pTexture);
    glBindTexture(target, pTexture);

    glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texture.levels() - 1));

    if (gli::gl::EXTERNAL_RED != format.External) // skip for proper greyscale-alpha font textures
#if defined(XR_PLATFORM_ANDROID)
    {
        // GL_TEXTURE_SWIZZLE_RGBA is a desktop aggregate pname.  GLES 3.x
        // exposes only the four component pnames and returns GL_INVALID_ENUM
        // for the aggregate form used by the desktop renderer.
        static constexpr GLenum SwizzleNames[] =
        {
            GL_TEXTURE_SWIZZLE_R,
            GL_TEXTURE_SWIZZLE_G,
            GL_TEXTURE_SWIZZLE_B,
            GL_TEXTURE_SWIZZLE_A
        };
        for (size_t component = 0; component < std::size(SwizzleNames); ++component)
            glTexParameteri(target, SwizzleNames[component], format.Swizzles[component]);
    }
#else
        glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, &format.Swizzles[gli::SWIZZLE_RED]);
#endif

#if defined(XR_PLATFORM_ANDROID)
    if (!pTexture || !check_texture_gl_error("texture creation and parameters", fn))
    {
        if (pTexture)
            glDeleteTextures(1, &pTexture);
        FS.r_close(S);
        return 0;
    }
#endif

    glm::tvec3<GLsizei> const tex_extent(texture.extent());
    if (ret_width)
        *ret_width = tex_extent.x;
    if (ret_height)
        *ret_height = tex_extent.y;

#if !defined(XR_PLATFORM_ANDROID)
    GLenum err;
#endif
    switch (texture.target())
    {
    case gli::TARGET_2D:
    case gli::TARGET_CUBE:
        glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal,
                       tex_extent.x, tex_extent.y);
#if defined(XR_PLATFORM_ANDROID)
        if (!check_texture_gl_error("2D immutable storage allocation", fn))
        {
            glDeleteTextures(1, &pTexture);
            FS.r_close(S);
            return 0;
        }
#else
        err = glGetError();
        if (err != GL_NO_ERROR)
        {
            VERIFY(err == GL_NO_ERROR);
            Msg("! OpenGL: 0x%x: Invalid 2D texture: '%s'", err, fn);
        }
#endif
        break;
    case gli::TARGET_3D:
    case gli::TARGET_CUBE_ARRAY:
        glTexStorage3D(target, static_cast<GLint>(texture.levels()), format.Internal,
                       tex_extent.x, tex_extent.y, tex_extent.z);
#if defined(XR_PLATFORM_ANDROID)
        if (!check_texture_gl_error("3D immutable storage allocation", fn))
        {
            glDeleteTextures(1, &pTexture);
            FS.r_close(S);
            return 0;
        }
#else
        err = glGetError();
        if (err != GL_NO_ERROR)
        {
            VERIFY(err == GL_NO_ERROR);
            Msg("! OpenGL: 0x%x: Invalid 3D texture: '%s'", err, fn);
        }
#endif
        break;
    default:
        NODEFAULT;
        break;
    }

    for (size_t layer = 0; layer < texture.layers(); ++layer)
    {
        for (size_t face = 0; face < texture.faces(); ++face)
        {
            for (size_t level = 0; level < texture.levels(); ++level)
            {
                glm::tvec3<GLsizei> const tex_level_extent(texture.extent(level));
                GLenum sub_target = gli::is_target_cube(texture.target())
                         ? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face)
                         : target;

                switch (texture.target())
                {
                case gli::TARGET_2D:
                case gli::TARGET_CUBE:
                {
                    if (gli::is_compressed(texture.format()))
                    {
                        glCompressedTexSubImage2D(sub_target, static_cast<GLint>(level),
                                    0, 0, tex_level_extent.x, tex_level_extent.y,
                                    format.Internal, static_cast<GLsizei>(texture.size(level)),
                                    texture.data(layer, face, level));
#if defined(XR_PLATFORM_ANDROID)
                        if (!check_texture_gl_error("compressed 2D texture upload", fn))
                        {
                            glDeleteTextures(1, &pTexture);
                            FS.r_close(S);
                            return 0;
                        }
#else
                        err = glGetError();
                        if (err != GL_NO_ERROR)
                        {
                            VERIFY(err == GL_NO_ERROR);
                            Msg("! OpenGL: 0x%x: Invalid 2D compressed subtexture: '%s'", err, fn);
                        }
#endif
                    }
                    else
                    {
                        glTexSubImage2D(sub_target, static_cast<GLint>(level),
                                    0, 0, tex_level_extent.x, tex_level_extent.y,
                                    format.External, format.Type,
                                    texture.data(layer, face, level));
#if defined(XR_PLATFORM_ANDROID)
                        if (!check_texture_gl_error("2D texture upload", fn))
                        {
                            glDeleteTextures(1, &pTexture);
                            FS.r_close(S);
                            return 0;
                        }
#else
                        err = glGetError();
                        if (err != GL_NO_ERROR)
                        {
                            VERIFY(err == GL_NO_ERROR);
                            Msg("! OpenGL: 0x%x: Invalid 2D subtexture: '%s'", err, fn);
                        }
#endif

                    }
                    break;
                }
                case gli::TARGET_3D:
                case gli::TARGET_CUBE_ARRAY:
                {
                    if (gli::is_compressed(texture.format()))
                    {
                        glCompressedTexSubImage3D(target, static_cast<GLint>(level),
                                    0, 0, 0, tex_level_extent.x, tex_level_extent.y, tex_level_extent.z,
                                    format.Internal, static_cast<GLsizei>(texture.size(level)),
                                    texture.data(layer, face, level));
#if defined(XR_PLATFORM_ANDROID)
                        if (!check_texture_gl_error("compressed 3D texture upload", fn))
                        {
                            glDeleteTextures(1, &pTexture);
                            FS.r_close(S);
                            return 0;
                        }
#else
                        err = glGetError();
                        if (err != GL_NO_ERROR)
                        {
                            VERIFY(err == GL_NO_ERROR);
                            Msg("! OpenGL: 0x%x: Invalid compressed 3D subtexture: '%s'", err, fn);
                        }
#endif
                    }
                    else
                    {
                        glTexSubImage3D(target, static_cast<GLint>(level),
                                    0, 0, 0, tex_level_extent.x, tex_level_extent.y, tex_level_extent.z,
                                    format.External, format.Type,
                                    texture.data(layer, face, level));
#if defined(XR_PLATFORM_ANDROID)
                        if (!check_texture_gl_error("3D texture upload", fn))
                        {
                            glDeleteTextures(1, &pTexture);
                            FS.r_close(S);
                            return 0;
                        }
#else
                        err = glGetError();
                        if (err != GL_NO_ERROR)
                        {
                            VERIFY(err == GL_NO_ERROR);
                            Msg("! OpenGL: 0x%x: Invalid 3D subtexture: '%s'", err, fn);
                        }
#endif
                    }
                    break;
                }
                default:
                    NODEFAULT;
                    break;
                }
            }
        }
    }

    FS.r_close(S);

    ret_desc = target;
#if defined(XR_PLATFORM_ANDROID)
    ret_msize = static_cast<u32>(std::min<size_t>(texture.size(), type_max<u32>));
#else
    ret_msize = calc_texture_size(appliedLod, sourceMipCount, img_size);
#endif
#if defined(XR_PLATFORM_ANDROID)
    const u64 textureElapsed = androidTextureTimer.GetElapsed_ms();
    if (textureElapsed >= 500)
        Msg("[texture-trace] slow-load name='%s' elapsed=%llu ms source=%zu bytes extent=%dx%d levels=%u "
            "source-levels=%u lod=%d software=%d uploaded=%u",
            fRName, static_cast<unsigned long long>(textureElapsed), img_size,
            tex_extent.x, tex_extent.y, mip_cnt, sourceMipCount, appliedLod,
            softwareDecode ? 1 : 0, ret_msize);
#endif
    return pTexture;
}
} // namespace xray::render::RENDER_NAMESPACE
