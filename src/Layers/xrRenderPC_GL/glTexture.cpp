// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <gli/gli.hpp>

constexpr cpcstr NOT_EXISTING_TEXTURE = "ed" DELIMITER "ed_not_existing_texture";

void fix_texture_name(LPSTR fn)
{
    LPSTR _ext = strext(fn);
    if (_ext &&
        (0 == xr_stricmp(_ext, ".tga") ||
            0 == xr_stricmp(_ext, ".dds") ||
            0 == xr_stricmp(_ext, ".bmp") ||
            0 == xr_stricmp(_ext, ".ogm")))
        *_ext = 0;
}

GLuint CRender::texture_load(LPCSTR fRName, u32& ret_msize, GLenum& ret_desc)
{
    GLuint pTexture = 0;
    string_path fn;
    size_t img_size = 0;
    gli::gl::format fmt;
    // validation
    R_ASSERT(fRName);
    R_ASSERT(fRName[0]);

    bool dummyTextureExist;

    // make file name
    string_path fname;
    strcpy_s(fname, fRName); //. andy if (strext(fname)) *strext(fname)=0;
    fix_texture_name(fname);
    IReader* S = nullptr;
    if (!FS.exist(fn, "$game_textures$", fname, ".dds") && strstr(fname, "_bump")) goto _BUMP_from_base;
    if (FS.exist(fn, "$level$", fname, ".dds")) goto _DDS;
    if (FS.exist(fn, "$game_saves$", fname, ".dds")) goto _DDS;
    if (FS.exist(fn, "$game_textures$", fname, ".dds")) goto _DDS;


#ifdef _EDITOR
    ELog.Msg(mtError, "Can't find texture '%s'", fname);
    return 0;
#else

    Msg("! Can't find texture '%s'", fname);
    dummyTextureExist = FS.exist(fn, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds");
    if (!ShadowOfChernobylMode)
        R_ASSERT3(dummyTextureExist, "Dummy texture doesn't exist", NOT_EXISTING_TEXTURE);
    if (!dummyTextureExist)
        return 0;

#endif

_DDS:
    {
        // Load and get header
        S = FS.r_open(fn);
#ifdef DEBUG
        Msg("* Loaded: %s[%d]b", fn, S->length());
#endif // DEBUG
        img_size = S->length();
        R_ASSERT(S);
        gli::texture texture = gli::load((char*)S->pointer(), img_size);
        R_ASSERT2(!texture.empty(), fn);
        
        
        gli::gl GL(gli::gl::PROFILE_GL33);

        gli::gl::format const format = GL.translate(texture.format(), texture.swizzles());
        GLenum target = GL.translate(texture.target());

        glGenTextures(1, &pTexture);
        glBindTexture(target, pTexture);

        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texture.levels() - 1));

        if (gli::gl::EXTERNAL_RED != format.External) // skip for properly greyscale-alpfa fonts textures
            glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, &format.Swizzles[gli::SWIZZLE_RED]);

        glm::tvec3<GLsizei> const tex_extent(texture.extent());

        switch (texture.target())
        {
        case gli::TARGET_2D:
        case gli::TARGET_CUBE:
            CHK_GL(glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal,
                           tex_extent.x, tex_extent.y));
            break;
        case gli::TARGET_3D:
            CHK_GL(glTexStorage3D(target, static_cast<GLint>(texture.levels()), format.Internal,
                           tex_extent.x, tex_extent.y, tex_extent.z));
            break;
        default:
            NODEFAULT;
            break;
        }

        for (std::size_t layer = 0; layer < texture.layers(); ++layer)
        {
            for (std::size_t face = 0; face < texture.faces(); ++face)
            {
                for (std::size_t level = 0; level < texture.levels(); ++level)
                {
                    glm::tvec3<GLsizei> const tex_level_extent(texture.extent(level));
                    target = gli::is_target_cube(texture.target())
                             ? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face)
                             : target;

                    switch (texture.target())
                    {
                    case gli::TARGET_2D:
                    case gli::TARGET_CUBE:
                    {
                        if (gli::is_compressed(texture.format()))
                        {
                            CHK_GL(glCompressedTexSubImage2D(target, static_cast<GLint>(level),
                                        0, 0, tex_level_extent.x, tex_level_extent.y,
                                        format.Internal, static_cast<GLsizei>(texture.size(level)),
                                        texture.data(layer, face, level)));
                        }
                        else
                        {
                            CHK_GL(glTexSubImage2D(target, static_cast<GLint>(level),
                                        0, 0, tex_level_extent.x, tex_level_extent.y,
                                        format.External, format.Type,
                                        texture.data(layer, face, level)));
                        }
                        break;
                    }
                    case gli::TARGET_3D:
                    {
                        if (gli::is_compressed(texture.format()))
                        {
                            CHK_GL(glCompressedTexSubImage3D(target, static_cast<GLint>(level),
                                        0, 0, 0, tex_level_extent.x, tex_level_extent.y, tex_level_extent.z,
                                        format.Internal, static_cast<GLsizei>(texture.size(level)),
                                        texture.data(layer, face, level)));
                        }
                        else
                        {
                            CHK_GL(glTexSubImage3D(target, static_cast<GLint>(level),
                                        0, 0, 0, tex_level_extent.x, tex_level_extent.y, tex_level_extent.z,
                                        format.External, format.Type,
                                        texture.data(layer, face, level)));
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

        xr_strlwr(fn);
        ret_desc = target;
        ret_msize = static_cast<u32>(texture.size());
        return pTexture;
    }

_BUMP_from_base:
    {
        //Msg			("! auto-generated bump map: %s",fname);
        Msg("! Fallback to default bump map: %s", fname);
        //////////////////
        if (strstr(fname, "_bump#"))
        {
            R_ASSERT2 (FS.exist(fn,"$game_textures$", "ed" DELIMITER "ed_dummy_bump#", ".dds"), "ed_dummy_bump#");
            S = FS.r_open(fn);
            R_ASSERT2 (S, fn);
            img_size = S->length();
            goto _DDS;
        }
        if (strstr(fname, "_bump"))
        {
            R_ASSERT2 (FS.exist(fn,"$game_textures$", "ed" DELIMITER "ed_dummy_bump", ".dds"),"ed_dummy_bump");
            S = FS.r_open(fn);

            R_ASSERT2 (S, fn);

            img_size = S->length();
            goto _DDS;
        }
        //////////////////
    }

    return 0;
}
