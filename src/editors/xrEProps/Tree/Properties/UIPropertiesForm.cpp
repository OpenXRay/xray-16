#include "stdafx.h"

UIPropertiesForm::UIPropertiesForm() : m_Root("", this)
{
	m_bModified = false;
	m_EditChooseValue = nullptr;
	m_EditShortcutValue = nullptr;
	m_EditTextureValue = nullptr;
	m_EditTextValueData = nullptr;
	m_EditTextValueInitial = nullptr;
	m_Flags.zero();
}

UIPropertiesForm::~UIPropertiesForm()
{
	if (m_EditTextValueData)
		xr_delete(m_EditTextValueData);
	ClearProperties();
}

void UIPropertiesForm::Draw()
{
	{
		if (m_EditChooseValue)
		{
			shared_str result;
			bool is_result;
			if (UIChooseForm::GetResult(is_result, result))
			{
				if (is_result)
				{
					if (m_EditChooseValue->AfterEdit<ChooseValue, shared_str>(result))
						if (m_EditChooseValue->ApplyValue<ChooseValue, shared_str>(result))
						{
							Modified();
						}
				}
				m_EditChooseValue = nullptr;
			}

			UIChooseForm::Update();
		}
		if (m_EditTextureValue)
		{
			shared_str result;
			bool is_result;
			if (UIChooseForm::GetResult(is_result, result))
			{
				if (is_result)
				{
					if (result.c_str() == nullptr)
					{
						xr_string result_as_str = "$null";
						if (m_EditTextureValue->AfterEdit<CTextValue, xr_string>(result_as_str))
							if (m_EditTextureValue->ApplyValue<CTextValue, LPCSTR>(result_as_str.c_str()))
							{
								Modified();
							}
					}
					else
					{
						xr_string result_as_str = result.c_str();
						if (m_EditTextureValue->AfterEdit<CTextValue, xr_string>(result_as_str))
							if (m_EditTextureValue->ApplyValue<CTextValue, LPCSTR>(result_as_str.c_str()))
							{
								Modified();
							}
					}
				}
				m_EditTextureValue = nullptr;
			}
			UIChooseForm::Update();
		}
		if (m_EditShortcutValue)
		{
			xr_shortcut result;
			bool ok;
			if (UIKeyPressForm::GetResult(ok, result))
			{
				if (ok)
				{
					if (m_EditShortcutValue->AfterEdit<ShortcutValue, xr_shortcut>(result))
						if (m_EditShortcutValue->ApplyValue<ShortcutValue, xr_shortcut>(result))
						{
							Modified();
						}
				}
				m_EditShortcutValue = nullptr;
			}
		}
	}

	static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
	const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

	if (ImGui::BeginTable("3ways", 2, flags))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
		ImGui::TableSetupColumn("Prop", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 6.0f);
		ImGui::TableHeadersRow();
		m_Root.DrawRoot();
		ImGui::EndTable();
	}
}

void UIPropertiesForm::AssignItems(PropItemVec &items)
{
	m_Items = items;

	for (PropItem *item : items)
	{
		item->m_Owner = this;
		UIPropertiesItem* Item = static_cast<UIPropertiesItem*>(m_Root.AppendItem(item->Key()));
		VERIFY(Item);
		Item->PItem = item;
	}
}

PropItem *UIPropertiesForm::FindItemOfName(shared_str name)
{
	for (PropItem *I : m_Items)
	{
		const char *key = I->Key();
		if (strrchr(key, '\\'))
		{
			key = strrchr(key, '\\') + 1;
		}
		if (name == key)
		{
			return I;
		}
	}
	return nullptr;
}

void UIPropertiesForm::ClearProperties()
{
	VERIFY(!m_EditChooseValue);

	for (PropItem *I : m_Items)
		xr_delete(I);
	
	m_Root = UIPropertiesItem("", this);
	m_Items.clear();
}

PropItem *UIPropertiesForm::FindItem(const char *name)
{	
	if (UIPropertiesItem* Item = static_cast<UIPropertiesItem*>(m_Root.FindItem(name)))
		return Item->PItem;
	
	return nullptr;
}

