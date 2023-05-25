#include "stdafx.h"

UIEditLibrary *UIEditLibrary::Form = nullptr;

UIEditLibrary::UIEditLibrary()
{
	{
		ref_texture texture_null;
		texture_null.create("ed\\ed_nodata");
		texture_null->Load();
		VERIFY(texture_null->surface_get());
		texture_null->surface_get()->AddRef();
		m_NullTexture = texture_null->surface_get();
		m_RealTexture = nullptr;
	}

	m_ObjectList = xr_new<UIItemListForm>();
	InitObjects();
	m_ObjectList->SetOnItemFocusedEvent(TOnILItemFocused(this, &UIEditLibrary::OnItemFocused));
	m_Props = xr_new<UIPropertiesForm>();
	m_Selected = nullptr;
	m_Preview = false;
	m_SelectLods = false;
}

void UIEditLibrary::OnItemFocused(ListItem *item)
{
	// TODO: возможно нужно текстуру удалять
	// if (m_RealTexture)m_RemoveTexture = m_RealTexture;
	m_RealTexture = nullptr;
	m_Props->ClearProperties();
	m_Current = nullptr;
	if (item)
	{
		m_Selected = item;
		m_Current = item->Key();
		auto *m_Thm = ImageLib.CreateThumbnail(m_Current, EImageThumbnail::ETObject);
		if (m_Thm)
		{
			m_Thm->Update(m_RealTexture);
			PropItemVec Info;
			m_Thm->FillInfo(Info);
			m_Props->AssignItems(Info);
		}

		if (m_Preview)
		{
			ListItemsVec vec;
			vec.push_back(item);
			SelectionToReference(&vec);
		}
	}

	// UpdateObjectProperties();
	UI->RedrawScene();
}

UIEditLibrary::~UIEditLibrary()
{
}

void UIEditLibrary::InitObjects()
{
	ListItemsVec items;
	FS_FileSet lst;

	if (Lib.GetObjects(lst))
	{
		FS_FileSetIt it = lst.begin();
		FS_FileSetIt _E = lst.end();
		for (; it != _E; it++)
		{
			xr_string fn;
			ListItem *I = LHelper().CreateItem(items, it->name.c_str(), 0, ListItem::flDrawThumbnail, 0);
		}
	}
	// if (m_RealTexture)m_RemoveTexture = m_RealTexture;
	// m_RealTexture = nullptr;
	// m_Props->ClearProperties();
	m_ObjectList->AssignItems(items);

	// m_Items.clear();
 //   //ListItemsVec items;
 //   FS_FileSet lst;
 //   Lib.GetObjects(lst);
 //   FS_FileSetIt it = lst.begin();
 //   FS_FileSetIt _E = lst.end();
 //   for (; it != _E; it++)
 //       LHelper().CreateItem(m_Items, it->name.c_str(), 0, 0, 0);
 //   //m_Items->AssignItems(items, false, true);
}

void UIEditLibrary::Update()
{
	if (!Form)
		return;

	if (!Form->IsClosed())
		Form->Draw();
	else
		Close();
}

void UIEditLibrary::Show()
{
	UI->BeginEState(esEditLibrary);

	if (!Form)
		Form = xr_new<UIEditLibrary>();
}

void UIEditLibrary::Close()
{
	UI->EndEState(esEditLibrary);
	// TODO: возможно еще кого то надо грохнуть
	xr_delete(Form);
}

