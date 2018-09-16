#include "stdafx.h"
#include "player_hud.h"
#include "Level.h"
#include "debug_renderer.h"
#include "xrEngine/xr_input.h"
#include "HudManager.h"
#include "HudItem.h"
#include "xrEngine/Effector.h"
#include "xrEngine/CameraManager.h"
#include "xrEngine/FDemoRecord.h"
#include "ui_base.h"
#include "debug_renderer.h"
#include "xrEngine/GameFont.h"

u32 hud_adj_mode = 0;
u32 hud_adj_item_idx = 0;
// "press SHIFT+NUM 0-return 1-hud_pos 2-hud_rot 3-itm_pos 4-itm_rot 5-fire_point 6-fire_2_point 7-shell_point";

float _delta_pos = 0.0005f;
float _delta_rot = 0.05f;

bool is_attachable_item_tuning_mode()
{
    return pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT) || pInput->iGetAsyncKeyState(SDL_SCANCODE_Z) ||
        pInput->iGetAsyncKeyState(SDL_SCANCODE_X) || pInput->iGetAsyncKeyState(SDL_SCANCODE_C);
}

void tune_remap(const Ivector& in_values, Ivector& out_values)
{
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
    {
        out_values = in_values;
    }
    else if (pInput->iGetAsyncKeyState(SDL_SCANCODE_Z))
    { // strict by X
        out_values.x = in_values.y;
        out_values.y = 0;
        out_values.z = 0;
    }
    else if (pInput->iGetAsyncKeyState(SDL_SCANCODE_X))
    { // strict by Y
        out_values.x = 0;
        out_values.y = in_values.y;
        out_values.z = 0;
    }
    else if (pInput->iGetAsyncKeyState(SDL_SCANCODE_C))
    { // strict by Z
        out_values.x = 0;
        out_values.y = 0;
        out_values.z = in_values.y;
    }
    else
    {
        out_values.set(0, 0, 0);
    }
}

void calc_cam_diff_pos(Fmatrix item_transform, Fvector diff, Fvector& res)
{
    Fmatrix cam_m;
    cam_m.i.set(Device.vCameraRight);
    cam_m.j.set(Device.vCameraTop);
    cam_m.k.set(Device.vCameraDirection);
    cam_m.c.set(Device.vCameraPosition);

    Fvector res1;
    cam_m.transform_dir(res1, diff);

    Fmatrix item_transform_i;
    item_transform_i.invert(item_transform);
    item_transform_i.transform_dir(res, res1);
}

void calc_cam_diff_rot(Fmatrix item_transform, Fvector diff, Fvector& res)
{
    Fmatrix cam_m;
    cam_m.i.set(Device.vCameraRight);
    cam_m.j.set(Device.vCameraTop);
    cam_m.k.set(Device.vCameraDirection);
    cam_m.c.set(Device.vCameraPosition);

    Fmatrix R;
    R.identity();
    if (!fis_zero(diff.x))
    {
        R.rotation(cam_m.i, diff.x);
    }
    else if (!fis_zero(diff.y))
    {
        R.rotation(cam_m.j, diff.y);
    }
    else if (!fis_zero(diff.z))
    {
        R.rotation(cam_m.k, diff.z);
    };

    Fmatrix item_transform_i;
    item_transform_i.invert(item_transform);
    R.mulB_43(item_transform);
    R.mulA_43(item_transform_i);

    R.getHPB(res);

    res.mul(180.0f / PI);
}

