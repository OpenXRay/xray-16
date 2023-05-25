#pragma once
class UISectorTool : public UIToolCustom
{
public:
	UISectorTool();
	virtual ~UISectorTool();
	virtual void Draw();
	IC void ShowEdit() { m_Edit = true; }
	IC void HideEdit() { m_Edit = false; }
	IC bool IsCreateNewMultiple() const { return m_CreateNewMultiple; }
	IC bool IsCreateNewSingle() const { return m_CreateNewSingle; }
	IC void SetCreateNewSingle(bool mode) { m_CreateNewSingle = mode; }
	IC bool IsMeshAdd() const { return m_MeshAdd; }
	IC bool IsBoxPick() const { return m_BoxPick; }

private:
	bool m_CreateNewMultiple;
	bool m_CreateNewSingle;
	bool m_MeshAdd;
	bool m_BoxPick;
	bool m_Edit;
};