void UIEditLibrary::DrawObjects()
{
	// if (ImGui::TreeNode("Object List"))
	//{
	//	ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
	ImGui::BeginChild("Object List");
	ImGui::Separator();
	// m_ObjectList->m_Flags.set(m_ObjectList->fMultiSelect, true);
	m_ObjectList->Draw();
	ImGui::Separator();
	ImGui::EndChild();
	//	ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
	//	ImGui::TreePop();
	//}

	/*for (ListItem *item : m_Items)
	{
		item->m_Object
	}


		if (it->first == OBJCLASS_DUMMY)
			continue;
		ObjectList& lst = ot->GetObjects();
		ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
		if (ImGui::TreeNode("folder", ("%ss", it->second->ClassDesc())))
		{
			if (OBJCLASS_GROUP == it->first)
			{
				for (ObjectIt _F = lst.begin(); _F != lst.end(); ++_F)
				{
					switch (m_Mode)
					{
					case UIObjectList::M_All:
						break;
					case UIObjectList::M_Visible:
						if (!(*_F)->Visible())continue;
						break;
					case UIObjectList::M_Inbvisible:
						if ((*_F)->Visible())continue;
						break;
					default:
						break;
					}
					{
						strcpy(str_name, ((CGroupObject*)(*_F))->GetName());
					}
					DrawObject(*_F, str_name);
					ImGui::PushID(str_name);
					if (ImGui::TreeNode(str_name))
					{
						ObjectList 					grp_lst;

						((CGroupObject*)(*_F))->GetObjects(grp_lst);

						for (ObjectIt _G = grp_lst.begin(); _G != grp_lst.end(); _G++)
						{
							DrawObject(*_G, 0);
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
			else
			{
				bool FindSelectedObj = false;
				for (ObjectIt _F = lst.begin(); _F != lst.end(); ++_F)
				{
					switch (m_Mode)
					{
					case UIObjectList::M_All:
						break;
					case UIObjectList::M_Visible:
						if (!(*_F)->Visible())continue;
						break;
					case UIObjectList::M_Inbvisible:
						if ((*_F)->Visible())continue;
						break;
					default:
						break;
					}
					DrawObject(*_F, 0);
					FindSelectedObj = FindSelectedObj | (*_F) == m_SelectedObject;
				}
				if (!FindSelectedObj)m_SelectedObject = nullptr;

			}
			ImGui::TreePop();
		}
	}*/

	// m_cur_cls = LTools->CurrentClassID();
	// string1024				str_name;

	// for (SceneToolsMapPairIt it = Scene->FirstTool(); it != Scene->LastTool(); ++it)
	//{
	//	ESceneCustomOTool* ot = dynamic_cast<ESceneCustomOTool*>(it->second);
	//	if (ot && ((m_cur_cls == OBJCLASS_DUMMY) || (it->first == m_cur_cls)))
	//	{
	//		if (it->first == OBJCLASS_DUMMY)
	//			continue;
	//		ObjectList& lst = ot->GetObjects();
	//		ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	//		if (ImGui::TreeNode("folder", ("%ss", it->second->ClassDesc())))
	//		{
	//			if (OBJCLASS_GROUP == it->first)
	//			{
	//				for (ObjectIt _F = lst.begin(); _F != lst.end(); ++_F)
	//				{
	//					switch (m_Mode)
	//					{
	//					case UIObjectList::M_All:
	//						break;
	//					case UIObjectList::M_Visible:
	//						if (!(*_F)->Visible())continue;
	//						break;
	//					case UIObjectList::M_Inbvisible:
	//						if ((*_F)->Visible())continue;
	//						break;
	//					default:
	//						break;
	//					}
	//					{
	//						strcpy(str_name, ((CGroupObject*)(*_F))->GetName());
	//					}
	//					DrawObject(*_F, str_name);
	//					ImGui::PushID(str_name);
	//					if (ImGui::TreeNode(str_name))
	//					{
	//						ObjectList 					grp_lst;

	//						((CGroupObject*)(*_F))->GetObjects(grp_lst);

	//						for (ObjectIt _G = grp_lst.begin(); _G != grp_lst.end(); _G++)
	//						{
	//							DrawObject(*_G, 0);
	//						}
	//						ImGui::TreePop();
	//					}
	//					ImGui::PopID();
	//				}
	//			}
	//			else
	//			{
	//				bool FindSelectedObj = false;
	//				for (ObjectIt _F = lst.begin(); _F != lst.end(); ++_F)
	//				{
	//					switch (m_Mode)
	//					{
	//					case UIObjectList::M_All:
	//						break;
	//					case UIObjectList::M_Visible:
	//						if (!(*_F)->Visible())continue;
	//						break;
	//					case UIObjectList::M_Inbvisible:
	//						if ((*_F)->Visible())continue;
	//						break;
	//					default:
	//						break;
	//					}
	//					DrawObject(*_F, 0);
	//					FindSelectedObj = FindSelectedObj | (*_F) == m_SelectedObject;
	//				}
	//				if (!FindSelectedObj)m_SelectedObject = nullptr;

	//			}
	//			ImGui::TreePop();
	//		}
	//	}
	//}
}