void attachable_hud_item::tune(Ivector values)
{
#ifndef MASTER_GOLD
    if (!is_attachable_item_tuning_mode())
        return;

    Fvector diff;
    diff.set(0, 0, 0);

    if (hud_adj_mode == 3 || hud_adj_mode == 4)
    {
        if (hud_adj_mode == 3)
        {
            if (values.x)
                diff.x = (values.x > 0) ? _delta_pos : -_delta_pos;
            if (values.y)
                diff.y = (values.y > 0) ? _delta_pos : -_delta_pos;
            if (values.z)
                diff.z = (values.z > 0) ? _delta_pos : -_delta_pos;

            Fvector d;
            Fmatrix ancor_m;
            m_parent->calc_transform(m_attach_place_idx, Fidentity, ancor_m);
            calc_cam_diff_pos(ancor_m, diff, d);
            m_measures.m_item_attach[0].add(d);
        }
        else if (hud_adj_mode == 4)
        {
            if (values.x)
                diff.x = (values.x > 0) ? _delta_rot : -_delta_rot;
            if (values.y)
                diff.y = (values.y > 0) ? _delta_rot : -_delta_rot;
            if (values.z)
                diff.z = (values.z > 0) ? _delta_rot : -_delta_rot;

            Fvector d;
            Fmatrix ancor_m;
            m_parent->calc_transform(m_attach_place_idx, Fidentity, ancor_m);

            calc_cam_diff_pos(m_item_transform, diff, d);
            m_measures.m_item_attach[1].add(d);
        }

        if ((values.x) || (values.y) || (values.z))
        {
            Msg("[%s]", m_sect_name.c_str());
            Msg("item_position				= %f,%f,%f", m_measures.m_item_attach[0].x, m_measures.m_item_attach[0].y,
                m_measures.m_item_attach[0].z);
            Msg("item_orientation			= %f,%f,%f", m_measures.m_item_attach[1].x, m_measures.m_item_attach[1].y,
                m_measures.m_item_attach[1].z);
            Log("-----------");
        }
    }

    if (hud_adj_mode == 5 || hud_adj_mode == 6 || hud_adj_mode == 7)
    {
        if (values.x)
            diff.x = (values.x > 0) ? _delta_pos : -_delta_pos;
        if (values.y)
            diff.y = (values.y > 0) ? _delta_pos : -_delta_pos;
        if (values.z)
            diff.z = (values.z > 0) ? _delta_pos : -_delta_pos;

        if (hud_adj_mode == 5)
        {
            m_measures.m_fire_point_offset.add(diff);
        }
        if (hud_adj_mode == 6)
        {
            m_measures.m_fire_point2_offset.add(diff);
        }
        if (hud_adj_mode == 7)
        {
            m_measures.m_shell_point_offset.add(diff);
        }
        if ((values.x) || (values.y) || (values.z))
        {
            Msg("[%s]", m_sect_name.c_str());
            Msg("fire_point				= %f,%f,%f", m_measures.m_fire_point_offset.x, m_measures.m_fire_point_offset.y,
                m_measures.m_fire_point_offset.z);
            Msg("fire_point2			= %f,%f,%f", m_measures.m_fire_point2_offset.x,
                m_measures.m_fire_point2_offset.y, m_measures.m_fire_point2_offset.z);
            Msg("shell_point			= %f,%f,%f", m_measures.m_shell_point_offset.x,
                m_measures.m_shell_point_offset.y, m_measures.m_shell_point_offset.z);
            Log("-----------");
        }
    }
#endif // #ifndef MASTER_GOLD
}

void attachable_hud_item::debug_draw_firedeps()
{
#ifdef DEBUG
    bool bForce = (hud_adj_mode == 3 || hud_adj_mode == 4);

    if (hud_adj_mode == 5 || hud_adj_mode == 6 || hud_adj_mode == 7 || bForce)
    {
        CDebugRenderer& render = Level().debug_renderer();

        firedeps fd;
        setup_firedeps(fd);

        if (hud_adj_mode == 5 || bForce)
            render.draw_aabb(fd.vLastFP, 0.005f, 0.005f, 0.005f, color_xrgb(255, 0, 0));

        if (hud_adj_mode == 6)
            render.draw_aabb(fd.vLastFP2, 0.005f, 0.005f, 0.005f, color_xrgb(0, 0, 255));

        if (hud_adj_mode == 7)
            render.draw_aabb(fd.vLastSP, 0.005f, 0.005f, 0.005f, color_xrgb(0, 255, 0));
    }
#endif // DEBUG
}