void UIPropertiesForm::DrawEditText()
{
	if (ImGui::BeginPopupContextItem("EditText", 0))
	{
		R_ASSERT(m_EditTextValueData);
		ImGui::BeginGroup();

		if (ImGui::Button("Ok"))
		{
			auto OnOkSuccessful = [&]()
			{
				xr_delete(m_EditTextValueData);
				xr_delete(m_EditTextValueInitial);
				Modified();
				ImGui::CloseCurrentPopup();
			};

			auto OnOkFailed = [&]()
			{
				if (!m_EditTextValueData || !m_EditTextValueInitial)
					return;

				// TSMP: close window on ok when nothing changed
				if (!xr_strcmp(m_EditTextValueData, m_EditTextValueInitial))
					OnOkSuccessful();
			};

			if (dynamic_cast<CTextValue*>(m_EditTextValue->GetFrontValue()))
			{
				xr_string out = m_EditTextValueData;

				if (m_EditTextValue->AfterEdit<CTextValue, xr_string>(out))
				{
					if (m_EditTextValue->ApplyValue<CTextValue, LPCSTR>(out.c_str()))
						OnOkSuccessful();
				}
			}
			else if (dynamic_cast<RTextValue*>(m_EditTextValue->GetFrontValue()))
			{
				shared_str out = m_EditTextValueData;

				if (m_EditTextValue->AfterEdit<RTextValue, shared_str>(out))
				{
					if (m_EditTextValue->ApplyValue<RTextValue, shared_str>(out))
						OnOkSuccessful();
				}
			}
			else if (dynamic_cast<STextValue*>(m_EditTextValue->GetFrontValue()))
			{
				xr_string out = m_EditTextValueData;

				if (m_EditTextValue->AfterEdit<STextValue, xr_string>(out))
				{
					if (m_EditTextValue->ApplyValue<STextValue, xr_string>(out))
						OnOkSuccessful();
				}
			}
			else
				R_ASSERT(false);

			OnOkFailed();
		}

		ImGui::SameLine(0);

		if (ImGui::Button("Cancel"))
		{
			xr_delete(m_EditTextValueData);
			xr_delete(m_EditTextValueInitial);
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine(0);

		if (ImGui::Button("Apply"))
		{
			auto OnApplySuccessful = [&]()
			{
				const char* newText = m_EditTextValueData;
				xr_delete(m_EditTextValueInitial);
				m_EditTextValueInitial = xr_strdup(newText ? newText : "");
				Modified();
			};

			if (dynamic_cast<CTextValue*>(m_EditTextValue->GetFrontValue()))
			{
				xr_string out = m_EditTextValueData;

				if (m_EditTextValue->AfterEdit<CTextValue, xr_string>(out))
				{
					if (m_EditTextValue->ApplyValue<CTextValue, LPCSTR>(out.c_str()))
						OnApplySuccessful();
				}
			}
			else if (dynamic_cast<RTextValue*>(m_EditTextValue->GetFrontValue()))
			{
				shared_str out = m_EditTextValueData;

				if (m_EditTextValue->AfterEdit<RTextValue, shared_str>(out))
				{
					if (m_EditTextValue->ApplyValue<RTextValue, shared_str>(out))
						OnApplySuccessful();
				}
			}
			else if (dynamic_cast<STextValue*>(m_EditTextValue->GetFrontValue()))
			{
				xr_string out = m_EditTextValueData;

				if (m_EditTextValue->AfterEdit<STextValue, xr_string>(out))
				{
					if (m_EditTextValue->ApplyValue<STextValue, xr_string>(out))
						OnApplySuccessful();
				}
			}
			else
				R_ASSERT(false);
		}

		ImGui::SameLine(150);

		if (ImGui::Button("Load"))
		{
			xr_string fn;

			if (EFS.GetOpenName(0, "$import$", fn, false, NULL, 2))
			{
				xr_string buf;
				IReader *F = FS.r_open(fn.c_str());
				F->r_stringZ(buf);
				xr_delete(m_EditTextValueData);
				m_EditTextValueData = xr_strdup(buf.c_str());
				m_EditTextValueDataSize = xr_strlen(m_EditTextValueData) + 1;
				FS.r_close(F);
			}
		}

		ImGui::SameLine(0);

		if (ImGui::Button("Save"))
		{
			xr_string fn;

			if (EFS.GetSaveName("$import$", fn, NULL, 2))
			{
				CMemoryWriter F;
				F.w_stringZ(m_EditTextValueData);

				if (!F.save_to(fn.c_str()))
					Log("!Can't save text file:", fn.c_str());
			}
		}

		ImGui::SameLine(0);

		if (ImGui::Button("Clear"))
			m_EditTextValueData[0] = '\0';

		ImGui::EndGroup();

		if (m_EditTextValueData)
		{
			ImGui::InputTextMultiline("##text", m_EditTextValueData, m_EditTextValueDataSize, ImVec2(500, 200),
				ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) -> int
				{
					return reinterpret_cast<UIPropertiesForm*>(data->UserData)->DrawEditText_Callback(data);
				}, this
			);
		}

		ImGui::EndPopup();
	}
}