void UIEditLibrary::DrawObject(CCustomObject *obj, const char *name)
{
	/*if (m_Filter[0])
	{
		if (name)
		{
			if (strstr(name, m_Filter) == 0)return;
		}
		else
		{
			if (strstr(obj->GetName(), m_Filter) == 0)return;
		}
	}
	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	if (obj->Selected())
	{
		Flags |= ImGuiTreeNodeFlags_Bullet;
	}
	if (m_SelectedObject == obj)
	{
		Flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (name)
		ImGui::TreeNodeEx(name, Flags);
	else
		ImGui::TreeNodeEx(obj->GetName(), Flags);
	if (ImGui::IsItemClicked())
	{
		if (!ImGui::GetIO().KeyCtrl)
			Scene->SelectObjects(false, OBJCLASS_DUMMY);
		obj->Select(true);
		m_SelectedObject = obj;
	}*/
}

void UIEditLibrary::GenerateLOD(RStringVec &props, bool bHighQuality)
{
	u32 lodsCnt = 0;
	SPBItem* pb = UI->ProgressStart(props.size(), "Making LOD");

	for (const shared_str &str: props)
	{
		RStringVec reference;
		reference.push_back(str);
		ChangeReference(reference); // select item

		R_ASSERT(m_pEditObjects.size() == 1);
		CSceneObject* SO = m_pEditObjects[0];
		CEditableObject* O = SO->GetReference();

		if (O && O->IsMUStatic())
		{
			pb->Inc(O->GetName());
			BOOL bLod = O->m_objectFlags.is(CEditableObject::eoUsingLOD);
			O->m_objectFlags.set(CEditableObject::eoUsingLOD, FALSE);
			xr_string tex_name;
			tex_name = EFS.ChangeFileExt(O->GetName(), "");

			string_path	tmp;
			strcpy(tmp, tex_name.c_str());
			_ChangeSymbol(tmp, '\\', '_');
			tex_name = xr_string("lod_") + tmp;
			tex_name = ImageLib.UpdateFileName(tex_name);
			ImageLib.CreateLODTexture(O, tex_name.c_str(), LOD_IMAGE_SIZE, LOD_IMAGE_SIZE, LOD_SAMPLE_COUNT, O->Version(), bHighQuality ? 4/*7*/ : 1);
			O->OnDeviceDestroy();
			O->m_objectFlags.set(CEditableObject::eoUsingLOD, bLod);
			ELog.Msg(mtInformation, "LOD for object '%s' successfully created.", O->GetName());
			lodsCnt++;
		}
		else
			ELog.Msg(mtError, "Can't create LOD texture from non 'Multiple Usage' object.", SO->RefName());

		if (UI->NeedAbort())
			break;
	}

	UI->ProgressEnd(pb);

	if(lodsCnt)
		ELog.DlgMsg(mtInformation, "'%u' LOD's succesfully created.", lodsCnt);
}

void UIEditLibrary::MakeLOD(bool bHighQuality)
{
	//if (ebSave->Enabled)
	//{
	//	ELog.DlgMsg(mtError, "Save library changes before generating LOD.");
	//	return;
	//}

	int res = ELog.DlgMsg(mtConfirmation, TMsgDlgButtons() | mbYes | mbNo | mbCancel, "Do you want to select multiple objects?");

	if (res == mrCancel)
		return;

	if (res == mrNo)
	{
		RStringVec sel_items;
		sel_items.push_back(m_Selected->Key());
		GenerateLOD(sel_items, bHighQuality);
		return;
	}

	R_ASSERT(res == mrYes);
	UIChooseForm::SelectItem(smObject, 512, 0);
	m_SelectLods = true;
	m_HighQualityLod = true;
	// answer is handled in DrawRightBar
}

