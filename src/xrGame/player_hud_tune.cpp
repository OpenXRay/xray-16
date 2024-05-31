#include "StdAfx.h"
#include "player_hud.h"
#include "Level.h"
#include "debug_renderer.h"
#include "xrEngine/xr_input.h"
#include "HUDManager.h"
#include "HudItem.h"
#include "xrEngine/Effector.h"
#include "xrEngine/CameraManager.h"
#include "xrEngine/FDemoRecord.h"
#include "xrUICore/ui_base.h"
#include "debug_renderer.h"
#include "xrEngine/GameFont.h"
#include "player_hud_tune.h"

u32 hud_adj_mode = 0;
u32 hud_adj_item_idx = 0;
// "press SHIFT+NUM 0-return 1-hud_pos 2-hud_rot 3-itm_pos 4-itm_rot 5-fire_point 6-fire_2_point 7-shell_point";

float _delta_pos = 0.0005f;
float _delta_rot = 0.05f;
extern ENGINE_API float psHUD_FOV;

CHudTuner::CHudTuner()
{
    ImGui::SetCurrentContext(Device.GetImGuiContext());
    paused = fsimilar(Device.time_factor(), EPS);
}

void CHudTuner::SetDefaultValues()
{
    if (current_hud_item)
    {
        curr_hand_pos = current_hud_item->m_measures.m_hands_attach[0];
        curr_hand_rot = current_hud_item->m_measures.m_hands_attach[1];
        curr_hand_pos_offset = current_hud_item->m_measures.m_hands_offset[0][0];
        curr_hand_rot_offset = current_hud_item->m_measures.m_hands_offset[1][0];
        curr_hand_pos_offset_aim = current_hud_item->m_measures.m_hands_offset[0][1];
        curr_hand_rot_offset_aim = current_hud_item->m_measures.m_hands_offset[1][1];
        curr_hand_pos_offset_gl = current_hud_item->m_measures.m_hands_offset[0][2];
        curr_hand_rot_offset_gl = current_hud_item->m_measures.m_hands_offset[1][2];
        curr_item_attach_pos_offset = current_hud_item->m_measures.m_item_attach[0];
        curr_item_attach_rot_offset = current_hud_item->m_measures.m_item_attach[1];
        curr_fire_point_offset = current_hud_item->m_measures.m_fire_point_offset;
        curr_fire_point_2_offset = current_hud_item->m_measures.m_fire_point2_offset;
        curr_shell_point_offset = current_hud_item->m_measures.m_shell_point_offset;
    }
    else
    {
        Fvector zero = { 0, 0, 0 };
        curr_hand_pos = zero;
        curr_hand_rot = zero;
        new_hand_pos_offset = curr_hand_pos_offset = zero;
        new_hand_rot_offset = curr_hand_rot_offset = zero;
        new_hand_pos_offset_aim = curr_hand_pos_offset_aim = zero;
        new_hand_rot_offset_aim = curr_hand_rot_offset_aim = zero;
        new_hand_pos_offset_gl = curr_hand_pos_offset_gl = zero;
        new_hand_rot_offset_gl = curr_hand_rot_offset_gl = zero;
        new_item_attach_pos_offset = curr_item_attach_pos_offset = zero;
        new_item_attach_rot_offset = curr_item_attach_rot_offset = zero;
        new_fire_point_offset = curr_fire_point_offset = zero;
        new_fire_point_2_offset = curr_fire_point_2_offset = zero;
        new_shell_point_offset = curr_shell_point_offset = zero;
    }
}

void CHudTuner::ResetToDefaultValues()
{
    new_hand_pos = curr_hand_pos;
    new_hand_rot = curr_hand_rot;
    new_hand_pos_offset = curr_hand_pos_offset;
    new_hand_rot_offset = curr_hand_rot_offset;
    new_hand_pos_offset_aim = curr_hand_pos_offset_aim;
    new_hand_rot_offset_aim = curr_hand_rot_offset_aim;
    new_hand_pos_offset_gl = curr_hand_pos_offset_gl;
    new_hand_rot_offset_gl = curr_hand_rot_offset_gl;
    new_item_attach_pos_offset = curr_item_attach_pos_offset;
    new_item_attach_rot_offset = curr_item_attach_rot_offset;
    new_fire_point_offset = curr_fire_point_offset;
    new_fire_point_2_offset = curr_fire_point_2_offset;
    new_shell_point_offset = curr_shell_point_offset;
}