int UIPropertiesForm::DrawEditText_Callback(ImGuiInputTextCallbackData *data)
{
	m_EditTextValueData = (char *)xr_realloc(m_EditTextValueData, data->BufSize);
	m_EditTextValueDataSize = data->BufSize;
	data->Buf = m_EditTextValueData;
	return 0;
}

void UIPropertiesForm::DrawEditGameType()
{
	if (ImGui::BeginPopupContextItem("EditGameType", 0))
	{
		R_ASSERT(m_EditGameTypeValue);

		bool test = false;
		{
			ImGui::BeginGroup();
			{
				bool cheked = m_EditGameTypeChooser.MatchType(eGameIDSingle);
				if (ImGui::Checkbox("Single", &cheked))
				{
					m_EditGameTypeChooser.m_GameType.set(eGameIDSingle, cheked);
				}
			}
			{
				bool cheked = m_EditGameTypeChooser.MatchType(eGameIDDeathmatch);
				if (ImGui::Checkbox("DM", &cheked))
				{
					m_EditGameTypeChooser.m_GameType.set(eGameIDDeathmatch, cheked);
				}
			}
			{
				bool cheked = m_EditGameTypeChooser.MatchType(eGameIDTeamDeathmatch);
				if (ImGui::Checkbox("TDM", &cheked))
				{
					m_EditGameTypeChooser.m_GameType.set(eGameIDTeamDeathmatch, cheked);
				}
			}
			{
				bool cheked = m_EditGameTypeChooser.MatchType(eGameIDArtefactHunt);
				if (ImGui::Checkbox("ArtefactHunt", &cheked))
				{
					m_EditGameTypeChooser.m_GameType.set(eGameIDArtefactHunt, cheked);
				}
			}
			{
				bool cheked = m_EditGameTypeChooser.MatchType(eGameIDCaptureTheArtefact);
				if (ImGui::Checkbox("CTA", &cheked))
				{
					m_EditGameTypeChooser.m_GameType.set(eGameIDCaptureTheArtefact, cheked);
				}
			}
			ImGui::EndGroup();
			ImGui::SameLine();
		}
		{
			ImGui::BeginGroup();
			if (ImGui::Button("Ok", ImVec2(ImGui::GetFrameHeight() * 6, 0)))
			{
				if (m_EditGameTypeValue->AfterEdit<GameTypeValue, GameTypeChooser>(m_EditGameTypeChooser))
					if (m_EditGameTypeValue->ApplyValue<GameTypeValue, GameTypeChooser>(m_EditGameTypeChooser))
					{
						Modified();
					}
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Cancel", ImVec2(ImGui::GetFrameHeight() * 6, 0)))
			{
				m_EditGameTypeValue = nullptr;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndGroup();
		}
		ImGui::EndPopup();
	}
}
