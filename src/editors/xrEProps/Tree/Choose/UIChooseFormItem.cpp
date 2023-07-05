#include "stdafx.h"

UIChooseFormItem::UIChooseFormItem(shared_str Name) : Object(nullptr), UITreeItem(Name)
{
	bIsFavorite = false;
	m_bIsMixed = false;
	bSelected = false;
	Index = -1;
	m_bOpenByDefault = false;
}

void UIChooseFormItem::Draw()
{
	if (!CheckFilter())
		return;

	ImGui::PushID(this);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	if (Object)
	{
		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		if (Form->m_SelectedItem == this || bSelected)
			Flags |= ImGuiTreeNodeFlags_Selected;

		if (Form->m_Flags.test(cfMultiSelect))
		{
			if (ImGui::Checkbox("##checkbox", &bIsFavorite))
			{
				if (bSelected)
				{
					Form->m_RootItem.SelectedToFavorite(bIsFavorite);
				}
				else
				{
					Form->m_RootItem.ClearSelection();
					Form->m_SelectedItem = this;
					bSelected = true;
				}
				Form->CheckFavorite();
			}

			ImGui::SameLine(0, 0);
		}

		ImGui::TreeNodeEx(Text.c_str(), Flags);

		if (ImGui::IsItemClicked())
		{
			if (Form->m_Flags.test(cfMultiSelect))
			{
				if (ImGui::GetIO().KeyShift)
				{
					if (Form->m_SelectedItem)
					{
						Form->m_RootItem.Selected(Index, Form->m_SelectedItem->Index);
					}
					bSelected = true;
				}
				else if (ImGui::GetIO().KeyCtrl)
				{
					bSelected = true;
					Form->UpdateSelected(this);
				}
				else
				{
					Form->m_RootItem.ClearSelection();
					Form->m_SelectedItem = this;
					bSelected = true;
					Form->UpdateSelected(this);
				}
			}
			else
			{
				Form->m_RootItem.ClearSelection();
				Form->m_SelectedItem = this;
				bSelected = true;
				Form->UpdateSelected(this);
			}
		}
	}
	else
	{
		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow;

		if (m_bOpenByDefault)
			Flags |= ImGuiTreeNodeFlags_DefaultOpen;

		if (Form->m_Flags.test(cfMultiSelect))
		{
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, m_bIsMixed);
			bool CheckChange = ImGui::Checkbox("##checkbox", &bIsFavorite);
			ImGui::PopItemFlag();

			if (CheckChange)
			{
				Form->m_RootItem.ClearSelection();
				Form->m_SelectedItem = nullptr;
				bSelected = false;
				SetFavorite(bIsFavorite);
				Form->CheckFavorite();
			}

			ImGui::SameLine(0, 0);
		}

		if (ImGui::TreeNodeEx(Name.c_str(), Flags))
		{
			for (UITreeItem *Item : Items)
				((UIChooseFormItem *)Item)->Draw();

			ImGui::TreePop();
		}
	}

	ImGui::PopID();
}

void UIChooseFormItem::DrawRoot()
{
	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->Draw();
}

void UIChooseFormItem::Sort()
{
	for (UITreeItem *Item : Items)
	{
		std::sort(Items.begin(), Items.end(), [](UITreeItem *Right, UITreeItem *Left) -> bool
				  { 
			UIChooseFormItem* pRight = ((UIChooseFormItem*)Right);
			UIChooseFormItem* pLeft = ((UIChooseFormItem*)Left);

			if(pRight->Object&& !pLeft->Object)
				return false;
			
			if (!pRight->Object && pLeft->Object)
				return true;

			return xr_strcmp(pRight->Name.c_str(), pLeft->Name.c_str()) < 0; });
	}

	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->Sort();
}

void UIChooseFormItem::CheckFavorited()
{
	if (Items.size() == 0)
		return;

	if (!Form->m_Flags.test(cfMultiSelect))
		return;

	m_bIsMixed = false;
	bool bIsLastFavorited = ((UIChooseFormItem *)Items.front())->bIsFavorite;

	for (UITreeItem *Item : Items)
	{
		((UIChooseFormItem *)Item)->CheckFavorited();
		m_bIsMixed |= ((UIChooseFormItem *)Item)->m_bIsMixed;

		if (bIsLastFavorited != ((UIChooseFormItem *)Item)->bIsFavorite)
		{
			bIsLastFavorited = false;
			m_bIsMixed = true;
		}
	}

	bIsFavorite = bIsLastFavorited;
}

void UIChooseFormItem::SetFavorite(bool bFavorited)
{
	if (!Form->m_Flags.test(cfMultiSelect))
		return;

	bIsFavorite = bFavorited;
	m_bIsMixed = false;

	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->SetFavorite(bFavorited);
}

void UIChooseFormItem::FillFavorited(xr_vector<SChooseItem *> &Favorited)
{
	if (!Form->m_Flags.test(cfMultiSelect))
		return;

	if (Object && bIsFavorite)
		Favorited.push_back(Object);

	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->FillFavorited(Favorited);
}

void UIChooseFormItem::CheckFavorited(xr_vector<SChooseItem *> &Favorited)
{
	if (!Form->m_Flags.test(cfMultiSelect))
		return;

	if (Object)
		bIsFavorite = std::find_if(Favorited.begin(), Favorited.end(), [&](SChooseItem *I)
								   { return Object == I; }) != Favorited.end();

	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->CheckFavorited(Favorited);
}

void UIChooseFormItem::ClearSelection()
{
	bSelected = false;

	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->ClearSelection();
}

void UIChooseFormItem::Selected(int Start, int End)
{
	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->Selected(Start, End);

	if (!Object)
		return;

	if (!CheckFilter())
		return;

	if (Start > End)
		std::swap(Start, End);

	bSelected = Index >= Start && Index <= End;
}

void UIChooseFormItem::SelectedToFavorite(bool Favorite)
{
	if (!Form->m_Flags.test(cfMultiSelect))
		return;

	for (UITreeItem *Item : Items)
		((UIChooseFormItem *)Item)->SelectedToFavorite(Favorite);

	if (bSelected)
		SetFavorite(Favorite);
}

bool UIChooseFormItem::CheckFilter()
{
	if (!Form->m_Filter.InputBuf[0])
		return true;

	if (Object)
	{
		if (Form->m_Filter.PassFilter(Object->name.c_str()))
			return true;
	}
	else
	{
		bool bTest = false;

		for (UITreeItem *Item : Items)
			bTest |= ((UIChooseFormItem *)Item)->CheckFilter();

		return bTest;
	}

	return false;
}

UITreeItem *UIChooseFormItem::CreateItem(shared_str Name)
{
	auto Item = xr_new<UIChooseFormItem>(Name);
	Item->Form = Form;
	return Item;
}

void UIChooseFormItem::OpenParentItems(const char* path, char delimiter)
{
	if (!delimiter || !strchr(path, delimiter)) // not a folder
		return;

	string_path itemName;
	xr_strcpy(itemName, path);
	strchr(itemName, delimiter)[0] = '\0';

	if (UIChooseFormItem* Item = dynamic_cast<UIChooseFormItem*>(FindItem(itemName)))
	{
		Item->m_bOpenByDefault = true;
		Item->OpenParentItems(strchr(path, delimiter) + 1);
	}
}
