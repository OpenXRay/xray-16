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
            if (ImGui::MenuItem("Clear", ""))
            {
                ExecCommand(COMMAND_CLEAR);
            }
            if (ImGui::MenuItem("Open...", ""))
            {
                ExecCommand(COMMAND_LOAD);
            }
            if (ImGui::MenuItem("Save", ""))
            {
                ExecCommand(COMMAND_SAVE, xr_string(LTools->m_LastFileName.c_str()));
            }
            if (ImGui::MenuItem("Save As ...", ""))
            {
                ExecCommand(COMMAND_SAVE, 0, 1);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Open Selection...", ""))
            {
                ExecCommand(COMMAND_LOAD_SELECTION);
            }
            if (ImGui::MenuItem("Save Selection As...", ""))
            {
                ExecCommand(COMMAND_SAVE_SELECTION);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Open Recent", ""))
            {
                for (auto &str : EPrefs->scene_recent_list)
                {
                    if (ImGui::MenuItem(str.c_str(), ""))
                    {
                        ExecCommand(COMMAND_LOAD, str);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", ""))
            {
                ExecCommand(COMMAND_QUIT);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("Validate", ""))
            {
                ExecCommand(COMMAND_VALIDATE_SCENE);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Summary Info", ""))
            {
                ExecCommand(COMMAND_CLEAR_SCENE_SUMMARY);
                ExecCommand(COMMAND_COLLECT_SCENE_SUMMARY);
                ExecCommand(COMMAND_SHOW_SCENE_SUMMARY);
            }
            if (ImGui::MenuItem("Highlight Texture...", ""))
            {
                ExecCommand(COMMAND_SCENE_HIGHLIGHT_TEXTURE);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear Debug Draw", ""))
            {
                ExecCommand(COMMAND_CLEAR_DEBUG_DRAW);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export entire Scene as Obj", ""))
            {
                Scene->ExportObj(false);
            }
            if (ImGui::MenuItem("Export selection as Obj", ""))
            {
                Scene->ExportObj(true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Compile"))
        {
            if (ImGui::MenuItem("Build", ""))
            {
                ExecCommand(COMMAND_BUILD);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Make Game", ""))
            {
                ExecCommand(COMMAND_MAKE_GAME);
            }
            if (ImGui::MenuItem("Make Details", ""))
            {
                ExecCommand(COMMAND_MAKE_DETAILS);
            }
            if (ImGui::MenuItem("Make Hom", ""))
            {
                ExecCommand(COMMAND_MAKE_HOM);
            }
            if (ImGui::MenuItem("Make ", ""))
            {
                ExecCommand(COMMAND_MAKE_SOM);
            }
            if (ImGui::MenuItem("Make AI-Map", ""))
            {
                ExecCommand(COMMAND_MAKE_AIMAP);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Import Error List", ""))
            {
                ExecCommand(COMMAND_IMPORT_COMPILER_ERROR);
            }
            if (ImGui::MenuItem("Export Error List", ""))
            {
                ExecCommand(COMMAND_EXPORT_COMPILER_ERROR);
            }
            if (ImGui::MenuItem("Clear Error List", ""))
            {
                ExecCommand(COMMAND_CLEAR_DEBUG_DRAW);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Editors"))
        {
            if (ImGui::BeginMenu("Objects"))
            {
                if (ImGui::MenuItem("Reload"))
                {
                    ExecCommand(COMMAND_RELOAD_OBJECTS);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Library editor"))
                {
                    ExecCommand(COMMAND_LIBRARY_EDITOR);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Images"))
            {
                if (ImGui::MenuItem("Image Editor", ""))
                {
                    ExecCommand(COMMAND_IMAGE_EDITOR);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reload Textures", ""))
                {
                    ExecCommand(COMMAND_RELOAD_TEXTURES);
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
                ImGui::Separator();
                if (ImGui::MenuItem("Sync THM", ""))
                {
                    FS_FileSet files;
                    FS.file_list(files, _textures_, FS_ListFiles, "*.thm");
                    FS_FileSet::iterator I = files.begin();
                    FS_FileSet::iterator E = files.end();

                    for (; I != E; ++I)
                    {
                        ETextureThumbnail *TH = xr_new<ETextureThumbnail>((*I).name.c_str(), false);
                        TH->Load((*I).name.c_str(), _textures_);
                        TH->Save();
                        xr_delete(TH);
                    }
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
                ImGui::Separator();
                if (ImGui::MenuItem("Refresh Environment Library", ""))
                {
                    ExecCommand(COMMAND_REFRESH_SOUND_ENVS);
                }
                if (ImGui::MenuItem("Refresh Environment Geometry", ""))
                {
                    ExecCommand(COMMAND_REFRESH_SOUND_ENV_GEOMETRY);
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
                bool selected = UIObjectList::IsOpen();
                if (ImGui::MenuItem("Object List", "", &selected))
                {
                    if (selected)
                        UIObjectList::Show();
                    else
                        UIObjectList::Close();
                }
            }
            {
                bool selected = !MainForm->GetPropertiesFrom()->IsClosed();
                if (ImGui::MenuItem("Properties", "", &selected))
                {
                    if (selected)
                        MainForm->GetPropertiesFrom()->Open();
                    else
                        MainForm->GetPropertiesFrom()->Close();
                }
            }
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
