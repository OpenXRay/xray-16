#include "stdafx.h"

#include <D3DX10Tex.h>

static void generate_jitter(u32* dest, u32 elem_count)
{
    const int cmax = 8;
    svector<Ivector2, cmax> samples;
    while (samples.size() < elem_count * 2)
    {
        Ivector2 test;
        test.set(::Random.randI(0, 256), ::Random.randI(0, 256));
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
    }
    // Build material(s)
    {
        //	Create immutable texture.
        //	So we need to init data _before_ the creation.
        // Surface
        // R_CHK
        // (D3DXCreateVolumeTexture(HW.pDevice,TEX_material_LdotN,TEX_material_LdotH,4,1,0,D3DFMT_A8L8,D3DPOOL_MANAGED,&t_material_surf));
        // t_material					= dxRenderDeviceRender::Instance().Resources->_CreateTexture(r2_material);
        // t_material->surface_set		(t_material_surf);
        //	Use DXGI_FORMAT_R8G8_UNORM

        u16 tempData[TEX_material_LdotN * TEX_material_LdotH * TEX_material_Count];

        D3D_TEXTURE3D_DESC desc;
        desc.Width = TEX_material_LdotN;
        desc.Height = TEX_material_LdotH;
        desc.Depth = TEX_material_Count;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8_UNORM;
        desc.Usage = D3D_USAGE_IMMUTABLE;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        D3D_SUBRESOURCE_DATA subData;

        subData.pSysMem = tempData;
        subData.SysMemPitch = desc.Width * 2;
        subData.SysMemSlicePitch = desc.Height * subData.SysMemPitch;

        // Fill it (addr: x=dot(L,N),y=dot(L,H))
        // D3DLOCKED_BOX				R;
        // R_CHK						(t_material_surf->LockBox	(0,&R,0,0));
        for (u32 slice = 0; slice < TEX_material_Count; slice++)
        {
            for (u32 y = 0; y < TEX_material_LdotH; y++)
            {
                for (u32 x = 0; x < TEX_material_LdotN; x++)
                {
                    u16* p = (u16*)((u8*)(subData.pSysMem) + slice * subData.SysMemSlicePitch +
                        y * subData.SysMemPitch + x * 2);
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
        // R_CHK		(t_material_surf->UnlockBox	(0));

        R_CHK(HW.pDevice->CreateTexture3D(&desc, &subData, &t_material_surf));
        t_material = RImplementation.Resources->_CreateTexture(r2_material);
        t_material->surface_set(t_material_surf);
        // R_CHK
        // (D3DXCreateVolumeTexture(HW.pDevice,TEX_material_LdotN,TEX_material_LdotH,4,1,0,D3DFMT_A8L8,D3DPOOL_MANAGED,&t_material_surf));
        // t_material					= dxRenderDeviceRender::Instance().Resources->_CreateTexture(r2_material);
        // t_material->surface_set		(t_material_surf);

        // #ifdef DEBUG
        // R_CHK	(D3DXSaveTextureToFile	("x:" DELIMITER "r2_material.dds",D3DXIFF_DDS,t_material_surf,0));
        // #endif
    }

    // Build noise table
    if (1)
    {
        // Surfaces
        // D3DLOCKED_RECT				R[TEX_jitter_count];

        // for (int it=0; it<TEX_jitter_count; it++)
        //{
        //	string_path					name;
        //	xr_sprintf						(name,"%s%d",r2_jitter,it);
        //	R_CHK	(D3DXCreateTexture
        //(HW.pDevice,TEX_jitter,TEX_jitter,1,0,D3DFMT_Q8W8V8U8,D3DPOOL_MANAGED,&t_noise_surf[it]));
        //	t_noise[it]					= dxRenderDeviceRender::Instance().Resources->_CreateTexture	(name);
        //	t_noise[it]->surface_set	(t_noise_surf[it]);
        //	R_CHK						(t_noise_surf[it]->LockRect	(0,&R[it],0,0));
        //}
        //	Use DXGI_FORMAT_R8G8B8A8_SNORM

        static const int sampleSize = 4;
        u32 tempData[TEX_jitter_count][TEX_jitter * TEX_jitter];

        D3D_TEXTURE2D_DESC desc;
        desc.Width = TEX_jitter;
        desc.Height = TEX_jitter;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
        // desc.Usage = D3D_USAGE_IMMUTABLE;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        D3D_SUBRESOURCE_DATA subData[TEX_jitter_count];

        for (int it = 0; it < TEX_jitter_count - 1; it++)
        {
            subData[it].pSysMem = tempData[it];
            subData[it].SysMemPitch = desc.Width * sampleSize;
        }

        // Fill it,
        for (u32 y = 0; y < TEX_jitter; y++)
        {
            for (u32 x = 0; x < TEX_jitter; x++)
            {
                u32 data[TEX_jitter_count - 1];
                generate_jitter(data, TEX_jitter_count - 1);
                for (u32 it = 0; it < TEX_jitter_count - 1; it++)
                {
                    u32* p = (u32*)((u8*)(subData[it].pSysMem) + y * subData[it].SysMemPitch + x * 4);

                    *p = data[it];
                }
            }
        }

        // for (int it=0; it<TEX_jitter_count; it++)	{
        //	R_CHK						(t_noise_surf[it]->UnlockRect(0));
        //}

        for (int it = 0; it < TEX_jitter_count - 1; it++)
        {
            string_path name;
            xr_sprintf(name, "%s%d", r2_jitter, it);
            // R_CHK	(D3DXCreateTexture
            // (HW.pDevice,TEX_jitter,TEX_jitter,1,0,D3DFMT_Q8W8V8U8,D3DPOOL_MANAGED,&t_noise_surf[it]));
            R_CHK(HW.pDevice->CreateTexture2D(&desc, &subData[it], &t_noise_surf[it]));
            t_noise[it] = RImplementation.Resources->_CreateTexture(name);
            t_noise[it]->surface_set(t_noise_surf[it]);
            // R_CHK						(t_noise_surf[it]->LockRect	(0,&R[it],0,0));
        }

        float tempDataHBAO[TEX_jitter * TEX_jitter * 4];

        // generate HBAO jitter texture (last)
        D3D_TEXTURE2D_DESC descHBAO;
        descHBAO.Width = TEX_jitter;
        descHBAO.Height = TEX_jitter;
        descHBAO.MipLevels = 1;
        descHBAO.ArraySize = 1;
        descHBAO.SampleDesc.Count = 1;
        descHBAO.SampleDesc.Quality = 0;
        descHBAO.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        // desc.Usage = D3D_USAGE_IMMUTABLE;
        descHBAO.Usage = D3D_USAGE_DEFAULT;
        descHBAO.BindFlags = D3D_BIND_SHADER_RESOURCE;
        descHBAO.CPUAccessFlags = 0;
        descHBAO.MiscFlags = 0;

        int it = TEX_jitter_count - 1;
        subData[it].pSysMem = tempDataHBAO;
        subData[it].SysMemPitch = descHBAO.Width * sampleSize * sizeof(float);

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
                case 4: numDir = 8.0f; break;
                }
                float angle = 2 * PI * ::Random.randF(0.0f, 1.0f) / numDir;
                float dist = ::Random.randF(0.0f, 1.0f);

                float* p =
                    (float*)((u8*)(subData[it].pSysMem) + y * subData[it].SysMemPitch + x * 4 * sizeof(float));
                *p = (float)(_cos(angle));
                *(p + 1) = (float)(_sin(angle));
                *(p + 2) = (float)(dist);
                *(p + 3) = 0;
            }
        }

        string_path name;
        xr_sprintf(name, "%s%d", r2_jitter, it);
        // R_CHK	(D3DXCreateTexture
        // (HW.pDevice,TEX_jitter,TEX_jitter,1,0,D3DFMT_Q8W8V8U8,D3DPOOL_MANAGED,&t_noise_surf[it]));
        R_CHK(HW.pDevice->CreateTexture2D(&descHBAO, &subData[it], &t_noise_surf[it]));
        t_noise[it] = RImplementation.Resources->_CreateTexture(name);
        t_noise[it]->surface_set(t_noise_surf[it]);

        //	Create noise mipped
        {
            //	Autogen mipmaps
            desc.MipLevels = 0;
            R_CHK(HW.pDevice->CreateTexture2D(&desc, 0, &t_noise_surf_mipped));
            t_noise_mipped = RImplementation.Resources->_CreateTexture(r2_jitter_mipped);
            t_noise_mipped->surface_set(t_noise_surf_mipped);

            //	Update texture. Generate mips.

            HW.pContext->CopySubresourceRegion(t_noise_surf_mipped, 0, 0, 0, 0, t_noise_surf[0], 0, 0);

            D3DX11FilterTexture(HW.pContext, t_noise_surf_mipped, 0, D3DX10_FILTER_POINT);
        }
    }
}
