#include "stdafx.h"
#include "UIEditLightAnim.h"
#include "LightAnimLibrary.h"
#define POINTER_HEIGHT 35
UIEditLightAnim *UIEditLightAnim::Form = nullptr;
UIEditLightAnim::UIEditLightAnim()
{
    m_Modife = false;
    m_Items = xr_new<UIItemListForm>();
    m_Items->SetOnItemFocusedEvent(TOnILItemFocused(this, &UIEditLightAnim::OnItemFocused));
    m_Items->SetOnItemCreaetEvent(TOnItemCreate(this, &UIEditLightAnim::OnCreateItem));
    m_Items->SetOnItemCloneEvent(TOnItemClone(this, &UIEditLightAnim::OnCloneItem));
    m_Items->SetOnItemRemoveEvent(TOnItemRemove(this, &UIEditLightAnim::OnRemoveItem));
    m_Items->SetOnItemRenameEvent(TOnItemRename(this, &UIEditLightAnim::OnRenameItem));
    m_Items->m_Flags.set(UIItemListForm::fMenuEdit, true);
    m_Props = xr_new<UIPropertiesForm>();
    m_Props->SetModifiedEvent(TOnModifiedEvent(this, &UIEditLightAnim::OnModified));
    m_CurrentItem = nullptr;

    m_TextureNull.create("ed\\ed_nodata");
    m_TextureNull->Load();
    m_Texture = nullptr;
    m_PointerWeight = -1;
    m_PointerResize = true;
    m_PointerTexture = nullptr;
    m_PointerValue = 0;
    m_RenderAlpha = false;
    R_CHK(HW.pDevice->CreateTexture(32, 32, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_ItemTexture, 0));

    InitializeItems();
}

UIEditLightAnim::~UIEditLightAnim()
{
    if (m_Modife)
    {
        if (ELog.DlgMsg(mtConfirmation, mbYes | mbNo, "Save current change?") == mrYes)
        {
            LALib.Save();
        }
        else
        {
            LALib.Reload();
        }
    }
    m_ItemTexture->Release();
    if (m_PointerTexture)
    {
        m_PointerTexture->Release();
        xr_delete(m_PointerRawImage);
    }
    if (m_Texture)
    {
        m_Texture->Release();
    }
    m_TextureNull.destroy();
    xr_delete(m_Props);
    xr_delete(m_Items);
}