void CHudTuner::UpdateValues()
{
    if (current_hud_item)
    {
        current_hud_item->m_measures.m_hands_offset[0][0] = new_hand_pos_offset;
        current_hud_item->m_measures.m_hands_offset[1][0] = new_hand_rot_offset;
        current_hud_item->m_measures.m_hands_offset[0][1] = new_hand_pos_offset_aim;
        current_hud_item->m_measures.m_hands_offset[1][1] = new_hand_rot_offset_aim;
        current_hud_item->m_measures.m_hands_offset[0][2] = new_hand_pos_offset_gl;
        current_hud_item->m_measures.m_hands_offset[1][2] = new_hand_rot_offset_gl;
        current_hud_item->m_measures.m_item_attach[0] = new_item_attach_pos_offset;
        current_hud_item->m_measures.m_item_attach[1] = new_item_attach_rot_offset;
        current_hud_item->m_measures.m_fire_point_offset = new_fire_point_offset;
        current_hud_item->m_measures.m_fire_point2_offset = new_fire_point_2_offset;
        current_hud_item->m_measures.m_shell_point_offset = new_shell_point_offset;

        u8 idx = current_hud_item->m_parent_hud_item->GetCurrentHudOffsetIdx();
        new_hand_pos.set(curr_hand_pos);
        new_hand_rot.set(curr_hand_rot);
        new_hand_pos.add(current_hud_item->m_measures.m_hands_offset[0][0]);
        new_hand_rot.add(current_hud_item->m_measures.m_hands_offset[1][0]);

        current_hud_item->m_measures.m_hands_attach[0] = new_hand_pos;
        current_hud_item->m_measures.m_hands_attach[1] = new_hand_rot;
    }
}

