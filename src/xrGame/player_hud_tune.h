#pragma once

class CHudTuner final : public xray::editor::ide_tool
{
public:
    CHudTuner();
    void OnFrame() override;

    void OnAppDeactivate();

private:
    pcstr tool_name() override { return "Hud Tuner"; }

    void SetDefaultValues();
    void ResetToDefaultValues();
    void UpdateValues();

    enum hud_adj_mode_keys
    {
        HUD_POS = 0,
        HUD_ROT,
        HUD_POS_AIM,
        HUD_ROT_AIM,
        HUD_POS_GL,
        HUD_ROT_GL,
        ITEM_POS,
        ITEM_ROT,
        FIRE_POINT,
        FIRE_POINT_2,
        SHELL_POINT,
    };
    enum hud_item_idx
    {
        MAIN_ITEM = 0,
        OFFHAND_ITEM,
    };
    xr_map<hud_item_idx, pcstr> hud_item_mode
    {
        { MAIN_ITEM, "Main hand item" },
        { OFFHAND_ITEM, "Off hand item" },
    };
    xr_map<hud_adj_mode_keys, pcstr> hud_adj_modes =
    {
        {HUD_POS, "Hud Position (Default)"},
        {HUD_ROT, "Hud Rotation (Default)"},
        {HUD_POS_AIM, "Hud Position (Aiming)"},
        {HUD_ROT_AIM, "Hud Rotation (Aiming)"},
        {HUD_POS_GL, "Hud Position (GL)"},
        {HUD_ROT_GL, "Hud Rotation (GL)"},
        {ITEM_POS, "Item Position"},
        {ITEM_ROT, "Item Rotation"},
        {FIRE_POINT, "Fire Point"},
        {FIRE_POINT_2, "Fire Point 2"},
        {SHELL_POINT, "Shell Point"},
    };

    bool paused{};
    bool draw_fp{};
    bool draw_fp2{};
    bool draw_fd{};
    bool draw_fd2{};
    bool draw_sp{};

    float debug_point_size{ 0.005f };
    float _delta_pos{ 0.0005f };
    float _delta_rot{ 0.05f };

    attachable_hud_item* current_hud_item{};
    hud_item_idx current_hud_idx{ MAIN_ITEM };

    Fvector curr_hand_pos{};
    Fvector curr_hand_rot{};
    Fvector curr_hand_pos_offset{};
    Fvector curr_hand_rot_offset{};
    Fvector curr_hand_pos_offset_aim{};
    Fvector curr_hand_rot_offset_aim{};
    Fvector curr_hand_pos_offset_gl{};
    Fvector curr_hand_rot_offset_gl{};
    Fvector curr_item_attach_pos_offset{};
    Fvector curr_item_attach_rot_offset{};
    Fvector curr_fire_point_offset{};
    Fvector curr_fire_point_2_offset{};
    Fvector curr_shell_point_offset{};

    Fvector new_hand_pos{};
    Fvector new_hand_rot{};
    Fvector new_hand_pos_offset{};
    Fvector new_hand_rot_offset{};
    Fvector new_hand_pos_offset_aim{};
    Fvector new_hand_rot_offset_aim{};
    Fvector new_hand_pos_offset_gl{};
    Fvector new_hand_rot_offset_gl{};
    Fvector new_item_attach_pos_offset{};
    Fvector new_item_attach_rot_offset{};
    Fvector new_fire_point_offset{};
    Fvector new_fire_point_2_offset{};
    Fvector new_shell_point_offset{};
};