void UIEditLibrary::OnMakeThmClick()
{
	U32Vec pixels;
	ListItemsVec sel_items;

	if (!m_Selected)
		return;

	sel_items.push_back(m_Selected);
	//m_Items->GetSelected(NULL, sel_items, false);
	ListItemsIt it = sel_items.begin();
	ListItemsIt it_e = sel_items.end();

	for (; it != it_e; ++it)
	{
		ListItem* item = *it;
		CEditableObject* obj = Lib.CreateEditObject(item->Key());

		if (obj && m_Preview)
		{
			string_path fn;
			FS.update_path(fn, _objects_, ChangeFileExt(obj->GetName(), ".thm").c_str());

			//m_Items->SelectItem(item->Key(), true, false, true);
			if (ImageLib.CreateOBJThumbnail(fn, obj, obj->Version()))			
				ELog.Msg(mtInformation, "Thumbnail successfully created.");			
		}
		else
			ELog.DlgMsg(mtError, "Can't create thumbnail. Set preview mode.");
		
		Lib.RemoveEditObject(obj);
	}

	ELog.DlgMsg(mtInformation, "Done.");
}

void UIEditLibrary::OnPropertiesClick()
{
	MessageBoxA(0, 0, 0, 0);
}

void UIEditLibrary::DrawRightBar()
{
	if (ImGui::BeginChild("Right", ImVec2(192, 0)))
	{
		if (m_NullTexture || m_RealTexture)
		{
			if (m_RealTexture)
				ImGui::Image(m_RealTexture, ImVec2(192, 192));
			else
				ImGui::Image(m_NullTexture, ImVec2(192, 192));
		}
		else
			ImGui::InvisibleButton("Image", ImVec2(192, 192));

		m_Props->Draw();

		// if (disabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		if (ImGui::Button("Properties", ImVec2(-1, 0)))
			OnPropertiesClick();

		//if (disabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		// Make Thumbnail & Lod
		{
			bool enableMakeThumbnailAndLod = m_Selected && m_Preview;

			if (!enableMakeThumbnailAndLod)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			if (ImGui::Button("Make Thumbnail", ImVec2(-1, 0)))
				OnMakeThmClick();

			if (ImGui::Button("Make LOD (High Quality)", ImVec2(-1, 0)))
				MakeLOD(true);

			if (ImGui::Button("Make LOD (Low Quality)", ImVec2(-1, 0)))
				MakeLOD(false);

			if (!enableMakeThumbnailAndLod)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
		}

		if (ImGui::Checkbox("Preview", &m_Preview))
			OnPreviewClick();

		// if (disabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		if (ImGui::Button("Rename Object", ImVec2(-1, 0)))
		{
		}
		if (ImGui::Button("Remove Object", ImVec2(-1, 0)))
		{
		}

		if (ImGui::Button("Import Object", ImVec2(-1, 0)))
		{
		}
		if (ImGui::Button("Export LWO", ImVec2(-1, 0)))
		{
		}

		if (ImGui::Button("Save", ImVec2(-1, 0)))
		{
		}

		// if (disabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		if (ImGui::Button("Close", ImVec2(-1, 0)))
			Close();

		ImGui::EndChild();
	}

	if (m_SelectLods)
	{
		bool change = false;
		SStringVec lst;

		if (UIChooseForm::GetResult(change, lst))
		{
			if (change)
			{
				RStringVec selItems;

				for (const xr_string &xrStr: lst)
					selItems.push_back(xrStr.c_str());

				GenerateLOD(selItems, m_HighQualityLod);
			}

			m_SelectLods = false;
		}

		UIChooseForm::Update();
	}
}

void UIEditLibrary::OnPreviewClick()
{
	RefreshSelected();
}

void UIEditLibrary::RefreshSelected()
{
	bool mt = false;

	if (m_Preview)
	{
		if (m_Selected)
		{
			ListItemsVec vec;
			vec.push_back(m_Selected);
			mt = SelectionToReference(&vec);
		}
		else
			mt = SelectionToReference(nullptr);
	}

	//ebMakeThm->Enabled = !bReadOnly && mt;
	//ebMakeLOD_high->Enabled = !bReadOnly && cbPreview->Checked;
	//ebMakeLOD_low->Enabled = !bReadOnly && cbPreview->Checked;
	UI->RedrawScene();
}

