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
    //  Igor: TMP
    //  Create an RT for online screenshot makining
    {
        D3DSURFACE_DESC desc;
        get_base_rt()->GetDesc(&desc);
        rt_async_ss.create(r2_async_ss, Device.dwWidth, Device.dwHeight, desc.Format, 1, { CRT::CreateSurface });
    }
    // Build material(s)
    {
        // Surface
        R_CHK(D3DXCreateVolumeTexture(HW.pDevice, TEX_material_LdotN, TEX_material_LdotH, 4, 1, 0, D3DFMT_A8L8,
            D3DPOOL_MANAGED, &t_material_surf));
        t_material = RImplementation.Resources->_CreateTexture(r2_material);
        t_material->surface_set(t_material_surf);

        // Fill it (addr: x=dot(L,N),y=dot(L,H))
        D3DLOCKED_BOX R;
        R_CHK(t_material_surf->LockBox(0, &R, 0, 0));
        for (u32 slice = 0; slice < 4; slice++)
        {
            for (u32 y = 0; y < TEX_material_LdotH; y++)
            {
                for (u32 x = 0; x < TEX_material_LdotN; x++)
                {
                    u16* p = (u16*)((u8*)(R.pBits) + slice * R.SlicePitch + y * R.RowPitch + x * 2);
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
                    if ((y == (TEX_material_LdotH - 1)) && (x == (TEX_material_LdotN - 1)))
                    {
                        _d = 255;
                        _s = 255;
                    }
                    *p = u16(_s * 256 + _d);
                }
            }
        }
        R_CHK(t_material_surf->UnlockBox(0));
        // #ifdef DEBUG
        // R_CHK    (D3DXSaveTextureToFile  ("x:" DELIMITER "r2_material.dds",D3DXIFF_DDS,t_material_surf,0));
        // #endif
    }

    // Build noise table
    {
        // Surfaces
        D3DLOCKED_RECT R[TEX_jitter_count];
        for (int it1 = 0; it1 < TEX_jitter_count - 1; it1++)
        {
            string_path name;
            xr_sprintf(name, "%s%d", r2_jitter, it1);
            R_CHK(D3DXCreateTexture(
                HW.pDevice, TEX_jitter, TEX_jitter, 1, 0, D3DFMT_Q8W8V8U8, D3DPOOL_MANAGED, &t_noise_surf[it1]));
            t_noise[it1] = RImplementation.Resources->_CreateTexture(name);
            t_noise[it1]->surface_set(t_noise_surf[it1]);
            R_CHK(t_noise_surf[it1]->LockRect(0, &R[it1], 0, 0));
        }

        // Fill it,
        for (u32 y = 0; y < TEX_jitter; y++)
        {
            for (u32 x = 0; x < TEX_jitter; x++)
            {
                u32 data[TEX_jitter_count - 1];
                generate_jitter(data, TEX_jitter_count - 1);
                for (u32 it2 = 0; it2 < TEX_jitter_count - 1; it2++)
                {
                    u32* p = (u32*)((u8*)(R[it2].pBits) + y * R[it2].Pitch + x * 4);
                    *p = data[it2];
                }
            }
        }

        for (int it3 = 0; it3 < TEX_jitter_count - 1; it3++)
        {
            R_CHK(t_noise_surf[it3]->UnlockRect(0));
        }

        // generate HBAO jitter texture (last)
        int it = TEX_jitter_count - 1;
        string_path name;
        xr_sprintf(name, "%s%d", r2_jitter, it);
        R_CHK(D3DXCreateTexture(
            HW.pDevice, TEX_jitter, TEX_jitter, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &t_noise_surf[it]));
        t_noise[it] = RImplementation.Resources->_CreateTexture(name);
        t_noise[it]->surface_set(t_noise_surf[it]);
        R_CHK(t_noise_surf[it]->LockRect(0, &R[it], 0, 0));

        // Fill it,
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
                float angle = 2 * PI * ::Random.randF(0.0f, 1.0f) / numDir;
                float dist = ::Random.randF(0.0f, 1.0f);
                // float dest[4];

                float* p = (float*)((u8*)(R[it].pBits) + y * R[it].Pitch + x * 4 * sizeof(float));
                *p = (float)(_cos(angle));
                *(p + 1) = (float)(_sin(angle));
                *(p + 2) = (float)(dist);
                *(p + 3) = 0;

                // generate_hbao_jitter (data,TEX_jitter*TEX_jitter);
            }
        }
        R_CHK(t_noise_surf[it]->UnlockRect(0));
    }
}
