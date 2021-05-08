#include "stdafx.h"

static void generate_jitter(u32* dest, u32 elem_count)
{
    const int cmax = 8;
    svector<Ivector2, cmax> samples;
    while (samples.size() < elem_count * 2)
    {
        Ivector2 test;
        test.set(Random.randI(0, 256), Random.randI(0, 256));
        BOOL valid = TRUE;
        for (u32 t = 0; t < samples.size(); t++)
        {
            int dist = _abs(test.x - samples[t].x) + _abs(test.y - samples[t].y);
            if (dist < 32)
            {
                valid = FALSE;
                break;
            }
        }
        if (valid)
            samples.push_back(test);
    }
    for (u32 it = 0; it < elem_count; it++, dest++)
        *dest = color_rgba(samples[2 * it].x, samples[2 * it].y, samples[2 * it + 1].y, samples[2 * it + 1].x);
}
void CRenderTarget::build_textures()
{
    // Texture for async sreenshots
    /* TODO: OGL: Implement screenshots
    {
        D3D_TEXTURE2D_DESC desc;
        desc.Width = Device.dwWidth;
        desc.Height = Device.dwHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
        desc.Usage = D3D_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        R_CHK(HW.pDevice->CreateTexture2D(&desc, 0, &t_ss_async));
    }*/
    // Build material(s)
    {
        // Surface
        glGenTextures(1, &t_material_surf);
        CHK_GL(glBindTexture(GL_TEXTURE_3D, t_material_surf));
        CHK_GL(glTexStorage3D(GL_TEXTURE_3D, 1, GL_RG8, TEX_material_LdotN, TEX_material_LdotH, TEX_material_Count)
        );
        t_material = RImplementation.Resources->_CreateTexture(r2_material);
        t_material->surface_set(GL_TEXTURE_3D, t_material_surf);

        // Fill it (addr: x=dot(L,N),y=dot(L,H))
        static const u32 RowPitch = TEX_material_LdotN * 2;
        static const u32 SlicePitch = TEX_material_LdotH * RowPitch;
        u16 pBits[TEX_material_LdotN * TEX_material_LdotH * TEX_material_Count];
        for (u32 slice = 0; slice < TEX_material_Count; slice++)
        {
            for (u32 y = 0; y < TEX_material_LdotH; y++)
            {
                for (u32 x = 0; x < TEX_material_LdotN; x++)
                {
                    u16* p = (u16*)((u8*)(pBits)+slice * SlicePitch +
                        y * RowPitch + x * 2);
                    float ld = float(x) / float(TEX_material_LdotN - 1);
                    float ls = float(y) / float(TEX_material_LdotH - 1) + EPS_S;
                    ls *= powf(ld, 1 / 32.f);
                    float fd, fs;

                    switch (slice)
                    {
                    case 0:
                    { // looks like OrenNayar
                        fd = powf(ld, 0.75f); // 0.75
                        fs = powf(ls, 16.f) * .5f;
                    }
                    break;
                    case 1:
                    { // looks like Blinn
                        fd = powf(ld, 0.90f); // 0.90
                        fs = powf(ls, 24.f);
                    }
                    break;
                    case 2:
                    { // looks like Phong
                        fd = ld; // 1.0
                        fs = powf(ls * 1.01f, 128.f);
                    }
                    break;
                    case 3:
                    { // looks like Metal
                        float s0 = _abs(1 - _abs(0.05f * _sin(33.f * ld) + ld - ls));
                        float s1 = _abs(1 - _abs(0.05f * _cos(33.f * ld * ls) + ld - ls));
                        float s2 = _abs(1 - _abs(ld - ls));
                        fd = ld; // 1.0
                        fs = powf(_max(_max(s0, s1), s2), 24.f);
                        fs *= powf(ld, 1 / 7.f);
                    }
                    break;
                    default: fd = fs = 0;
                    }
                    s32 _d = clampr(iFloor(fd * 255.5f), 0, 255);
                    s32 _s = clampr(iFloor(fs * 255.5f), 0, 255);
                    if (y == (TEX_material_LdotH - 1) && x == (TEX_material_LdotN - 1))
                    {
                        _d = 255;
                        _s = 255;
                    }
                    *p = u16(_s * 256 + _d);
                }
            }
        }
        CHK_GL(glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, TEX_material_LdotN, TEX_material_LdotH, TEX_material_Count
            , GL_RG, GL_UNSIGNED_BYTE, pBits));
        // #ifdef DEBUG
        // R_CHK	(D3DXSaveTextureToFile	("x:" DELIMITER "r2_material.dds",D3DXIFF_DDS,t_material_surf,0));
        // #endif
    }

    // Build noise table
    if (true)
    {
        glGenTextures(TEX_jitter_count, t_noise_surf);

        static const int sampleSize = 4;
        u32 tempData[TEX_jitter_count][TEX_jitter * TEX_jitter];

        // Surfaces
        for (int it1 = 0; it1 < TEX_jitter_count - 1; it1++)
        {
            string_path name;
            xr_sprintf(name, "%s%d", r2_jitter, it1);
            CHK_GL(glBindTexture(GL_TEXTURE_2D, t_noise_surf[it1]));
            CHK_GL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, TEX_jitter, TEX_jitter));
            t_noise[it1] = RImplementation.Resources->_CreateTexture(name);
            t_noise[it1]->surface_set(GL_TEXTURE_2D, t_noise_surf[it1]);
        }

        // Fill it,
        static const u32 Pitch = TEX_jitter * sampleSize;
        for (u32 y = 0; y < TEX_jitter; y++)
        {
            for (u32 x = 0; x < TEX_jitter; x++)
            {
                u32 data[TEX_jitter_count - 1];
                generate_jitter(data, TEX_jitter_count - 1);
                for (u32 it2 = 0; it2 < TEX_jitter_count - 1; it2++)
                {
                    u32* p = (u32*)((u8*)(tempData[it2]) + y * Pitch + x * 4);
                    *p = data[it2];
                }
            }
        }
        int it3 = 0;
        while (it3 < TEX_jitter_count - 1)
        {
            CHK_GL(glBindTexture(GL_TEXTURE_2D, t_noise_surf[it3]));
            CHK_GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEX_jitter, TEX_jitter, GL_RGBA, GL_UNSIGNED_BYTE,
                tempData[it3]));
            it3++;
        }

        float tempDataHBAO[TEX_jitter * TEX_jitter * 4];

        // generate HBAO jitter texture (last)
        int it = TEX_jitter_count - 1;
        string_path name;
        xr_sprintf(name, "%s%d", r2_jitter, it);
        CHK_GL(glBindTexture(GL_TEXTURE_2D, t_noise_surf[it]));
        CHK_GL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, TEX_jitter, TEX_jitter));
        t_noise[it] = RImplementation.Resources->_CreateTexture(name);
        t_noise[it]->surface_set(GL_TEXTURE_2D, t_noise_surf[it]);

        // Fill it,
        static const int HBAOPitch = TEX_jitter * sampleSize * sizeof(float);
        for (u32 y = 0; y < TEX_jitter; y++)
        {
            for (u32 x = 0; x < TEX_jitter; x++)
            {
                float numDir = 1.0f;
                switch (ps_r_ssao)
                {
                case 1: numDir = 4.0f; break;
                case 2: numDir = 6.0f; break;
                case 3: numDir = 8.0f; break;
                }
                float angle = 2 * PI * Random.randF(0.0f, 1.0f) / numDir;
                float dist = Random.randF(0.0f, 1.0f);

                float* p =
                    (float*)((u8*)(tempDataHBAO)+y * HBAOPitch + x * 4 * sizeof(float));
                *p = (float)_cos(angle);
                *(p + 1) = (float)_sin(angle);
                *(p + 2) = (float)dist;
                *(p + 3) = 0;

                //generate_hbao_jitter	(data,TEX_jitter*TEX_jitter);
            }
        }
        CHK_GL(glBindTexture(GL_TEXTURE_2D, t_noise_surf[it3]));
        CHK_GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEX_jitter, TEX_jitter, GL_RGBA, GL_FLOAT, tempDataHBAO));


        //	Create noise mipped
        {
            //	Autogen mipmaps
            glGenTextures(1, &t_noise_surf_mipped);
            CHK_GL(glBindTexture(GL_TEXTURE_2D, t_noise_surf_mipped));
            CHK_GL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, TEX_jitter, TEX_jitter));
            t_noise_mipped = RImplementation.Resources->_CreateTexture(r2_jitter_mipped);
            t_noise_mipped->surface_set(GL_TEXTURE_2D, t_noise_surf_mipped);

            //	Update texture. Generate mips.
            CHK_GL(glCopyImageSubData(t_noise_surf[0], GL_TEXTURE_2D, 0, 0, 0, 0, t_noise_surf_mipped, GL_TEXTURE_2D
                , 0, 0, 0, 0, TEX_jitter, TEX_jitter, 1));

            glBindTexture(GL_TEXTURE_2D, t_noise_surf_mipped);
            CHK_GL(glGenerateMipmap(GL_TEXTURE_2D));
        }
    }
}