void CHudTuner::OnFrame()
{
#ifndef MASTER_GOLD
    if (!get_open_state())
        return;

    if (!g_player_hud)
        return;

    auto hud_item = g_player_hud->attached_item(current_hud_idx);
    if (current_hud_item != hud_item)
    {
        current_hud_item = hud_item;
        SetDefaultValues();
        ResetToDefaultValues();
    }

    if (ImGui::Begin(tool_name(), &get_open_state(), get_default_window_flags()))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::RadioButton("Pause", paused))
            {
                paused = !paused;
                float time_factor = 1.f;
                if (paused)
                {
                    time_factor = EPS;
                }
                Device.time_factor(time_factor);
            }

            ImGui::EndMenuBar();
        }

        if (ImGui::CollapsingHeader("Main Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginCombo("Hud Item Mode", hud_item_mode[current_hud_idx]))
            {
                for (const auto& [idx, value] : hud_item_mode)
                {
                    if (ImGui::Selectable(value, current_hud_idx == idx))
                    {
                        current_hud_idx = idx;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::LabelText("Current item", "%s", hud_item ? hud_item->m_sect_name.c_str() : "none");

            ImGui::SliderFloat("HUD FOV", &psHUD_FOV, 0.1f, 1.0f);

            ImGui::NewLine();

            ImGui::SliderFloat("Position step", &_delta_pos, 0.0000001f, 0.001f, "%.7f");
            ImGui::SliderFloat("Rotation step", &_delta_rot, 0.000001f, 0.1f, "%.6f");

            ImGui::DragFloat3(hud_adj_modes[HUD_POS], (float*)&new_hand_pos_offset, _delta_pos, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[HUD_ROT], (float*)&new_hand_rot_offset, _delta_rot, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[HUD_POS_AIM], (float*)&new_hand_pos_offset_aim, _delta_pos, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[HUD_ROT_AIM], (float*)&new_hand_rot_offset_aim, _delta_rot, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[HUD_POS_GL], (float*)&new_hand_pos_offset_gl, _delta_pos, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[HUD_ROT_GL], (float*)&new_hand_rot_offset_gl, _delta_rot, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[ITEM_POS], (float*)&new_item_attach_pos_offset, _delta_pos, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[ITEM_ROT], (float*)&new_item_attach_rot_offset, _delta_rot, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[FIRE_POINT], (float*)&new_fire_point_offset, _delta_pos, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[FIRE_POINT_2], (float*)&new_fire_point_2_offset, _delta_pos, 0.f, 0.f, "%.7f");
            ImGui::DragFloat3(hud_adj_modes[SHELL_POINT], (float*)&new_shell_point_offset, _delta_pos, 0.f, 0.f, "%.7f");

            UpdateValues();

            string128 selectable;

            if (current_hud_item)
            {
                bool is_16x9 = UI().is_widescreen();
                shared_str m_sect_name = current_hud_item->m_sect_name;

                ImGuiIO& io = ImGui::GetIO();

                if (ImGui::Button("Copy formatted values to clipboard"))
                {
                    ImGui::LogToClipboard();
                    xr_sprintf(selectable, "[%s]\n", m_sect_name.c_str());
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "item_position = %f,%f,%f\n", new_item_attach_pos_offset.x, new_item_attach_pos_offset.y, new_item_attach_pos_offset.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "item_orientation = %f,%f,%f\n", new_item_attach_rot_offset.x, new_item_attach_rot_offset.y, new_item_attach_rot_offset.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "fire_point = %f,%f,%f\n", new_fire_point_offset.x, new_fire_point_offset.y, new_fire_point_offset.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "fire_point = %f,%f,%f\n", new_fire_point_2_offset.x, new_fire_point_2_offset.y, new_fire_point_2_offset.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "shell_point = %f,%f,%f\n", new_shell_point_offset.x, new_shell_point_offset.y, new_shell_point_offset.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "hands_position%s = %f,%f,%f\n", (is_16x9) ? "_16x9" : "", new_hand_pos.x, new_hand_pos.y, new_hand_pos.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "hands_orientation%s = %f,%f,%f\n", (is_16x9) ? "_16x9" : "", new_hand_rot.x, new_hand_rot.y, new_hand_rot.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "aim_hud_offset_pos%s = %f,%f,%f\n", (is_16x9) ? "_16x9" : "", new_hand_pos_offset_aim.x, new_hand_pos_offset_aim.y, new_hand_pos_offset_aim.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "aim_hud_offset_rot%s = %f,%f,%f\n", (is_16x9) ? "_16x9" : "", new_hand_rot_offset_aim.x, new_hand_rot_offset_aim.y, new_hand_rot_offset_aim.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "gl_hud_offset_pos%s = %f,%f,%f\n", (is_16x9) ? "_16x9" : "", new_hand_pos_offset_gl.x, new_hand_pos_offset_gl.y, new_hand_pos_offset_gl.z);
                    ImGui::LogText(selectable);
                    xr_sprintf(selectable, "gl_hud_offset_rot%s = %f,%f,%f\n", (is_16x9) ? "_16x9" : "", new_hand_rot_offset_gl.x, new_hand_rot_offset_gl.y, new_hand_rot_offset_gl.z);
                    ImGui::LogText(selectable);
                    ImGui::LogFinish();
                }

                ImGui::NewLine();

#ifdef DEBUG
                firedeps fd;
                current_hud_item->setup_firedeps(fd);
                collide::rq_result& RQ = HUD().GetCurrentRayQuery();

                CDebugRenderer& render = Level().debug_renderer();

                ImGui::SliderFloat("Debug Point Size", &debug_point_size, 0.00005f, 1.f, "%.5f");
                if (ImGui::RadioButton("Draw Fire Point", draw_fp)) { draw_fp = !draw_fp; }; ImGui::SameLine();
                if (ImGui::RadioButton("Draw Fire Point (GL)", draw_fp2)) { draw_fp2 = !draw_fp2; } ImGui::SameLine();
                if (ImGui::RadioButton("Draw Fire Direction", draw_fd)) { draw_fd = !draw_fd; }
                if (ImGui::RadioButton("Draw Fire Direction (GL)", draw_fd2)) { draw_fd2 = !draw_fd2; } ImGui::SameLine();
                if (ImGui::RadioButton("Draw Shell Point", draw_sp)) { draw_sp = !draw_sp; }

                if (draw_fp)
                {
                    Fvector point;
                    point.set(fd.vLastFP);
                    current_hud_item->m_parent_hud_item->TransformPosFromWorldToHud(point);
                    render.draw_aabb(point, debug_point_size, debug_point_size, debug_point_size, color_xrgb(255, 0, 0));
                }
                
                if (draw_fp2)
                {
                    Fvector point;
                    point.set(fd.vLastFP2);
                    current_hud_item->m_parent_hud_item->TransformPosFromWorldToHud(point);
                    render.draw_aabb(point, debug_point_size, debug_point_size, debug_point_size, color_xrgb(255, 0, 0));
                }

                if (draw_fd)
                {
                    Fvector point;
                    Fvector dir;
                    point.set(fd.vLastFP);
                    dir.set(fd.vLastFD);
                    current_hud_item->m_parent_hud_item->TransformPosFromWorldToHud(point);
                    current_hud_item->m_parent_hud_item->TransformDirFromWorldToHud(dir);

                    Fvector parallelPoint;
                    parallelPoint.set(point);
                    parallelPoint.mad(dir, RQ.range);
                    render.draw_aabb(parallelPoint, debug_point_size, debug_point_size, debug_point_size, color_xrgb(255, 0, 0));
                    render.draw_line(Fidentity, point, parallelPoint, color_xrgb(255, 0, 0));
                }

                if (draw_fd2)
                {
                    Fvector point;
                    Fvector dir;
                    point.set(fd.vLastFP2);
                    dir.set(fd.vLastFD);
                    current_hud_item->m_parent_hud_item->TransformPosFromWorldToHud(point);
                    current_hud_item->m_parent_hud_item->TransformDirFromWorldToHud(dir);

                    Fvector parallelPoint;
                    parallelPoint.set(point);
                    parallelPoint.mad(dir, RQ.range);
                    render.draw_aabb(parallelPoint, debug_point_size, debug_point_size, debug_point_size, color_xrgb(255, 0, 0));
                    render.draw_line(Fidentity, point, parallelPoint, color_xrgb(255, 0, 0));
                }

                if (draw_sp)
                {
                    Fvector point;
                    point.set(fd.vLastSP);
                    current_hud_item->m_parent_hud_item->TransformPosFromWorldToHud(point);
                    render.draw_aabb(point, debug_point_size, debug_point_size, debug_point_size, color_xrgb(255, 0, 0));
                }
#endif // DEBUG
            }

            if (ImGui::Button("Reset to default values"))
            {
                ResetToDefaultValues();
            }
        }
    }
    ImGui::End();
#endif
}

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
            Msg("item_position				= %f,%f,%f", m_measures.m_item_attach[0].x, m_measures.m_item_attach[0].y, m_measures.m_item_attach[0].z);
            Msg("item_orientation			= %f,%f,%f", m_measures.m_item_attach[1].x, m_measures.m_item_attach[1].y, m_measures.m_item_attach[1].z);
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
            Msg("fire_point				= %f,%f,%f", m_measures.m_fire_point_offset.x, m_measures.m_fire_point_offset.y, m_measures.m_fire_point_offset.z);
            Msg("fire_point2			= %f,%f,%f", m_measures.m_fire_point2_offset.x, m_measures.m_fire_point2_offset.y, m_measures.m_fire_point2_offset.z);
            Msg("shell_point			= %f,%f,%f", m_measures.m_shell_point_offset.x, m_measures.m_shell_point_offset.y, m_measures.m_shell_point_offset.z);
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

        attachable_hud_item* hi = m_attached_items[hud_adj_item_idx];
        if (!hi)
            return;

        u8 idx = hi->m_parent_hud_item->GetCurrentHudOffsetIdx();
        if (idx)
            _curr_dr /= 20.0f;

        Fvector& pos_ = (idx != 0) ? hi->hands_offset_pos() : hi->hands_attach_pos();
        Fvector& rot_ = (idx != 0) ? hi->hands_offset_rot() : hi->hands_attach_rot();

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
                Msg("[%s]", hi->m_sect_name.c_str());
                Msg("hands_position%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
                Msg("hands_orientation%s			= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
                Log("-----------");
            }
            else if (idx == 1)
            {
                Msg("[%s]", hi->m_sect_name.c_str());
                Msg("aim_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
                Msg("aim_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
                Log("-----------");
            }
            else if (idx == 2)
            {
                Msg("[%s]", hi->m_sect_name.c_str());
                Msg("gl_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
                Msg("gl_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
                Log("-----------");
            }
        }
    }
    else if (hud_adj_mode == 8 || hud_adj_mode == 9)
    {
        if (hud_adj_mode == 8 && (values.z))
            _delta_pos += (values.z > 0) ? _delta_pos * 0.1f : _delta_pos * -0.1f;

        if (hud_adj_mode == 9 && (values.z))
            _delta_rot += (values.z > 0) ? _delta_rot * .1f : _delta_rot * -0.1f;
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
        if (dik == SDL_SCANCODE_KP_0 || dik == SDL_SCANCODE_0)
            hud_adj_mode = 0;
        if (dik == SDL_SCANCODE_KP_1 || dik == SDL_SCANCODE_1)
            hud_adj_mode = 1;
        if (dik == SDL_SCANCODE_KP_2 || dik == SDL_SCANCODE_2)
            hud_adj_mode = 2;
        if (dik == SDL_SCANCODE_KP_3 || dik == SDL_SCANCODE_3)
            hud_adj_mode = 3;
        if (dik == SDL_SCANCODE_KP_4 || dik == SDL_SCANCODE_4)
            hud_adj_mode = 4;
        if (dik == SDL_SCANCODE_KP_5 || dik == SDL_SCANCODE_5)
            hud_adj_mode = 5;
        if (dik == SDL_SCANCODE_KP_6 || dik == SDL_SCANCODE_6)
            hud_adj_mode = 6;
        if (dik == SDL_SCANCODE_KP_7 || dik == SDL_SCANCODE_7)
            hud_adj_mode = 7;
        if (dik == SDL_SCANCODE_KP_8 || dik == SDL_SCANCODE_8)
            hud_adj_mode = 8;
        if (dik == SDL_SCANCODE_KP_9 || dik == SDL_SCANCODE_9)
            hud_adj_mode = 9;
    }
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LCTRL))
    {
        if (dik == SDL_SCANCODE_KP_0 || dik == SDL_SCANCODE_0)
            hud_adj_item_idx = 0;
        if (dik == SDL_SCANCODE_KP_1 || dik == SDL_SCANCODE_1)
            hud_adj_item_idx = 1;
    }
}
