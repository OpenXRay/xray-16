#include "stdafx.h"

UIPropertiesItem::UIPropertiesItem(shared_str Name, UIPropertiesForm* propertiesFrom) : UITreeItem(Name), PropertiesFrom(propertiesFrom)
{
	PItem = nullptr;
}

void UIPropertiesItem::Draw()
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	if (PItem && PItem->m_Flags.test(PropItem::flShowCB))
	{
		if (ImGui::CheckboxFlags("##value", &PItem->m_Flags.flags, PropItem::flCBChecked))
		{
			PItem->OnChange();
			PropertiesFrom->Modified();
		}

		ImGui::SameLine(0, 2);
	}

	if (Items.size())
	{
		ImGuiTreeNodeFlags FloderFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
		bool open = ImGui::TreeNodeEx(Name.c_str(), FloderFlags);
		ImGui::TableNextColumn();
		DrawItem();

		if (open)
		{
			for (UITreeItem* Item : Items)			
				static_cast<UIPropertiesItem*>(Item)->Draw();
			
			ImGui::TreePop();
		}
	}
	else
	{
		ImGui::TreeNodeEx(Name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
		ImGui::TableNextColumn();
		DrawItem();
	}
}

void UIPropertiesItem::DrawRoot()
{
	VERIFY(!PItem);

	for (UITreeItem* Item : Items)	
		static_cast<UIPropertiesItem*>(Item)->Draw();	
}

void UIPropertiesItem::DrawItem()
{
	if (!PItem)
		return;

	EPropType type = PItem->Type();

	switch (type)
	{
	case PROP_CANVAS:
	{
		if (PItem->m_Flags.test(PropItem::flMixed))
		{
			ImGui::TextDisabled(PItem->GetDrawText().c_str());

		}
		else
		{
			ImGui::PushItemWidth(-1);
			CanvasValue* val = dynamic_cast<CanvasValue*>(PItem->GetFrontValue()); R_ASSERT(val);
			if (!val->OnDrawCanvasEvent.empty())
				val->OnDrawCanvasEvent(val);
		}
	}
	break;

	case PROP_BUTTON:
		if (PItem->m_Flags.test(PropItem::flMixed))		
			ImGui::TextDisabled(PItem->GetDrawText().c_str());		
		else
		{
			ImGui::PushID(Name.c_str());
			bool bRes = false;
			bool bSafe = false;
			ButtonValue* V = dynamic_cast<ButtonValue*>(PItem->GetFrontValue()); 
			R_ASSERT(V);

			if (!V->value.empty())
			{
				ImGui::PushItemWidth(-1);
				float size = float(ImGui::CalcItemWidth());
				float dx = floorf(size / float(V->value.size()));
				float offset = size - (dx * V->value.size());
				V->btn_num = V->value.size();

				for (RStringVecIt it = V->value.begin(); it != V->value.end(); it++)
				{
					int k = it - V->value.begin();

					if (ImGui::Button(it->c_str(), ImVec2(dx + offset, 0)))
					{
						V->btn_num = k;
						bRes |= V->OnBtnClick(bSafe);
					}

					offset = 0;
					ImGui::SameLine(0, 2);
				}
			}
			else			
				ImGui::Text("");
			
			ImGui::PopID();
		}
		break;

	case PROP_WAVE:
	case PROP_UNDEF:
		break;

	case PROP_CAPTION:	
		ImGui::TextDisabled(PItem->GetDrawText().c_str());
		break;

	default:
		ImGui::PushID(Name.c_str());

		if (PropertiesFrom->IsReadOnly())
		{
			if (type == PROP_BOOLEAN)
			{
				FlagValueCustom* V = dynamic_cast<FlagValueCustom*>(PItem->GetFrontValue()); VERIFY(V);
				ImGui::TextDisabled(V->GetValueEx() ? "true" : "false");
			}
			else			
				ImGui::TextDisabled(PItem->GetDrawText().c_str());			
		}
		else if (PItem->m_Flags.test(PropItem::flMixed) && !PItem->m_Flags.test(PropItem::flIgnoreMixed))
		{
			if (ImGui::Button("(Mixed)", ImVec2(-1, 0)))			
				RemoveMixed();			
		}
		else
		{
			if (PItem->m_Flags.test(PropItem::flDisabled))
			{
				if (type == PROP_FLAG)
				{
					FlagValueCustom* V = dynamic_cast<FlagValueCustom*>(PItem->GetFrontValue()); VERIFY(V);
					ImGui::TextDisabled(V->GetValueEx() ? "true" : "false");
				}
				else				
					ImGui::TextDisabled(PItem->GetDrawText().c_str());				
			}
			else
			{
				ImGui::PushItemWidth(-1);
				DrawProp();
			}
		}
		ImGui::PopID();
		break;
	}
}

UITreeItem* UIPropertiesItem::CreateItem(shared_str Name)
{
	return xr_new<UIPropertiesItem>(Name, PropertiesFrom);
}
