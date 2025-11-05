#include "StdAfx.h"
#include "ozz_animation_utility.h"
#include "Level.h"
#include "Actor.h"
#include "xrEngine/xr_input.h"
#include "player_hud.h"

COzzAnimationUtility::COzzAnimationUtility()
{
    ImGui::SetCurrentContext(Device.GetImGuiContext());
    paused_ = fsimilar(Device.time_factor(), EPS);
}

bool COzzAnimationUtility::is_active() const
{
    return is_open() && Device.editor().IsActiveState();
}

OzzKinematicsAnimated* COzzAnimationUtility::GetCurrentAnimatedObject()
{
    // Prefer g_actor if available (most efficient for single player)
    if (g_actor && g_actor->Visual())
    {
        return smart_cast<OzzKinematicsAnimated*>(g_actor->Visual()->dcast_PKinematicsAnimated());
    }

    // Fallback: Try to get the current view entity
    if (Level().CurrentViewEntity())
    {
        CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
        if (actor && actor->Visual())
        {
            return smart_cast<OzzKinematicsAnimated*>(actor->Visual()->dcast_PKinematicsAnimated());
        }
    }

    return nullptr;
}

bool COzzAnimationUtility::IsInHudMode() const
{
    // Check if we have an actor
    if (!g_actor)
        return false;

    // Check if camera is in first-person mode
    if (g_actor->active_cam() != ACTOR_DEFS::eacFirstEye)
        return false;

    // Check if player HUD is active and valid
    if (!g_player_hud)
        return false;

    return true;
}

void COzzAnimationUtility::on_tool_frame()
{
#ifndef MASTER_GOLD
    if (!get_open_state())
        return;

    // Update current object reference
    const u32 current_frame = Device.dwFrame;
    if (current_frame != last_update_frame_)
    {
        last_update_frame_ = current_frame;

        // Check if we're in HUD mode
        is_hud_mode_ = IsInHudMode();

        if (is_hud_mode_)
        {
            // Update HUD mode objects
            auto populate_hud_item = [](HudItemState& item_state, IKinematics* model, const char* name)
            {
                OzzKinematicsAnimated* anim_obj = model ? smart_cast<OzzKinematicsAnimated*>(model->dcast_PKinematicsAnimated()) : nullptr;

                if (anim_obj != item_state.anim_object)
                {
                    item_state.anim_object = anim_obj;
                    item_state.available_animations.clear();
                    item_state.display_name = name;

                    if (anim_obj)
                    {
                        const u16 motion_count = anim_obj->GetAvailableMotionCount();
                        item_state.available_animations.reserve(motion_count);

                        for (u16 i = 0; i < motion_count; ++i)
                        {
                            xr_string motion_name;
                            if (anim_obj->GetMotionName(i, motion_name))
                            {
                                item_state.available_animations.push_back(shared_str(motion_name.c_str()));
                            }
                        }

                        if (motion_count > 0)
                            item_state.playback.current_animation_index = 0;
                        else
                            item_state.playback.current_animation_index = -1;
                    }
                }
            };

            // Update HUD hands model (the arms visual)
            // The player_hud->hands_model() returns the hands/arms IKinematicsAnimated model
            IKinematics* hands_model = g_player_hud->hands_model()->dcast_PKinematics();
            populate_hud_item(hud_hands_, hands_model, "HUD Arms");

            // Update attached item slot 0
            if (g_player_hud->attached_item(0) && g_player_hud->attached_item(0)->m_model)
            {
                populate_hud_item(hud_item_slot0_, g_player_hud->attached_item(0)->m_model,
                                  g_player_hud->attached_item(0)->m_sect_name.c_str());
            }
            else
            {
                hud_item_slot0_.anim_object = nullptr;
                hud_item_slot0_.available_animations.clear();
            }

            // Update attached item slot 1
            if (g_player_hud->attached_item(1) && g_player_hud->attached_item(1)->m_model)
            {
                populate_hud_item(hud_item_slot1_, g_player_hud->attached_item(1)->m_model,
                                  g_player_hud->attached_item(1)->m_sect_name.c_str());
            }
            else
            {
                hud_item_slot1_.anim_object = nullptr;
                hud_item_slot1_.available_animations.clear();
            }
        }
        else
        {
            // Update third-person mode object
            OzzKinematicsAnimated* obj = GetCurrentAnimatedObject();
            if (obj != current_object_)
            {
                current_object_ = obj;
                available_animations_.clear();

                if (current_object_)
                {
                    // Get available animations from the kinematics object
                    const u16 motion_count = current_object_->GetAvailableMotionCount();
                    available_animations_.reserve(motion_count);

                    for (u16 i = 0; i < motion_count; ++i)
                    {
                        xr_string motion_name;
                        if (current_object_->GetMotionName(i, motion_name))
                        {
                            available_animations_.push_back(shared_str(motion_name.c_str()));
                        }
                    }

                    if (motion_count > 0)
                        playback_.current_animation_index = 0;
                    else
                        playback_.current_animation_index = -1;
                }
            }
        }
    }

    if (ImGui::Begin(tool_name(), &get_open_state(), get_default_window_flags()))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::RadioButton("Pause", paused_))
            {
                paused_ = !paused_;
                float time_factor = 1.f;
                if (paused_)
                {
                    time_factor = EPS;
                }
                Device.time_factor(time_factor);
            }
            ImGui::EndMenuBar();
        }

        if (is_hud_mode_)
        {
            DrawHudModeUI();
        }
        else
        {
            if (!current_object_)
            {
                ImGui::TextUnformatted("No animated object selected.");
                ImGui::TextUnformatted("Enter game and control a character to use this tool.");
            }
            else
            {
                DrawAnimationPanel();
                ImGui::Separator();
                DrawPlaybackControls();
                ImGui::Separator();
                DrawBlendingControls();
                ImGui::Separator();
                DrawIKControls();
                ImGui::Separator();
                DrawSkeletonDebug();
            }
        }
    }
    ImGui::End();