void player_hud::tune(Ivector _values)
{
#ifndef MASTER_GOLD
    Ivector values;
    tune_remap(_values, values);

    bool is_16x9 = UI().is_widescreen();

    if (hud_adj_mode == 1 || hud_adj_mode == 2)
    {
        Fvector diff;
        diff.set(0, 0, 0);

        float _curr_dr = _delta_rot;

        u8 idx = m_attached_items[hud_adj_item_idx]->m_parent_hud_item->GetCurrentHudOffsetIdx();
        if (idx)
            _curr_dr /= 20.0f;

        Fvector& pos_ = (idx != 0) ? m_attached_items[hud_adj_item_idx]->hands_offset_pos() :
                                     m_attached_items[hud_adj_item_idx]->hands_attach_pos();
        Fvector& rot_ = (idx != 0) ? m_attached_items[hud_adj_item_idx]->hands_offset_rot() :
                                     m_attached_items[hud_adj_item_idx]->hands_attach_rot();

        if (hud_adj_mode == 1)
        {
            if (values.x)
                diff.x = (values.x < 0) ? _delta_pos : -_delta_pos;
            if (values.y)
                diff.y = (values.y > 0) ? _delta_pos : -_delta_pos;
            if (values.z)
                diff.z = (values.z > 0) ? _delta_pos : -_delta_pos;

            pos_.add(diff);
        }

        if (hud_adj_mode == 2)
        {
            if (values.x)
                diff.x = (values.x > 0) ? _curr_dr : -_curr_dr;
            if (values.y)
                diff.y = (values.y > 0) ? _curr_dr : -_curr_dr;
            if (values.z)
                diff.z = (values.z > 0) ? _curr_dr : -_curr_dr;

            rot_.add(diff);
        }
        if ((values.x) || (values.y) || (values.z))
        {
            if (idx == 0)
            {
                Msg("[%s]", m_attached_items[hud_adj_item_idx]->m_sect_name.c_str());
                Msg("hands_position%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
                Msg("hands_orientation%s			= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
                Log("-----------");
            }
            else if (idx == 1)
            {
                Msg("[%s]", m_attached_items[hud_adj_item_idx]->m_sect_name.c_str());
                Msg("aim_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
                Msg("aim_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
                Log("-----------");
            }
            else if (idx == 2)
            {
                Msg("[%s]", m_attached_items[hud_adj_item_idx]->m_sect_name.c_str());
                Msg("gl_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
                Msg("gl_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
                Log("-----------");
            }
        }
    }
    else if (hud_adj_mode == 8 || hud_adj_mode == 9)
    {
        if (hud_adj_mode == 8 && (values.z))
            _delta_pos += (values.z > 0) ? 0.001f : -0.001f;

        if (hud_adj_mode == 9 && (values.z))
            _delta_rot += (values.z > 0) ? 0.1f : -0.1f;
    }
    else
    {
        attachable_hud_item* hi = m_attached_items[hud_adj_item_idx];
        if (!hi)
            return;
        hi->tune(values);
    }
#endif // #ifndef MASTER_GOLD
}

void hud_draw_adjust_mode()
{
    if (!hud_adj_mode)
        return;

    LPCSTR _text = NULL;
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT) && hud_adj_mode)
        _text =
            "press SHIFT+NUM 0-return 1-hud_pos 2-hud_rot 3-itm_pos 4-itm_rot 5-fire_point 6-fire_2_point "
            "7-shell_point "
            "8-pos_step 9-rot_step";

    switch (hud_adj_mode)
    {
    case 1: _text = "adjusting HUD POSITION"; break;
    case 2: _text = "adjusting HUD ROTATION"; break;
    case 3: _text = "adjusting ITEM POSITION"; break;
    case 4: _text = "adjusting ITEM ROTATION"; break;
    case 5: _text = "adjusting FIRE POINT"; break;
    case 6: _text = "adjusting FIRE 2 POINT"; break;
    case 7: _text = "adjusting SHELL POINT"; break;
    case 8: _text = "adjusting pos STEP"; break;
    case 9: _text = "adjusting rot STEP"; break;
    };
    if (_text)
    {
        CGameFont* F = UI().Font().pFontDI;
        F->SetAligment(CGameFont::alCenter);
        F->OutSetI(0.f, -0.8f);
        F->SetColor(0xffffffff);
        F->OutNext(_text);
        F->OutNext("for item [%d]", hud_adj_item_idx);
        F->OutNext("delta values dP=%f dR=%f", _delta_pos, _delta_rot);
        F->OutNext("[Z]-x axis [X]-y axis [C]-z axis");
    }
}

void hud_adjust_mode_keyb(int dik)
{
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
    {
        if (dik == SDL_SCANCODE_KP_0)
            hud_adj_mode = 0;
        if (dik == SDL_SCANCODE_KP_1)
            hud_adj_mode = 1;
        if (dik == SDL_SCANCODE_KP_2)
            hud_adj_mode = 2;
        if (dik == SDL_SCANCODE_KP_3)
            hud_adj_mode = 3;
        if (dik == SDL_SCANCODE_KP_4)
            hud_adj_mode = 4;
        if (dik == SDL_SCANCODE_KP_5)
            hud_adj_mode = 5;
        if (dik == SDL_SCANCODE_KP_6)
            hud_adj_mode = 6;
        if (dik == SDL_SCANCODE_KP_7)
            hud_adj_mode = 7;
        if (dik == SDL_SCANCODE_KP_8)
            hud_adj_mode = 8;
        if (dik == SDL_SCANCODE_KP_9)
            hud_adj_mode = 9;
    }
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LCTRL))
    {
        if (dik == SDL_SCANCODE_KP_0)
            hud_adj_item_idx = 0;
        if (dik == SDL_SCANCODE_KP_1)
            hud_adj_item_idx = 1;
    }
}
