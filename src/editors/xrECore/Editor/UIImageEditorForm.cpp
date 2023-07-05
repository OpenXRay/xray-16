//---------------------------------------------------------------------------
#include "stdafx.h"
#include "UIImageEditorForm.h"
#include "EThumbnail.h"
UIImageEditorForm *UIImageEditorForm::Form = nullptr;
UIImageEditorForm::UIImageEditorForm()
{
    m_ItemProps = xr_new<UIPropertiesForm>();
    m_ItemList = xr_new<UIItemListForm>();
    m_ItemList->SetOnItemFocusedEvent(TOnILItemFocused(this, &UIImageEditorForm::OnItemsFocused));
    m_ItemList->SetOnItemRemoveEvent(TOnItemRemove(&ImageLib, &CImageManager::RemoveTexture));
    m_Texture = nullptr;
    m_bFilterImage = true;
    m_bFilterCube = true;
    m_bFilterBump = true;
    m_bFilterNormal = true;
    m_bFilterTerrain = true;
    m_bUpdateProperties = false;
    m_TextureRemove = nullptr;
}

UIImageEditorForm::~UIImageEditorForm()
{
    if (m_Texture)
        m_Texture->Release();
    xr_delete(m_ItemList);
    xr_delete(m_ItemProps);
}

void UIImageEditorForm::Draw()
{
    if (m_bUpdateProperties)
    {
        UpdateProperties();
        m_bUpdateProperties = false;
    }

    if (m_TextureRemove)
    {
        m_TextureRemove->Release();
        m_TextureRemove = nullptr;
    }
    ImGui::BeginChild("Left", ImVec2(200, 400), true);
    {
        m_ItemList->Draw();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Right", ImVec2(300, 400));
    {
        if (m_Texture == nullptr)
        {
            u32 mem = 0;
            m_Texture = RImplementation.texture_load("ed\\ed_nodata", mem);
        }
        ImGui::Image(m_Texture, ImVec2(128, 128));
        m_ItemProps->Draw();
    }
    ImGui::EndChild();
    ImGui::Separator();
    if (!bImportMode)
    {
        if (ImGui::Checkbox("Image", &m_bFilterImage))
            FilterUpdate();
        ImGui::SameLine();
        if (ImGui::Checkbox("Cube", &m_bFilterCube))
            FilterUpdate();
        ImGui::SameLine();
        if (ImGui::Checkbox("Bump", &m_bFilterBump))
            FilterUpdate();
        ImGui::SameLine();
        if (ImGui::Checkbox("Normal", &m_bFilterNormal))
            FilterUpdate();
        ImGui::SameLine();
        if (ImGui::Checkbox("Terrain", &m_bFilterTerrain))
            FilterUpdate();
        ImGui::Separator();
    }
    if (ImGui::Button("Close"))
    {
        HideLib();
    }
    ImGui::SameLine();
    if (ImGui::Button("Ok"))
    {
        UpdateLib();
        HideLib();
    }
    if (!bImportMode)
    {
        ImGui::SameLine();
        if (ImGui::Button("Remove Texture"))
        {
            m_ItemList->RemoveSelectItem();
        }
    }
}

void UIImageEditorForm::Update()
{
    if (Form)
    {
        if (!Form->IsClosed())
        {
            if (ImGui::BeginPopupModal("ImageEditor", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize, true))
            {
                Form->Draw();
                ImGui::EndPopup();
            }
        }
        else
        {
            xr_delete(Form);
        }
    }
}

void UIImageEditorForm::Show(bool bImport)
{
    if (Form == nullptr)
        Form = xr_new<UIImageEditorForm>();
    Form->bImportMode = bImport;
    //.        form->ebRebuildAssociation->Enabled = !bImport;
    Form->bReadonlyMode = !FS.can_write_to_alias(_textures_);
    if (Form->bReadonlyMode)
    {
        Log("#!You don't have permisions to modify textures.");
        Form->m_ItemProps->SetReadOnly(TRUE);
    }
    Form->modif_map.clear();
    Form->InitItemList();
}

void UIImageEditorForm::ImportTextures()
{
    VERIFY(!Form);
    FS_FileSet TextureMap;
    int new_cnt = ImageLib.GetLocalNewTextures(TextureMap);
    if (new_cnt)
    {
        if (ELog.DlgMsg(mtInformation, "Found %d new texture(s)", new_cnt))
        {
            Form = xr_new<UIImageEditorForm>();
            Form->texture_map.swap(TextureMap);
            Show(true);
        }
    }
    else
    {
        ELog.DlgMsg(mtInformation, "Can't find new textures.");
    }
}

ETextureThumbnail *UIImageEditorForm::FindUsedTHM(const shared_str &name)
{
    THMMapIt it = m_THM_Used.find(name);
    if (it != m_THM_Used.end())
        return it->second;

    ETextureThumbnail *thm = xr_new<ETextureThumbnail>(name.c_str(), false);
    m_THM_Used[name] = thm;

    if (bImportMode)
    {
        xr_string fn = name.c_str();
        ImageLib.UpdateFileName(fn);

        if (!thm->Load(name.c_str(), _import_))
        {
            bool bLoad = thm->Load(fn.c_str(), _game_textures_);
            ImageLib.CreateTextureThumbnail(thm, name.c_str(), _import_, !bLoad);
        }
    }
    else
    {
        thm->Load();
    }
    return thm;
}

void UIImageEditorForm::RegisterModifiedTHM()
{
    if (m_ItemProps->IsModified() || bImportMode)
    {
        for (THMIt t_it = m_THM_Current.begin(); t_it != m_THM_Current.end(); ++t_it)
        {
            auto it = texture_map.find(FS_File((*t_it)->SrcName()));
            R_ASSERT(it != texture_map.end());
            modif_map.insert(*it);
        }
    }
}

void UIImageEditorForm::OnCubeMapBtnClick(ButtonValue *value, bool &bModif, bool &bSafe)
{
    ButtonValue *B = dynamic_cast<ButtonValue *>(value);
    R_ASSERT(B);
    bModif = false;
    switch (B->btn_num)
    {
    case 0:
    {
        RStringVec items;
        if (0 != m_ItemList->GetSelected(items))
        {
            for (RStringVecIt it = items.begin(); it != items.end(); it++)
            {
                xr_string new_name = xr_string(it->c_str()) + "#small";
                ImageLib.CreateSmallerCubeMap(it->c_str(), new_name.c_str());
            }
        }
    }
    break;
    }
}

void UIImageEditorForm::OnTypeChange(PropValue *prop)
{
    m_bUpdateProperties = true;
}

void UIImageEditorForm::InitItemList()
{
    R_ASSERT(m_THM_Used.empty());
    if (!bImportMode)
        ImageLib.GetTexturesRaw(texture_map);

    ListItemsVec items;
    // fill
    auto it = texture_map.begin();
    auto _E = texture_map.end();
    for (; it != _E; it++)
    {
        ListItem *I = LHelper().CreateItem(items, it->name.c_str(), 0);
        I->m_Object = (void *)(FindUsedTHM(it->name.c_str()));
        R_ASSERT2(I->m_Object, it->name.c_str());
    }
    m_ItemList->AssignItems(items);
}

void UIImageEditorForm::HideLib()
{
    bOpen = false;
    ImGui::CloseCurrentPopup();
}

void UIImageEditorForm::UpdateLib()
{
    VERIFY(!bReadonlyMode);
    RegisterModifiedTHM();
    SaveUsedTHM();
    if (bImportMode && !texture_map.empty())
    {
        AStringVec modif;
        ImageLib.SafeCopyLocalToServer(texture_map);
        // rename with folder
        FS_FileSet files = texture_map;
        texture_map.clear();
        xr_string fn;
        auto it = files.begin();
        auto _E = files.end();

        for (; it != _E; it++)
        {
            fn = EFS.ChangeFileExt(it->name.c_str(), "");
            ImageLib.UpdateFileName(fn);
            FS_File F(*it);
            F.name = fn;
            texture_map.insert(F);
        }
        // sync
        ImageLib.SynchronizeTextures(true, true, true, &texture_map, &modif);
        ImageLib.RefreshTextures(&modif);
    }
    else
    {
        // save game textures
        if (modif_map.size())
        {
            AStringVec modif;
            ImageLib.SynchronizeTextures(true, true, true, &modif_map, &modif);
            ImageLib.RefreshTextures(&modif);
        }
    }
}

void UIImageEditorForm::OnItemsFocused(ListItem* item)
{
	PropItemVec props;

	RegisterModifiedTHM();
	m_THM_Current.clear();
	m_TextureRemove = m_Texture;
	m_Texture = nullptr;

	if (ListItem* prop = item)
	{
		ETextureThumbnail* thm = FindUsedTHM(prop->Key());
		m_THM_Current.push_back(thm);
        m_ItemProps->ClearProperties();

		// fill prop
		thm->FillProp(props, PropValue::TOnChange(this, &UIImageEditorForm::OnTypeChange));

		if (thm->_Format().type == STextureParams::ttCubeMap)
		{
			ButtonValue* B = PHelper().CreateButton(props, "CubeMap\\Edit", "Make Small", 0);
			B->OnBtnClickEvent.bind(this, &UIImageEditorForm::OnCubeMapBtnClick);
		}

		thm->Update(m_Texture);
	}

	m_ItemProps->AssignItems(props);
}

void UIImageEditorForm::UpdateProperties()
{
    ListItemsVec vec;
    m_ItemList->GetSelected(nullptr, vec, false);

    if (vec.size() == 1)
    {
        m_ItemProps->ClearProperties();
        OnItemsFocused(vec[0]);
    }
}

void UIImageEditorForm::SaveUsedTHM()
{
    for (THMMapIt t_it = m_THM_Used.begin(); t_it != m_THM_Used.end(); ++t_it)
    {
        if (modif_map.find(FS_File(t_it->second->SrcName())) != modif_map.end())
            t_it->second->Save();
    }
}

void UIImageEditorForm::FilterUpdate()
{
    const ListItemsVec &items = m_ItemList->GetItems();

    u32 cnt = items.size();
    for (u32 k = 0; k < cnt; ++k)
    {
        ListItem *I = items[k];

        ETextureThumbnail *thm = (ETextureThumbnail *)I->m_Object;

        BOOL bVis = FALSE;
        int type = thm->_Format().type;
        if (STextureParams::ttImage == type && m_bFilterImage)
            bVis = TRUE;
        else if (STextureParams::ttCubeMap == type && m_bFilterCube)
            bVis = TRUE;
        else if (STextureParams::ttBumpMap == type && m_bFilterBump)
            bVis = TRUE;
        else if (STextureParams::ttNormalMap == type && m_bFilterNormal)
            bVis = TRUE;
        else if (STextureParams::ttTerrain == type && m_bFilterTerrain)
            bVis = TRUE;

        I->Visible(bVis);
    }
    m_ItemList->ClearSelected();
}