#endif
}

void COzzAnimationUtility::DrawAnimationPanel()
{
    if (!ImGui::CollapsingHeader("Animation Selection", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    if (available_animations_.empty())
    {
        ImGui::TextUnformatted("No animations available or not yet loaded.");
        if (ImGui::Button("Refresh Animation List"))
        {
            // Force refresh by resetting current object
            current_object_ = nullptr;
        }
        return;
    }

    // Previous/Next buttons
    if (ImGui::Button("Previous"))
    {
        if (playback_.current_animation_index > 0)
            playback_.current_animation_index--;
        else
            playback_.current_animation_index = static_cast<int>(available_animations_.size()) - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Next"))
    {
        playback_.current_animation_index = (playback_.current_animation_index + 1) % available_animations_.size();
    }

    // Animation list
    if (ImGui::BeginListBox("##AnimationList", ImVec2(-1, 150)))
    {
        for (int i = 0; i < static_cast<int>(available_animations_.size()); ++i)
        {
            const bool selected = playback_.current_animation_index == i;
            if (ImGui::Selectable(available_animations_[i].c_str(), selected))
            {
                playback_.current_animation_index = i;

                // Trigger animation change in OzzKinematicsAnimated
                if (current_object_)
                {
                    const char* anim_name = available_animations_[i].c_str();
                    MotionID motion = current_object_->ID_Cycle_Safe(anim_name);
                    if (motion.valid())
                    {
                        current_object_->PlayCycle(motion, FALSE, nullptr, nullptr, 0);
                        playback_.playing = true;
                        playback_.time_ratio = 0.0f;
                    }
                }
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
}

void COzzAnimationUtility::DrawPlaybackControls()
{
    if (!ImGui::CollapsingHeader("Playback Controls", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Stop button
    if (ImGui::Button("Stop Animation"))
    {
        if (current_object_)
        {
            current_object_->LL_CloseCycle(BI_NONE, 0xFF);
            playback_.playing = false;
            playback_.time_ratio = 0.0f;
        }
    }

    ImGui::Separator();

    // Display current animation info
    if (playback_.current_animation_index >= 0 &&
        playback_.current_animation_index < static_cast<int>(available_animations_.size()) &&
        current_object_)
    {
        const char* anim_name = available_animations_[playback_.current_animation_index].c_str();
        ImGui::Text("Current: %s", anim_name);

        // Get and display animation duration
        xr_string motion_name;
        float duration = 0.0f;
        if (current_object_->GetMotionInfo(playback_.current_animation_index, motion_name, duration))
        {
            ImGui::Text("Duration: %.2f seconds", duration);

            // Calculate frame count
            const u32 frame_count = static_cast<u32>(duration * 30.0f); // Assuming 30 FPS
            ImGui::Text("Frames: %u", frame_count);
        }

        // Get motion ID to check active blends
        MotionID motion = current_object_->ID_Cycle_Safe(anim_name);
        if (motion.valid())
        {
            // Check if animation is actively playing
            const u32 blend_count = current_object_->LL_PartBlendsCount(0);
            ImGui::Text("Active Blends: %u", blend_count);
            playback_.playing = (blend_count > 0);
        }
    }
    else
    {
        ImGui::TextDisabled("No animation selected");
    }

    ImGui::Separator();
    ImGui::TextWrapped("Note: Animation playback is controlled by the game's blend system. "
                      "Use the animation list to select and play animations.");
}

void COzzAnimationUtility::DrawBlendingControls()
{
    if (!ImGui::CollapsingHeader("Animation Blending"))
        return;

    ImGui::TextUnformatted("Blend two animations together");

    // Blend weight slider
    if (ImGui::SliderFloat("Blend Weight", &blending_.blend_weight, 0.0f, 1.0f, "%.3f"))
    {
        // TODO: Update blend weight in OzzKinematicsAnimated
    }

    ImGui::Text("0.0 = First anim, 1.0 = Second anim");

    // Second animation selection
    if (ImGui::BeginCombo("Blend Animation",
        blending_.blend_animation_index >= 0 ?
        available_animations_[blending_.blend_animation_index].c_str() :
        "None"))
    {
        if (ImGui::Selectable("None", blending_.blend_animation_index == -1))
        {
            blending_.blend_animation_index = -1;
        }

        for (int i = 0; i < static_cast<int>(available_animations_.size()); ++i)
        {
            const bool selected = blending_.blend_animation_index == i;
            if (ImGui::Selectable(available_animations_[i].c_str(), selected))
            {
                blending_.blend_animation_index = i;
                // TODO: Start blending in OzzKinematicsAnimated
            }
        }
        ImGui::EndCombo();
    }

    // Transition time
    ImGui::SliderFloat("Transition Time", &blending_.blend_transition_time, 0.0f, 2.0f, "%.2fs");
}

void COzzAnimationUtility::DrawIKControls()
{
    if (!ImGui::CollapsingHeader("IK Controls"))
        return;

    // Foot IK
    if (ImGui::TreeNode("Foot IK"))
    {
        ImGui::Checkbox("Enable Leg IK", &ik_.enable_foot_ik);

        if (ik_.enable_foot_ik)
        {
            ImGui::SliderFloat("Ground Height", &ik_.ground_height, -2.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("IK Weight", &ik_.ik_weight, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("IK Soften", &ik_.ik_soften, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("Twist (deg)", &ik_.twist_angle_deg, -180.0f, 180.0f, "%.1f");

            // TODO: Apply foot IK settings to OzzKinematicsAnimated
        }

        ImGui::TreePop();
    }

    // Arm IK
    if (ImGui::TreeNode("Arm IK"))
    {
        ImGui::Checkbox("Enable Arm IK", &ik_.enable_arm_ik);

        if (ik_.enable_arm_ik)
        {
            ImGui::SliderFloat("IK Weight##arm", &ik_.arm_ik_weight, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("IK Soften##arm", &ik_.arm_ik_soften, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("Twist (deg)##arm", &ik_.arm_twist_angle_deg, -180.0f, 180.0f, "%.1f");

            // TODO: Apply arm IK settings to OzzKinematicsAnimated
        }

        ImGui::TreePop();
    }
}

void COzzAnimationUtility::DrawSkeletonDebug()
{
    if (!ImGui::CollapsingHeader("Skeleton Debug"))
        return;

    ImGui::Checkbox("Draw Skeleton", &skeleton_debug_.draw_skeleton);
    ImGui::Checkbox("Draw Bone Names", &skeleton_debug_.draw_bone_names);
    ImGui::Checkbox("Draw Bone Axes", &skeleton_debug_.draw_bone_axes);

    if (!current_object_)
        return;

    // Get bone count from kinematics
    const u16 bone_count = current_object_->LL_BoneCount();

    ImGui::Text("Bone Count: %d", bone_count);

    // Ensure bone visibility vector is sized correctly
    if (skeleton_debug_.bone_visibility.size() != bone_count)
    {
        skeleton_debug_.bone_visibility.resize(bone_count, true);
    }

    // Bone visibility controls
    if (ImGui::TreeNode("Bone Visibility"))
    {
        const int display_limit = std::min(bone_count, static_cast<u16>(skeleton_debug_.bone_display_limit));

        ImGui::SliderInt("Display Limit", &skeleton_debug_.bone_display_limit, 1, bone_count);

        if (ImGui::BeginTable("BoneVisibility", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            for (u16 bone_id = 0; bone_id < display_limit; ++bone_id)
            {
                ImGui::TableNextColumn();

                const char* bone_name = current_object_->LL_BoneName_dbg(bone_id);
                bool visible = skeleton_debug_.bone_visibility[bone_id];

                if (ImGui::Checkbox(bone_name ? bone_name : "Unknown", &visible))
                {
                    skeleton_debug_.bone_visibility[bone_id] = visible;
                    current_object_->LL_SetBoneVisible(bone_id, visible, FALSE);
                }
            }
            ImGui::EndTable();
        }

        if (display_limit < bone_count)
        {
            ImGui::TextDisabled("(%d more bones not shown, increase display limit)", bone_count - display_limit);
        }

        ImGui::TreePop();
    }

    // Quick toggle buttons
    if (ImGui::Button("Show All Bones"))
    {
        for (u16 i = 0; i < bone_count; ++i)
        {
            skeleton_debug_.bone_visibility[i] = true;
            current_object_->LL_SetBoneVisible(i, true, FALSE);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Hide All Bones"))
    {
        for (u16 i = 0; i < bone_count; ++i)
        {
            skeleton_debug_.bone_visibility[i] = false;
            current_object_->LL_SetBoneVisible(i, false, FALSE);
        }
    }
}

void COzzAnimationUtility::DrawHudModeUI()
{
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "HUD MODE (First-Person)");
    ImGui::Separator();

    // Draw panels for each HUD component
    bool has_any_item = false;

    // HUD Arms Model (base hands model - always rendered when in HUD mode)
    if (hud_hands_.anim_object)
    {
        has_any_item = true;
        if (ImGui::CollapsingHeader("HUD Arms Model", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawAnimatedObjectPanel(hud_hands_.display_name.c_str(),
                                    hud_hands_.anim_object,
                                    hud_hands_.available_animations,
                                    hud_hands_.playback,
                                    hud_hands_.skeleton_debug);
        }
        ImGui::Separator();
    }

    // HUD Item Slot 0 (usually right hand weapon)
    if (hud_item_slot0_.anim_object)
    {
        has_any_item = true;
        if (ImGui::CollapsingHeader("Slot 0 (Right Hand Item)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawAnimatedObjectPanel(hud_item_slot0_.display_name.c_str(),
                                    hud_item_slot0_.anim_object,
                                    hud_item_slot0_.available_animations,
                                    hud_item_slot0_.playback,
                                    hud_item_slot0_.skeleton_debug);
        }
        ImGui::Separator();
    }

    // HUD Item Slot 1 (usually left hand detector/item)
    if (hud_item_slot1_.anim_object)
    {
        has_any_item = true;
        if (ImGui::CollapsingHeader("Slot 1 (Left Hand Item)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawAnimatedObjectPanel(hud_item_slot1_.display_name.c_str(),
                                    hud_item_slot1_.anim_object,
                                    hud_item_slot1_.available_animations,
                                    hud_item_slot1_.playback,
                                    hud_item_slot1_.skeleton_debug);
        }
        ImGui::Separator();
    }

    if (!has_any_item)
    {
        ImGui::TextUnformatted("No HUD model available.");
        ImGui::TextUnformatted("Switch to first-person view to see HUD animations.");
    }
}

void COzzAnimationUtility::DrawAnimatedObjectPanel(const char* object_name, OzzKinematicsAnimated* anim_obj,
                                                     xr_vector<shared_str>& animations, PlaybackState& playback,
                                                     SkeletonDebugState& skeleton_debug)
{
    if (!anim_obj)
    {
        ImGui::TextDisabled("No animated object");
        return;
    }

    ImGui::PushID(object_name);

    ImGui::Text("Object: %s", object_name);
    ImGui::Separator();

    // Animation Selection Panel
    if (ImGui::TreeNode("Animation Selection"))
    {
        if (animations.empty())
        {
            ImGui::TextUnformatted("No animations available or not yet loaded.");
        }
        else
        {
            // Previous/Next buttons
            if (ImGui::Button("Previous"))
            {
                if (playback.current_animation_index > 0)
                    playback.current_animation_index--;
                else
                    playback.current_animation_index = static_cast<int>(animations.size()) - 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Next"))
            {
                playback.current_animation_index = (playback.current_animation_index + 1) % animations.size();
            }

            // Animation list
            if (ImGui::BeginListBox("##AnimationList", ImVec2(-1, 150)))
            {
                for (int i = 0; i < static_cast<int>(animations.size()); ++i)
                {
                    const bool selected = playback.current_animation_index == i;
                    if (ImGui::Selectable(animations[i].c_str(), selected))
                    {
                        playback.current_animation_index = i;

                        // Trigger animation change
                        const char* anim_name = animations[i].c_str();
                        MotionID motion = anim_obj->ID_Cycle_Safe(anim_name);
                        if (motion.valid())
                        {
                            anim_obj->PlayCycle(motion, FALSE, nullptr, nullptr, 0);
                            playback.playing = true;
                            playback.time_ratio = 0.0f;
                        }
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }
        }
        ImGui::TreePop();
    }

    // Playback Controls
    if (ImGui::TreeNode("Playback Controls"))
    {
        // Stop button
        if (ImGui::Button("Stop Animation"))
        {
            anim_obj->LL_CloseCycle(BI_NONE, 0xFF);
            playback.playing = false;
            playback.time_ratio = 0.0f;
        }

        ImGui::Separator();

        // Display current animation info
        if (playback.current_animation_index >= 0 &&
            playback.current_animation_index < static_cast<int>(animations.size()))
        {
            const char* anim_name = animations[playback.current_animation_index].c_str();
            ImGui::Text("Current: %s", anim_name);

            // Get and display animation duration
            xr_string motion_name;
            float duration = 0.0f;
            if (anim_obj->GetMotionInfo(playback.current_animation_index, motion_name, duration))
            {
                ImGui::Text("Duration: %.2f seconds", duration);

                // Calculate frame count
                const u32 frame_count = static_cast<u32>(duration * 30.0f); // Assuming 30 FPS
                ImGui::Text("Frames: %u", frame_count);
            }

            // Get motion ID to check active blends
            MotionID motion = anim_obj->ID_Cycle_Safe(anim_name);
            if (motion.valid())
            {
                // Check if animation is actively playing
                const u32 blend_count = anim_obj->LL_PartBlendsCount(0);
                ImGui::Text("Active Blends: %u", blend_count);
                playback.playing = (blend_count > 0);
            }
        }
        else
        {
            ImGui::TextDisabled("No animation selected");
        }
        ImGui::TreePop();
    }

    // Skeleton Debug
    if (ImGui::TreeNode("Skeleton Debug"))
    {
        ImGui::Checkbox("Draw Skeleton", &skeleton_debug.draw_skeleton);
        ImGui::Checkbox("Draw Bone Names", &skeleton_debug.draw_bone_names);
        ImGui::Checkbox("Draw Bone Axes", &skeleton_debug.draw_bone_axes);

        // Get bone count from kinematics
        const u16 bone_count = anim_obj->LL_BoneCount();

        ImGui::Text("Bone Count: %d", bone_count);

        // Ensure bone visibility vector is sized correctly
        if (skeleton_debug.bone_visibility.size() != bone_count)
        {
            skeleton_debug.bone_visibility.resize(bone_count, true);
        }

        // Bone visibility controls
        if (ImGui::TreeNode("Bone Visibility"))
        {
            const int display_limit = std::min(bone_count, static_cast<u16>(skeleton_debug.bone_display_limit));

            ImGui::SliderInt("Display Limit", &skeleton_debug.bone_display_limit, 1, bone_count);

            if (ImGui::BeginTable("BoneVisibility", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                for (u16 bone_id = 0; bone_id < display_limit; ++bone_id)
                {
                    ImGui::TableNextColumn();

                    const char* bone_name = anim_obj->LL_BoneName_dbg(bone_id);
                    bool visible = skeleton_debug.bone_visibility[bone_id];

                    if (ImGui::Checkbox(bone_name ? bone_name : "Unknown", &visible))
                    {
                        skeleton_debug.bone_visibility[bone_id] = visible;
                        anim_obj->LL_SetBoneVisible(bone_id, visible, FALSE);
                    }
                }
                ImGui::EndTable();
            }

            if (display_limit < bone_count)
            {
                ImGui::TextDisabled("(%d more bones not shown, increase display limit)", bone_count - display_limit);
            }

            ImGui::TreePop();
        }

        // Quick toggle buttons
        if (ImGui::Button("Show All Bones"))
        {
            for (u16 i = 0; i < bone_count; ++i)
            {
                skeleton_debug.bone_visibility[i] = true;
                anim_obj->LL_SetBoneVisible(i, true, FALSE);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Hide All Bones"))
        {
            for (u16 i = 0; i < bone_count; ++i)
            {
                skeleton_debug.bone_visibility[i] = false;
                anim_obj->LL_SetBoneVisible(i, false, FALSE);
            }
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}
