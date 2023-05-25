#include "stdafx.h"
#include "UIMinimapEditorForm.h"

UIMinimapEditorForm *UIMinimapEditorForm::Form = nullptr;

UIMinimapEditorForm::UIMinimapEditorForm()
{
    m_TextureRemove = nullptr;
    m_Texture = nullptr;
}

UIMinimapEditorForm::~UIMinimapEditorForm()
{
    if (m_Texture)
        m_Texture->Release();
}

void UIMinimapEditorForm::Draw()
{
    if (m_TextureRemove)
    {
        m_TextureRemove->Release();
        m_TextureRemove = nullptr;
    }

    if (!m_Texture)
    {
        u32 mem = 0;
        m_Texture = RImplementation.texture_load("ed\\ed_nodata", mem);
    }

    ImGui::Image(m_Texture, ImVec2(330, 530));

	if (ImGui::Button("Load"))
        LoadClick();
}

void UIMinimapEditorForm::Update()
{
    if (Form)
    {
        if (!Form->IsClosed())
        {
            if (ImGui::BeginPopupModal("MinimapEditor", &Form->bOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize, true))
            {
                Form->Draw();
                ImGui::EndPopup();
            }
            else
            {
                Form->bOpen = false;
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            xr_delete(Form);
        }
    }
}

void UIMinimapEditorForm::Show()
{
    VERIFY(!Form);
    Form = xr_new<UIMinimapEditorForm>();
    Form->bOpen = true;
}

extern bool Stbi_Load(LPCSTR full_name, U32Vec &data, u32 &w, u32 &h, u32 &a);

void UIMinimapEditorForm::LoadClick()
{
    xr_string fn;
    m_ImageData.clear();

    if (EFS.GetOpenName(EDevice.m_hWnd, "$app_root$", fn, false, NULL, 0))
    {
        if (Stbi_Load(fn.c_str(), m_ImageData, m_ImageW, m_ImageH, m_ImageA))
        {
            m_TextureRemove = m_Texture;
            ID3DTexture2D *pTexture = nullptr;
            {
                R_CHK(HW.pDevice->CreateTexture(m_ImageW, m_ImageH, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pTexture, 0));
                m_Texture = pTexture;

                {
                    D3DLOCKED_RECT rect;
                    R_CHK(pTexture->LockRect(0, &rect, 0, D3DLOCK_DISCARD));
                    for (int i = 0; i < m_ImageH; i++)
                    {

                        unsigned char *dest = static_cast<unsigned char *>(rect.pBits) + (rect.Pitch * i);
                        memcpy(dest, m_ImageData.data() + (m_ImageW * i), sizeof(unsigned char) * m_ImageW * 4);
                    }
                    R_CHK(pTexture->UnlockRect(0));
                }
            }
            /*LPCSTR _mark = "_[";
            if (fn.find(_mark) != fn.npos)
            {
                LPCSTR _str = fn.c_str() + fn.find(_mark);
                int cnt = sscanf(_str, "_[%f, %f]-[%f, %f]", &map_bb.min.x, &map_bb.min.y, &map_bb.max.x, &map_bb.max.y);
                //                "ss_andy_05-08-07_15-24-11_#ai_test_[-150.000, -100.000]-[52.927, 50.000].tga"
                if (cnt != 4) {
                    map_bb.min.x = 0.0f;
                    map_bb.min.y = 0.0f;
                    map_bb.max.x = 100.0f;
                    map_bb.max.y = 100.0f;
                    map_bb_loaded = map_bb;

                    ApplyPoints(true);
                    return;
                }
                map_bb_loaded = map_bb;

                ApplyPoints(true);
                imgPanelResize(NULL);
            }*/
        }
    }
}