bool UIEditLibrary::SelectionToReference(ListItemsVec *props)
{
	RStringVec sel_strings;
	ListItemsVec sel_items;

	if(props)
		sel_items = *props;
	// else
	// m_Items->GetSelected(NULL, sel_items, false /*true*/);	

	ListItemsIt it = sel_items.begin();
	ListItemsIt it_e = sel_items.end();

	for (; it != it_e; ++it)
	{
		ListItem *item = *it;
		sel_strings.push_back(item->Key());
	}
	ChangeReference(sel_strings);
	return sel_strings.size() > 0;
}

void UIEditLibrary::ChangeReference(const RStringVec &items)
{
	xr_vector<CSceneObject *>::iterator it = m_pEditObjects.begin();
	xr_vector<CSceneObject *>::iterator it_e = m_pEditObjects.end();
	for (; it != it_e; ++it)
	{
		CSceneObject *SO = *it;
		xr_delete(SO);
	}

	m_pEditObjects.clear();

	RStringVec::const_iterator sit = items.begin();
	RStringVec::const_iterator sit_e = items.end();

	for (; sit != sit_e; ++sit)
	{
		CSceneObject *SO = xr_new<CSceneObject>((LPVOID)0, (LPSTR)0);
		m_pEditObjects.push_back(SO);
		SO->SetReference((*sit).c_str());

		CEditableObject *NE = SO->GetReference();
		if (NE)
		{
			SO->FPosition = NE->t_vPosition;
			SO->FScale = NE->t_vScale;
			SO->FRotation = NE->t_vRotate;
		}
		// update transformation
		SO->UpdateTransform();

		/*
			// save new position
			CEditableObject* E				= m_pEditObject->GetReference();

			if (E && new_name && (stricmp(E->GetName(),new_name))==0 ) return;

			if (E)
			{
				E->t_vPosition.set			(m_pEditObject->PPosition);
				E->t_vScale.set				(m_pEditObject->PScale);
				E->t_vRotate.set			(m_pEditObject->PRotation);
			}
			m_pEditObject->SetReference		(new_name);
			// get old position
			E								= m_pEditObject->GetReference();
			if (E)
			{
				m_pEditObject->PPosition 	= E->t_vPosition;
				m_pEditObject->PScale 		= E->t_vScale;
				m_pEditObject->PRotation	= E->t_vRotate;
			}
			// update transformation
			m_pEditObject->UpdateTransform	();
		*/
	}

	ExecCommand(COMMAND_EVICT_OBJECTS);
	ExecCommand(COMMAND_EVICT_TEXTURES);
}

void UIEditLibrary::OnRender()
{
	if (!Form)
		return;

	if (!Form->m_Preview)
		return;

	for (auto &it : Form->m_pEditObjects)
	{
		CSceneObject *SO = it;
		CSceneObject *S = SO;

		CEditableObject *O = SO->GetReference();
		if (O)
		{
			S->m_RT_Flags.set(S->flRT_Visible, true);

			if (!S->FPosition.similar(O->t_vPosition))
				S->FPosition = O->t_vPosition;

			if (!S->FRotation.similar(O->t_vRotate))
				S->FRotation = O->t_vRotate;

			if (!S->FScale.similar(O->t_vScale))
				S->FScale = O->t_vScale;

			SO->OnFrame();
			SO->RenderSingle();
			// static bool strict = true;
			// SO->Render(0, strict);
		}
	}
}

void UIEditLibrary::Draw()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(600, 600));

	if (!ImGui::Begin("Object Library", &bOpen))
	{
		ImGui::PopStyleVar(1);
		ImGui::End();
		return;
	}

	{
		ImGui::BeginGroup();

		if (ImGui::BeginChild("Left", ImVec2(-192, -ImGui::GetFrameHeight() - 4), true))
			DrawObjects();

		ImGui::EndChild();
		ImGui::SetNextItemWidth(-192);
		ImGui::Text(" Items count: %u", m_ObjectList->m_Items.size());
		// ImGui::InputText("##value", m_Filter, sizeof(m_Filter));
		ImGui::EndGroup();
	}

	ImGui::SameLine();
	DrawRightBar();

	ImGui::PopStyleVar(1);
	ImGui::End();
}
