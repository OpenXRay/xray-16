#include "stdafx.h"

UIMainMenuForm::UIMainMenuForm()
{
}

UIMainMenuForm::~UIMainMenuForm()
{
}

void UIMainMenuForm::Draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save"))
            {
                ExecCommand(COMMAND_SAVE);
            }
            if (ImGui::MenuItem("Reload"))
            {
                ExecCommand(COMMAND_LOAD);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save .xr"))
            {
                ExecCommand(COMMAND_SAVE_XR);
            }
            if (ImGui::MenuItem("Load .xr"))
            {
                ExecCommand(COMMAND_LOAD_XR);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Validate"))
            {
                ExecCommand(COMMAND_VALIDATE);
            }
            if (ImGui::MenuItem("Compact"))
            {
                ExecCommand(COMMAND_COMPACT_PARTICLES);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Editors"))
        {
            if (ImGui::BeginMenu("Image"))
            {
                if (ImGui::MenuItem("Image Editor", ""))
                {
                    ExecCommand(COMMAND_IMAGE_EDITOR);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Synchronize Textures", ""))
                {
                    ExecCommand(COMMAND_REFRESH_TEXTURES);
                }
                if (ImGui::MenuItem("Check New Textures", ""))
                {
                    ExecCommand(COMMAND_CHECK_TEXTURES);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Sounds"))
            {
                if (ImGui::MenuItem("Sound Editor", ""))
                {
                    ExecCommand(COMMAND_SOUND_EDITOR);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Synchronize Sounds", ""))
                {
                    ExecCommand(COMMAND_SYNC_SOUNDS);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Light Anim Editor", ""))
            {
                ExecCommand(COMMAND_LIGHTANIM_EDITOR);
            }
            if (ImGui::MenuItem("Minimap Editor", ""))
            {
                ExecCommand(COMMAND_MINIMAP_EDITOR);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options"))
        {
            if (ImGui::BeginMenu("Render"))
            {
                if (ImGui::BeginMenu("Quality"))
                {
                    static bool selected[4] = {false, false, true, false};
                    if (ImGui::MenuItem("25%", "", &selected[0]))
                    {
                        selected[1] = selected[2] = selected[3] = false;
                        UI->SetRenderQuality(1 / 4.f);
                        UI->RedrawScene();
                    }
                    if (ImGui::MenuItem("50%", "", &selected[1]))
                    {
                        selected[0] = selected[2] = selected[3] = false;
                        UI->SetRenderQuality(1 / 2.f);
                        UI->RedrawScene();
                    }
                    if (ImGui::MenuItem("100%", "", &selected[2]))
                    {
                        selected[1] = selected[0] = selected[3] = false;
                        UI->SetRenderQuality(1.f);
                        UI->RedrawScene();
                    }
                    if (ImGui::MenuItem("200%", "", &selected[3]))
                    {
                        selected[1] = selected[2] = selected[0] = false;
                        UI->SetRenderQuality(2.f);
                        UI->RedrawScene();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Fill Mode"))
                {
                    bool selected[3] = {EDevice.dwFillMode == D3DFILL_POINT, EDevice.dwFillMode == D3DFILL_WIREFRAME, EDevice.dwFillMode == D3DFILL_SOLID};
                    if (ImGui::MenuItem("Point", "", &selected[0]))
                    {
                        EDevice.dwFillMode = D3DFILL_POINT;
                        UI->RedrawScene();
                    }
                    if (ImGui::MenuItem("Wireframe", "", &selected[1]))
                    {
                        EDevice.dwFillMode = D3DFILL_WIREFRAME;
                        UI->RedrawScene();
                    }
                    if (ImGui::MenuItem("Solid", "", &selected[2]))
                    {
                        EDevice.dwFillMode = D3DFILL_SOLID;
                        UI->RedrawScene();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Shader Mode"))
                {
                    bool selected[2] = {EDevice.dwShadeMode == D3DSHADE_FLAT, EDevice.dwShadeMode == D3DSHADE_GOURAUD};
                    if (ImGui::MenuItem("Flat", "", &selected[0]))
                    {
                        EDevice.dwShadeMode = D3DSHADE_FLAT;
                        UI->RedrawScene();
                    }
                    if (ImGui::MenuItem("Gouraud", "", &selected[1]))
                    {
                        EDevice.dwShadeMode = D3DSHADE_GOURAUD;
                        UI->RedrawScene();
                    }
                    ImGui::EndMenu();
                }
                {
                    bool selected = psDeviceFlags.test(rsEdgedFaces);
                    if (ImGui::MenuItem("Edged Faces", "", &selected))
                    {
                        psDeviceFlags.set(rsEdgedFaces, selected);
                        UI->RedrawScene();
                    }
                }
                ImGui::Separator();
                {
                    bool selected = !HW.Caps.bForceGPU_SW;
                    if (ImGui::MenuItem("RenderHW", "", &selected))
                    {
                        HW.Caps.bForceGPU_SW = !selected;
                        UI->Resize();
                    }
                }
                ImGui::Separator();
                {
                    bool selected = psDeviceFlags.test(rsFilterLinear);
                    if (ImGui::MenuItem("Filter Linear", "", &selected))
                    {
                        psDeviceFlags.set(rsFilterLinear, selected);
                        UI->RedrawScene();
                    }
                }
                {
                    bool selected = psDeviceFlags.test(rsRenderTextures);
                    if (ImGui::MenuItem("Textures", "", &selected))
                    {
                        psDeviceFlags.set(rsRenderTextures, selected);
                        UI->RedrawScene();
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            {
                bool selected = psDeviceFlags.test(rsDrawSafeRect);
                if (ImGui::MenuItem("Draw Safe Rect", "", &selected))
                {
                    psDeviceFlags.set(rsDrawSafeRect, selected);
                    UI->RedrawScene();
                }
            }
            {
                bool selected = psDeviceFlags.test(rsDrawGrid);
                if (ImGui::MenuItem("Draw Grid", "", &selected))
                {
                    psDeviceFlags.set(rsDrawGrid, selected);
                    UI->RedrawScene();
                }
            }
            ImGui::Separator();
            {
                bool selected = psDeviceFlags.test(rsFog);
                if (ImGui::MenuItem("Fog", "", &selected))
                {
                    psDeviceFlags.set(rsFog, selected);
                    UI->RedrawScene();
                }
            }
            ImGui::Separator();
            {
                bool selected = psDeviceFlags.test(rsLighting);
                ;
                if (ImGui::MenuItem("Lighting", "", &selected))
                {
                    psDeviceFlags.set(rsLighting, selected);
                    UI->RedrawScene();
                }
            }
            {
                bool selected = psDeviceFlags.test(rsMuteSounds);
                if (ImGui::MenuItem("Mute Sounds", "", &selected))
                {
                    psDeviceFlags.set(rsMuteSounds, selected);
                }
            }
            {
                bool selected = psDeviceFlags.test(rsRenderRealTime);
                if (ImGui::MenuItem("Real Time", "", &selected))
                {
                    psDeviceFlags.set(rsRenderRealTime, selected);
                }
            }

            ImGui::Separator();
            {
                bool selected = psDeviceFlags.test(rsStatistic);
                if (ImGui::MenuItem("Stats", "", &selected))
                {
                    psDeviceFlags.set(rsStatistic, selected);
                    UI->RedrawScene();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences", ""))
            {
                ExecCommand(COMMAND_EDITOR_PREF);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            {
                bool selected = AllowLogCommands();

                if (ImGui::MenuItem("Log", "", &selected))
                {
                    ExecCommand(COMMAND_LOG_COMMANDS);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