void UIEditLightAnim::Draw()
{
    if (ImGui::BeginChild("Left", ImVec2(230, 0)))
    {

        if (ImGui::Button("Save", ImVec2(0, ImGui::GetFrameHeight())))
        {
            m_Modife = false;
            LALib.Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload", ImVec2(0, ImGui::GetFrameHeight())))
        {
            m_Modife = false;
            LALib.Reload();
            OnItemFocused(nullptr);
            InitializeItems();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Render Alpha", &m_RenderAlpha);
        if (ImGui::BeginChild("Left", ImVec2(0, 0), true))
        {
            m_Items->Draw();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    if (ImGui::BeginChild("Midle", ImVec2(-230, 0)))
    {
        {
            ImGui::SetNextItemWidth(-1);
            float width = ImGui::CalcItemWidth();
            if (width >= 1)
            {

                if (m_PointerWeight != floorf(width))
                {
                    m_PointerWeight = floorf(width);
                    m_PointerResize = true;
                }
                if (m_CurrentItem)
                {
                    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
                    if (canvas_size.x > m_PointerWeight - 2)
                        canvas_size.x = m_PointerWeight - 2;
                    if (canvas_size.y > POINTER_HEIGHT)
                        canvas_size.y = POINTER_HEIGHT;
                    if ((ImGui::GetIO().MousePos.x >= canvas_pos.x && ImGui::GetIO().MousePos.y >= canvas_pos.y) &&
                        (ImGui::GetIO().MousePos.x <= (canvas_pos.x + canvas_size.x) && ImGui::GetIO().MousePos.y <= (canvas_pos.y + canvas_size.y)))
                    {
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            OnCreateKeyClick();
                        }
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            if (float(ImGui::GetIO().MousePos.x - canvas_pos.x) >= 1)
                                m_PointerValue = iFloor(float(m_CurrentItem->iFrameCount) * float(ImGui::GetIO().MousePos.x - canvas_pos.x) / float(m_PointerWeight - 2));
                            UpdateProperties();
                        }
                    }
                }
                RenderPointer();
                ImGui::Image(m_PointerTexture, ImVec2(m_PointerWeight, POINTER_HEIGHT));
            }
            m_Props->Draw();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    if (ImGui::BeginChild("Right", ImVec2(230, 0)))
    {
        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 1));
        {
            if (ImGui::Button("|<<", ImVec2(ImGui::GetFrameHeight() * 3, ImGui::GetFrameHeight())))
            {
                m_PointerValue = 0;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() * 3);
            if (ImGui::InputInt("##value", &m_PointerValue, 1, 2))
            {
                if (m_CurrentItem)
                {
                    if (m_PointerValue > m_CurrentItem->iFrameCount - 1)
                        m_PointerValue = m_CurrentItem->iFrameCount - 1;
                    if (m_PointerValue < 0)
                        m_PointerValue = 0;
                }
                else
                {
                    m_PointerValue = 0;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(">>|", ImVec2(ImGui::GetFrameHeight() * 3, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                m_PointerValue = m_CurrentItem->iFrameCount - 1;
            }
        }
        {

            float button_w = ImGui::GetWindowWidth() - (12 * ImGui::GetFrameHeight() + 6 * 2);
            button_w = button_w / 2;
            if (button_w < ImGui::GetFrameHeight())
                button_w = ImGui::GetFrameHeight();
            if (ImGui::Button("<-", ImVec2(ImGui::GetFrameHeight() * 2, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                if ((m_PointerValue != 0) && (m_CurrentItem->IsKey(m_PointerValue)))
                {
                    int f0, f1;
                    f1 = f0 = m_PointerValue;
                    f1--;
                    while (f1 >= 0)
                    {
                        if (!(m_CurrentItem->IsKey(f1)))
                        {
                            m_CurrentItem->MoveKey(f0, f1);
                            m_PointerValue = f1;
                            OnModified();
                            break;
                        }
                        f1--;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("|<<", ImVec2(ImGui::GetFrameHeight() * 3, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                m_PointerValue = m_CurrentItem->FirstKeyFrame();
            }
            ImGui::SameLine();
            if (ImGui::Button("<", ImVec2(ImGui::GetFrameHeight() * 1, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                m_PointerValue = m_CurrentItem->PrevKeyFrame(m_PointerValue);
            }
            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(button_w, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                OnCreateKeyClick();
            }
            ImGui::SameLine();
            if (ImGui::Button("-", ImVec2(button_w, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                m_CurrentItem->DeleteKey(m_PointerValue);
                if (m_PointerValue > m_CurrentItem->iFrameCount - 1)
                    m_PointerValue = m_CurrentItem->iFrameCount - 1;
                if (m_PointerValue < 0)
                    m_PointerValue = 0;
                UpdateProperties();
                OnModified();
            }
            ImGui::SameLine();
            if (ImGui::Button(">", ImVec2(ImGui::GetFrameHeight() * 1, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                m_PointerValue = m_CurrentItem->NextKeyFrame(m_PointerValue);
            }
            ImGui::SameLine();
            if (ImGui::Button(">>|", ImVec2(ImGui::GetFrameHeight() * 3, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                m_PointerValue = m_CurrentItem->LastKeyFrame();
            }
            ImGui::SameLine();
            if (ImGui::Button("->", ImVec2(ImGui::GetFrameHeight() * 2, ImGui::GetFrameHeight())) && m_CurrentItem)
            {
                if ((m_PointerValue != m_CurrentItem->iFrameCount - 1) && (m_CurrentItem->IsKey(m_PointerValue)))
                {
                    int f0, f1;
                    f1 = f0 = m_PointerValue;
                    f1++;
                    while (f1 <= m_CurrentItem->iFrameCount - 1)
                    {
                        if (!(m_CurrentItem->IsKey(f1)))
                        {
                            m_CurrentItem->MoveKey(f0, f1);
                            m_PointerValue = f1;
                            OnModified();
                            break;
                        }
                        f1++;
                    }
                }
            }
        }
        ImGui::PopStyleVar(2);
        ImGui::EndGroup();
        if (m_CurrentItem)
        {

            RenderItem();
        }
        ImGui::Image(m_CurrentItem ? m_ItemTexture : m_TextureNull->surface_get(), ImGui::CalcItemSize(ImVec2(-1, -1), 32, 32));
    }
    ImGui::EndChild();
}

void UIEditLightAnim::Update()
{
    if (Form)
    {
        if (!Form->IsClosed())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(800, 400));
            if (ImGui::BeginPopupModal("LightAnim Editor", &Form->bOpen, 0, true))
            {
                ImGui::PopStyleVar(1);
                Form->Draw();
                ImGui::EndPopup();
            }
            else
            {
                ImGui::PopStyleVar(1);
            }
        }
        else
        {
            xr_delete(Form);
        }
    }
}

void UIEditLightAnim::Show()
{
    if (Form == nullptr)
        Form = xr_new<UIEditLightAnim>();
}

void UIEditLightAnim::UpdateProperties()
{
    PropItemVec items;
    if (m_CurrentItem)
    {
        // PHelper().CreateName(items, "Name", &m_CurrentItem->cName, m_CurrentOwner);
        PHelper().CreateFloat(items, "FPS", &m_CurrentItem->fFPS, 0.1f, 1000, 1.f, 1);
        S32Value *V;
        V = PHelper().CreateS32(items, "Frame Count", &m_CurrentItem->iFrameCount, 1, 100000, 1);
        V->OnAfterEditEvent.bind(this, &UIEditLightAnim::OnFrameCountAfterEdit);

        u32 frame = m_PointerValue;
        PHelper().CreateCaption(items, "Current\\Frame", shared_str().printf("%d", frame));
        u32 *val = m_CurrentItem->GetKey(m_PointerValue);
        if (val)
        {
            PHelper().CreateColor(items, "Current\\Color", val);
            PHelper().CreateU8(items, "Current\\Alpha", ((u8 *)val) + 3, 0x00, 0xFF);
        }
    }
    m_Props->AssignItems(items);
}

void UIEditLightAnim::InitializeItems()
{
    ListItemsVec items;
    for (LAItemIt it = LALib.Items.begin(); it != LALib.Items.end(); it++)
        LHelper().CreateItem(items, *(*it)->cName, 0, 0, 0);
    m_Items->AssignItems(items);
}

void UIEditLightAnim::RenderItem()
{
    u32 Color;
    {
        int frame;
        Color = m_CurrentItem->CalculateBGR(EDevice.fTimeGlobal, frame);
        if (!m_RenderAlpha)
            Color = subst_alpha(Color, 0xFF);
    }
    {
        D3DLOCKED_RECT rect;
        R_CHK(m_ItemTexture->LockRect(0, &rect, 0, 0));
        u32 *dest = nullptr;

        for (u32 y = 0; y < 32; y++)
        {
            dest = reinterpret_cast<u32 *>(reinterpret_cast<char *>(rect.pBits) + (rect.Pitch * y));
            for (u32 i = 0; i < 32; i++)
            {
                dest[i] = Color;
            }
        }
        R_CHK(m_ItemTexture->UnlockRect(0));
    }
}

void UIEditLightAnim::OnCreateKeyClick()
{
    if (!(m_CurrentItem->IsKey(m_PointerValue)))
    {
        u32 color = m_CurrentItem->InterpolateRGB(m_PointerValue);

        m_CurrentItem->InsertKey(m_PointerValue, color);
        UpdateProperties();
        OnModified();
    }
}

void UIEditLightAnim::OnModified()
{
    m_Modife = true;
}

void UIEditLightAnim::OnItemFocused(ListItem *item)
{
    m_CurrentItem = nullptr;
    if (item)
    {
        m_CurrentItem = LALib.FindItem(item->Key());
        UpdateProperties();
    }
    else
    {
        m_Props->ClearProperties();
    }
}

bool UIEditLightAnim::OnFrameCountAfterEdit(PropValue *v, s32 &val)
{
    if (val != m_CurrentItem->iFrameCount)
        OnModified();
    m_CurrentItem->Resize(val);
    if (m_PointerValue > m_CurrentItem->iFrameCount - 1)
        m_PointerValue = m_CurrentItem->iFrameCount - 1;
    if (m_PointerValue < 0)
        m_PointerValue = 0;
    return true;
}

void UIEditLightAnim::OnCloneItem(LPCSTR parent_path, LPCSTR new_full_name)
{
    LALib.AppendItem(new_full_name, LALib.FindItem(parent_path));
    ;
    InitializeItems();
    m_Items->SelectItem(new_full_name);
    OnModified();
}

void UIEditLightAnim::OnCreateItem(LPCSTR path)
{
    LALib.AppendItem(path, 0);
    ;
    InitializeItems();
    m_Items->SelectItem(path);
    OnModified();
}

void UIEditLightAnim::OnRemoveItem(LPCSTR name, EItemType type)
{
    bool res = false;
    if (m_CurrentItem->cName == name)
    {
        OnItemFocused(nullptr);
    }
    LALib.RemoveObject(name, type, res);

    OnModified();
}

void UIEditLightAnim::OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type)
{
    bool res = false;
    LALib.RenameObject(old_full_name, new_full_name, type);

    OnModified();
}

void UIEditLightAnim::RenderPointer()
{
    if (m_PointerResize)
    {
        if (m_PointerTexture)
        {
            m_PointerTexture->Release();
            xr_delete(m_PointerRawImage);
        }
        R_CHK(HW.pDevice->CreateTexture(m_PointerWeight, POINTER_HEIGHT, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_PointerTexture, 0));
        m_PointerRawImage = xr_alloc<u32>(POINTER_HEIGHT * m_PointerWeight);
    }
    for (int x = 0; x < m_PointerWeight; x++)
    {
        for (int y = 0; y < POINTER_HEIGHT; y++)
        {
            {
                m_PointerRawImage[y * int(m_PointerWeight) + x] = 0xFF000000;
            }
        }
    }
    if (m_CurrentItem)
    {
        float segment = float(m_PointerWeight - 2) / (float(m_CurrentItem->iFrameCount));
        int half_segment = iFloor(segment / 2);

        {
            CLAItem::KeyMap &Keys = m_CurrentItem->Keys;
            int last = m_CurrentItem->iFrameCount;
            Keys[last] = Keys.rbegin()->second;
            CLAItem::KeyPairIt prev_key = Keys.begin();
            CLAItem::KeyPairIt it = prev_key;
            it++;
            float x_prev = (float(prev_key->first) / float(m_CurrentItem->iFrameCount)) * (m_PointerWeight - 2);
            ImVec4 cb;
            cb.y = 1;
            cb.w = floorf((POINTER_HEIGHT - 2) - (POINTER_HEIGHT - 2) * 0.3f);
            for (; it != Keys.end(); it++)
            {
                float x = (it->first / float(m_CurrentItem->iFrameCount)) * (m_PointerWeight - 2);
                float g_cnt = it->first - prev_key->first;
                for (int k = 0; k < g_cnt; k++)
                {
                    cb.x = floorf(x_prev + k * segment + 1);
                    cb.z = floorf(x_prev + k * segment + segment + 1);
                    if (!m_RenderAlpha)
                        FillRectPointer(cb, subst_alpha(m_CurrentItem->InterpolateBGR(prev_key->first + k), 0xFF), true);
                    else
                        FillRectPointer(cb, m_CurrentItem->InterpolateBGR(prev_key->first + k), true);
                }
                prev_key = it;
                x_prev = x;
            }
            Keys.erase(Keys.find(last));
        }
        {
            // draw keys
            u32 Color = 0xFFBFFFFF;
            ImVec4 cb;
            cb.y = floorf((POINTER_HEIGHT - 2) - (POINTER_HEIGHT - 2) * 0.1f) + 1;
            cb.w = floorf(POINTER_HEIGHT - 2);
            for (auto it = m_CurrentItem->Keys.begin(); it != m_CurrentItem->Keys.end(); it++)
            {
                int t = iFloor((it->first / float(m_CurrentItem->iFrameCount)) * (m_PointerWeight));
                cb.x = t - 1 + half_segment;
                cb.z = t + 2 + half_segment;
                FillRectPointer(cb, Color);
            }
        }
        // draw pointer
        {
            u32 Color = 0xFF00FF00;
            int t = iFloor((float(m_PointerValue) / float(m_CurrentItem->iFrameCount)) * (m_PointerWeight)) + half_segment;
            ImVec4 rp;
            rp.x = t;
            rp.w = 1 + (POINTER_HEIGHT - 2);
            rp.z = t + 1;
            rp.y = 1 + (POINTER_HEIGHT - 2) * 0.75f;
            FillRectPointer(rp, 0xFF00FF00);
            /* if ((iMoveKey >= 0) && (iTgtMoveKey != iMoveKey))
                 t = iFloor((iTgtMoveKey / float(m_CurrentItem->iFrameCount)) * R.Width()) + half_segment;*/

            rp.x = t - 2;
            rp.y = 1 + 1;
            rp.z = t + 3;
            rp.w = (POINTER_HEIGHT - 2) - (POINTER_HEIGHT - 2) * 0.3f - 1;
            FrameRectPointer(rp, 0xFFAAAAAA);
            rp.x -= 1;
            rp.z += 1;
            rp.y -= 1;
            rp.w += 1;
            FrameRectPointer(rp, 0xFF000000);
        }
    }
    for (int x = 0; x < m_PointerWeight; x++)
    {
        for (int y = 0; y < POINTER_HEIGHT; y++)
        {
            if (x == 0 || x == m_PointerWeight - 1)
            {
                m_PointerRawImage[y * int(m_PointerWeight) + x] = 0xFF707070;
            }
            else if (y == 0 || y == POINTER_HEIGHT - 1)
            {
                m_PointerRawImage[y * int(m_PointerWeight) + x] = 0xFF707070;
            }
        }
    }
    {
        D3DLOCKED_RECT rect;
        R_CHK(m_PointerTexture->LockRect(0, &rect, 0, 0));
        u32 *dest = nullptr;

        for (u32 y = 0; y < POINTER_HEIGHT; y++)
        {
            dest = reinterpret_cast<u32 *>(reinterpret_cast<char *>(rect.pBits) + (rect.Pitch * y));
            for (u32 i = 0; i < m_PointerWeight; i++)
            {
                dest[i] = m_PointerRawImage[y * int(m_PointerWeight) + i];
            }
        }
        R_CHK(m_PointerTexture->UnlockRect(0));
    }
}

void UIEditLightAnim::FillRectPointer(const ImVec4 &rect, u32 color, bool plus_one)
{
    int r_x = iFloor(rect.x);
    int r_y = iFloor(rect.y);
    int r_z = iFloor(rect.z);
    int r_w = iFloor(rect.w);
    for (int x = 0; x < m_PointerWeight; x++)
    {
        for (int y = 0; y < POINTER_HEIGHT; y++)
        {
            if (plus_one)
            {
                if (x >= r_x && x <= r_z)
                    if (y >= r_y && y <= r_w)
                    {
                        m_PointerRawImage[y * int(m_PointerWeight) + x] = color;
                    }
            }
            else
            {
                if (x >= r_x && x < r_z)
                    if (y >= r_y && y < r_w)
                    {
                        m_PointerRawImage[y * int(m_PointerWeight) + x] = color;
                    }
            }
        }
    }
}

void UIEditLightAnim::FrameRectPointer(const ImVec4 &rect, u32 color)
{
    int r_x = iFloor(rect.x);
    int r_y = iFloor(rect.y);
    int r_z = iFloor(rect.z);
    int r_w = iFloor(rect.w);
    for (int x = 0; x < m_PointerWeight; x++)
    {
        for (int y = 0; y < POINTER_HEIGHT; y++)
        {
            if (x >= r_x && x < r_z)
                if (y >= r_y && y < r_w)
                {
                    if ((x == r_x || x == r_z - 1) || (y == r_y || y == r_w - 1))
                    {
                        m_PointerRawImage[y * int(m_PointerWeight) + x] = color;
                    }
                }
        }
    }
}
